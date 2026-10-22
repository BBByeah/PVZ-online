#pragma once
#include <string>
#include <cstdint>

namespace PrivateEndpoint {
struct Address { std::string host, code; uint16_t port=27777; };
inline bool Parse(std::string input, std::string code, Address& out) {
    Address result;
    auto slash=input.find('/');
    if(slash!=std::string::npos) {
        if(input.find('/',slash+1)!=std::string::npos)return false;
        code=input.substr(slash+1);input.resize(slash);
    }
    auto colon=input.find(':');
    if(colon!=std::string::npos) {
        const auto digits=input.substr(colon+1);input.resize(colon);
        if(digits.empty()||digits.size()>5)return false;
        unsigned port=0;for(char c:digits){if(c<'0'||c>'9')return false;port=port*10+c-'0';}
        if(port<1||port>65535)return false;result.port=uint16_t(port);
    }
    if(input.empty()||input.size()>63||input.front()=='-'||input.back()=='-')return false;
    for(char c:input)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'))return false;
    for(char& c:code)if(c>='a'&&c<='z')c=char(c-'a'+'A');
    if(!code.empty()) {
        if(code.size()!=4)return false;
        for(char c:code)if(std::string("ABCDEFGHJKLMNPQRSTUVWXYZ23456789").find(c)==std::string::npos)return false;
    }
    result.host=input;result.code=code;out=result;return true;
}
inline std::string Invite(const std::string& host,uint16_t port,const std::string& code) {
    return host+":"+std::to_string(port)+(code.empty()?"":"/"+code);
}
}
