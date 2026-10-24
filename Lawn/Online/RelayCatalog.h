#pragma once
#include "RelayNodeTypes.h"
#include "NetLink.h"
#include <chrono>
#include <memory>
#include <vector>

namespace RelayNodes {
constexpr size_t MAX_CATALOG_BYTES=8192,MAX_CATALOG_NODES=64;
constexpr int MAX_SHARE_CODE_CHARS=10; // u16 node ID, separator, four room characters
const std::vector<Node>& Catalog();
uint64_t CatalogRevision();
void SetCatalogCachePath(const std::string& path);
bool ApplyCatalogBlob(const std::vector<uint8_t>& blob,bool cached=false);
bool CatalogCacheWriteFailed();

// Several seed sources are queried concurrently with a fixed overall deadline.
// No discovery work happens during rendering or after a room has been joined.
class CatalogDiscovery {
    struct Source {
        NetLink link;
        std::vector<uint8_t> bytes;
        uint16_t expected=0,total=0;
        bool sent=false,done=false;
    };
    std::vector<std::unique_ptr<Source>> sources;
    std::chrono::steady_clock::time_point began{};
public:
    bool loading=false;
    void Close(){sources.clear();loading=false;}
    bool Start(bool force=false);
    bool StartSources(const std::vector<Endpoint>& endpoints);
    void Update();
};
}
