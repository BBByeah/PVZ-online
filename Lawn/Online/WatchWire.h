#pragma once
#include <cstdint>
#include <vector>
#include <cstring>

// @pvz-online: explicit little-endian wire fields; never expose native object layouts.
namespace WatchWire {
using Bytes=std::vector<unsigned char>;
constexpr uint32_t Magic=0x32565750, Version=2, HeaderBytes=44;
constexpr uint32_t MaxConfig=256*1024, MaxState=7*1024*1024;
constexpr unsigned char Ack=0x84;
inline uint32_t Get(const unsigned char* p) {return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline void Put(Bytes& b,uint32_t v) {for(int i=0;i<4;++i)b.push_back(static_cast<unsigned char>(v>>(8*i)));}
struct Header {uint32_t epoch=0,revision=0,sequence=0,seat=0,mode=0,paused=0,relay=0,configBytes=0,stateBytes=0;};
inline Bytes Pack(Header h,const Bytes& config,const Bytes& state) {
    Bytes b;Put(b,Magic);Put(b,Version);Put(b,h.epoch);Put(b,h.revision);Put(b,h.sequence);
    Put(b,h.seat|(h.paused<<8)|(h.relay<<9));Put(b,h.mode);
    Put(b,static_cast<uint32_t>(config.size()));Put(b,static_cast<uint32_t>(state.size()));
    Put(b,0);Put(b,0); // Reserved, must be zero in version 1.
    b.insert(b.end(),config.begin(),config.end());b.insert(b.end(),state.begin(),state.end());return b;
}
inline bool Parse(const Bytes& b,Header& h) {
    if(b.size()<HeaderBytes || Get(b.data())!=Magic || Get(b.data()+4)!=Version)return false;
    h.epoch=Get(b.data()+8);h.revision=Get(b.data()+12);h.sequence=Get(b.data()+16);
    uint32_t flags=Get(b.data()+20);h.seat=flags&255;h.paused=(flags>>8)&1;h.relay=(flags>>9)&1;
    h.mode=Get(b.data()+24);h.configBytes=Get(b.data()+28);h.stateBytes=Get(b.data()+32);
    return h.epoch && h.revision && h.seat>=1 && h.seat<=6 && !(flags&~1023u) &&
        !Get(b.data()+36) && !Get(b.data()+40) && h.configBytes<=MaxConfig && h.stateBytes>0 &&
        h.stateBytes<=MaxState && uint64_t(HeaderBytes)+h.configBytes+h.stateBytes==b.size();
}
struct Cache {
    uint32_t epoch=0,revision=0;Bytes config;
    void Reset(){epoch=revision=0;config.clear();}
    bool Accept(const Bytes& wire,Bytes& complete,Header& h) {
        if(!Parse(wire,h))return false;
        if(h.configBytes){config.assign(wire.begin()+HeaderBytes,wire.begin()+HeaderBytes+h.configBytes);epoch=h.epoch;revision=h.revision;}
        if(config.empty() || epoch!=h.epoch || revision!=h.revision)return false;
        Bytes state(wire.begin()+HeaderBytes+h.configBytes,wire.end());complete=Pack(h,config,state);return true;
    }
};
// First edit sends at the next board tick. Bursts coalesce at the earliest allowed time.
struct Schedule {
    uint32_t last=0,lastAttempt=0;bool sent=false,dirty=false,attempted=false;
    void Edit(){dirty=true;}
    bool Due(uint32_t now,bool newcomer=false)const{return (!attempted || now-lastAttempt>=1000) && (!sent || (now-last>=1000 && (dirty || newcomer || now-last>=5000)));}
    void Attempt(uint32_t now){lastAttempt=now;attempted=true;}
    void Sent(uint32_t now){Attempt(now);last=now;sent=true;dirty=false;}
};
}
