#pragma once
#include "RunWaveHitPacking.h"
#include "../Board.h"
#include "../Projectile.h"
#include <algorithm>
#include <cstring>
#include <vector>

// The base save retains the original 1024 full IDs. This optional extension is
// anchored immediately after the base payload, before the older optional tails.
namespace RunWaveHitSave {
inline uint32_t Get(const unsigned char* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline void Put(std::vector<unsigned char>& out, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<unsigned char>(value >> (i * 8)));
}
inline uint32_t Hash(const unsigned char* data, size_t size) {
    uint32_t value = 2166136261U;
    for (size_t i = 0; i < size; ++i) value = (value ^ data[i]) * 16777619U;
    return value;
}
inline std::vector<unsigned char> SaveTrailer(Board* board) {
    std::vector<unsigned char> out{'P','V','Z','W','H','I','0','1'};
    Put(out, 0); Put(out, 0);
    uint32_t count = 0;
    Projectile* projectile = nullptr;
    const unsigned limit = std::min(board->mZombies.mMaxSize, RunWaveHitPacking::Slots);
    while (board->IterateProjectiles(projectile)) {
        if (!projectile->mRunIceWave) continue;
        unsigned hits = 0;
        for (unsigned slot = RunWaveHitPacking::LegacySlots; slot < limit; ++slot)
            hits += RunWaveHitPacking::Generation(projectile->mRunWaveHitIds, slot) != 0;
        if (!hits) continue;
        Put(out, board->mProjectiles.DataArrayGetID(projectile)); Put(out, hits); ++count;
        for (unsigned slot = RunWaveHitPacking::LegacySlots; slot < limit; ++slot) {
            const uint32_t id = RunWaveHitPacking::LegacyId(projectile->mRunWaveHitIds, slot);
            if (id) Put(out, id);
        }
    }
    const uint32_t bytes = static_cast<uint32_t>(out.size() + 4);
    for (unsigned i = 0; i < 4; ++i) { out[8 + i] = static_cast<unsigned char>(bytes >> (i * 8)); out[12 + i] = static_cast<unsigned char>(count >> (i * 8)); }
    Put(out, Hash(out.data(), out.size()));
    return out;
}
inline bool LegacyTail(const unsigned char* data, size_t size) {
    if (!size) return true;
    if (size >= 12 && std::memcmp(data, "NWV1", 4) == 0) return true;
    if (size >= 8 && Get(data + size - 4) == 0x54585344U && Get(data + size - 8) == size - 8) return true;
    if (size < 8) return false;
    for (const char* tag : {"PVZWSP01", "PVZASP01", "PVZPSP01", "PVZWHP01", "PVZDOT01", "PVZNXT01"})
        if (std::memcmp(data, tag, 8) == 0) return true;
    return false;
}
inline bool LoadTrailer(Board* board, const unsigned char* data, size_t size, size_t baseEnd) {
    if (baseEnd > size) return false;
    const unsigned char* tail = data + baseEnd;
    const size_t remaining = size - baseEnd;
    const bool present = remaining >= 8 && std::memcmp(tail, "PVZWHI01", 8) == 0;
    uint32_t bytes = 0, count = 0;
    const unsigned limit = std::min(board->mZombies.mMaxSize, RunWaveHitPacking::Slots);
    if (present) {
        if (remaining < 20) return false;
        bytes = Get(tail + 8); count = Get(tail + 12);
        if (bytes < 20 || bytes > remaining || count > board->mProjectiles.mSize ||
            Get(tail + bytes - 4) != Hash(tail, bytes - 4)) return false;
        size_t pos = 16;
        unsigned previousProjectile = 0;
        for (uint32_t i = 0; i < count; ++i) {
            if (pos > bytes - 4 || bytes - 4 - pos < 8) return false;
            const uint32_t id = Get(tail + pos), hits = Get(tail + pos + 4); pos += 8;
            const unsigned slot = id & 0xffffU;
            auto* projectile = board->mProjectiles.DataArrayTryToGet(id);
            if (!projectile || !projectile->mRunIceWave || (i && slot <= previousProjectile) ||
                !hits || limit <= RunWaveHitPacking::LegacySlots || hits > limit - RunWaveHitPacking::LegacySlots ||
                hits > (bytes - 4 - pos) / 4) return false;
            previousProjectile = slot;
            unsigned previousZombie = RunWaveHitPacking::LegacySlots - 1;
            for (uint32_t j = 0; j < hits; ++j) {
                const uint32_t zombieId = Get(tail + pos); pos += 4;
                const unsigned zombieSlot = zombieId & 0xffffU;
                // Killed zombies may already be gone. Keep their generation, not a live pointer.
                if (!(zombieId >> 16) || zombieSlot <= previousZombie || zombieSlot >= limit) return false;
                previousZombie = zombieSlot;
            }
        }
        if (pos != bytes - 4 || !LegacyTail(tail + bytes, remaining - bytes)) return false;
    } else if (!LegacyTail(tail, remaining)) return false;

    // Validate the whole extension before changing any projectile's hit history.
    Projectile* projectile = nullptr;
    while (board->IterateProjectiles(projectile))
        if (projectile->mRunIceWave) RunWaveHitPacking::ConvertLegacy(projectile->mRunWaveHitIds);
    if (present) {
        size_t pos = 16;
        for (uint32_t i = 0; i < count; ++i) {
            projectile = board->mProjectiles.DataArrayTryToGet(Get(tail + pos));
            const uint32_t hits = Get(tail + pos + 4); pos += 8;
            for (uint32_t j = 0; j < hits; ++j) { RunWaveHitPacking::Record(projectile->mRunWaveHitIds, Get(tail + pos)); pos += 4; }
        }
    }
    return true;
}
}
