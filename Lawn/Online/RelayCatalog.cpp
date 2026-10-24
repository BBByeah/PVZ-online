#include "RelayCatalog.h"
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <cstring>

extern "C" int ed25519_verify(const unsigned char*,const unsigned char*,size_t,const unsigned char*);

namespace RelayNodes {
namespace {
// D003 release anchor: generated once by the offline maintenance tool. Public.
const uint8_t catalogKey[32]={ 0x13,0x22,0x59,0x8f,0x10,0xfc,0xec,0x17,0x62,0x88,0x3b,0xc5,0x9d,0x72,0x81,0xba,0xf7,0x4e,0x64,0xa8,0xd4,0xd2,0xfb,0x77,0x59,0xd6,0x77,0x51,0xa0,0x96,0xdd,0x00 };
struct Store {
    std::vector<Node> nodes=Defaults();
    std::vector<uint8_t> blob;
    uint64_t revision=0,expires=0;
    std::string path;
    bool cacheFailed=false;
    std::chrono::steady_clock::time_point lastAttempt{};
    size_t rotation=0;
};
Store& Shared(){static Store value;return value;}
bool CanonicalS(const uint8_t* s) {
    static const uint8_t order[32]={0xed,0xd3,0xf5,0x5c,0x1a,0x63,0x12,0x58,0xd6,0x9c,0xf7,0xa2,0xde,0xf9,0xde,0x14,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x10};
    for(int i=31;i>=0;--i){if(s[i]<order[i])return true;if(s[i]>order[i])return false;}return false;
}
bool SaveCache(const std::string& path,const std::vector<uint8_t>& blob) {
    if(path.empty())return true;
    const auto target=std::filesystem::u8path(path),temporary=std::filesystem::u8path(path+".tmp");
    {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file)return false;file.write(reinterpret_cast<const char*>(blob.data()),blob.size());file.flush();if(!file)return false;}
    std::error_code error;std::filesystem::rename(temporary,target,error);
    if(error){std::filesystem::remove(temporary,error);return false;}return true;
}
}
const std::vector<Node>& Catalog(){return Shared().nodes;}
uint64_t CatalogRevision(){return Shared().revision;}
bool CatalogCacheWriteFailed(){return Shared().cacheFailed;}

bool ApplyCatalogBlob(const std::vector<uint8_t>& blob,bool cached) {
    if(blob.size()<90||blob.size()>MAX_CATALOG_BYTES)return false;
    const size_t bodySize=blob.size()-64;const uint8_t* b=blob.data();
    if(std::memcmp(b,"PVZNODE1",8)||!CanonicalS(b+bodySize+32)||!ed25519_verify(b+bodySize,b,bodySize,catalogKey))return false;
    NetProto::Reader r(b+8,int(bodySize-8));
    auto u64=[&](){uint64_t low=r.U32();return low|(uint64_t(r.U32())<<32);};
    const uint64_t revision=u64(),expires=u64();const int count=r.U16();
    if(!revision||revision<Shared().revision||!expires||expires>uint64_t(INT64_MAX)||(!cached&&uint64_t(std::time(nullptr))>=expires)||count<1||count>int(MAX_CATALOG_NODES))return false;
    size_t off=26;std::vector<Node> nodes;std::set<std::string> ids,endpoints;
    for(int i=0;i<count;++i){
        if(off+10>bodySize)return false;
        auto at=[&](int n){return uint16_t(b[off+n]|uint16_t(b[off+n+1])<<8);};
        const auto id=at(0),flags=at(2),weight=at(4),port=at(6),length=at(8);off+=10;
        if(!id||(flags&~3)||!weight||weight>1000||!port||!length||length>253||off+length>bodySize)return false;
        Endpoint endpoint;const std::string host(reinterpret_cast<const char*>(b+off),length);off+=length;
        if(!ParseEndpoint(host+":"+std::to_string(port),endpoint))return false;
        auto lower=host;for(char& c:lower)if(c>='A'&&c<='Z')c=char(c+'a'-'A');
        const std::string textId=std::to_string(id);
        if(!ids.insert(textId).second||!endpoints.insert(lower+":"+std::to_string(port)).second)return false;
        nodes.push_back({textId,"","",endpoint,weight,(flags&1)!=0,(flags&2)!=0});
    }
    if(off!=bodySize)return false;
    for(const auto& old:Shared().nodes){
        auto found=std::find_if(nodes.begin(),nodes.end(),[&](const Node& n){return old.id==n.id;});
        if(found==nodes.end()||!Same(old.endpoint,found->endpoint))return false;
    }
    if(revision==Shared().revision)return blob==Shared().blob;
    Shared().nodes=std::move(nodes);Shared().blob=blob;Shared().revision=revision;Shared().expires=expires;
    if(!cached)Shared().cacheFailed=!SaveCache(Shared().path,blob);return true;
}
void SetCatalogCachePath(const std::string& path) {
    if(path==Shared().path)return;Shared().path=path;if(path.empty())return;
    std::ifstream file(std::filesystem::u8path(path),std::ios::binary);if(!file)return;
    std::vector<uint8_t> bytes(MAX_CATALOG_BYTES+1);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());bytes.resize(size_t(file.gcount()));
    ApplyCatalogBlob(bytes,true);
}

