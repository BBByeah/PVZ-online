#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace DamageStatsWire
{
constexpr uint64_t LIMIT = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
constexpr int MAX_ROWS = 32;
constexpr int NAME_BYTES = 64;
enum Operation : uint8_t { BEGIN = 0, REPORT = 1, RELAY = 2 };

struct Counters
{
    uint64_t mAll = 0;
    uint64_t mBoss = 0;
    bool Valid() const { return mBoss <= mAll && mAll <= LIMIT; }
    void Add(uint64_t amount, bool boss)
    {
        const uint64_t accepted = amount < LIMIT - mAll ? amount : LIMIT - mAll;
        mAll += accepted;
        if (boss) mBoss += accepted;
    }
};

struct Snapshot
{
    uint8_t mSrc = 0, mDst = 0, mOp = BEGIN, mSeat = 0, mMode = 0, mPartial = 0;
    uint32_t mRun = 0, mLevel = 0;
    uint64_t mAttempt = 0, mPlayer = 0, mSequence = 0;
    Counters mLevelDamage, mRunDamage;
    char mName[NAME_BYTES]{};
    bool Valid() const
    {
        return mOp <= RELAY && mMode <= 3 && mPartial <= 1 && mAttempt &&
            mLevelDamage.Valid() && mRunDamage.Valid() &&
            mLevelDamage.mAll <= mRunDamage.mAll && mLevelDamage.mBoss <= mRunDamage.mBoss &&
            mRunDamage.mBoss - mLevelDamage.mBoss <= mRunDamage.mAll - mLevelDamage.mAll &&
            (mOp == BEGIN || (mPlayer && mSequence && mSeat >= 1 && mSeat <= 6)) &&
            mName[NAME_BYTES - 1] == 0;
    }
};

struct Writer
{
    std::vector<unsigned char> mData;
    void Put(uint64_t value, int bytes)
    {
        for (int offset = 0; offset < bytes; ++offset) mData.push_back(static_cast<unsigned char>(value >> (offset * 8)));
    }
    void Text(const std::string& value)
    {
        size_t length = value.size() < NAME_BYTES - 1 ? value.size() : NAME_BYTES - 1;
        while (length < value.size() && length && (static_cast<unsigned char>(value[length]) & 0xc0) == 0x80) --length;
        for (size_t index = 0; index < NAME_BYTES; ++index)
            mData.push_back(index < length ? static_cast<unsigned char>(value[index]) : 0);
    }
};

struct Reader
{
    const unsigned char* mData;
    size_t mSize, mOffset = 0;
    bool mOk = true;
    Reader(const unsigned char* data, size_t size) : mData(data), mSize(size) {}
    uint64_t Get(int bytes)
    {
        if (!mOk || static_cast<size_t>(bytes) > mSize - mOffset) { mOk = false; return 0; }
        uint64_t value = 0;
        for (int offset = 0; offset < bytes; ++offset) value |= static_cast<uint64_t>(mData[mOffset++]) << (offset * 8);
        return value;
    }
    std::string Text()
    {
        std::string value;
        bool ended = false;
        for (int index = 0; index < NAME_BYTES; ++index)
        {
            const char character = static_cast<char>(Get(1));
            if (!character) ended = true;
            else if (!ended) value.push_back(character);
        }
        if (!ended) mOk = false;
        return value;
    }
};

inline uint32_t Hash(const unsigned char* data, size_t size)
{
    uint32_t hash = 2166136261u;
    for (size_t index = 0; index < size; ++index) hash = (hash ^ data[index]) * 16777619u;
    return hash;
}

inline std::vector<unsigned char> Encode(const Snapshot& snapshot)
{
    if (!snapshot.Valid()) return {};
    Writer writer;
    writer.Put(snapshot.mSrc, 1); writer.Put(snapshot.mDst, 1);
    writer.Put(snapshot.mOp, 1); writer.Put(snapshot.mSeat, 1);
    writer.Put(snapshot.mMode, 1); writer.Put(snapshot.mPartial, 1);
    writer.Put(snapshot.mRun, 4); writer.Put(snapshot.mLevel, 4);
    writer.Put(snapshot.mAttempt, 8); writer.Put(snapshot.mPlayer, 8); writer.Put(snapshot.mSequence, 8);
    writer.Put(snapshot.mLevelDamage.mAll, 8); writer.Put(snapshot.mLevelDamage.mBoss, 8);
    writer.Put(snapshot.mRunDamage.mAll, 8); writer.Put(snapshot.mRunDamage.mBoss, 8);
    writer.Text(snapshot.mName);
    return writer.mData;
}

inline bool Decode(const unsigned char* bytes, size_t size, Snapshot& snapshot)
{
    if (size != 134) return false;
    Reader reader(bytes, size);
    snapshot.mSrc = static_cast<uint8_t>(reader.Get(1)); snapshot.mDst = static_cast<uint8_t>(reader.Get(1));
    snapshot.mOp = static_cast<uint8_t>(reader.Get(1)); snapshot.mSeat = static_cast<uint8_t>(reader.Get(1));
    snapshot.mMode = static_cast<uint8_t>(reader.Get(1)); snapshot.mPartial = static_cast<uint8_t>(reader.Get(1));
    snapshot.mRun = static_cast<uint32_t>(reader.Get(4)); snapshot.mLevel = static_cast<uint32_t>(reader.Get(4));
    snapshot.mAttempt = reader.Get(8); snapshot.mPlayer = reader.Get(8); snapshot.mSequence = reader.Get(8);
    snapshot.mLevelDamage.mAll = reader.Get(8); snapshot.mLevelDamage.mBoss = reader.Get(8);
    snapshot.mRunDamage.mAll = reader.Get(8); snapshot.mRunDamage.mBoss = reader.Get(8);
    const auto name = reader.Text();
    std::memset(snapshot.mName, 0, sizeof(snapshot.mName));
    std::memcpy(snapshot.mName, name.data(), name.size());
    return reader.mOk && reader.mOffset == size && snapshot.Valid();
}
}
