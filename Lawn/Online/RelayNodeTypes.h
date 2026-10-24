#pragma once
#include "NetProtocol.h"
#include <algorithm>
#include <string>
#include <vector>

namespace RelayNodes {
struct Endpoint {
    std::string host;
    uint16_t port=NetProto::DEFAULT_RELAY_PORT;
    std::string Text() const {return host+(port==NetProto::DEFAULT_RELAY_PORT?"":":"+std::to_string(port));}
};
struct Node {std::string id,zh,en;Endpoint endpoint;uint16_t weight=100;bool create=true,directory=true;};
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
}
