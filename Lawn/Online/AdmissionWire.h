#pragma once
#include "NetProtocol.h"

namespace AdmissionWire {
enum Op : uint8_t { HELLO=1, VALUE, OFFER, CONTEXT, CANCEL };
struct Message {
    uint8_t src=0,dst=0,op=0;
    uint32_t epoch=0,level=0,budget=0,joined=0;
    uint64_t run=0,player=0;
};
constexpr int TYPE=26,SIZE=35;
inline int Encode(uint8_t* bytes,const Message& m) {
    NetProto::Writer w(bytes,SIZE);w.U8(m.src);w.U8(m.dst);w.U8(m.op);w.U32(m.epoch);w.U32(m.level);w.U32(m.budget);w.U32(m.joined);
    w.U32(uint32_t(m.run));w.U32(uint32_t(m.run>>32));w.U32(uint32_t(m.player));w.U32(uint32_t(m.player>>32));return w.Size();
}
inline bool Decode(const uint8_t* bytes,int size,Message& m) {
    if(size!=SIZE)return false;NetProto::Reader r(bytes,size);m.src=r.U8();m.dst=r.U8();m.op=r.U8();m.epoch=r.U32();m.level=r.U32();m.budget=r.U32();m.joined=r.U32();
    m.run=r.U32();m.run|=uint64_t(r.U32())<<32;m.player=r.U32();m.player|=uint64_t(r.U32())<<32;
    return !r.Overflowed()&&m.src>=1&&m.src<=6&&m.dst>=1&&m.dst<=6&&m.op>=HELLO&&m.op<=CANCEL&&m.player&&m.level<60000&&m.joined<60000&&m.budget<=1000000;
}
}
