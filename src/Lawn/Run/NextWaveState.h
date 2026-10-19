#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

class Board;
namespace NextWave {
constexpr int COOLDOWN_MS = 10000;
struct State {
    int remainingMs = 0;
    uint64_t lastTick = 0;
    bool running = false;
    bool keyHeld = false;
    bool captured = false;
    int pressedWave = -1;
    int pendingWave = -1;
    void Tick(uint64_t now, bool active) {
        if (running && active && now >= lastTick)
            remainingMs -= static_cast<int>(std::min<uint64_t>(now-lastTick, remainingMs));
        lastTick = now; running = active;
    }
    void Suspend() { running=false; captured=false; pressedWave=pendingWave=-1; }
};
// Kept outside Board so existing native lawn snapshots retain their layout.
inline std::unordered_map<Board*,State>& States() { static std::unordered_map<Board*,State> states; return states; }
inline State& For(Board* board) { return States()[board]; }
inline void Reset(Board* board) { States().erase(board); }
inline uint64_t Now() { return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }
inline void Put(std::vector<unsigned char>& out,uint32_t value) { for(int i=0;i<4;++i)out.push_back(static_cast<unsigned char>(value>>(8*i))); }
inline uint32_t Get(const unsigned char* p) { return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24; }
inline std::vector<unsigned char> Trailer(int remaining) {
    std::vector<unsigned char> out;
    Put(out,0x3156574eu);Put(out,static_cast<uint32_t>(remaining));Put(out,~static_cast<uint32_t>(remaining));return out;
}
inline int ReadTrailer(const unsigned char* bytes,size_t size) {
    // Damage statistics owns the final footer. Our fixed extension precedes it.
    if(size>=8 && Get(bytes+size-4)==0x54585344u) {
        uint32_t length=Get(bytes+size-8);
        if(length>8192 || length>size-8)return 0;
        size-=length+8;
    }
    if(size<12)return 0;
    const auto* p=bytes+size-12;
    uint32_t remaining=Get(p+4);
    return Get(p)==0x3156574eu && remaining<=COOLDOWN_MS && Get(p+8)==~remaining ? static_cast<int>(remaining):0;
}
}
