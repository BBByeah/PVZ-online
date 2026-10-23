#pragma once
#include "FullWatchCodec.h"

namespace snapshot {
constexpr WORD FragmentMagic=0x5753;
constexpr DWORD ChunkBytes=240;
constexpr DWORD MaxEnvelope=MaxPacked+sizeof(Header);
constexpr DWORD MaxChunks=(MaxEnvelope+ChunkBytes-1)/ChunkBytes;
struct FragmentHeader {BYTE sender,recipient;WORD magic;DWORD sequence,total,offset;};
static_assert(sizeof(FragmentHeader)==16,"256-byte game payload: 16-byte header plus 240 bytes");
inline DWORD fragment(const Buffer& envelope,DWORD sequence,BYTE sender,BYTE recipient,DWORD offset,BYTE (&out)[256]) {
    if(!envelope.data || !envelope.size || envelope.size>MaxEnvelope || offset>=envelope.size || offset%ChunkBytes) return 0;
    FragmentHeader h{sender,recipient,FragmentMagic,sequence,envelope.size,offset};
    const BYTE* header=reinterpret_cast<const BYTE*>(&h);
    for(DWORD i=0;i<sizeof(h);++i) out[i]=header[i];
    DWORD n=envelope.size-offset;if(n>ChunkBytes)n=ChunkBytes;
    for(DWORD i=0;i<n;++i) out[sizeof(h)+i]=envelope.data[offset+i];
    return sizeof(h)+n;
}
enum class Receive {Ignored,Partial,Complete,Invalid};
// One instance per observed seat. A new subscription must reset the instance.
// No retransmit backlog: a newer full snapshot supersedes an incomplete older one.
struct Reassembler {
    Buffer pending;
    DWORD bitmap[(MaxChunks+31)/32];
    DWORD sequence,received,started;
    bool seen,finished;
    void discard() {release(pending);received=0;finished=true;}
    void reset() {discard();seen=false;sequence=0;}
    Receive accept(const BYTE* bytes,DWORD length,BYTE expectedSender,BYTE localSeat,DWORD now,Buffer& complete) {
        if(complete.data || !bytes || length<=sizeof(FragmentHeader) || length>256) return Receive::Invalid;
        FragmentHeader h;BYTE* header=reinterpret_cast<BYTE*>(&h);
        for(DWORD i=0;i<sizeof(h);++i) header[i]=bytes[i];
        if(h.magic!=FragmentMagic || h.sender!=expectedSender || h.recipient!=localSeat ||
           h.total<sizeof(Header) || h.total>MaxEnvelope || h.offset>=h.total || h.offset%ChunkBytes) return Receive::Invalid;
        DWORD count=h.total-h.offset;if(count>ChunkBytes)count=ChunkBytes;
        if(length!=sizeof(h)+count) return Receive::Invalid;
        if(seen) {
            LONG age=static_cast<LONG>(h.sequence-sequence);
            if(age<0 || (age==0 && finished)) return Receive::Ignored;
            if(age==0 && now-started>=10000) {discard();return Receive::Ignored;}
            if(age==0 && pending.size!=h.total) {discard();return Receive::Invalid;}
            if(age>0) discard();
        }
        if(!pending.data) {
            if(!allocate(pending,h.total)) return Receive::Invalid;
            for(DWORD i=0;i<(MaxChunks+31)/32;++i)bitmap[i]=0;
            sequence=h.sequence;seen=true;finished=false;received=0;started=now;
        }
        DWORD index=h.offset/ChunkBytes,word=index/32,mask=1u<<(index%32);
        if(bitmap[word]&mask) {
            for(DWORD i=0;i<count;++i) if(pending.data[h.offset+i]!=bytes[sizeof(h)+i]) {discard();return Receive::Invalid;}
            return Receive::Partial;
        }
        for(DWORD i=0;i<count;++i)pending.data[h.offset+i]=bytes[sizeof(h)+i];
        bitmap[word]|=mask;received+=count;
        if(received!=h.total)return Receive::Partial;
        if(reinterpret_cast<const Header*>(pending.data)->sequence!=sequence) {discard();return Receive::Invalid;}
        complete=pending;pending={};finished=true;
        return Receive::Complete;
    }
};
}
