#pragma once
#include "../../LawnApp.h"
#include "../Board.h"
#include "../Zombie.h"
#include "RunBuffs.h"
#include "../System/DamageStats.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace RunDotEffects {
inline unsigned Id(Zombie* z) { return z && z->mBoard ? z->mBoard->mZombies.DataArrayGetID(z) : 0; }
inline bool Alive(Zombie* z) { return z && z->mBoard && z->mBodyHealth>0 && !z->IsDeadOrDying(); }
inline bool CanCoat(Zombie* z) {
    return Alive(z) && !z->IsFlying() && !z->IsTangleKelpTarget() && !z->IsBobsledTeamWithSled() &&
        (z->mHasHead || z->mZombieType==ZOMBIE_ZAMBONI || z->mZombieType==ZOMBIE_BOSS);
}
inline void Clear(Zombie* z) { if(z && z->mBoard)z->mBoard->mRunDotStates.erase(Id(z)); }
inline bool HasButter(Zombie* z) {
    if(!Alive(z))return false;
    auto it=z->mBoard->mRunDotStates.find(Id(z));
    return it!=z->mBoard->mRunDotStates.end() && it->second.butterFrames>0 && Alive(z);
}
inline void Refresh(int& frames,int& tick,int duration) {
    if(frames<=0 || tick<=0)tick=100;
    frames=std::max(frames,duration);
}
inline void ButterHit(Zombie* z,int stunFrames,int coatFrames) {
    if(!z || !z->mBoard)return;
    // Keep the original hard-control eligibility, including immunity, untouched.
    if(z->mButteredCounter<stunFrames)z->ApplyButter(stunFrames);
    if(z->mApp->RunExtraBuffCount(RUN_BUFF2_BUTTER_HEAVY)>0 && CanCoat(z)) {
        auto& state=z->mBoard->mRunDotStates[Id(z)];
        Refresh(state.butterFrames,state.butterTick,coatFrames);
    }
}
inline void Scorch(Zombie* z) {
    if(!Alive(z))return;
    auto& state=z->mBoard->mRunDotStates[Id(z)];
    Refresh(z->mScorchedCounter,state.scorchTick,300);
}
inline void Ember(Zombie* z) {
    if(!Alive(z))return;
    auto& state=z->mBoard->mRunDotStates[Id(z)];
    Refresh(state.emberFrames,state.emberTick,300);
}
inline int ButterDamage(int body) { return std::min(2000,std::max(0,body)/100); }
inline int BurnDamage(int body,int stacks) { return std::max(300,std::max(0,body)/100)*std::max(0,std::min(stacks,2)); }
inline bool Tick(int& frames,int& next) {
    if(frames<=0)return false;
    --frames;
    if(next<=0)next=100;
    if(--next>0)return false;
    next=100;return true;
}
inline void Update(Zombie* z) {
    if(!z || !z->mBoard)return;
    auto& states=z->mBoard->mRunDotStates;const unsigned id=Id(z);
    if(!Alive(z)){states.erase(id);z->mScorchedCounter=0;return;}
    auto it=states.find(id);
    if(it==states.end() && z->mScorchedCounter<=0)return;
    RunDotState state=it==states.end()?RunDotState{}:it->second;
    // Old saves already contain Scorched Earth duration. Preserve its former next-tick phase.
    if(z->mScorchedCounter>0 && state.scorchTick<=0)state.scorchTick=(z->mScorchedCounter-1)%100+1;
    DamageStats::Source source(z->mApp->mDamageStats,true);
    if(Tick(state.butterFrames,state.butterTick)) {
        const int damage=ButterDamage(z->mBodyHealth);
        if(damage>0)z->TakeDamage(damage,0U);
    }
    if(!Alive(z)){states.erase(id);z->mScorchedCounter=0;return;}
    if(Tick(z->mScorchedCounter,state.scorchTick)) {
        const int stacks=z->mApp->RunPlantBuff2Count(SEED_CHERRYBOMB);
        if(stacks>0)z->TakeDamage(BurnDamage(z->mBodyHealth,stacks),0U);
    }
    if(!Alive(z)){states.erase(id);z->mScorchedCounter=0;return;}
    if(Tick(state.emberFrames,state.emberTick))z->TakeDamage(BurnDamage(z->mBodyHealth,1),0U);
    if(!Alive(z)){states.erase(id);z->mScorchedCounter=0;return;}
    if(state.butterFrames>0 || z->mScorchedCounter>0 || state.emberFrames>0)states[id]=state;
    else states.erase(id);
}

inline void Put(std::vector<unsigned char>& out,uint32_t v) { for(int i=0;i<4;++i)out.push_back((unsigned char)(v>>(i*8))); }
inline uint32_t Get(const unsigned char* p) { return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24); }
inline uint32_t Hash(const unsigned char* p,size_t n) { uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h; }
inline std::vector<unsigned char> SaveTrailer(Board* b) {
    std::vector<unsigned> ids;
    for(auto it=b->mRunDotStates.begin();it!=b->mRunDotStates.end();) {
        auto* z=b->mZombies.DataArrayTryToGet(it->first);
        if(!Alive(z))it=b->mRunDotStates.erase(it);
        else {ids.push_back(it->first);++it;}
    }
    std::sort(ids.begin(),ids.end());
    std::vector<unsigned char> out={'P','V','Z','D','O','T','0','1'};
    Put(out,uint32_t(20+24*ids.size()));Put(out,uint32_t(ids.size()));
    for(unsigned id:ids) {
        const auto& s=b->mRunDotStates.at(id);Put(out,id);
        for(int v:{s.butterFrames,s.butterTick,s.scorchTick,s.emberFrames,s.emberTick})Put(out,uint32_t(v));
    }
    Put(out,Hash(out.data(),out.size()));return out;
}
inline bool LoadTrailer(Board* b,const unsigned char* data,size_t size) {
    b->mRunDotStates.clear();
    if(size<20)return true;
    for(size_t pos=size-20;;--pos) {
        if(std::memcmp(data+pos,"PVZDOT01",8)==0) {
            const auto* p=data+pos;const uint32_t bytes=Get(p+8),count=Get(p+12);
            if(count>b->mZombies.mMaxSize || bytes!=20ull+24ull*count || bytes>size-pos ||
                Get(p+bytes-4)!=Hash(p,bytes-4))return false;
            decltype(b->mRunDotStates) restored;
            for(uint32_t i=0;i<count;++i) {
                const auto* row=p+16+24*i;const unsigned id=Get(row);
                RunDotState s{int(Get(row+4)),int(Get(row+8)),int(Get(row+12)),int(Get(row+16)),int(Get(row+20))};
                if(!b->mZombies.DataArrayTryToGet(id) || restored.count(id) || s.butterFrames<0 || s.butterFrames>1200 ||
                    s.emberFrames<0 || s.emberFrames>300 || s.butterTick<0 || s.butterTick>100 ||
                    s.scorchTick<0 || s.scorchTick>100 || s.emberTick<0 || s.emberTick>100)return false;
                restored.emplace(id,s);
            }
            b->mRunDotStates=std::move(restored);return true;
        }
        if(pos==0)break;
    }
    return true; // pre-Build77 saves have no optional DOT trailer
}
}
