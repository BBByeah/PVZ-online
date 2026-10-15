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
        Zombie* normal=mBoard->AddZombieInRow(ZOMBIE_NORMAL,1,0);
        Zombie* relay=mBoard->AddRelayedZombie(2,ZOMBIE_TRAFFIC_CONE,270,370,0,0,0,270,370,0,0,2);
        Zombie* giant=mBoard->AddRelayedZombie(3,ZOMBIE_GARGANTUAR,3000,0,0,0,0,3000,0,0,0,2);
        Zombie* zombies[]={normal,relay,giant};
        for(int i=0;i<3;++i){zombies[i]->mPosX=450;zombies[i]->mX=450;}
        MemoryImage image(this);image.Create(800,600);Graphics g(&image);
        mBoard->Draw(&g);save(image,"fixed.bmp");
        for(int i=0;i<3;++i){Rect r=zombies[i]->GetZombieRect();std::printf("type=%d marked=%d body=%d,%d,%d,%d marker=%d,%d\n",zombies[i]->mZombieType,mBoard->IsRelayedZombie(zombies[i]),r.mX,r.mY,r.mWidth,r.mHeight,r.mX+r.mWidth/2-9,r.mY-24);}
        // Reproduce the old ordering in the real sprite renderer, with the rest of the scene identical.
        mBoard->mRelayedZombieIds.clear();mBoard->Draw(&g);
        for(int i=1;i<3;++i){auto* z=zombies[i];Graphics local(g);z->BeginDraw(&local);
            local.SetColor(Color(35,30,20,230));local.FillRect(38,-9,18,18);local.SetColor(Color(255,215,70));
            local.FillRect(45,-7,4,8);local.FillRect(41,-1,12,3);local.FillRect(43,2,8,3);local.FillRect(45,5,4,2);
            z->Draw(&local);z->EndDraw(&local);
        }
        save(image,"old-order.bmp");
        mBoard->MarkRelayedZombie(relay);mBoard->MarkRelayedZombie(giant);
        auto* child=mBoard->mZombies.DataArrayAlloc();child->ZombieInitialize(3,ZOMBIE_IMP,false,giant,0);
        bool inherited=mBoard->IsRelayedZombie(child);
        unsigned int ids[]={mBoard->mZombies.DataArrayGetID(normal),mBoard->mZombies.DataArrayGetID(relay),mBoard->mZombies.DataArrayGetID(giant),mBoard->mZombies.DataArrayGetID(child)};
        std::string path=std::string(outRoot)+"relay-save.dat";
        bool saved=LawnSaveGame(mBoard,path);mBoard->mRelayedZombieIds.clear();
        bool loaded=LawnLoadGame(mBoard,path);
        bool retained=loaded && !mBoard->mRelayedZombieIds.count(ids[0]) && mBoard->mRelayedZombieIds.count(ids[1]) && mBoard->mRelayedZombieIds.count(ids[2]) && mBoard->mRelayedZombieIds.count(ids[3]);
        std::ifstream in(path,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(in)),{});in.close();
        bool hasTail=bytes.size()>=20 && bytes[bytes.size()-20]=='R' && bytes[bytes.size()-19]=='L';
        bytes.resize(bytes.size()-20);std::string old=std::string(outRoot)+"old-save.dat";
        std::ofstream out(old,std::ios::binary);out.write(bytes.data(),bytes.size());out.close();
        bool oldLoaded=LawnLoadGame(mBoard,old) && mBoard->mRelayedZombieIds.empty();
        std::printf("CHECK inherited=%d saved=%d loaded=%d retained=%d tail=%d old_compatible=%d\n",inherited,saved,loaded,retained,hasTail,oldLoaded);
        std::fflush(stdout);ExitProcess(inherited&&saved&&retained&&hasTail&&oldLoaded?0:5);
    }
};
int main(int argc,char** argv){outRoot=std::filesystem::absolute(argc>1?argv[1]:"relay-marker-probe").string()+"/";std::filesystem::create_directories(outRoot);if(auto f=(BOOL(WINAPI*)())GetProcAddress(GetModuleHandleA("user32.dll"),"SetProcessDPIAware"))f();gExtractResourcesByName=ExtractResourcesByName;
    auto* app=new ProbeApp();gLawnApp=app;app->mCmdLineParsed=true;app->mProdName="PvZRelayMarkerProbe";
    app->mTitle="Relay marker probe";app->mChangeDirTo=std::filesystem::absolute(argc>2?argv[2]:".").string();
    TodStringListSetColors(gLawnStringFormats,gLawnStringFormatCount); std::fprintf(stderr,"INIT\n"); gGetCurrentLevelName=LawnGetCurrentLevelName;gAppCloseRequest=LawnGetCloseRequest;gAppHasUsedCheatKeys=LawnHasUsedCheatKeys;app->Init();std::fprintf(stderr,"START\n");app->Start();return 2;
}
