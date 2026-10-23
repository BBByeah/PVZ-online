#include "../Lawn/Online/WatchWire.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    using namespace WatchWire;
    Header h;h.epoch=10;h.revision=1;h.sequence=7;h.seat=2;
    Bytes config{1,2,3},state{4,5,6,7},full=Pack(h,config,state),dynamic=Pack(h,{},state),out;
    Header parsed;Cache seats[7];
    for(int i=1;i<=6;++i){
        CHECK(!seats[i].Accept(dynamic,out,parsed));
        CHECK(seats[i].Accept(full,out,parsed));CHECK(out==full);
        CHECK(seats[i].Accept(dynamic,out,parsed));CHECK(out==full);
    }
    seats[3].Reset();CHECK(!seats[3].Accept(dynamic,out,parsed));CHECK(seats[4].Accept(dynamic,out,parsed));
    ++h.revision;CHECK(!seats[4].Accept(Pack(h,{},state),out,parsed));
    CHECK(seats[4].Accept(Pack(h,Bytes{9},state),out,parsed));
    ++h.epoch;CHECK(!seats[4].Accept(Pack(h,{},state),out,parsed));
    for(size_t n=0;n<full.size();++n){Bytes truncated(full.begin(),full.begin()+n);CHECK(!Parse(truncated,parsed));}
    auto bad=full;bad[36]=1;CHECK(!Parse(bad,parsed));bad=full;bad[28]=255;CHECK(!Parse(bad,parsed));
    Schedule s;CHECK(s.Due(100));s.Sent(100);CHECK(!s.Due(1099));
    s.Edit();CHECK(!s.Due(1099));CHECK(s.Due(1100));s.Sent(1100);
    s.Edit();s.Edit();CHECK(!s.Due(2099));CHECK(s.Due(2100));s.Sent(2100);
    CHECK(!s.Due(7099));CHECK(s.Due(7100));CHECK(s.Due(3100,true));
    s.Sent(0xffffff00u);s.Edit();CHECK(!s.Due(0x100u));CHECK(s.Due(0x300u));
    std::puts("PWV2: six independent caches, late join, revision/epoch changes, malformed packets and 1s coalescing passed");
}
