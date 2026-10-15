#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/System/PlayerInfo.h"
#include "SexyAppFramework/graphics/MemoryImage.h"
#include "SexyAppFramework/graphics/Graphics.h"
#include "Resources.h"
#include "Sexy.TodLib/TodStringFile.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "Lawn/System/SaveGame.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/CursorObject.h"
#include "SexyAppFramework/widget/WidgetManager.h"
using namespace Sexy;
// Standalone probe: arg1=output directory, arg2=runtime resources. It uses an isolated product profile.
static std::string outRoot;
static void save(MemoryImage& image,const char* name) {
    std::string path=std::string(outRoot)+name;FILE* f=fopen(path.c_str(),"wb");
    BITMAPFILEHEADER fh={};BITMAPINFOHEADER ih={};
    fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=fh.bfOffBits+800*600*4;
    ih.biSize=sizeof(ih);ih.biWidth=800;ih.biHeight=-600;ih.biPlanes=1;ih.biBitCount=32;ih.biSizeImage=800*600*4;
    fwrite(&fh,sizeof(fh),1,f);fwrite(&ih,sizeof(ih),1,f);fwrite(image.GetBits(),800*600*4,1,f);fclose(f);
}
class ProbeApp:public LawnApp {
public:
    void DoParseCmdLine() override {mCmdLineParsed=true;}
    void ReadFromRegistry() override {mRegKey="";mReadFromRegistry=true;mIsWindowed=true;}
    void UpdateFrames() override {
        if(!mLoadingThreadCompleted){LawnApp::UpdateFrames();return;}
        std::fprintf(stderr,"READY RESOURCES\n"); if(!mPlayerInfo)mPlayerInfo=new PlayerInfo();
        mPlayerInfo->mLevel=1;mGameMode=GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
        MakeNewBoard();mBoard->InitLevel();mGameScene=SCENE_PLAYING;
        auto* z=mBoard->AddZombieInRow(ZOMBIE_TRAFFIC_CONE,2,0);z->mPosX=z->mX=450;
        std::fprintf(stderr,"ADD PLANT\n"); mBoard->AddPlant(2,1,SEED_SUNFLOWER,SEED_NONE);
        std::fprintf(stderr,"SEED BANK\n"); mBoard->mSunMoney=9990;mBoard->mShowShovel=true;mActive=true;mMinimized=false;
        mBoard->mSeedBank->mNumPackets=10;
        for(int i=0;i<10;++i)mBoard->mSeedBank->mSeedPackets[i].SetPacketType(SEED_SUNFLOWER);
        mBoard->mSeedBank->mSeedPackets[0].SetPacketType(SEED_SUNFLOWER);
        mBoard->mSeedBank->mSeedPackets[1].SetPacketType(SEED_PEASHOOTER);
        std::fprintf(stderr,"FOCUS\n"); mWidgetManager->SetFocus(mBoard);
        std::fprintf(stderr,"CONNECT\n"); if(!mOnlineSession)mOnlineSession=new NetSession(); NetSession peer;bool connected=mOnlineSession->StartHost(28362)&&peer.StartJoin("127.0.0.1",28362);
        for(int i=0;i<200&&(!mOnlineSession->IsConnected()||!peer.IsConnected());++i){mOnlineSession->Update();peer.Update();Sleep(2);}
        connected=connected&&mOnlineSession->IsConnected()&&peer.IsConnected();
        auto press=[&](int k){mWidgetManager->KeyDown((KeyCode)k);mWidgetManager->KeyUp((KeyCode)k);};
        MemoryImage image(this);image.Create(800,600);Graphics g(&image);
        auto render=[&](const char* name){mBoard->Draw(&g);save(image,name);return std::vector<ulong>(image.GetBits(),image.GetBits()+800*600);};
        std::fprintf(stderr,"RENDER\n"); auto base=render("health-off.bmp");press('H');bool hChat=!mBoard->mChatPanelOpen;auto h=render("zombie-health.bmp");
        press('J');auto hj=render("both-health.bmp");
        mWidgetManager->KeyDown((KeyCode)'H',true);mWidgetManager->KeyUp((KeyCode)'H');auto held=render("held-h.bmp");
        press('H');auto j=render("plant-health.bmp");press('J');auto off=render("health-restored.bmp");
        press(KEYCODE_F5);press(KEYCODE_F7);bool noFnAlias=!mBoard->mChatPanelOpen&&!mBoard->mWatchKeyHeld;
        press('2');bool top2=mBoard->mCursorObject->mCursorType==CURSOR_TYPE_PLANT_FROM_BANK&&mBoard->mCursorObject->mSeedBankIndex==1;
        press('1');bool top1=mBoard->mCursorObject->mSeedBankIndex==0;
        press('0');bool top0=mBoard->mCursorObject->mSeedBankIndex==9;
        bool topRow=top1&&top2&&top0;
        press(KEYCODE_NUMPAD2);bool numpad=mBoard->mCursorObject->mCursorType==CURSOR_TYPE_PLANT_FROM_BANK&&mBoard->mCursorObject->mSeedBankIndex==1&&!GetDialog(DIALOG_COOP_PLAY);
        mBoard->RefreshSeedPacketFromCursor();press('T');bool chat=mBoard->mChatPanelOpen;
        mBoard->mChatInputCooldown=0;press(KEYCODE_NUMPAD2);bool chatChoice=!mBoard->mChatPanelOpen&&mBoard->mCursorObject->mCursorType==CURSOR_TYPE_NORMAL;
        mDebugKeysEnabled=true;auto count=mBoard->mZombies.mSize;
        mBoard->KeyChar('h');mBoard->KeyChar('j');mBoard->KeyChar('H');mBoard->KeyChar('J');bool noSpawn=count==mBoard->mZombies.mSize;
        // Compare glyph pixels only: reanimation Draw has small sprite changes between frames.
        auto white=[](const std::vector<ulong>& pixels,int x,int y,int w,int h){int n=0;for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx){auto c=pixels[yy*800+xx];if((c&255)>=250&&((c>>8)&255)>=250&&((c>>16)&255)>=250)++n;}return n;};
        auto zw=[&](const std::vector<ulong>& v){return white(v,475,210,65,40);};
        auto pw=[&](const std::vector<ulong>& v){return white(v,210,155,70,30);};
        bool renderOk=zw(h)>zw(base)&&pw(hj)>pw(h)&&zw(held)==zw(hj)&&pw(held)==pw(hj)&&zw(off)==zw(base)&&pw(off)==pw(base);
        std::printf("PIXELS zombie_off=%d zombie_H=%d plant_off=%d plant_J=%d held_z=%d restored_z=%d restored_p=%d\n",zw(base),zw(h),pw(base),pw(hj),zw(held),zw(off),pw(off));
        std::printf("CHECK connected=%d h_no_chat=%d health_render=%d no_fn_alias=%d top_row_1_2_0=%d optional_keypad2=%d chat_T=%d chat_numpad2=%d no_debug_spawn=%d\n",connected,hChat,renderOk,noFnAlias,topRow,numpad,chat,chatChoice,noSpawn);
        std::fflush(stdout);ExitProcess(connected&&hChat&&renderOk&&noFnAlias&&topRow&&numpad&&chat&&chatChoice&&noSpawn?0:5);
    }
};
int main(int argc,char** argv){outRoot=std::filesystem::absolute(argc>1?argv[1]:"keyboard-health-probe").string()+"/";std::filesystem::create_directories(outRoot);if(auto f=(BOOL(WINAPI*)())GetProcAddress(GetModuleHandleA("user32.dll"),"SetProcessDPIAware"))f();gExtractResourcesByName=ExtractResourcesByName;
    auto* app=new ProbeApp();gLawnApp=app;app->mCmdLineParsed=true;app->mProdName="PvZKeyboardHealthProbe";
    app->mTitle="Keyboard health probe";app->mChangeDirTo=std::filesystem::absolute(argc>2?argv[2]:".").string();
    TodStringListSetColors(gLawnStringFormats,gLawnStringFormatCount); std::fprintf(stderr,"INIT\n"); gGetCurrentLevelName=LawnGetCurrentLevelName;gAppCloseRequest=LawnGetCloseRequest;gAppHasUsedCheatKeys=LawnHasUsedCheatKeys;app->Init();std::fprintf(stderr,"START\n");app->Start();return 2;
}
