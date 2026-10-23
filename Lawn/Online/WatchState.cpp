#include "WatchState.h"
#include "FullWatch.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../Board.h"
#include "../Plant.h"
#include "../Zombie.h"
#include "../Projectile.h"
#include "../LawnMower.h"
#include "../GridItem.h"
#include "../SeedPacket.h"
#include "../Challenge.h"
#include "../Run/RunState.h"
#include "../../Sexy.TodLib/EffectSystem.h"
#include "../../Sexy.TodLib/Reanimator.h"
#include "../../Sexy.TodLib/Attachment.h"
#include "../../SexyAppFramework/Common.h"
#include <set>
#include <cmath>
#include <stdexcept>
#include <type_traits>

// @pvz-online: AI-generated, preliminarily tested. Pinned field lists define PWV2 v1;
// adding a native class member does NOT silently change the wire format.
namespace WatchState {
namespace {
using namespace WatchWire;
void Require(bool ok){if(!ok)throw std::runtime_error("invalid watch state");}
struct Archive {
    bool read;bool incremental=false;Bytes& bytes;size_t pos=0;Board* board;
    std::set<unsigned> animations,attachments;
    Archive(bool r,Bytes& b,Board* p):read(r),bytes(b),board(p){}
    template<class T> void Number(T& value) {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        uint32_t v=static_cast<uint32_t>(value);
        if(read){Require(pos+4<=bytes.size());v=Get(bytes.data()+pos);pos+=4;value=static_cast<T>(v);}
        else Put(bytes,v);
    }
    template<class T> void S(T& value){Number(value);}
    void S(bool& value){unsigned char v=value?1:0;if(read){Require(pos<bytes.size());v=bytes[pos++];Require(v<=1);value=v!=0;}else bytes.push_back(v);}
    void S(int64_t& value){uint64_t v=static_cast<uint64_t>(value);uint32_t lo=static_cast<uint32_t>(v),hi=static_cast<uint32_t>(v>>32);Number(lo);Number(hi);if(read)value=static_cast<int64_t>((uint64_t(hi)<<32)|lo);}
    void S(float& value){uint32_t v=0;if(!read)memcpy(&v,&value,4);Number(v);if(read)memcpy(&value,&v,4);Require(std::isfinite(value));}
    template<class T,size_t N> void S(T (&items)[N]){for(auto& item:items)S(item);}
    void S(Rect& v){S(v.mX);S(v.mY);S(v.mWidth);S(v.mHeight);}
    void S(Color& v){S(v.mRed);S(v.mGreen);S(v.mBlue);S(v.mAlpha);}
    void S(SexyTransform2D& v){S(v.m);}
    void S(MagnetItem& v){S(v.mPosX);S(v.mPosY);S(v.mDestOffsetX);S(v.mDestOffsetY);S(v.mItemType);}
    void S(MotionTrailFrame& v){S(v.mPosX);S(v.mPosY);S(v.mAnimTime);}
    void S(RunState::BuffStack& v){S(v.mId);S(v.mCount);Require(v.mId<1024);}
    void S(ReanimationID& value){
        unsigned id=static_cast<unsigned>(value);
        if(!read){auto* p=board->mApp->ReanimationTryToGet(value);if(!p || p->mDead)id=0;else animations.insert(id);}
        Number(id);if(read)value=static_cast<ReanimationID>(id);
    }
    void S(AttachmentID& value){
        unsigned id=static_cast<unsigned>(value);
        if(!read){auto* p=board->mApp->mEffectSystem->mAttachmentHolder->mAttachments.DataArrayTryToGet(id);
            bool needed=false;if(p&&!p->mDead)for(int i=0;i<p->mNumEffects;++i)if(p->mEffectArray[i].mEffectType==EFFECT_REANIM)needed=true;
            if(needed)attachments.insert(id);else id=0;}
        Number(id);if(read)value=static_cast<AttachmentID>(id);
    }
    void ImageRef(Image*& image){int id=read?0:(image?static_cast<int>(GetIdByImage(image)):static_cast<int>(ResourceId::RESOURCE_ID_MAX));S(id);Require(id>=0&&id<=static_cast<int>(ResourceId::RESOURCE_ID_MAX));if(read)image=id==static_cast<int>(ResourceId::RESOURCE_ID_MAX)?nullptr:GetImageById(static_cast<ResourceId>(id));}
    unsigned Count(unsigned n,unsigned limit){S(n);Require(n<=limit);return n;}
    template<class T> void Vector(std::vector<T>& values,unsigned limit){unsigned n=Count(static_cast<unsigned>(values.size()),limit);if(read)values.resize(n);for(auto& value:values)S(value);}
    void String(std::string& value,unsigned limit){unsigned n=Count(static_cast<unsigned>(value.size()),limit);if(read){Require(pos+n<=bytes.size());value.assign(reinterpret_cast<const char*>(bytes.data()+pos),n);pos+=n;}else bytes.insert(bytes.end(),value.begin(),value.end());}
    void End(){if(read)Require(pos==bytes.size());}
};
void Common(Archive& a,GameObject& v){a.S(v.mX);a.S(v.mY);a.S(v.mWidth);a.S(v.mHeight);a.S(v.mVisible);a.S(v.mRow);a.S(v.mRenderOrder);Require(v.mRow>=-1&&v.mRow<MAX_GRID_SIZE_Y);}
void Fields(Archive& a,Plant& v) {
    Common(a,v);
    a.S(v.mSeedType);
    a.S(v.mPlantCol);
    Require(v.mRow>=0);
    a.S(v.mAnimCounter);
    a.S(v.mFrame);
    a.S(v.mFrameLength);
    a.S(v.mNumFrames);
    a.S(v.mState);
    a.S(v.mPlantHealth);
    a.S(v.mPlantMaxHealth);
    a.S(v.mSubclass);
    a.S(v.mDisappearCountdown);
    a.S(v.mDoSpecialCountdown);
    a.S(v.mStateCountdown);
    a.S(v.mLaunchCounter);
    a.S(v.mLaunchRate);
    a.S(v.mPlantRect);
    a.S(v.mPlantAttackRect);
    a.S(v.mTargetX);
    a.S(v.mTargetY);
    a.S(v.mStartRow);
    a.S(v.mShootingCounter);
    a.S(v.mBodyReanimID);
    a.S(v.mHeadReanimID);
    a.S(v.mHeadReanimID2);
    a.S(v.mHeadReanimID3);
    a.S(v.mBlinkReanimID);
    a.S(v.mLightReanimID);
    a.S(v.mSleepingReanimID);
    a.S(v.mBlinkCountdown);
    a.S(v.mRecentlyEatenCountdown);
    a.S(v.mEatenFlashCountdown);
    a.S(v.mBeghouledFlashCountdown);
    a.S(v.mShakeOffsetX);
    a.S(v.mShakeOffsetY);
    a.S(v.mMagnetItems);
    a.S(v.mTargetZombieID);
    a.S(v.mWakeUpCounter);
    a.S(v.mOnBungeeState);
    a.S(v.mImitaterType);
    a.S(v.mAnimPing);
    a.S(v.mDead);
    a.S(v.mSquished);
    a.S(v.mIsAsleep);
    a.S(v.mIsOnBoard);
    a.S(v.mExtraTanglekelpIDs);
    a.S(v.mExtraTanglekelpCount);
    a.S(v.mSquashSmashesLeft);
    a.S(v.mSquashFromX);
    a.S(v.mSquashFromY);
    a.S(v.mRunDispatchCursor);
    a.S(v.mRunAliveTicks);
    a.S(v.mRunGarlicCooldown);
    a.S(v.mRunPurpleShots);
    a.S(v.mMineregrowsLeft);
    a.S(v.mChainBitesLeft);
    Require(v.mSeedType>=0&&v.mSeedType<NUM_SEED_TYPES && v.mPlantCol>=0&&v.mPlantCol<MAX_GRID_SIZE_X && v.mExtraTanglekelpCount>=0&&v.mExtraTanglekelpCount<=2);
}
void Fields(Archive& a,Zombie& v) {
    Common(a,v);
    a.S(v.mZombieType);
    Require(v.mRow>=0);
    a.S(v.mZombiePhase);
    a.S(v.mPosX);
    a.S(v.mPosY);
    a.S(v.mVelX);
    a.S(v.mAnimCounter);
    a.S(v.mAnimTicksPerFrame);
    a.S(v.mAnimFrames);
    a.S(v.mFrame);
    a.S(v.mPrevFrame);
    a.S(v.mVariant);
    a.S(v.mIsEating);
    a.S(v.mJustGotShotCounter);
    a.S(v.mShieldJustGotShotCounter);
    a.S(v.mShieldRecoilCounter);
    a.S(v.mZombieAge);
    a.S(v.mZombieHeight);
    a.S(v.mPhaseCounter);
    a.S(v.mFromWave);
    a.S(v.mZombieFade);
    a.S(v.mFlatTires);
    a.S(v.mUseLadderCol);
    a.S(v.mTargetCol);
    a.S(v.mAltitude);
    a.S(v.mHitUmbrella);
    a.S(v.mZombieRect);
    a.S(v.mZombieAttackRect);
    a.S(v.mChilledCounter);
    a.S(v.mButteredCounter);
    a.S(v.mIceTrapCounter);
    a.S(v.mRunBarbedCounter);
    a.S(v.mFetidSlowCounter);
    a.S(v.mScorchedCounter);
    a.S(v.mFrostArmorCounter);
    a.S(v.mIceShroomTrap);
    a.S(v.mControlImmune);
    a.S(v.mHeartguardUsed);
    a.S(v.mMindControlled);
    a.S(v.mBlowingAway);
    a.S(v.mHasHead);
    a.S(v.mHasArm);
    a.S(v.mHasObject);
    a.S(v.mInPool);
    a.S(v.mOnHighGround);
    a.S(v.mYuckyFace);
    a.S(v.mYuckyFaceCounter);
    a.S(v.mHelmType);
    a.S(v.mBodyHealth);
    a.S(v.mBodyMaxHealth);
    a.S(v.mHelmHealth);
    a.S(v.mHelmMaxHealth);
    a.S(v.mShieldType);
    a.S(v.mShieldHealth);
    a.S(v.mShieldMaxHealth);
    a.S(v.mFlyingHealth);
    a.S(v.mFlyingMaxHealth);
    a.S(v.mDead);
    a.S(v.mRelatedZombieID);
    a.S(v.mFollowerZombieID);
    a.S(v.mParticleOffsetX);
    a.S(v.mParticleOffsetY);
    a.S(v.mAttachmentID);
    a.S(v.mSummonCounter);
    a.S(v.mBodyReanimID);
    a.S(v.mScaleZombie);
    a.S(v.mVelZ);
    a.S(v.mOriginalAnimRate);
    a.S(v.mTargetPlantID);
    a.S(v.mBossMode);
    a.S(v.mTargetRow);
    a.S(v.mBossBungeeCounter);
    a.S(v.mBossStompCounter);
    a.S(v.mBossHeadCounter);
    a.S(v.mBossFireBallReanimID);
    a.S(v.mSpecialHeadReanimID);
    a.S(v.mPaperHelmReanimID);
    a.S(v.mFireballRow);
    a.S(v.mIsFireBall);
    a.S(v.mMoweredReanimID);
    a.S(v.mLastPortalX);
    Require(v.mZombieType>=0&&v.mZombieType<NUM_ZOMBIE_TYPES);if(a.read)v.mDroppedLoot=true;
}
void Fields(Archive& a,Projectile& v) {
    Common(a,v);
    a.S(v.mFrame);
    a.S(v.mNumFrames);
    a.S(v.mAnimCounter);
    a.S(v.mPosX);
    a.S(v.mPosY);
    a.S(v.mPosZ);
    a.S(v.mVelX);
    a.S(v.mVelY);
    a.S(v.mVelZ);
    a.S(v.mAccZ);
    a.S(v.mShadowY);
    a.S(v.mDead);
    a.S(v.mAnimTicksPerFrame);
    a.S(v.mMotionType);
    a.S(v.mProjectileType);
    a.S(v.mProjectileAge);
    a.S(v.mClickBackoffCounter);
    a.S(v.mRotation);
    a.S(v.mRotationSpeed);
    a.S(v.mOnHighGround);
    a.S(v.mDamageRangeFlags);
    a.S(v.mHitTorchwoodGridX);
    a.S(v.mAttachmentID);
    a.S(v.mCobTargetX);
    a.S(v.mCobTargetRow);
    a.S(v.mTargetZombieID);
    a.S(v.mLastPortalX);
    a.S(v.mPricklyHitsLeft);
    a.S(v.mPricklyHitIds);
    a.S(v.mPricklyHitCount);
    a.S(v.mRunIceSource);
    a.S(v.mRunCactusSource);
    a.S(v.mRunScaredySource);
    a.S(v.mRunCrystalDamage);
    a.S(v.mRunSourceSeed);
    a.S(v.mRunHuntHitsLeft);
    a.S(v.mRunLaunchDelay);
    a.S(v.mRunIceWave);
    // Sparse hit membership: do not send 1024 empty ID slots per projectile.
    unsigned hits=0;for(auto id:v.mRunWaveHitIds)if(id)++hits;hits=a.Count(hits,1024);
    if(a.read){for(auto& id:v.mRunWaveHitIds)id=ZOMBIEID_NULL;for(unsigned i=0;i<hits;++i){unsigned index=0;a.S(index);Require(index<1024);a.S(v.mRunWaveHitIds[index]);}}
    else for(unsigned i=0;i<1024;++i)if(v.mRunWaveHitIds[i]){unsigned index=i;a.S(index);a.S(v.mRunWaveHitIds[i]);}
    Require(v.mProjectileType>=0&&v.mProjectileType<NUM_PROJECTILES);
}
void Fields(Archive& a,LawnMower& v) {
    a.S(v.mPosX);
    a.S(v.mPosY);
    a.S(v.mRenderOrder);
    a.S(v.mRow);
    a.S(v.mAnimTicksPerFrame);
    a.S(v.mReanimID);
    a.S(v.mChompCounter);
    a.S(v.mRollingInCounter);
    a.S(v.mSquishedCounter);
    Require(v.mRow>=0&&v.mRow<MAX_GRID_SIZE_Y);
    a.S(v.mMowerState);
    a.S(v.mDead);
    a.S(v.mVisible);
    a.S(v.mMowerType);
    a.S(v.mAltitude);
    a.S(v.mMowerHeight);
    a.S(v.mLastPortalX);
}
void Fields(Archive& a,GridItem& v) {
    a.S(v.mGridItemType);
    a.S(v.mGridItemState);
    a.S(v.mGridX);
    a.S(v.mGridY);
    a.S(v.mGridItemCounter);
    a.S(v.mRenderOrder);
    a.S(v.mDead);
    a.S(v.mPosX);
    a.S(v.mPosY);
    a.S(v.mGoalX);
    a.S(v.mGoalY);
    a.S(v.mGridItemReanimID);
    a.S(v.mZombieType);
    a.S(v.mSeedType);
    a.S(v.mScaryPotType);
    a.S(v.mTransparentCounter);
    Require(v.mGridX>=0&&v.mGridX<MAX_GRID_SIZE_X&&v.mGridY>=0&&v.mGridY<MAX_GRID_SIZE_Y);
}
void ConfigBoard(Archive& a,Board& v) {
    a.S(v.mBackground);
    a.S(v.mLevel);
    a.S(v.mNumWaves);
    a.S(v.mPlantRow);
    a.S(v.mZombieAllowed);
    a.S(v.mBoardRandSeed);
    a.S(v.mMustacheMode);
    a.S(v.mSuperMowerMode);
    a.S(v.mFutureMode);
    a.S(v.mPinataMode);
    a.S(v.mDanceMode);
    a.S(v.mDaisyMode);
    a.S(v.mSukhbirMode);
}
void DynamicBoard(Archive& a,Board& v) {
    a.S(v.mGridSquareType);
    a.S(v.mGridCelLook);
    a.S(v.mGridCelOffset);
    a.S(v.mGridCelFog);
    a.S(v.mEnableGraveStones);
    a.S(v.mSpecialGraveStoneX);
    a.S(v.mSpecialGraveStoneY);
    a.S(v.mFogOffset);
    a.S(v.mFogBlownCountDown);
    a.S(v.mWaveRowGotLawnMowered);
    a.S(v.mBonusLawnMowersRemaining);
    a.S(v.mIceMinX);
    a.S(v.mIceTimer);
    a.S(v.mShakeCounter);
    a.S(v.mShakeAmountX);
    a.S(v.mShakeAmountY);
    a.S(v.mSodPosition);
    a.S(v.mSunMoney);
    a.S(v.mMainCounter);
    a.S(v.mEffectCounter);
    a.S(v.mRiseFromGraveCounter);
    a.S(v.mCurrentWave);
    a.S(v.mTotalSpawnedWaves);
    a.S(v.mLastBungeeWave);
    a.S(v.mZombieHealthToNextWave);
    a.S(v.mZombieHealthWaveStart);
    a.S(v.mZombieCountDown);
    a.S(v.mZombieCountDownStart);
    a.S(v.mHugeWaveCountDown);
    a.S(v.mFinalBossKilled);
    a.S(v.mLevelComplete);
    a.S(v.mLevelAwardSpawned);
    a.S(v.mProgressMeterWidth);
    a.S(v.mFlagRaiseCounter);
    a.S(v.mIceTrapCounter);
    a.S(v.mFwooshID);
    a.S(v.mFwooshCountDown);
    a.S(v.mTimeStopCounter);
}
void Config(Archive& a,Board& b){
    ConfigBoard(a,b);Require(b.mBackground>=0 && b.mBackground<=BACKGROUND_ZOMBIQUARIUM && b.mNumWaves>=0&&b.mNumWaves<=MAX_ZOMBIE_WAVES);
    // Only occupied entries in each wave, no fixed-capacity empty wave table.
    for(int w=0;w<b.mNumWaves;++w){unsigned n=0;if(!a.read)while(n<MAX_ZOMBIES_IN_WAVE&&b.mZombiesInWave[w][n]!=ZOMBIE_INVALID)++n;
        n=a.Count(n,MAX_ZOMBIES_IN_WAVE);if(a.read)for(auto& z:b.mZombiesInWave[w])z=ZOMBIE_INVALID;
        for(unsigned i=0;i<n;++i){a.S(b.mZombiesInWave[w][i]);Require(b.mZombiesInWave[w][i]>=0&&b.mZombiesInWave[w][i]<NUM_ZOMBIE_TYPES);}}
    auto& bank=*b.mSeedBank;a.S(bank.mNumPackets);Require(bank.mNumPackets>=0&&bank.mNumPackets<=SEEDBANK_MAX);
    Common(a,bank);
    for(int i=0;i<bank.mNumPackets;++i){auto& p=bank.mSeedPackets[i];Common(a,p);a.S(p.mIndex);a.S(p.mOffsetX);a.S(p.mPacketType);a.S(p.mImitaterType);Require(p.mIndex>=0&&p.mIndex<SEEDBANK_MAX);}
    bool hasRun=b.mApp->mRunState!=nullptr;a.S(hasRun);
    if(hasRun){if(a.read)b.mApp->mRunState=new RunState();auto& r=*b.mApp->mRunState;
        a.S(r.mRunSeed);
        a.S(r.mMode);
        a.S(r.mDiff);
        a.S(r.mLevelIndex);
        a.S(r.mScale);
        a.S(r.mTempo);
        a.S(r.mZombotany);
        a.S(r.mBuffMult);
        a.S(r.mLadder);
        a.S(r.mZombieBuffMode);
        a.S(r.mSpeedTier);
        a.S(r.mFirstWave);
        a.S(r.mBossFlag);
        a.S(r.mZombieBuffStacks);
        a.S(r.mLoanPlant);
        a.S(r.mLoanLevel);
        a.S(r.mCrowdRule);
        a.Vector(r.mPool,48);a.Vector(r.mBuffs,512);a.Vector(r.mLoanBuffs,512);
        Require(r.mMode>=0&&r.mMode<=RunState::RUN_MODE_ENDLESS&&r.mLevelIndex>=0&&r.mLevelIndex<60000);
    }
}
void Animation(Archive& a,Reanimation& r){
    ReanimationType type=r.mReanimationType;a.S(type);Require(type>=0&&type<NUM_REANIMS);
    const bool reuse=a.read && a.incremental && r.mTrackInstances && r.mReanimationType==type;
    const float oldTime=reuse?r.mAnimTime:0,oldLast=reuse?r.mLastFrameTime:0;
    const int oldStart=reuse?r.mFrameStart:0,oldCount=reuse?r.mFrameCount:0;
    const auto oldLoop=r.mLoopType;
    if(a.read && !reuse){
        if(r.mTrackInstances){r.mDead=true;r.ReanimationDelete();}
        r.mReanimationHolder=a.board->mApp->mEffectSystem->mReanimationHolder;r.ReanimationInitializeType(0,0,type);
    }
    a.S(r.mAnimTime);
    a.S(r.mAnimRate);
    a.S(r.mLoopType);
    a.S(r.mDead);
    a.S(r.mFrameStart);
    a.S(r.mFrameCount);
    a.S(r.mFrameBasePose);
    a.S(r.mOverlayMatrix);
    a.S(r.mColorOverride);
    a.S(r.mLoopCount);
    a.S(r.mIsAttachment);
    a.S(r.mRenderOrder);
    a.S(r.mExtraAdditiveColor);
    a.S(r.mEnableExtraAdditiveDraw);
    a.S(r.mExtraOverlayColor);
    a.S(r.mEnableExtraOverlayDraw);
    a.S(r.mLastFrameTime);
    a.S(r.mFilterEffect);
    unsigned count=a.Count(r.mDefinition->mTracks.count,1024);Require(count==static_cast<unsigned>(r.mDefinition->mTracks.count));
    for(unsigned i=0;i<count;++i){auto& t=r.mTrackInstances[i];
        a.S(t.mBlendCounter);
        a.S(t.mBlendTime);
        a.S(t.mShakeOverride);
        a.S(t.mShakeX);
        a.S(t.mShakeY);
        a.S(t.mAttachmentID);
        a.S(t.mRenderGroup);
        a.S(t.mTrackColor);
        a.S(t.mIgnoreClipRect);
        a.S(t.mTruncateDisappearingFrames);
        a.S(t.mIgnoreColorOverride);
        a.S(t.mIgnoreExtraAdditiveColor);
        a.S(t.mBlendTransform.mTransX);
        a.S(t.mBlendTransform.mTransY);
        a.S(t.mBlendTransform.mSkewX);
        a.S(t.mBlendTransform.mSkewY);
        a.S(t.mBlendTransform.mScaleX);
        a.S(t.mBlendTransform.mScaleY);
        a.S(t.mBlendTransform.mFrame);
        a.S(t.mBlendTransform.mAlpha);
        a.ImageRef(t.mImageOverride);
    }
    if(reuse && oldStart==r.mFrameStart && oldCount==r.mFrameCount && oldLoop==r.mLoopType &&
       std::fabs(oldTime-r.mAnimTime)<=0.08f){r.mAnimTime=oldTime;r.mLastFrameTime=oldLast;}
    if(a.read){Require(count>0);Require(r.mFrameStart>=0&&r.mFrameCount>=0&&uint64_t(r.mFrameStart)+r.mFrameCount<=static_cast<unsigned>(r.mDefinition->mTracks.tracks[0].mTransforms.count));}
}
void Effects(Archive& a,Attachment& v){
    unsigned count=0;if(!a.read)for(int i=0;i<v.mNumEffects;++i)if(v.mEffectArray[i].mEffectType==EFFECT_REANIM)++count;
    count=a.Count(count,MAX_EFFECTS_PER_ATTACHMENT);
    if(a.read){v.mNumEffects=count;for(unsigned i=0;i<count;++i){auto& e=v.mEffectArray[i];e.mEffectType=EFFECT_REANIM;auto id=REANIMATIONID_NULL;a.S(id);e.mEffectID=id;a.S(e.mOffset);a.S(e.mDontDrawIfParentHidden);a.S(e.mDontPropogateColor);}}
    else for(int i=0;i<v.mNumEffects;++i){auto& e=v.mEffectArray[i];if(e.mEffectType!=EFFECT_REANIM)continue;auto id=static_cast<ReanimationID>(e.mEffectID);a.S(id);a.S(e.mOffset);a.S(e.mDontDrawIfParentHidden);a.S(e.mDontPropogateColor);}
}
// Preserve stable entity IDs so cob targets, bungees, dancers and projectile hit lists
// keep their relationships. Only live records are transmitted, never pool capacities.
template<class T,class F> void Pool(Archive& a,DataArray<T>& pool,F fields,const std::set<unsigned>* selected=nullptr){
    unsigned count=0;if(!a.read){T* p=nullptr;while(pool.IterateNext(p))if(!p->mDead&&(!selected||selected->count(pool.DataArrayGetID(p))))++count;}
    count=a.Count(count,pool.mMaxSize);
    if(!a.read){T* p=nullptr;while(pool.IterateNext(p))if(!p->mDead&&(!selected||selected->count(pool.DataArrayGetID(p)))){auto id=pool.DataArrayGetID(p);a.S(id);fields(a,*p);}return;}
    if(!a.incremental){Require(pool.mSize==0);memset(pool.mBlock,0,pool.mMaxSize*sizeof(typename DataArray<T>::DataArrayItem));}
    std::set<unsigned> seen;
    auto dispose=[&](T& v){
        // Snapshot graph ownership is reconciled separately. Do not recursively kill
        // an attachment/animation that is still referenced by an incoming entity.
        if constexpr(std::is_same_v<T,Zombie>||std::is_same_v<T,Projectile>)v.mAttachmentID=ATTACHMENTID_NULL;
        if constexpr(std::is_same_v<T,Reanimation>||std::is_same_v<T,Attachment>)v.mDead=true;
        pool.DataArrayFree(&v);
    };
    for(unsigned i=0;i<count;++i){unsigned id=0;a.S(id);unsigned index=id&DATA_ARRAY_INDEX_MASK;
        Require((id&DATA_ARRAY_KEY_MASK)&&index<pool.mMaxSize&&seen.insert(index).second);
        auto& slot=pool.mBlock[index];
        const bool exists=index<pool.mMaxUsedCount && slot.mID==id;
        if(!exists){
            if(index<pool.mMaxUsedCount && (slot.mID&DATA_ARRAY_KEY_MASK))dispose(slot.mItem);
            if(index>=pool.mMaxUsedCount)for(unsigned k=pool.mMaxUsedCount;k<=index;++k)pool.mBlock[k].mID=0;
            memset(&slot,0,sizeof(slot));new(&slot.mItem)T();slot.mID=id;pool.mSize++;
        }
        pool.mMaxUsedCount=std::max(pool.mMaxUsedCount,index+1);
        pool.mNextKey=std::max(pool.mNextKey,(id>>16)+1);if(pool.mNextKey>=65536)pool.mNextKey=1;
        if constexpr(std::is_same_v<T,Plant>||std::is_same_v<T,Zombie>||std::is_same_v<T,Projectile>||std::is_same_v<T,GridItem>||std::is_same_v<T,LawnMower>){slot.mItem.mApp=a.board->mApp;slot.mItem.mBoard=a.board;}
        if constexpr(std::is_same_v<T,Zombie>){
            auto& v=slot.mItem;
            const float x=v.mPosX,y=v.mPosY,alt=v.mAltitude;const int ix=v.mX,iy=v.mY,row=v.mRow;
            const auto type=v.mZombieType;const auto phase=v.mZombiePhase;const auto height=v.mZombieHeight;const bool eating=v.mIsEating,alive=!v.mDead;
            fields(a,v);
            if(a.incremental && exists && alive && !v.mDead && type==v.mZombieType && phase==v.mZombiePhase &&
               row==v.mRow && height==v.mZombieHeight && eating==v.mIsEating && std::fabs(alt-v.mAltitude)<=1.0f &&
               std::hypot(x-v.mPosX,y-v.mPosY)<=4.0f){v.mPosX=x;v.mPosY=y;v.mX=ix;v.mY=iy;v.mAltitude=alt;}
        }else if constexpr(std::is_same_v<T,Projectile>){
            auto& v=slot.mItem;
            const float x=v.mPosX,y=v.mPosY,z=v.mPosZ,shadow=v.mShadowY;const int ix=v.mX,iy=v.mY,row=v.mRow;
            const auto type=v.mProjectileType;const auto motion=v.mMotionType;const auto target=v.mTargetZombieID;const bool alive=!v.mDead;
            fields(a,v);
            if(a.incremental && exists && alive && !v.mDead && type==v.mProjectileType && motion==v.mMotionType &&
               row==v.mRow && target==v.mTargetZombieID && std::fabs(z-v.mPosZ)<=2.0f &&
               std::hypot(x-v.mPosX,y-v.mPosY)<=6.0f){v.mPosX=x;v.mPosY=y;v.mPosZ=z;v.mShadowY=shadow;v.mX=ix;v.mY=iy;}
        }else fields(a,slot.mItem);
    }
    for(unsigned i=0;i<pool.mMaxUsedCount;++i)if((pool.mBlock[i].mID&DATA_ARRAY_KEY_MASK)&&!seen.count(i))dispose(pool.mBlock[i].mItem);
    pool.mFreeListHead=pool.mMaxUsedCount;
    for(unsigned i=pool.mMaxUsedCount;i-->0;)if(!(pool.mBlock[i].mID&DATA_ARRAY_KEY_MASK)){pool.mBlock[i].mID=pool.mFreeListHead;pool.mFreeListHead=i;}
    Require(pool.DataArrayValidateState());
}
void Dynamic(Archive& a,Board& b){
    DynamicBoard(a,b);
    a.S(b.mWaveHealth.start);a.S(b.mWaveHealth.next);
    a.S(b.mWaveSpawnRetry.wave);a.S(b.mWaveSpawnRetry.index);a.S(b.mWaveSpawnRetry.part);
    Require(b.mWaveSpawnRetry.wave>=-1&&b.mWaveSpawnRetry.wave<MAX_ZOMBIE_WAVES&&b.mWaveSpawnRetry.index>=0&&b.mWaveSpawnRetry.index<=MAX_ZOMBIES_IN_WAVE);
    unsigned pending=0;if(!a.read){Require(b.mDeferredZombieSpawns.next<=b.mDeferredZombieSpawns.pending.size());pending=static_cast<unsigned>(b.mDeferredZombieSpawns.pending.size()-b.mDeferredZombieSpawns.next);}
    pending=a.Count(pending,65536);
    if(a.read){b.mDeferredZombieSpawns.pending.resize(pending);b.mDeferredZombieSpawns.next=0;}
    for(unsigned i=0;i<pending;++i){auto& v=b.mDeferredZombieSpawns.pending[b.mDeferredZombieSpawns.next+i];a.S(v.type);a.S(v.x);a.S(v.y);a.S(v.wave);a.S(v.kind);Require(v.type>=0&&v.type<NUM_ZOMBIE_TYPES&&v.y>=0&&v.y<MAX_GRID_SIZE_Y);}

    Require(b.mCurrentWave>=0&&b.mCurrentWave<=MAX_ZOMBIE_WAVES);
    auto& bank=*b.mSeedBank;for(int i=0;i<bank.mNumPackets;++i){auto& p=bank.mSeedPackets[i];a.S(p.mRefreshCounter);a.S(p.mRefreshTime);a.S(p.mActive);a.S(p.mRefreshing);a.S(p.mTimesUsed);}
    a.S(b.mChallenge->mSurvivalStage);a.S(b.mChallenge->mChallengeState);a.S(b.mChallenge->mChallengeStateCounter);
    Pool(a,b.mPlants,[](Archive& a,Plant& v){Fields(a,v);});
    Pool(a,b.mZombies,[](Archive& a,Zombie& v){Fields(a,v);});
    Pool(a,b.mProjectiles,[](Archive& a,Projectile& v){Fields(a,v);});
    Pool(a,b.mLawnMowers,[](Archive& a,LawnMower& v){Fields(a,v);});
    Pool(a,b.mGridItems,[](Archive& a,GridItem& v){Fields(a,v);});
    unsigned n=a.Count(static_cast<unsigned>(b.mRunDotStates.size()),b.mZombies.mMaxSize);
    auto dot=[&](unsigned& id,RunDotState& v){a.S(id);a.S(v.butterFrames);a.S(v.butterTick);a.S(v.scorchTick);a.S(v.emberFrames);a.S(v.emberTick);};
    if(a.read){b.mRunDotStates.clear();for(unsigned i=0;i<n;++i){unsigned id=0;RunDotState v;dot(id,v);b.mRunDotStates[id]=v;}}
    else for(auto& entry:b.mRunDotStates){unsigned id=entry.first;dot(id,entry.second);}
    n=a.Count(static_cast<unsigned>(b.mRelayedZombieIds.size()),b.mZombies.mMaxSize);
    if(a.read){b.mRelayedZombieIds.clear();for(unsigned i=0;i<n;++i){unsigned id=0;a.S(id);b.mRelayedZombieIds.insert(id);}}else for(unsigned id:b.mRelayedZombieIds)a.S(id);
    auto& animations=b.mApp->mEffectSystem->mReanimationHolder->mReanimations;
    auto& attachments=b.mApp->mEffectSystem->mAttachmentHolder->mAttachments;
    if(!a.read){
        // Discover only entity-rooted animation attachments. Coins and transient particles
        // have no roots here and can never leak back into a snapshot via the effect graph.
        size_t old=0;do{old=a.animations.size()+a.attachments.size();
            auto ids=a.animations;for(auto id:ids){auto* r=animations.DataArrayTryToGet(id);if(r)for(int i=0;i<r->mDefinition->mTracks.count;++i){auto v=r->mTrackInstances[i].mAttachmentID;Bytes scratch;Archive scan(false,scratch,&b);scan.S(v);a.attachments.insert(scan.attachments.begin(),scan.attachments.end());}}
            auto links=a.attachments;for(auto id:links){auto* p=attachments.DataArrayTryToGet(id);if(p)for(int i=0;i<p->mNumEffects;++i){auto& e=p->mEffectArray[i];if(e.mEffectType==EFFECT_REANIM&&animations.DataArrayTryToGet(e.mEffectID))a.animations.insert(e.mEffectID);}}
        }while(old!=a.animations.size()+a.attachments.size());
    }
    Pool(a,animations,Animation,&a.animations);Pool(a,attachments,Effects,&a.attachments);
    std::string rng=a.read?std::string():Sexy::GetRandState();a.String(rng,2500);Require(rng.size()==2500);if(a.read)Sexy::RestoreRandState(rng);
}
}
bool Capture(Board* b,Bytes& config,Bytes& state){
    try{Archive c(false,config,b);Config(c,*b);Archive d(false,state,b);Dynamic(d,*b);return config.size()<=MaxConfig&&state.size()<=MaxState;}
    catch(const std::exception& e){static bool reported=false;if(!reported){TodLog("[watch] capture rejected: %s",e.what());reported=true;}config.clear();state.clear();return false;}
}
bool Restore(LawnApp* app,const Bytes& complete,const Header& h,bool incremental){
    if(!FullWatch::IsObserver()||!h.configBytes||h.mode>=NUM_GAME_MODES)return false;
    try{
        Bytes config(complete.begin()+HeaderBytes,complete.begin()+HeaderBytes+h.configBytes),state(complete.begin()+HeaderBytes+h.configBytes,complete.end());
        incremental=incremental && app->mBoard;
        if(!incremental){app->KillBoard();delete app->mRunState;app->mRunState=nullptr;app->mGameMode=static_cast<GameMode>(h.mode);app->MakeNewBoard();}
        Board& b=*app->mBoard;
        if(!incremental){Archive c(true,config,&b);Config(c,b);c.End();}
        Archive d(true,state,&b);d.incremental=incremental;Dynamic(d,b);d.End();
        b.mShowShovel=false;b.mSunCountDown=1000000000;
        b.LoadBackgroundImages();b.UpdateLayers();b.ResetFPSStats();b.SyncLegacyWaveHealth();
        app->mGameScene=SCENE_PLAYING;app->ClearUpdateBacklog();return true;
    }catch(const std::exception& e){TodLog("[watch] restore rejected: %s",e.what());return false;}
}
}