bool CatalogDiscovery::Start(bool force) {
    Close();const auto now=std::chrono::steady_clock::now();
    if(!force&&now-Shared().lastAttempt<std::chrono::seconds(30))return false;
    Shared().lastAttempt=now;began=now;
    const auto nodes=Catalog();const auto seeds=Defaults();std::vector<Endpoint> endpoints;
    auto add=[&](const Endpoint& e){if(std::none_of(endpoints.begin(),endpoints.end(),[&](const Endpoint& n){return Same(n,e);}))endpoints.push_back(e);};
    // Rotate cached nodes alongside built-in seeds so retired seeds are not a
    // permanent dependency once the pool grows.
    for(size_t i=0;i<std::min<size_t>(2,nodes.size());++i)add(nodes[(Shared().rotation+i)%nodes.size()].endpoint);
    Shared().rotation+=2;for(const auto& seed:seeds)add(seed.endpoint);
    return StartSources(endpoints);
}
bool CatalogDiscovery::StartSources(const std::vector<Endpoint>& endpoints) {
    Close();began=std::chrono::steady_clock::now();
    for(size_t i=0;i<std::min<size_t>(4,endpoints.size());++i){const auto& endpoint=endpoints[i];auto s=std::make_unique<Source>();if(s->link.Connect(endpoint.host.c_str(),endpoint.port))sources.push_back(std::move(s));}
    loading=!sources.empty();return loading;
}
void CatalogDiscovery::Update() {
    if(!loading)return;
    if(std::chrono::steady_clock::now()-began>std::chrono::milliseconds(1800)){Close();return;}
    bool pending=false;
    for(auto& s:sources){
        if(s->done)continue;
        if(s->link.IsConnected()&&!s->sent){const uint8_t frame[]={0x1e,0xf0,2,0,1,0};if(!s->link.Send(frame,6)){s->done=true;s->link.Close();continue;}s->sent=true;}
        NetLink::Packet packet;
        while(!s->done&&s->link.Poll(packet)){
            if(packet.mSize<12||packet.mData[0]!=0x0f||packet.mData[1]!=0xf0||packet.mData[2]!=packet.mSize-4||packet.mData[3]){s->done=true;break;}
            NetProto::Reader r(packet.mData+4,packet.mSize-4);const auto version=r.U16(),index=r.U16(),total=r.U16(),length=r.U16();
            if(version!=1||index!=s->expected||total>35||!total||length>240||length!=packet.mSize-12||(s->total&&total!=s->total)||s->bytes.size()+length>MAX_CATALOG_BYTES){s->done=true;break;}
            s->total=total;++s->expected;s->bytes.insert(s->bytes.end(),packet.mData+12,packet.mData+packet.mSize);
            if(s->expected==s->total){ApplyCatalogBlob(s->bytes);s->done=true;}
        }
        if(s->link.GetState()==NetLink::State::FAILED)s->done=true;
        if(s->done)s->link.Close();else pending=true;
    }
    if(!pending)Close();
}
}
