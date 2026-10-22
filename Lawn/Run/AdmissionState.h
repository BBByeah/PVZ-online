#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

// @pvz-online: admission survives retries and process restarts without changing Board's save ABI.
namespace Admission {
constexpr int MAX_PLAYERS_HISTORY=256;
constexpr int MAX_BUDGET=1000000;
enum Phase : uint8_t { NONE=0, PICKS, DECK, BUILD, PREPARED, PLAYED };
struct Entry { uint64_t player=0; int joined=0,budget=0; };
struct State {
    uint64_t run=0,player=0;
    int joined=-1,budget=0,totalBudget=0;
    Phase phase=NONE;
    bool fast=false,allow=true;
    std::vector<Entry> history;
    bool Building() const { return phase==BUILD; }
    bool Pending() const { return phase>=PICKS && phase<=BUILD; }
};
inline uint64_t NewId() {
    static std::atomic<uint64_t> serial{0};
    uint64_t id=uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count())^(++serial*0x9e3779b97f4a7c15ull);
    return id?id:++serial;
}
inline int PicksBefore(int nextLevel,int multiplier,int period) {
    if(nextLevel<0 || nextLevel>=60000 || multiplier<1 || multiplier>3 || (period!=0 && period!=5 && period!=10))return -1;
    return (4+2*nextLevel+(period?nextLevel/period:0))*multiplier;
}
inline int Median(std::vector<int> values) {
    if(values.empty())return 0;
    for(int& n:values)n=std::clamp(n,0,MAX_BUDGET);
    std::sort(values.begin(),values.end());size_t n=values.size();
    return n%2?values[n/2]:(values[n/2-1]+values[n/2])/2;
}
inline void Put(std::vector<unsigned char>& b,uint64_t n,int size=4) {for(int i=0;i<size;++i)b.push_back(uint8_t(n>>(8*i)));}
inline uint64_t Get(const unsigned char*& p,int size=4) {uint64_t n=0;for(int i=0;i<size;++i)n|=uint64_t(*p++)<<(8*i);return n;}
inline std::vector<unsigned char> Encode(const State& s) {
    std::vector<unsigned char> b;Put(b,0x314d4441);Put(b,s.run,8);Put(b,s.player,8);Put(b,uint32_t(s.joined));
    Put(b,s.budget);Put(b,s.totalBudget);Put(b,s.phase,1);Put(b,s.fast,1);Put(b,s.allow,1);Put(b,s.history.size(),2);
    for(const auto& e:s.history){Put(b,e.player,8);Put(b,e.joined);Put(b,e.budget);}return b;
}
inline bool Decode(const std::vector<unsigned char>& b,State& out) {
    if(b.size()<37)return false;const auto* p=b.data();
    if(Get(p)!=0x314d4441)return false;State s;
    s.run=Get(p,8);s.player=Get(p,8);s.joined=int32_t(Get(p));s.budget=int(Get(p));s.totalBudget=int(Get(p));
    int phase=int(Get(p,1)),fast=int(Get(p,1)),allow=int(Get(p,1)),count=int(Get(p,2));
    if(phase>PLAYED||fast>1||allow>1||s.joined < -1||s.joined>=60000||s.budget<0||s.budget>MAX_BUDGET||s.totalBudget<s.budget||s.totalBudget>MAX_BUDGET||count>MAX_PLAYERS_HISTORY||b.size()!=37+size_t(count)*16)return false;
    if(phase!=NONE && (!s.run||!s.player||s.joined<0))return false;
    s.phase=Phase(phase);s.fast=fast!=0;s.allow=allow!=0;
    for(int i=0;i<count;++i){Entry e;e.player=Get(p,8);e.joined=int(Get(p));e.budget=int(Get(p));
        if(!e.player||e.joined<0||e.joined>=60000||e.budget<0||e.budget>MAX_BUDGET)return false;
        for(const auto& prior:s.history)if(prior.player==e.player)return false;s.history.push_back(e);}
    out=std::move(s);return true;
}
}
