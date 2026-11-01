#pragma once
#include "FullWatchFragments.h"
#include "WatchWire.h"
#include <vector>
#include <memory>

// @pvz-online: network buffers and per-viewer initialization acknowledgements
// stay outside Board and never become part of player save files.
struct FullWatchNet {
    struct RoomPeer { uint64_t identity=0,started=0,requested=0,renew=0; bool present=false,failed=false,ready=false; };
    RoomPeer peers[7]{};
    uint32_t subscription=0,viewerSubscription[7]{},lastSequence=0,lastEpoch=0;
    uint64_t requestedAt=0,pictureAt=0,directAt=0;
    uint64_t pairWindow=0;unsigned pairRequests=0;
    bool receivedDirect=false,forceRelay[7]{},urgent[7]{},fallbackRequested=false;
    snapshot::Reassembler assembly{};
    snapshot::Codec codec{};
    snapshot::Buffer outgoing[7]{};
    snapshot::Buffer pending[7]{};
    DWORD offsets[7]{};
    std::vector<unsigned char> received;
    BYTE nextSeat=1;
    DWORD ackEpoch[7]{},ackRevision[7]{};
    DWORD lastPump=0;
    bool pumped=false;
    WatchWire::Cache configCache;
    void ClearSend(BYTE seat) {
        snapshot::release(outgoing[seat]);snapshot::release(pending[seat]);offsets[seat]=0;
    }
    int PumpBudget(DWORD now) {
        if(pumped && now-lastPump<20)return 0;
        pumped=true;lastPump=now;
        return 8; // bounded wall-clock bursts; never catch up with a large burst
    }
    bool QueueSnapshot(BYTE seat,const BYTE* bytes,DWORD size) {
        // Finish the in-flight generation; retain only the newest waiting one.
        auto& target=outgoing[seat].data && offsets[seat]>0 && offsets[seat]<outgoing[seat].size
            ? pending[seat] : outgoing[seat];
        snapshot::Buffer replacement{};
        if(!snapshot::allocate(replacement,size))return false;
        memcpy(replacement.data,bytes,size);
        snapshot::release(target);target=replacement;
        if(&target==&outgoing[seat]){offsets[seat]=0;snapshot::release(pending[seat]);}
        return true;
    }
    void Promote(BYTE seat) {
        if(outgoing[seat].data && offsets[seat]>=outgoing[seat].size)snapshot::release(outgoing[seat]);
        if(!outgoing[seat].data && pending[seat].data){outgoing[seat]=pending[seat];pending[seat]={};offsets[seat]=0;}
    }
    ~FullWatchNet() {
        assembly.reset();
        for(auto& packet:outgoing)snapshot::release(packet);
        for(auto& packet:pending)snapshot::release(packet);
    }
};
