#pragma once
#include "NetProtocol.h"

// @pvz-online: seats route packets; player tokens and preparation revisions bind consent.
namespace PreparationWire {
enum Op : uint8_t { REPORT=1, REQUEST, ANSWER, READY, ACK, STATE, QUERY };
enum Phase : uint8_t { WAIT=0, OPEN, ASK, COMMIT, SEAL, GO };
struct Message {
    uint8_t src=0,dst=0,op=0;
    uint32_t epoch=0,revision=0,request=0;
    uint64_t sender=0,actor=0,target=0;
    uint8_t phase=0,entered=0,ready=0,a=0,b=0;
    uint64_t players[NetProto::MAX_PLAYERS+1]{};
};
constexpr int SIZE=92;
inline void Put64(NetProto::Writer& w,uint64_t n){w.U32((uint32_t)n);w.U32((uint32_t)(n>>32));}
inline uint64_t Get64(NetProto::Reader& r){uint64_t low=r.U32();return low|(uint64_t(r.U32())<<32);}
inline int Encode(uint8_t* bytes,const Message& m) {
    NetProto::Writer w(bytes,SIZE);
    w.U8(m.src);w.U8(m.dst);w.U8(m.op);w.U32(m.epoch);w.U32(m.revision);w.U32(m.request);
    Put64(w,m.sender);Put64(w,m.actor);Put64(w,m.target);
    w.U8(m.phase);w.U8(m.entered);w.U8(m.ready);w.U8(m.a);w.U8(m.b);
    for(int seat=1;seat<=NetProto::MAX_PLAYERS;++seat)Put64(w,m.players[seat]);
    return w.Size();
}
inline bool Decode(const uint8_t* bytes,int size,Message& m) {
    if(size!=SIZE)return false;
    NetProto::Reader r(bytes,size);
    m.src=r.U8();m.dst=r.U8();m.op=r.U8();m.epoch=r.U32();m.revision=r.U32();m.request=r.U32();
    m.sender=Get64(r);m.actor=Get64(r);m.target=Get64(r);
    m.phase=r.U8();m.entered=r.U8();m.ready=r.U8();m.a=r.U8();m.b=r.U8();
    for(int seat=1;seat<=NetProto::MAX_PLAYERS;++seat)m.players[seat]=Get64(r);
    return !r.Overflowed() && r.Remain()==0 && m.src>=1 && m.src<=NetProto::MAX_PLAYERS &&
        m.dst>=1 && m.dst<=NetProto::MAX_PLAYERS && m.sender && m.epoch &&
        m.op>=REPORT && m.op<=QUERY && m.phase<=GO && m.a<=NetProto::MAX_PLAYERS &&
        m.b<=NetProto::MAX_PLAYERS && !(m.entered&0x81) &&
        ((m.op==READY || m.op==ANSWER)?m.ready<=1:!(m.ready&0x81));
}
struct StateData {
    uint32_t epoch=0,revision=0,request=0,appliedRequest=0;
    uint64_t actor=0,target=0;
    uint64_t participants[NetProto::MAX_PLAYERS+1]{};
    uint8_t phase=WAIT,entered=0,ready=0,acks=0,a=0,b=0;
    bool local=false,commitSent=false,gameManaged=false;
    uint32_t configuredRevision=0;
    bool recovering=false,rejoinAlternate=false,rejoinRoster=false;
    uint8_t recoveryReports=0;
    uint8_t occupiedMask=0;
    int ticks=0,reportTicks=0;
};
}
