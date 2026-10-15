#include "RunCheckpointIO.h"
#include "../../SexyAppFramework/SexyAppBase.h"
#include "misc/Buffer.h"
#include <cstdio>
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace RunCheckpointIO {
namespace {
constexpr uint32_t MAGIC = 0x31585452; // RTX1
constexpr size_t MAX_BYTES = 8 * 1024 * 1024;
uint32_t Hash(const unsigned char* data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) hash = (hash ^ data[i]) * 16777619u;
    return hash;
}
void Put(std::vector<unsigned char>& out, uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back((unsigned char)(value >> (i * 8)));
}
uint32_t Get(const unsigned char* data) {
    return uint32_t(data[0]) | uint32_t(data[1]) << 8 | uint32_t(data[2]) << 16 | uint32_t(data[3]) << 24;
}
}

bool AtomicWrite(const std::string& path, const std::vector<unsigned char>& bytes) {
    const std::string temp = path + ".tmp";
    FILE* file = std::fopen(temp.c_str(), "wb");
    if (!file) return false;
    bool ok = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size() && std::fflush(file) == 0;
#ifdef _WIN32
    if (ok) ok = _commit(_fileno(file)) == 0;
#else
    if (ok) ok = fsync(fileno(file)) == 0;
#endif
    if (std::fclose(file) != 0) ok = false;
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = std::rename(temp.c_str(), path.c_str()) == 0;
#endif
    }
    if (!ok) std::remove(temp.c_str());
    return ok;
}

bool Recover(const std::string& runPath, const std::string& userPath) {
    if (!Sexy::gSexyAppBase) return false;
    const std::string journal = runPath + ".txn";
    if (!Sexy::gSexyAppBase->FileExists(journal)) return true;
    Sexy::Buffer buffer;
    if (!Sexy::gSexyAppBase->ReadBufferFromFile(journal, &buffer, false)) return false;
    const auto* data = (const unsigned char*)buffer.GetDataPtr();
    const size_t size = buffer.GetDataLen();
    if (size < 20 || size > MAX_BYTES || Get(data) != MAGIC) return false;
    const size_t runSize = Get(data + 4), userSize = Get(data + 8);
    if (!runSize || !userSize || runSize > MAX_BYTES || userSize > MAX_BYTES || 20 + runSize + userSize != size ||
        Hash(data + 20, runSize) != Get(data + 12) || Hash(data + 20 + runSize, userSize) != Get(data + 16)) return false;
    // Replaying both full snapshots is idempotent even after a crash between the two renames.
    std::vector<unsigned char> run(data + 20, data + 20 + runSize), user(data + 20 + runSize, data + size);
    if (!AtomicWrite(userPath, user) || !AtomicWrite(runPath, run)) return false;
    return std::remove(journal.c_str()) == 0;
}

bool Commit(const std::string& runPath, const std::string& userPath,
            const std::vector<unsigned char>& run, const std::vector<unsigned char>& user) {
    if (run.empty() || user.empty() || run.size() + user.size() + 20 > MAX_BYTES || !Recover(runPath, userPath)) return false;
    std::vector<unsigned char> journal;
    Put(journal, MAGIC); Put(journal, (uint32_t)run.size()); Put(journal, (uint32_t)user.size());
    Put(journal, Hash(run.data(), run.size())); Put(journal, Hash(user.data(), user.size()));
    journal.insert(journal.end(), run.begin(), run.end()); journal.insert(journal.end(), user.begin(), user.end());
    if (!AtomicWrite(runPath + ".txn", journal)) return false;
    // The durable journal is the commit point. A failed publish is completed on the next read/write.
    Recover(runPath, userPath);
    return true;
}
}
