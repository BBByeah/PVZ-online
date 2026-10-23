#pragma once
#include "FullWatchFragments.h"
#include "WatchWire.h"
#include <vector>
#include <memory>

// @pvz-online: network buffers and per-viewer initialization acknowledgements
// stay outside Board and never become part of player save files.
struct FullWatchNet {
    snapshot::Reassembler assembly{};
    snapshot::Codec codec{};
    snapshot::Buffer outgoing[7]{};
    DWORD offsets[7]{};
    std::vector<unsigned char> received;
    BYTE nextSeat=1;
    DWORD ackEpoch[7]{},ackRevision[7]{};
    WatchWire::Cache configCache;
    ~FullWatchNet() {
        assembly.reset();
        for(auto& packet:outgoing)snapshot::release(packet);
    }
};
