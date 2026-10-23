#include "LawnApp.h"
#include "Resources.h"
#include "Lawn/Board.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Run/RunState.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/Widget/TitleScreen.h"
#include "Lawn/Widget/CoopHud.h"
#include "Lawn/Online/FullWatch.h"
#include "Lawn/Online/WatchState.h"
#include "SexyAppFramework/widget/WidgetManager.h"
#include "Sexy.TodLib/TodStringFile.h"
#include <filesystem>
#include <cstdio>
#include <fstream>

// Standalone QA entry point, linked against the same game objects. Not shipped.
bool (*gAppCloseRequest)();
bool (*gAppHasUsedCheatKeys)();
SexyString (*gGetCurrentLevelName)();
using namespace Sexy;
static bool host=false;
static bool pauseQA=false;
static std::string relayHost,relayCode;
class ProbeApp : public LawnApp {
    bool ready=false,watchStarted=false,returned=false,reentered=false,roomLogged=false;
    int editStep=0;
    unsigned pauseSteps=0;int pauseFrame=0;
    bool relayStarted=false;
    DWORD begun=GetTickCount(),connectedAt=0,lastReport=0;
public:
    void InitPropertiesHook() override {
        SexyApp::InitPropertiesHook();
        mProdName=host?"PVZ-ByeahFeature-QA-A":"PVZ-ByeahFeature-QA-B";
        mOnlyAllowOneCopyToRun=false;mForceWindowed=true;mIsWindowed=true;
        mTitle=host?"ByeahFeature QA - A":"ByeahFeature QA - B";
        mRegKey="";
    }
    void WriteToRegistry() override {} // QA never changes the user's settings.
    void LoadingCompleted() override {
        mWidgetManager->RemoveWidget(mTitleScreen);SafeDeleteWidget(mTitleScreen);mTitleScreen=nullptr;
        if(!mPlayerInfo)mPlayerInfo=mProfileMgr->AddProfile(host?"WatchQA_A":"WatchQA_B");
        std::filesystem::create_directories(GetAppDataFolder()+"userdata");
        mPlayerInfo->mLevel=20;mPlayerInfo->SaveDetails();mProfileMgr->Save();
        mRunState=new RunState();mRunState->StartNew(host?1234:5678);
        mRunState->mPendingPlantPicks=0;mRunState->mPendingBuffPicks=0;
        mGameMode=GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
        MakeNewBoard();mBoard->InitLevel();mGameScene=SCENE_PLAYING;
        mBoard->mSunMoney=host?1111:2222;mBoard->mSeedBank->mNumPackets=8;
        for(int i=0;i<8;++i)mBoard->mSeedBank->mSeedPackets[i].SetPacketType(i%2?SEED_SUNFLOWER:SEED_PEASHOOTER);
        mBoard->AddPlant(host?2:3,2,host?SEED_PEASHOOTER:SEED_SUNFLOWER,SEED_NONE);
        auto* z=mBoard->AddZombieInRow(host?ZOMBIE_TRAFFIC_CONE:ZOMBIE_NORMAL,2,0);
        z->mPosX=z->mX=650;z->mBodyHealth=z->mBodyMaxHealth=10000;
        mBoard->AddCoin(400,180,COIN_SUN,COIN_MOTION_FROM_SKY);
        if(host){
            WatchWire::Bytes configBefore,stateBefore,configAfter,stateAfter;
            bool before=WatchState::Capture(mBoard,configBefore,stateBefore);
            for(int i=0;i<5;++i)mBoard->AddCoin(200+i*40,180,COIN_SUN,COIN_MOTION_FROM_SKY);
            mRunState->mDamageHistory.assign(4096,123);mRunState->mEndlessLawn.assign(2048,88);
            bool after=WatchState::Capture(mBoard,configAfter,stateAfter);
            // Coin creation advances the global RNG; compare the combat fields before that trailer.
            bool same=before&&after&&configBefore==configAfter&&stateBefore.size()==stateAfter.size()&&stateBefore.size()>2504&&
                std::equal(stateBefore.begin(),stateBefore.end()-2504,stateAfter.begin());
            fprintf(stderr,"QA EXCLUSION sun/history/endless=%d config=%zu state=%zu\n",same,configAfter.size(),stateAfter.size());
            if(!same){mShutdown=true;return;}
        }
        mWidgetManager->SetFocus(mBoard);
        mOnlineSession=new NetSession();
        mOnlineSession->SetLocalName(host?"WatchQA_A":"WatchQA_B");
        bool ok=relayHost.empty()?(host?mOnlineSession->StartHost(28463):mOnlineSession->StartJoin("127.0.0.1",28463)):
            (host?mOnlineSession->StartRoomHost(relayHost.c_str(),97):mOnlineSession->StartRoomJoin(relayHost.c_str(),relayCode.c_str(),97));
        if(!relayHost.empty())mOnlineSession->mDirectoryPublic=false; // Code-only QA room, not a public listing.
        fprintf(stderr,"QA TRANSPORT relay=%d server=%s port=%d\n",!relayHost.empty(),relayHost.empty()?"127.0.0.1":relayHost.c_str(),relayHost.empty()?28463:97);
        fprintf(stderr,"QA START role=%c net=%d data=%s\n",host?'A':'B',ok,GetAppDataFolder().c_str());fflush(stderr);
        if(pauseQA)EnableWindow(mHWnd,FALSE); // Keep manual input out of the timed assertions.
        ready=true;SetWindowPos(mHWnd,HWND_TOP,host?20:840,50,0,0,SWP_NOSIZE|SWP_NOACTIVATE);
    }
    void UpdateFrames() override {
        if(mTitleScreen && mTitleScreen->mLoadingThreadComplete)LoadingCompleted();
        LawnApp::UpdateFrames();
        DWORD now=GetTickCount();
        if(now-begun>90000){FullWatch::Close();if(mOnlineSession)mOnlineSession->Close();mShutdown=true;return;}
        if(!ready || !mBoard || !mOnlineSession)return;
        if(!relayHost.empty()&&!roomLogged&&mOnlineSession->IsConnected()){
            roomLogged=true;fprintf(stderr,"QA ROOM code=%s seat=%u endpoint=%s:%u\n",mOnlineSession->GetRoomCode().c_str(),mOnlineSession->GetLocalSeat(),mOnlineSession->GetRelayHost().c_str(),mOnlineSession->GetRelayPort());
            if(host){std::ofstream room("relay-qa-room.txt",std::ios::trunc);room<<mOnlineSession->GetRoomCode();}
        }
        if(mOnlineSession->IsConnected() && mOnlineSession->HasOtherSeats() && !connectedAt)connectedAt=now;
        if(!relayHost.empty() && connectedAt && host && !relayStarted && mOnlineSession->CanHostStart()){
            relayStarted=mOnlineSession->SendStartLevel((uint8_t)mGameMode,4,1234,true,1234,0);
            fprintf(stderr,"QA RELAY START sent=%d epoch=%u\n",relayStarted,mOnlineSession->GetReadyEpoch());
        }
        if(!relayHost.empty() && connectedAt){
            if(mRunState){mRunState->mPendingPlantPicks=0;mRunState->mPendingBuffPicks=0;}
            if(mOnlineSession->CanReadyPreparation())mOnlineSession->SendSeedsReady(true);
            KillDialog(Dialogs::DIALOG_ONLINE);
        }
        if(pauseQA && connectedAt){
            DWORD age=now-connectedAt;
            auto step=[&](int n,DWORD when){if(age<when || (pauseSteps&(1u<<n)))return false;pauseSteps|=1u<<n;return true;};
            auto press=[&](){mWidgetManager->KeyDown((KeyCode)'P');mWidgetManager->KeyUp((KeyCode)'P');};
            auto check=[&](const char* label,bool expected,bool menu){fprintf(stderr,"QA PAUSE %s role=%c ok=%d paused=%d menu=%d frame=%d\n",label,host?'A':'B',mBoard->mPaused==expected && (GetDialog(DIALOG_NEWOPTIONS)!=nullptr)==menu,mBoard->mPaused,GetDialog(DIALOG_NEWOPTIONS)!=nullptr,mBoard->mMainCounter);fflush(stderr);};
            if(step(0,3000)){pauseFrame=mBoard->mMainCounter;if(host)DoNewOptions(false);}
            if(step(1,5000)){check("menu-local",false,host);fprintf(stderr,"QA MENU advancing=%d\n",mBoard->mMainCounter>pauseFrame);}
            if(step(2,6000)&&host)press();
            if(step(3,7500)){check("P-pauses",true,host);pauseFrame=mBoard->mMainCounter;}
            if(step(4,8500)){fprintf(stderr,"QA FROZEN role=%c ok=%d\n",host?'A':'B',mBoard->mMainCounter==pauseFrame);if(host)KillNewOptionsDialog();}
            if(step(5,9500))check("menu-close-keeps-pause",true,false);
            if(step(6,10500)&&!host)press();
            if(step(7,12000))check("peer-resumes",false,false);
            if(step(8,13000)&&host)DoNewOptions(false);
            if(step(9,14500)&&!host)press();
            if(step(10,16000))check("peer-pause-keeps-menu",true,host);
            if(step(11,17000)&&host)press();
            if(step(12,18500))check("menu-P-resumes",false,host);
            if(age>20000){if(host)KillNewOptionsDialog();mOnlineSession->Close();mShutdown=true;}
            return;
        }
        if(connectedAt && host){
            DWORD age=now-connectedAt;
            if(editStep==0&&age>10000){mBoard->AddPlant(4,1,SEED_WALLNUT,SEED_NONE);editStep++;fprintf(stderr,"QA EDIT plant at=%lu\n",now);}
            if(editStep==1&&age>10200){mBoard->AddPlant(5,1,SEED_PEASHOOTER,SEED_NONE);editStep++;fprintf(stderr,"QA EDIT plant at=%lu\n",now);}
            if(editStep==2&&age>10400){mBoard->MouseDownWithTool(mBoard->GridToPixelX(4,1)+30,mBoard->GridToPixelY(4,1)+40,1,CURSOR_TYPE_SHOVEL);editStep++;fprintf(stderr,"QA EDIT shovel at=%lu\n",now);}
            if(editStep==3&&age>18000){mBoard->mSeedBank->mSeedPackets[0].SetPacketType(SEED_WALLNUT);FullWatch::BoardEdited(mBoard);editStep++;fprintf(stderr,"QA CONFIG change at=%lu\n",now);}

        }
        if(connectedAt && !host) {
            DWORD age=now-connectedAt;
            if(age>3000 && !watchStarted){CoopHud::Action(mBoard,5);watchStarted=true;fprintf(stderr,"QA WATCH via in-game action\n");}
            if(age>25000 && !returned){FullWatch::Close();returned=true;fprintf(stderr,"QA RETURN sun=%d cards=%d\n",mBoard->mSunMoney,mBoard->mSeedBank->mNumPackets);}
            if(age>29000 && !reentered){mBoard->KeyDown(KEYCODE_F10);reentered=true;fprintf(stderr,"QA REENTER via F10\n");}
        }
        if(now-lastReport>3000){lastReport=now;fprintf(stderr,"QA STATE role=%c linked=%d watch=%d sun=%d cards=%d plants=%u zombies=%u frame=%d\n",host?'A':'B',mOnlineSession->IsConnected(),mOnlineSession->IsWatching(),mBoard->mSunMoney,mBoard->mSeedBank->mNumPackets,mBoard->mPlants.mSize,mBoard->mZombies.mSize,mBoard->mMainCounter);fflush(stderr);}
    }
};
// Exercise a mixed-error packet inside the isolated observer, using real entities.
class ObserverProbe : public LawnApp {
    bool checked=false,fullClicked=false,incrementalClicked=false;DWORD started=0;
public:
    void UpdateFrames() override {
        LawnApp::UpdateFrames();
        if(!mBoard || FullWatch::ObserverPaused())return;
        if(!started)started=GetTickCount();
        if(!checked){
            Zombie* zombie=nullptr;if(!mBoard->mZombies.IterateNext(zombie))return;
            checked=true;auto* board=mBoard;
            auto* bullet=board->AddProjectile(400,250,0,2,PROJECTILE_PEA);
            auto zid=board->mZombies.DataArrayGetID(zombie),pid=board->mProjectiles.DataArrayGetID(bullet);
            const float zx=zombie->mPosX,px=bullet->mPosX;const int hp=zombie->mBodyHealth;
            WatchWire::Bytes config,state;WatchWire::Header h;h.epoch=1;h.revision=1;h.seat=1;h.mode=mGameMode;
            bool ok=WatchState::Capture(board,config,state);auto bytes=WatchWire::Pack(h,config,state);ok=ok&&WatchWire::Parse(bytes,h);
            zombie->mPosX+=2;zombie->mX+=2;zombie->mBodyHealth--;
            bullet->mPosX+=20;bullet->mX+=20;
            auto* extra=board->AddProjectile(500,250,0,2,PROJECTILE_PEA);auto extraId=board->mProjectiles.DataArrayGetID(extra);
            ok=ok&&WatchState::Restore(this,bytes,h,true);
            zombie=mBoard->mZombies.DataArrayTryToGet(zid);bullet=mBoard->mProjectiles.DataArrayTryToGet(pid);
            bool partial=ok && mBoard==board && zombie && bullet && zombie->mPosX==zx+2 && zombie->mBodyHealth==hp && bullet->mPosX==px && !mBoard->mProjectiles.DataArrayTryToGet(extraId);
            if(bullet)mBoard->mProjectiles.DataArrayFree(bullet);
            bool added=WatchState::Restore(this,bytes,h,true)&&mBoard->mProjectiles.DataArrayTryToGet(pid);
            bool full=WatchState::Restore(this,bytes,h,false)&&mBoard->mZombies.DataArrayTryToGet(zid)->mPosX==zx;
            fprintf(stderr,"QA INCREMENTAL mixed=%d add=%d full=%d\n",partial,added,full);fflush(stderr);
            if(!partial||!added||!full){mShutdown=true;return;}
        }
        DWORD age=GetTickCount()-started;
        if(age>11000&&!fullClicked){FullWatch::Click(720,78);fullClicked=true;}
        if(age>18000&&!incrementalClicked){FullWatch::Click(720,78);incrementalClicked=true;}
    }
};
int main(int argc,char** argv) {
    pauseQA=GetEnvironmentVariableA("PVZ_WATCH_QA_PAUSE",nullptr,0)>0;
    char role[8]{};GetEnvironmentVariableA("PVZ_WATCH_QA_ROLE",role,sizeof(role));host=role[0]=='A';
    char server[256]{},code[16]{};GetEnvironmentVariableA("PVZ_WATCH_QA_RELAY",server,sizeof(server));GetEnvironmentVariableA("PVZ_WATCH_QA_CODE",code,sizeof(code));relayHost=server;relayCode=code;
    if(auto f=(BOOL(WINAPI*)())GetProcAddress(GetModuleHandleA("user32.dll"),"SetProcessDPIAware"))f();
    TodStringListSetColors(gLawnStringFormats,gLawnStringFormatCount);
    gGetCurrentLevelName=LawnGetCurrentLevelName;gAppCloseRequest=LawnGetCloseRequest;
    gAppHasUsedCheatKeys=LawnHasUsedCheatKeys;gExtractResourcesByName=ExtractResourcesByName;
    gLawnApp=FullWatch::IsObserver()?static_cast<LawnApp*>(new ObserverProbe()):new ProbeApp();
    gLawnApp->mChangeDirTo=".";gLawnApp->mCmdLineParsed=true;
    gLawnApp->Init();gLawnApp->Start();gLawnApp->Shutdown();delete gLawnApp;gLawnApp=nullptr;
    return 0;
}
