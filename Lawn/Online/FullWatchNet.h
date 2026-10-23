#pragma once
#include "FullWatchFragments.h"
#include <vector>
#include <memory>

// @pvz-online: keep incomplete generations outside Board so native saves never
// contain owning network buffers or observer process handles.
struct FullWatchNet {
    snapshot::Reassembler assembly{};
    snapshot::Codec codec{};
    snapshot::Buffer outgoing[7]{};
    DWORD offsets[7]{};
    std::vector<unsigned char> received;
    BYTE nextSeat=1;
    ~FullWatchNet() {
        assembly.reset();
        for(auto& packet:outgoing)snapshot::release(packet);
    }
};
