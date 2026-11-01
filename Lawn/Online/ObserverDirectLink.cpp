#include "../Automation/Diagnostics.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ObserverDirectLink.h"
#include "ObserverTransfer.h"
#include <array>
#include <bitset>
#include <deque>
#include <random>
#include <cstdio>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
namespace ObserverDirect {
struct DirectLink::State {
 uint32_t nextFrameSequence=0;intptr_t sock=-1;bool hello=false,endpoint=false;uint8_t helloToken[16]{};
 uint32_t relayIp=0,publicIp=0;uint16_t relayPort=0,publicPort=0;uint64_t helloAt=0,now=0,budgetAt=0;unsigned helloCount=0;size_t spent=0;
 struct Peer{
  bool used=false,ready=false,reported=false,producer=false;uint8_t seat=0,token[16]{};uint32_t ip=0;uint16_t port=0;
  Context context,outContext;uint64_t openedAt=0,expires=0,probeAt=0,rxAt=0,pingAt=0,queueAt=0,rateAt=0;uint64_t ackDeadline=0;uint32_t ackSequence=0;uint32_t probes=0,rtt=0,sequence=0,waitingAck=0,missed=0;size_t spent=0;
  std::vector<uint8_t> outgoing;size_t sendAt=0;uint32_t sendSeq=0,sendTick=0,sendCrc=0;
  std::vector<uint8_t> incoming;std::bitset<1024> parts;uint32_t inSeq=0,inTick=0,inCrc=0,lastSeq=0;uint16_t partCount=0;uint64_t inAt=0;
 };
 std::array<Peer,MAX_PROBES> peers;std::deque<Frame> frames;
 int Send(const void*p,int n,uint32_t ip,uint16_t port){if(sock<0||TestDiagnostics::blockTestUdp)return -1;sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons(port);uint8_t b[]={uint8_t(ip),uint8_t(ip>>8),uint8_t(ip>>16),uint8_t(ip>>24)};std::memcpy(&a.sin_addr,b,4);
#ifdef _WIN32
  int result=sendto(SOCKET(sock),static_cast<const char*>(p),n,0,reinterpret_cast<sockaddr*>(&a),sizeof(a));if(result>0){TestDiagnostics::counters.udpTx+=result;if(n>=65&&static_cast<const uint8_t*>(p)[4]==UDP_DATA)TestDiagnostics::counters.udpWatchTx+=result;}return result;
#else
  return int(sendto(int(sock),p,n,0,reinterpret_cast<sockaddr*>(&a),sizeof(a)));
#endif
 }
 int Receive(uint8_t*p,int cap,uint32_t&ip,uint16_t&port){if(TestDiagnostics::blockTestUdp)return -1;sockaddr_in a{};
#ifdef _WIN32
  int len=sizeof(a);int n=recvfrom(SOCKET(sock),reinterpret_cast<char*>(p),cap,0,reinterpret_cast<sockaddr*>(&a),&len);
#else
  socklen_t len=sizeof(a);int n=int(recvfrom(int(sock),p,cap,MSG_TRUNC,reinterpret_cast<sockaddr*>(&a),&len));
#endif
  if(n>0){TestDiagnostics::counters.udpRx+=n;if(n>=65&&p[4]==UDP_DATA)TestDiagnostics::counters.udpWatchRx+=n;}if(n<0||n>cap)return -1;auto*b=reinterpret_cast<uint8_t*>(&a.sin_addr);ip=Get32(b);port=ntohs(a.sin_port);return n;
 }
 void Control(Peer&p,UDPOp op,uint32_t seq){uint8_t b[32];PutPacket(b,op,p.token,seq,0,0);Send(b,32,p.ip,p.port);}
 void Report(Peer&p,bool ok,std::vector<Result>*out){if(!p.reported&&out){Result r;r.peerSeat=p.seat;r.ok=ok;r.rttMs=p.rtt;std::memcpy(r.token,p.token,16);out->push_back(r);}p.reported=true;}
 bool Live(const Peer&p)const{return p.used&&p.ready&&now>=p.rxAt&&now-p.rxAt<5000&&now<p.expires&&p.rtt<=800&&p.missed<3;}
 void Data(Peer&p,const uint8_t*b,int n){
  if((p.context.Valid()&&p.producer)||n<65||n>1200||b[61]||b[62]||b[63]||Get16(b+25)!=n-64)return;
  Context c{Get32(b+43),Get32(b+47),Get32(b+51),Get32(b+55),b[59],b[60]};if(p.context.Valid()?!(c==p.context):(!c.Valid()||c.src!=p.seat))return;
  uint32_t seq=Get32(b+21),total=Get32(b+31),crc=Get32(b+35),tick=Get32(b+39);uint16_t part=Get16(b+27),count=Get16(b+29);
  if(!seq||!ObserverTransfer::Newer(seq,p.lastSeq)||total<5||total>MAX_FRAME||count!=(total+DATA_BYTES-1)/DATA_BYTES||part>=count||count>1024||size_t(n-64)!=std::min(DATA_BYTES,size_t(total)-size_t(part)*DATA_BYTES))return;
  if(p.inSeq!=seq){if(!p.incoming.empty())++TestDiagnostics::counters.dropped;if(p.inSeq&&!ObserverTransfer::Newer(seq,p.inSeq))return;p.incoming.assign(total,0);p.parts.reset();p.inSeq=seq;p.inTick=tick;p.inCrc=crc;p.partCount=count;p.inAt=now;}
  if(total!=p.incoming.size()||crc!=p.inCrc||tick!=p.inTick||count!=p.partCount||now-p.inAt>20000){p.incoming.clear();p.parts.reset();return;}
  if(p.parts.test(part))return;std::memcpy(p.incoming.data()+size_t(part)*DATA_BYTES,b+64,n-64);p.parts.set(part);
  if(p.parts.count()!=count)return;
  if(ObserverTransfer::Crc(p.incoming.data(),p.incoming.size())!=crc){++TestDiagnostics::counters.crcFailed;p.incoming.clear();p.parts.reset();return;}
  if(p.context.Valid())++TestDiagnostics::counters.complete;if(frames.size()>=2){++TestDiagnostics::counters.dropped;frames.pop_front();}frames.push_back({c,seq,tick,std::move(p.incoming)});p.lastSeq=seq;p.parts.reset();Control(p,UDP_FRAME_ACK,seq);
 }
};
DirectLink::DirectLink():state(new State){}DirectLink::~DirectLink(){Close();}
bool DirectLink::Opened()const{return state->sock!=-1;}
bool DirectLink::Open(){if(Opened())return true;
#ifdef _WIN32
 WSADATA w;if(WSAStartup(MAKEWORD(2,2),&w))return false;SOCKET s=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(s==INVALID_SOCKET){WSACleanup();return false;}u_long on=1;if(ioctlsocket(s,FIONBIO,&on)){closesocket(s);WSACleanup();return false;}state->sock=intptr_t(s);
#else
 int s=socket(AF_INET,SOCK_DGRAM,0);if(s<0)return false;if(fcntl(s,F_SETFL,fcntl(s,F_GETFL,0)|O_NONBLOCK)<0){close(s);return false;}state->sock=s;
#endif
 int receiveBytes=256*1024,sendBytes=64*1024;
#ifdef _WIN32
 setsockopt(SOCKET(state->sock),SOL_SOCKET,SO_RCVBUF,reinterpret_cast<const char*>(&receiveBytes),sizeof(receiveBytes));
 setsockopt(SOCKET(state->sock),SOL_SOCKET,SO_SNDBUF,reinterpret_cast<const char*>(&sendBytes),sizeof(sendBytes));
#else
 setsockopt(int(state->sock),SOL_SOCKET,SO_RCVBUF,&receiveBytes,sizeof(receiveBytes));
 setsockopt(int(state->sock),SOL_SOCKET,SO_SNDBUF,&sendBytes,sizeof(sendBytes));
#endif
 sockaddr_in address{};address.sin_family=AF_INET;
 if(bind(SOCKET(state->sock),reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0){Close();return false;}
 return true;}
void DirectLink::Close(){if(Opened()){
#ifdef _WIN32
 closesocket(SOCKET(state->sock));WSACleanup();
#else
 close(int(state->sock));
#endif
 }state.reset(new State);}
void DirectLink::ClearPeers(){for(auto&p:state->peers)p={};state->frames.clear();}
void DirectLink::Cancel(uint32_t watch){for(auto&p:state->peers)if(p.used&&p.context.watch==watch){state->Control(p,UDP_STOP,0);p={};}for(auto it=state->frames.begin();it!=state->frames.end();)if(it->context.watch==watch)it=state->frames.erase(it);else++it;}
void DirectLink::Cancel(const Context&c){for(auto&p:state->peers)if(p.used&&p.context==c){state->Control(p,UDP_STOP,0);p={};}for(auto it=state->frames.begin();it!=state->frames.end();)if(it->context==c)it=state->frames.erase(it);else++it;}
bool DirectLink::StartHello(const char*host,uint16_t port,const uint8_t*token,uint64_t now){if(!Opened()||!host||!*host||!port||!token)return false;addrinfo h{};h.ai_family=AF_INET;h.ai_socktype=SOCK_DGRAM;addrinfo*r=nullptr;if(getaddrinfo(host,nullptr,&h,&r)||!r)return false;auto&x=*state;x.relayIp=Get32(reinterpret_cast<uint8_t*>(&reinterpret_cast<sockaddr_in*>(r->ai_addr)->sin_addr));freeaddrinfo(r);x.relayPort=port;std::memcpy(x.helloToken,token,16);x.hello=true;x.endpoint=false;x.helloAt=now;x.helloCount=0;return true;}
bool DirectLink::HasEndpoint()const{return state->endpoint;}uint32_t DirectLink::PublicIp()const{return state->publicIp;}uint16_t DirectLink::PublicPort()const{return state->publicPort;}
bool DirectLink::StartProbe(uint8_t seat,uint32_t ip,uint16_t port,const uint8_t*token,uint64_t now,const Context*c,bool producer,uint16_t ttl){
 if(!Opened()||seat<1||seat>6||!ip||!port||!token||!ttl||ttl>120||(c&&!c->Valid()))return false;auto&x=*state;
 for(auto&p:x.peers)if(p.used&&!std::memcmp(p.token,token,16))return true;
 State::Peer*slot=nullptr;
 for(auto&p:x.peers)if(p.used&&c&&p.context==*c){slot=&p;break;}
 if(!c)for(auto&p:x.peers)if(p.used&&!p.context.Valid()&&p.seat==seat){slot=&p;break;}
 if(!slot)for(auto&p:x.peers)if(!p.used||now>=p.expires){slot=&p;break;}if(!slot)return false;
 if(producer&&ActiveProducers()>=3&&!(slot->used&&slot->producer))return false;
 auto&p=*slot;p={};p.used=true;p.producer=producer;p.seat=seat;p.ip=ip;p.port=port;std::memcpy(p.token,token,16);if(c)p.context=*c;p.openedAt=now;p.expires=now+uint64_t(ttl)*1000;p.probeAt=now>=150?now-150:0;return true;
}
size_t DirectLink::ActiveProducers()const{size_t n=0;for(auto&p:state->peers)if(p.used&&p.producer)++n;return n;}
bool DirectLink::Ready(const Context&c)const{for(auto&p:state->peers)if(p.context==c&&state->Live(p))return true;return false;}
uint32_t DirectLink::Rtt(const Context&c)const{for(auto&p:state->peers)if(p.context==c)return p.rtt;return 0;}
bool DirectLink::Writable(const Context&c)const{for(auto&p:state->peers)if(p.context==c&&p.producer&&state->Live(p)&&p.outgoing.empty())return true;return false;}
uint64_t DirectLink::Interval(const Context&c)const{for(auto&p:state->peers)if(p.context==c)return std::clamp<uint64_t>(500+uint64_t(p.rtt)*2+uint64_t(p.missed)*500,500,2000);return 2000;}
bool DirectLink::Queue(const Context&c,const std::vector<uint8_t>&data,uint32_t tick,uint64_t now){if(data.size()<5||data.size()>MAX_FRAME)return false;for(auto&p:state->peers)if(p.context==c&&p.producer&&state->Live(p)&&p.outgoing.empty()){
 if(!p.ackDeadline)p.ackDeadline=now+2500;
 TestDiagnostics::counters.queued+=data.size();p.outContext=c;p.outgoing=data;p.sendAt=0;if(!++state->nextFrameSequence)++state->nextFrameSequence;p.sendSeq=state->nextFrameSequence;p.sendTick=tick;p.sendCrc=ObserverTransfer::Crc(data.data(),data.size());p.queueAt=now;p.waitingAck=p.sendSeq;return true;}return false;}
bool DirectLink::RoomReady(uint8_t seat)const{for(auto&p:state->peers)if(!p.context.Valid()&&p.seat==seat&&state->Live(p))return true;return false;}
uint32_t DirectLink::RoomRtt(uint8_t seat)const{for(auto&p:state->peers)if(!p.context.Valid()&&p.seat==seat&&state->Live(p))return p.rtt;return 0;}
void DirectLink::DropRoomPeer(uint8_t seat){for(auto&p:state->peers)if(p.used&&!p.context.Valid()&&p.seat==seat){state->Control(p,UDP_STOP,0);p={};}}
bool DirectLink::QueueRoom(uint8_t seat,const Context&c,const std::vector<uint8_t>&data,uint32_t tick,uint64_t now){
 if(!c.Valid()||data.size()<5||data.size()>MAX_FRAME)return false;
 for(auto&p:state->peers)if(p.seat==seat&&!p.context.Valid()&&state->Live(p)&&p.outgoing.empty()&&!p.waitingAck){
  p.outContext=c;p.outgoing=data;p.sendAt=0;p.sendSeq=++state->nextFrameSequence;if(!p.sendSeq)p.sendSeq=++state->nextFrameSequence;
  p.sendTick=tick;p.sendCrc=ObserverTransfer::Crc(data.data(),data.size());p.queueAt=now;p.waitingAck=p.sendSeq;
  p.ackDeadline=now+20000;return true;
 }return false;
}
bool DirectLink::Take(Frame&out){if(state->frames.empty())return false;out=std::move(state->frames.front());state->frames.pop_front();return true;}
void DirectLink::Pump(uint64_t now,std::vector<Result>*results){if(!Opened())return;auto&x=*state;x.now=now;uint8_t b[DATAGRAM_SIZE+1];
 if(x.hello&&!x.endpoint&&now>=x.helloAt){if(x.helloCount>=6)x.hello=false;else{PutPacket(b,UDP_HELLO,x.helloToken,0,0,0);x.Send(b,32,x.relayIp,x.relayPort);++x.helloCount;x.helloAt=now+500;}}
 for(auto&p:x.peers){if(!p.used)continue;
  if(p.ready&&p.ackDeadline&&now>=p.ackDeadline){++p.missed;p.ackDeadline=now+2500;}
  if(now>=p.expires||(p.ready&&now-p.rxAt>=5000)||p.missed>=3){if(!p.ready)x.Report(p,false,results);p={};continue;}
  if(!p.ready){if(now-p.openedAt>=3000){x.Report(p,false,results);p={};continue;}if(!p.probes||now-p.probeAt>=150){x.Control(p,UDP_PROBE,p.probes++);p.probeAt=now;}}
  else if(now-p.pingAt>=1000){x.Control(p,UDP_PING,uint32_t(now));p.pingAt=now;}
 }
 for(int i=0;i<64;++i){uint32_t ip=0;uint16_t port=0;int n=x.Receive(b,int(sizeof(b)),ip,port);if(n<=0)break;
  if(n==32){auto in=ParsePacket(b,n);if(!in.ok)continue;
   if(in.op==UDP_HELLO_ACK&&x.hello&&ip==x.relayIp&&port==x.relayPort&&!std::memcmp(in.token,x.helloToken,16)&&in.ip&&in.port){x.endpoint=true;x.hello=false;x.publicIp=in.ip;x.publicPort=in.port;continue;}
   for(auto&p:x.peers){if(!p.used||p.ip!=ip||p.port!=port||std::memcmp(p.token,in.token,16)||now>=p.expires)continue;
    if(in.op==UDP_STOP){p={};break;}
    if(in.op==UDP_PROBE||in.op==UDP_PROBE_ACK){if(in.op==UDP_PROBE)x.Control(p,UDP_PROBE_ACK,in.seq);if(!p.ready){p.rtt=uint32_t(std::min<uint64_t>(now-p.probeAt,60000));p.ready=true;p.rxAt=now;x.Report(p,p.rtt<=800,results);}else p.rxAt=now;}
    else if(p.ready&&in.op==UDP_PING){p.rxAt=now;x.Control(p,UDP_PONG,in.seq);}
    else if(p.ready&&in.op==UDP_PONG){p.rxAt=now;uint32_t r=uint32_t(now)-in.seq;if(r<5000)p.rtt=(p.rtt*3+r)/4;}
    else if(p.ready&&in.op==UDP_FRAME_ACK&&ObserverTransfer::Newer(in.seq,p.ackSequence)&&!ObserverTransfer::Newer(in.seq,p.sendSeq)){p.rxAt=now;p.missed=0;p.ackSequence=in.seq;p.ackDeadline=in.seq==p.sendSeq?0:now+2500;if(in.seq==p.waitingAck)p.waitingAck=0;}
    break;
   }
  }else if(n>=65&&n<=1200&&!std::memcmp(b,"PZP2",4)&&b[4]==UDP_DATA){for(auto&p:x.peers)if(p.used&&p.ready&&p.ip==ip&&p.port==port&&!std::memcmp(p.token,b+5,16)&&now<p.expires){x.Data(p,b,n);break;}}
 }
 if(now-x.budgetAt>=1000){x.budgetAt=now;x.spent=0;}
 for(auto&p:x.peers){if(!x.Live(p)||p.outgoing.empty())continue;if(now-p.rateAt>=1000){p.rateAt=now;p.spent=0;}
  for(int quota=0;quota<8&&p.sendAt<p.outgoing.size();++quota){size_t n=std::min(DATA_BYTES,p.outgoing.size()-p.sendAt);if(p.spent+n+64>80*1024||x.spent+n+64>240*1024)break;
   std::memset(b,0,64);std::memcpy(b,"PZP2",4);b[4]=UDP_DATA;std::memcpy(b+5,p.token,16);Put32(b+21,p.sendSeq);Put16(b+25,uint16_t(n));Put16(b+27,uint16_t(p.sendAt/DATA_BYTES));Put16(b+29,uint16_t((p.outgoing.size()+DATA_BYTES-1)/DATA_BYTES));Put32(b+31,uint32_t(p.outgoing.size()));Put32(b+35,p.sendCrc);Put32(b+39,p.sendTick);Put32(b+43,p.outContext.watch);Put32(b+47,p.outContext.stream);Put32(b+51,p.outContext.epoch);Put32(b+55,p.outContext.term);b[59]=p.outContext.src;b[60]=p.outContext.dst;std::memcpy(b+64,p.outgoing.data()+p.sendAt,n);
   if(x.Send(b,int(n+64),p.ip,p.port)!=int(n+64))break;p.sendAt+=n;p.spent+=n+64;x.spent+=n+64;
  }if(p.sendAt==p.outgoing.size()){TestDiagnostics::Line("watch_udp_sent","peer="+std::to_string(p.seat)+" subscription="+std::to_string(p.outContext.watch)+" bytes="+std::to_string(p.outgoing.size())+" queue_ms="+std::to_string(now-p.queueAt));p.outgoing.clear();p.sendAt=0;}
 }
}
}

namespace ObserverDirect {
uint16_t DirectLink::LocalPort()const{if(!Opened())return 0;sockaddr_in address{};
#ifdef _WIN32
 int n=sizeof(address);if(getsockname(SOCKET(state->sock),reinterpret_cast<sockaddr*>(&address),&n))return 0;
#else
 socklen_t n=sizeof(address);if(getsockname(int(state->sock),reinterpret_cast<sockaddr*>(&address),&n))return 0;
#endif
 return ntohs(address.sin_port);}
bool DirectLink::Token(uint8_t*out)try{if(!out)return false;std::random_device random;for(int i=0;i<16;i+=4)Put32(out+i,random());return true;}catch(...){return false;}
bool DirectLink::StartLanProbe(uint8_t peer,const char*text,uint16_t port,const uint8_t*token,uint64_t now,const Context&c,bool producer){unsigned a,b,d,e;char tail;if(!text||std::sscanf(text,"%u.%u.%u.%u%c",&a,&b,&d,&e,&tail)!=4||a>255||b>255||d>255||e>255)return false;return StartProbe(peer,a|(b<<8)|(d<<16)|(e<<24),port,token,now,&c,producer);}
}
