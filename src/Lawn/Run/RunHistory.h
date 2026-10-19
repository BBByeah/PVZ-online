#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

class LawnApp;
namespace RunHistory {
constexpr size_t CAPACITY=50, RECORD_BYTES=79;
enum Status : uint8_t { SAVED,FAILED,COMPLETE,LEFT };
enum Setting { MODE,SCENE,DIFF,SCALE,TEMPO,ZOMBOTANY,BUFFMULT,BOSS,LADDER,ZOMBIEBUFF,SPEED,FIRSTWAVE,ENDLESS,POOL_RESERVED,BUILD_RESERVED,SETTING_COUNT };
struct Record {
    uint32_t seed=0,stage=1,joined=1,failures=0;
    uint64_t team=0,started=0,updated=0,totalDamage=0,bossDamage=0;
    uint8_t players=1,seat=1,status=SAVED,partial=0;
    uint16_t pool=0,buffs=0;
    uint8_t settings[SETTING_COUNT]{};
    bool SameRun(const Record& other) const {
        return seed==other.seed && team==other.team && settings[MODE]==other.settings[MODE] && settings[SCENE]==other.settings[SCENE];
    }
    bool Valid() const {
        static constexpr uint32_t limits[]={25,10,5,60000};
        return settings[MODE]<=3 && stage>=1 && stage<=limits[settings[MODE]] && joined>=1 && joined<=stage && players>=1 && players<=6 && seat>=1 && seat<=6 && status<=LEFT && partial<=1 &&
            started>0 && updated>=started && bossDamage<=totalDamage && totalDamage<=uint64_t(INT64_MAX) && pool<=48 &&
            settings[MODE]<=3 && settings[SCENE]<=4 && settings[DIFF]<=2 && settings[SCALE]<=3 && settings[TEMPO]<=2 && settings[ZOMBOTANY]<=1 &&
            settings[BUFFMULT]<=2 && settings[BOSS]<=2 && settings[LADDER]<=4 && settings[ZOMBIEBUFF]<=2 && settings[SPEED]<=2 && settings[FIRSTWAVE]<=3 &&
            (status!=COMPLETE || (settings[MODE]!=3 && stage==limits[settings[MODE]]));
    }
};
inline uint32_t Hash(const unsigned char* p,size_t n) {uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
inline void Put(std::vector<unsigned char>& out,uint64_t v,int bytes) {for(int i=0;i<bytes;++i)out.push_back(uint8_t(v>>(i*8)));}
inline std::vector<unsigned char> Encode(const std::vector<Record>& records) {
    std::vector<unsigned char> out;Put(out,0x38364852,4);Put(out,1,2);Put(out,records.size(),2);
    for(const auto& r:records) {
        Put(out,r.seed,4);Put(out,r.team,8);Put(out,r.started,8);Put(out,r.updated,8);Put(out,r.stage,4);Put(out,r.joined,4);
        Put(out,r.players,1);Put(out,r.seat,1);Put(out,r.status,1);Put(out,r.partial,1);Put(out,r.failures,4);
        Put(out,r.totalDamage,8);Put(out,r.bossDamage,8);Put(out,r.pool,2);Put(out,r.buffs,2);
        for(uint8_t setting:r.settings)Put(out,setting,1);
    }
    Put(out,Hash(out.data(),out.size()),4);return out;
}
inline bool Decode(const std::vector<unsigned char>& data,std::vector<Record>& records) {
    size_t pos=0;
    auto get=[&](int bytes){uint64_t v=0;for(int i=0;i<bytes;++i)v|=uint64_t(data[pos++])<<(i*8);return v;};
    if(data.size()<12 || data.size()>12+CAPACITY*RECORD_BYTES)return false;
    if(get(4)!=0x38364852 || get(2)!=1)return false;
    const auto count=get(2);
    if(count>CAPACITY || data.size()!=12+count*RECORD_BYTES)return false;
    std::vector<Record> decoded;
    for(size_t i=0;i<count;++i) {
        Record r;r.seed=uint32_t(get(4));r.team=get(8);r.started=get(8);r.updated=get(8);r.stage=uint32_t(get(4));r.joined=uint32_t(get(4));
        r.players=uint8_t(get(1));r.seat=uint8_t(get(1));r.status=uint8_t(get(1));r.partial=uint8_t(get(1));r.failures=uint32_t(get(4));
        r.totalDamage=get(8);r.bossDamage=get(8);r.pool=uint16_t(get(2));r.buffs=uint16_t(get(2));for(auto& s:r.settings)s=uint8_t(get(1));
        if(!r.Valid())return false;
        for(const auto& prior:decoded)if(prior.SameRun(r))return false;
        decoded.push_back(r);
    }
    const uint32_t hash=Hash(data.data(),pos);
    if(get(4)!=hash)return false;
    records.swap(decoded);return true;
}
inline void Upsert(std::vector<Record>& records,Record record) {
    for(auto it=records.begin();it!=records.end();++it)if(it->SameRun(record) ||
        (!it->team && record.team && it->seed==record.seed && it->settings[MODE]==record.settings[MODE] && it->settings[SCENE]==record.settings[SCENE])) {
        record.started=it->started;record.updated=std::max(record.updated,record.started);
        record.stage=std::max(record.stage,it->stage);record.players=std::max(record.players,it->players);
        if(record.team==it->team)record.joined=std::min(record.joined,it->joined);
        if(record.partial && !record.totalDamage){record.totalDamage=it->totalDamage;record.bossDamage=it->bossDamage;record.partial=it->partial;}
        if(it->status==COMPLETE)record.status=COMPLETE;
        if(it->status==FAILED && record.status==LEFT)record.status=FAILED;
        records.erase(it);break;
    }
    records.insert(records.begin(),record);
    if(records.size()>CAPACITY)records.resize(CAPACITY);
}
enum ReadState { EMPTY,OK,RECOVERED,DAMAGED };
std::string Path(int profile);
ReadState Read(int profile,std::vector<Record>& records);
bool Write(int profile,const Record& record);
bool Capture(LawnApp* app,Status status=SAVED);
bool& WriteFailed();
}
