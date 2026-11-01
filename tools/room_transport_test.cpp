#include "../Lawn/Online/ObserverDirectLink.h"
#include "../Lawn/Online/WatchWire.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include "../Lawn/Automation/Diagnostics.h"
namespace TestDiagnostics { void Line(const std::string&,const std::string&){} }
using namespace ObserverDirect;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main()try{
 std::array<DirectLink,6> links;for(auto&l:links)check(l.Open(),"socket");
 uint64_t now=10000;
 for(int i=1;i<6;++i){uint8_t token[16]{};check(DirectLink::Token(token),"token");
  check(links[0].StartProbe(uint8_t(i+1),0x0100007f,links[i].LocalPort(),token,now),"host probe");
  check(links[i].StartProbe(1,0x0100007f,links[0].LocalPort(),token,now),"peer probe");
 }
 auto pump=[&]{now+=10;for(auto&l:links)l.Pump(now,nullptr);};
 for(int t=0;t<100;++t)pump();
 for(int i=1;i<6;++i)check(links[0].RoomReady(uint8_t(i+1))&&links[i].RoomReady(1),"five room peers");
 for(int i=1;i<6;++i){
  std::vector<uint8_t> bytes(i==1?MAX_FRAME:4096);for(size_t j=0;j<bytes.size();++j)bytes[j]=uint8_t(j*73+19);
  Context c{123,1,9,1,1,uint8_t(i+1)};
  check(links[0].QueueRoom(uint8_t(i+1),c,bytes,99,now),"queue");
  Frame f;bool got=false;for(int t=0;t<1900&&!got;++t){pump();got=links[i].Take(f);}
  check(got&&f.context==c&&f.data==bytes,"complete large payload");
  for(int t=0;t<10;++t)pump();
  Context back{456,1,9,1,uint8_t(i+1),1};std::vector<uint8_t> reply(1024,42);
  check(links[i].QueueRoom(1,back,reply,100,now),"duplex queue");got=false;
  for(int t=0;t<100&&!got;++t){pump();got=links[0].Take(f);}check(got&&f.context==back&&f.data==reply,"duplex result");
 }
 WatchWire::Header h;h.epoch=1;h.revision=1;h.seat=1;h.subscription=777;
 auto b=WatchWire::Pack(h,{1,2},{3,4});WatchWire::Header parsed;check(WatchWire::Parse(b,parsed)&&parsed.subscription==777,"PWV2 subscription");
 WatchWire::Cache cache;WatchWire::Bytes out;check(cache.Accept(b,out,parsed),"config cache");
 b=WatchWire::Pack(h,{}, {5,6});check(cache.Accept(b,out,parsed)&&parsed.subscription==777,"dynamic cache");
 links[0].DropRoomPeer(2);check(!links[0].RoomReady(2)&&links[0].RoomReady(3),"peer scoped removal");
 std::cout<<"PASS: five peers, 1 MiB envelope, bidirectional data, subscription, config cache, scoped removal\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
