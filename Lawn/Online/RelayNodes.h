#pragma once
#include "RoomDirectory.h"
#include <algorithm>
#include <fstream>

namespace RelayNodes {
struct Endpoint {
    std::string host;
    uint16_t port=NetProto::DEFAULT_RELAY_PORT;
    std::string Text() const {return host+(port==NetProto::DEFAULT_RELAY_PORT?"":":"+std::to_string(port));}
};
struct Node {std::string id,zh,en;Endpoint endpoint;};
inline bool ParseEndpoint(const std::string& input,Endpoint& out) {
    if(input.empty()||input.size()>259)return false;
    const auto colon=input.find(':');
    const std::string host=input.substr(0,colon);
    if(host.empty()||host.size()>253||host.front()=='.'||host.back()=='.')return false;
    for(unsigned char c:host)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'))return false;
    unsigned port=NetProto::DEFAULT_RELAY_PORT;
    if(colon!=std::string::npos) {
        const auto value=input.substr(colon+1);if(value.empty()||value.size()>5)return false;
        port=0;for(char c:value){if(c<'0'||c>'9')return false;port=port*10+unsigned(c-'0');}
        if(!port||port>65535)return false;
    }
    out={host,uint16_t(port)};return true;
}
inline bool Same(const Endpoint& a,const Endpoint& b) {
    if(a.port!=b.port||a.host.size()!=b.host.size())return false;
    for(size_t i=0;i<a.host.size();++i) {
        auto lower=[](char c){return c>='A'&&c<='Z'?char(c+'a'-'A'):c;};
        if(lower(a.host[i])!=lower(b.host[i]))return false;
    }
    return true;
}
inline std::vector<Node> Defaults() {return {
    {"1","服务器 1","Server 1",{"101.200.131.182",97}},
    {"2","服务器 2","Server 2",{"101.201.226.1",97}}
};}
inline const std::vector<Node>& Catalog() {static const auto nodes=Defaults();return nodes;}
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
struct Capacity {int rooms=0,roomLimit=0,players=0,connections=0,connectionLimit=0;};
inline bool DecodeCapacity(const uint8_t* p,int size,Capacity& out) {
    if(size!=16)return false;NetProto::Reader r(p,size);
    if(r.U16()!=1||r.U16()!=NetProto::PROTOCOL_VERSION)return false;
    Capacity v;v.rooms=r.U16();v.roomLimit=r.U16();v.players=r.U16();v.connections=r.U16();v.connectionLimit=r.U16();
    const auto flags=r.U16();
    if(!v.roomLimit||!v.connectionLimit||v.rooms>v.roomLimit||v.connections>v.connectionLimit||v.players>v.connectionLimit||(flags&~3))return false;
    out=v;return true;
}
class Probe {
    NetLink link;
    RoomDirectory::Client legacy;
    Endpoint endpoint;
    std::chrono::steady_clock::time_point began{},last{};
    bool sent=false,fallback=false;
    void Fallback() {
        link.Close();fallback=true;
        if(!legacy.Refresh(endpoint.host,0,endpoint.port)){state=UNAVAILABLE;}
    }
public:
    enum State {IDLE,LOADING,READY,LEGACY,UNAVAILABLE} state=IDLE;
    Capacity capacity;
    int latencyMs=0;
    bool Refresh(const Endpoint& value) {
        const auto now=std::chrono::steady_clock::now();
        if(state==LOADING||now-last<std::chrono::seconds(2))return false;
        link.Close();legacy.Close();endpoint=value;began=last=now;sent=fallback=false;capacity={};state=LOADING;
        if(!link.Connect(endpoint.host.c_str(),endpoint.port))state=UNAVAILABLE;
        return true;
    }
    void Update() {
        if(state!=LOADING)return;
        const auto elapsed=std::chrono::steady_clock::now()-began;
        if(elapsed>std::chrono::seconds(5)){link.Close();legacy.Close();state=UNAVAILABLE;return;}
        if(fallback) {
            legacy.Update();
            if(legacy.state==RoomDirectory::Client::READY){state=LEGACY;latencyMs=int(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());}
            else if(legacy.state==RoomDirectory::Client::UNAVAILABLE)state=UNAVAILABLE;
            return;
        }
        if(link.IsConnected()&&!sent) {
            const uint8_t frame[]={0x1d,0xf0,2,0,1,0};
            if(!link.Send(frame,sizeof(frame))){Fallback();return;}sent=true;
        }
        NetLink::Packet packet;
        while(link.Poll(packet)) {
            if(packet.mSize!=20||packet.mData[0]!=0x0e||packet.mData[1]!=0xf0||packet.mData[2]!=16||packet.mData[3]!=0||!DecodeCapacity(packet.mData+4,16,capacity)) {
                link.Close();state=UNAVAILABLE;return;
            }
            latencyMs=int(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());state=READY;link.Close();return;
        }
        if(link.GetState()==NetLink::State::FAILED||elapsed>std::chrono::seconds(1))Fallback();
    }
};
}
