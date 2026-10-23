#include "../Lawn/Online/FullWatchFragments.h"
#include "../Lawn/Online/FullWatchSendQueue.h"
#include "../Lawn/Online/FullWatchNet.h"
#include <vector>
#include <cstring>
#include <cstdio>

// @pvz-online: focused transport checks; no game window or profile is needed.
int main() {
    static BYTE raw[16000];DWORD random=1;
    for(auto& value:raw){random=random*1664525u+1013904223u;value=BYTE(random>>24);}
    snapshot::Codec codec{};snapshot::Buffer packed{},complete{},decoded{};
    if(!codec.encode(raw,sizeof(raw),17,200,packed))return 1;
    snapshot::Reassembler receiver{};BYTE frame[256];
    DWORD chunks=(packed.size+239)/240;
    // Reverse order and duplicate fragments must still reconstruct one generation.
    for(DWORD i=chunks;i>0;--i) {
        DWORD n=snapshot::fragment(packed,17,2,1,(i-1)*240,frame);
        auto state=receiver.accept(frame,n,2,1,200,complete);
        if(i>1 && state!=snapshot::Receive::Partial)return 2;
        if(i>1 && receiver.accept(frame,n,2,1,201,complete)!=snapshot::Receive::Partial)return 3;
    }
    if(!codec.decode(complete.data,complete.size,decoded) || decoded.size!=sizeof(raw))return 4;
    for(DWORD i=0;i<sizeof(raw);++i)if(raw[i]!=decoded.data[i])return 5;
    snapshot::release(decoded);
    complete.data[complete.size-1]^=1;
    if(codec.decode(complete.data,complete.size,decoded))return 6;
    snapshot::release(complete);
    receiver.reset();
    DWORD n=snapshot::fragment(packed,17,2,1,0,frame);
    if(receiver.accept(frame,n,3,1,300,complete)!=snapshot::Receive::Invalid)return 7;
    if(receiver.accept(frame,n,2,1,300,complete)!=snapshot::Receive::Partial)return 8;
    if(receiver.accept(frame,n,2,1,10300,complete)!=snapshot::Receive::Ignored)return 9;
    receiver.reset();snapshot::release(packed);

    static transport::Queue queue{};
    BYTE a[]{1,2,3},b[]{4,5},control[]{9};
    if(!queue.push(a,3,1,1))return 10;
    if(queue.pump([](const BYTE*,UINT){return 1;},1)!=1)return 11;
    queue.push(b,2,1,1);queue.push(control,1);
    if(queue.discardOlder(1,2)!=1)return 12; // never discard a partially sent TCP frame
    BYTE result[3]{};int offset=0;
    if(queue.pump([&](const BYTE* bytes,UINT count){for(UINT i=0;i<count;++i)result[offset++]=bytes[i];return int(count);},8)!=3)return 13;
    if(result[0]!=2 || result[1]!=3 || result[2]!=9 || queue.count)return 14;
    queue.push(b,2,1,2);
    if(queue.pump([](const BYTE*,UINT){return 0;},8)!=0 || queue.offset)return 15;
    if(snapshot::crc32("123456789",9)!=0xcbf43926u)return 16;
    // A raw snapshot above 1 MiB must be accepted when its compressed packet fits.
    std::vector<BYTE> large(2*1024*1024,42);
    if(!codec.encode(large.data(),static_cast<DWORD>(large.size()),18,300,packed))return 17;
    if(!codec.decode(packed.data,packed.size,decoded) || decoded.size!=large.size() ||
        std::memcmp(decoded.data,large.data(),large.size()))return 18;
    std::printf("large snapshot raw=%zu packed=%lu roundtrip passed\n",large.size(),packed.size);
    snapshot::release(packed);snapshot::release(decoded);
    for(auto& value:large){random^=random<<13;random^=random>>17;random^=random<<5;value=BYTE(random);}
    // The actual compressed payload limit is still enforced and failures free memory.
    if(codec.encode(large.data(),static_cast<DWORD>(large.size()),19,400,packed) || packed.data || packed.size)return 19;
    std::puts("oversized compressed snapshot rejected; CRC wire compatibility passed");
    FullWatchNet paced;
    if(paced.PumpBudget(100)!=8 || paced.PumpBudget(100)!=0 || paced.PumpBudget(119)!=0 ||
       paced.PumpBudget(120)!=8 || paced.PumpBudget(10000)!=8 || paced.PumpBudget(10000)!=0)return 20;
    paced.lastPump=0xfffffff0u;
    if(paced.PumpBudget(3)!=0 || paced.PumpBudget(4)!=8)return 21;
    if(!paced.QueueSnapshot(2,a,3))return 22;
    paced.offsets[2]=1;
    if(!paced.QueueSnapshot(2,b,2) || !paced.QueueSnapshot(2,control,1) ||
       paced.outgoing[2].size!=3 || paced.outgoing[2].data[0]!=1)return 23;
    paced.offsets[2]=3;paced.Promote(2);
    if(paced.outgoing[2].size!=1 || paced.outgoing[2].data[0]!=9 || paced.pending[2].data || paced.offsets[2])return 24;
    std::puts("snapshot wall-clock pacing and in-flight generation preservation passed");
    return 0;
}
