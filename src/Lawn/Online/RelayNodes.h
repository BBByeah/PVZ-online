#pragma once
#include "RoomDirectory.h"
#include "RelayCatalog.h"
#include <fstream>

namespace RelayNodes {
inline const Node* Find(const std::string& id) {for(const auto& n:Catalog())if(n.id==id)return &n;return nullptr;}
inline const Node* Find(const Endpoint& endpoint) {for(const auto& n:Catalog())if(Same(n.endpoint,endpoint))return &n;return nullptr;}
inline Endpoint LoadChoice(const std::string& path) {
    std::ifstream in(path,std::ios::binary);std::string value(261,'\0');in.read(&value[0],value.size());value.resize(size_t(in.gcount()));
    Endpoint endpoint;return ParseEndpoint(value,endpoint)?endpoint:Catalog().front().endpoint;
}
inline bool SaveChoice(const std::string& path,const Endpoint& endpoint) {
    Endpoint validated;if(!ParseEndpoint(endpoint.Text(),validated))return false;
    std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<endpoint.Text();out.close();return bool(out);
}
inline std::string ShareCode(const Endpoint& endpoint,const std::string& code) {
    const auto* node=Find(endpoint);return node?node->id+"-"+code:code;
}
// Four characters always mean the explicitly selected endpoint. A node prefix
// resolves exactly one configured node; never search other servers for a code.
inline bool ResolveCode(const std::string& input,const Endpoint& selected,Endpoint& target,std::string& code) {
    std::string value=input;for(char& c:value)if(c>='a'&&c<='z')c=char(c+'A'-'a');
    target=selected;const auto dash=value.find('-');
    if(dash!=std::string::npos) {
        const auto* node=Find(value.substr(0,dash));if(!node)return false;
        target=node->endpoint;value=value.substr(dash+1);
    }
    if(value.size()!=4)return false;
    for(char c:value)if(std::string("ABCDEFGHJKLMNPQRSTUVWXYZ23456789").find(c)==std::string::npos)return false;
    code=value;return true;
}
struct Capacity {
    int rooms=0,roomLimit=0,players=0,connections=0,connectionLimit=0;
    bool detailed=false,acceptNew=true,busy=false;
    double score=0;
    uint16_t weight=100;
    uint32_t sampleAgeMs=0,sentBitsPerSecond=0,budgetBitsPerSecond=0;
    uint64_t configRevision=0;
};
inline bool DecodeCapacity(const uint8_t* p,int size,Capacity& out) {
    if(size!=16&&size!=44)return false;NetProto::Reader r(p,size);
    const auto version=r.U16();if((version!=1&&version!=2)||(version==1&&size!=16)||(version==2&&size!=44)||r.U16()!=NetProto::PROTOCOL_VERSION)return false;
    Capacity v;v.rooms=r.U16();v.roomLimit=r.U16();v.players=r.U16();v.connections=r.U16();v.connectionLimit=r.U16();const auto flags=r.U16();
    if(!v.roomLimit||!v.connectionLimit||v.rooms>v.roomLimit||v.connections>v.connectionLimit||v.players>v.connectionLimit||(flags&~3))return false;
    if(version==2){const auto state=r.U16();v.score=r.U16()/1000.0;v.weight=r.U16();const auto reserved=r.U16();
        v.sampleAgeMs=r.U32();v.sentBitsPerSecond=r.U32();v.budgetBitsPerSecond=r.U32();v.configRevision=r.U32();v.configRevision|=uint64_t(r.U32())<<32;
        if((state&~11)||reserved||!v.weight||v.weight>1000||!v.configRevision||v.sampleAgeMs>60000)return false;
        v.detailed=true;v.acceptNew=(state&1)!=0;v.busy=(state&2)!=0;
    }
    out=v;return !r.Overflowed();
}
class Probe {
    NetLink link;
    RoomDirectory::Client legacy;
    Endpoint endpoint;
    std::chrono::steady_clock::time_point began{},phaseBegan{},last{};
    bool sent=false;
    int phase=0;
    void Fallback() {
        link.Close();sent=false;phaseBegan=std::chrono::steady_clock::now();++phase;
        if(phase==1){if(!link.Connect(endpoint.host.c_str(),endpoint.port))Fallback();}
        else if(phase==2){if(!legacy.Refresh(endpoint.host,0,endpoint.port))state=UNAVAILABLE;}
        else state=UNAVAILABLE;
    }
public:
    enum State {IDLE,LOADING,READY,LEGACY,UNAVAILABLE} state=IDLE;
    Capacity capacity;
    int latencyMs=0;
    bool Refresh(const Endpoint& value) {
        const auto now=std::chrono::steady_clock::now();
        if(state==LOADING||now-last<std::chrono::seconds(2))return false;
        link.Close();legacy.Close();endpoint=value;began=phaseBegan=last=now;sent=false;phase=0;capacity={};state=LOADING;
        if(!link.Connect(endpoint.host.c_str(),endpoint.port))Fallback();return true;
    }
    void Update() {
        if(state!=LOADING)return;
        const auto now=std::chrono::steady_clock::now();const auto elapsed=now-began;
        if(elapsed>std::chrono::seconds(5)){link.Close();legacy.Close();state=UNAVAILABLE;return;}
        if(phase==2){legacy.Update();if(legacy.state==RoomDirectory::Client::READY){state=LEGACY;latencyMs=int(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());}
            else if(legacy.state==RoomDirectory::Client::UNAVAILABLE)state=UNAVAILABLE;return;}
        if(link.IsConnected()&&!sent){const uint8_t frame[]={0x1d,0xf0,2,0,uint8_t(phase==0?2:1),0};if(!link.Send(frame,sizeof(frame))){Fallback();return;}sent=true;}
        NetLink::Packet packet;
        while(link.Poll(packet)) {
            if(packet.mSize<4||packet.mData[0]!=0x0e||packet.mData[1]!=0xf0||packet.mData[2]!=packet.mSize-4||packet.mData[3]!=0||!DecodeCapacity(packet.mData+4,packet.mSize-4,capacity)) {Fallback();return;}
            latencyMs=int(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());state=READY;link.Close();return;
        }
        if(link.GetState()==NetLink::State::FAILED||now-phaseBegan>std::chrono::milliseconds(650))Fallback();
    }
};
}
