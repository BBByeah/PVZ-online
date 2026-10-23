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
#include "SexyAppFramework/widget/WidgetManager.h"
#include "Sexy.TodLib/TodStringFile.h"
#include <filesystem>
#include <cstdio>

// Standalone QA entry point, linked against the same game objects. Not shipped.
bool (*gAppCloseRequest)();
bool (*gAppHasUsedCheatKeys)();
SexyString (*gGetCurrentLevelName)();
using namespace Sexy;
static bool host=false;
class ProbeApp : public LawnApp {
    bool ready=false,watchStarted=false,returned=false,reentered=false;
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
        mWidgetManager->SetFocus(mBoard);
        mOnlineSession=new NetSession();
        bool ok=host?mOnlineSession->StartHost(28463):mOnlineSession->StartJoin("127.0.0.1",28463);
        fprintf(stderr,"QA START role=%c net=%d data=%s\n",host?'A':'B',ok,GetAppDataFolder().c_str());fflush(stderr);
        ready=true;SetWindowPos(mHWnd,HWND_TOP,host?20:840,50,0,0,SWP_NOSIZE|SWP_NOACTIVATE);
    }
    void UpdateFrames() override {
        if(mTitleScreen && mTitleScreen->mLoadingThreadComplete)LoadingCompleted();
        LawnApp::UpdateFrames();
        DWORD now=GetTickCount();
        if(now-begun>65000){FullWatch::Close();mShutdown=true;return;}
        if(!ready || !mBoard || !mOnlineSession)return;
        if(mOnlineSession->IsConnected() && !connectedAt)connectedAt=now;
        if(connectedAt && !host) {
            DWORD age=now-connectedAt;
            if(age>3000 && !watchStarted){CoopHud::Action(mBoard,5);watchStarted=true;fprintf(stderr,"QA WATCH via in-game action\n");}
            if(age>25000 && !returned){FullWatch::Close();returned=true;fprintf(stderr,"QA RETURN sun=%d cards=%d\n",mBoard->mSunMoney,mBoard->mSeedBank->mNumPackets);}
            if(age>29000 && !reentered){mBoard->KeyDown(KEYCODE_F10);reentered=true;fprintf(stderr,"QA REENTER via F10\n");}
        }
        if(now-lastReport>3000){lastReport=now;fprintf(stderr,"QA STATE role=%c linked=%d watch=%d sun=%d cards=%d plants=%u zombies=%u frame=%d\n",host?'A':'B',mOnlineSession->IsConnected(),mOnlineSession->IsWatching(),mBoard->mSunMoney,mBoard->mSeedBank->mNumPackets,mBoard->mPlants.mSize,mBoard->mZombies.mSize,mBoard->mMainCounter);fflush(stderr);}
    }
};
int main(int argc,char** argv) {
    char role[8]{};GetEnvironmentVariableA("PVZ_WATCH_QA_ROLE",role,sizeof(role));host=role[0]=='A';
    if(auto f=(BOOL(WINAPI*)())GetProcAddress(GetModuleHandleA("user32.dll"),"SetProcessDPIAware"))f();
    TodStringListSetColors(gLawnStringFormats,gLawnStringFormatCount);
    gGetCurrentLevelName=LawnGetCurrentLevelName;gAppCloseRequest=LawnGetCloseRequest;
    gAppHasUsedCheatKeys=LawnHasUsedCheatKeys;gExtractResourcesByName=ExtractResourcesByName;
    gLawnApp=FullWatch::IsObserver()?new LawnApp():new ProbeApp();
    gLawnApp->mChangeDirTo=".";gLawnApp->mCmdLineParsed=true;
    gLawnApp->Init();gLawnApp->Start();gLawnApp->Shutdown();delete gLawnApp;gLawnApp=nullptr;
    return 0;
}
