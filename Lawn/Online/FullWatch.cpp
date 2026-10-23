#include "FullWatch.h"
#include "NetSession.h"
#include "../../LawnApp.h"
#include "../Board.h"
#include "../Run/RunState.h"
#include "../System/SaveGame.h"
#include "../Widget/TitleScreen.h"
#include "../Widget/CoopHud.h"
#include "../Widget/GameButton.h"
#include "../../SexyAppFramework/widget/ButtonWidget.h"
#include "../../SexyAppFramework/Common.h"
#include "../../Sexy.TodLib/TodDebug.h"
#include "../SeedPacket.h"
#include <filesystem>
#include <fstream>
#include <cstring>

// @pvz-online: MOD63 full-board spectator, ported to source APIs. Native saves
// contain process-global effects: restore only in an isolated child process.
namespace FullWatch {
namespace {
namespace fs=std::filesystem;
constexpr DWORD BundleMagic=0x32574650, BundleVersion=1;
constexpr UINT Command=WM_APP+0x331;
constexpr int ObserverProfile=2147483000;
struct Bundle {
    DWORD magic,version,boardShape,sequence,boardBytes,runBytes;
    int gameMode,runMode,scene;
    DWORD paused,relay,seat;
};
static_assert(sizeof(Bundle)==48);
LawnApp* app=nullptr;
PROCESS_INFORMATION child{};
HANDLE childJob=nullptr;
WNDPROC previousProc=nullptr;
DWORD captured=0,sequence=0,receivedAt=0,loadedSequence=0,pollAt=0,childStarted=0;
BYTE currentTarget=0;
bool remotePaused=true,remoteRelay=false,loaded=false;
std::string Env(const char* key) {
    char value[32768]{};DWORD n=GetEnvironmentVariableA(key,value,sizeof(value));
    return n && n<sizeof(value)?std::string(value,n):std::string();
}
HWND Parent() {static HWND value=reinterpret_cast<HWND>(strtoull(Env("PVZ_FULL_WATCH_PARENT").c_str(),nullptr,10));return value;}
fs::path Root() {
    if(IsObserver())return fs::path(ObserverDataRoot());
    wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
    return fs::path(temp)/(L"PvZFullWatch-"+std::to_wstring(GetCurrentProcessId()));
}
bool Write(const fs::path& path,const void* bytes,size_t count) {
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    return out.write(static_cast<const char*>(bytes),count).flush().good();
}
std::vector<unsigned char> Read(const fs::path& path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    auto n=in.tellg();if(n<=0 || n>snapshot::MaxRaw)return {};
    std::vector<unsigned char> bytes(static_cast<size_t>(n));
    in.seekg(0);if(!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))return {};
    return bytes;
}
bool Validate(const std::vector<unsigned char>& bytes,Bundle& h) {
    if(bytes.size()<sizeof(h))return false;
    memcpy(&h,bytes.data(),sizeof(h));
    return h.magic==BundleMagic && h.version==BundleVersion && h.boardShape==sizeof(Board) &&
        h.boardBytes>=16 && h.boardBytes<=snapshot::MaxRaw && h.runBytes<=snapshot::MaxRaw &&
        uint64_t(sizeof(h))+h.boardBytes+h.runBytes==bytes.size() && h.paused<=1 &&
        h.relay<=1 && h.seat>=1 && h.seat<=NetProto::MAX_PLAYERS &&
        h.gameMode>=0 && h.gameMode<NUM_GAME_MODES;
}
void Select(int seat) {
    if(!app || !app->mBoard || !app->mOnlineSession)return;
    auto* s=app->mOnlineSession;auto* b=app->mBoard;
    if(seat==0){Close();return;}
    if(seat<1 || seat>NetProto::MAX_PLAYERS || seat==s->GetLocalSeat() || !s->IsSeatOccupied((BYTE)seat))return;
    StopChild();
    if(s->IsWatching())s->EndWatch(true);
    if(s->BeginWatch((BYTE)seat)){b->mWatchKeyHeld=true;b->mCoopWatchLatched=true;b->mWatchLastSeat=(BYTE)seat;}
}
LRESULT CALLBACK ParentProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==Command){Select(static_cast<int>(w));return 0;}
    return CallWindowProcW(previousProc,window,message,w,l);
}
bool StartChild() {
    std::error_code ec;
    fs::create_directories(Root()/"userdata",ec);if(ec)return false;
    fs::remove(Root()/"ready",ec);
    // Clone profile metadata, not active boards/checkpoints. All child writes stay here.
    const fs::path source=fs::path(Sexy::GetAppDataFolder())/"userdata";
    for(fs::directory_iterator it(source,ec),end;!ec && it!=end;it.increment(ec)) {
        auto name=it->path().filename().string();
        if(it->is_regular_file(ec) && (name=="users.dat" || (name.starts_with("user") && name.ends_with(".dat"))))
            fs::copy_file(it->path(),Root()/"userdata"/it->path().filename(),fs::copy_options::overwrite_existing,ec);
    }
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
    std::wstring command=L"\""+std::wstring(exe)+L"\"";
    const auto oldParent=Env("PVZ_FULL_WATCH_PARENT"),oldDir=Env("PVZ_FULL_WATCH_DIR");
    SetEnvironmentVariableA("PVZ_FULL_WATCH_PARENT",std::to_string(reinterpret_cast<uintptr_t>(app->mHWnd)).c_str());
    SetEnvironmentVariableA("PVZ_FULL_WATCH_DIR",(Root().string()+"\\").c_str());
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    bool ok=CreateProcessW(exe,command.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED,nullptr,nullptr,&startup,&child)!=FALSE;
    SetEnvironmentVariableA("PVZ_FULL_WATCH_PARENT",oldParent.empty()?nullptr:oldParent.c_str());
    SetEnvironmentVariableA("PVZ_FULL_WATCH_DIR",oldDir.empty()?nullptr:oldDir.c_str());
    if(!ok)return false;
    childJob=CreateJobObjectW(nullptr,nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(childJob){SetInformationJobObject(childJob,JobObjectExtendedLimitInformation,&limits,sizeof(limits));AssignProcessToJobObject(childJob,child.hProcess);}
    ResumeThread(child.hThread);CloseHandle(child.hThread);child.hThread=nullptr;childStarted=GetTickCount();return true;
}
bool Restore(const std::vector<unsigned char>& bytes,const Bundle& h) {
    std::unique_ptr<RunState> run;
    const auto* boardData=bytes.data()+sizeof(h);
    if(h.runBytes) {
        run=std::make_unique<RunState>();
        auto file=RunState::GetCheckpointName(ObserverProfile,h.runMode,h.scene);
        // No fallback to an older checkpoint if the current generation is invalid.
        std::error_code ec;fs::remove(file+".bak",ec);fs::remove(file+".pending",ec);
        if(!Write(file,boardData+h.boardBytes,h.runBytes) || !run->Load(ObserverProfile,h.runMode,h.scene))return false;
        auto check=run->EncodeCheckpoint();
        if(check.size()!=h.runBytes || memcmp(check.data(),boardData+h.boardBytes,h.runBytes))return false;
    }
    const auto boardFile=Root()/"remote-board.dat";
    if(!Write(boardFile,boardData,h.boardBytes))return false;
    app->KillBoard();delete app->mRunState;app->mRunState=run.release();
    app->mGameMode=static_cast<GameMode>(h.gameMode);
    app->MakeNewBoard();
    // @pvz-online: the Board wrapper also loads scene artwork and refreshes draw layers.
    if(!app->mBoard->LoadGame(boardFile.string())){app->KillBoard();return false;}
    app->mBoard->mMenuButton->mBtnNoDraw=true;
    app->mBoard->mMenuButton->mDisabled=true;
    remotePaused=h.paused!=0;remoteRelay=h.relay!=0;currentTarget=(BYTE)h.seat;
    loadedSequence=h.sequence;receivedAt=GetTickCount();loaded=true;
    app->mBoard->Pause(remotePaused);
    TodLog("[watch] restore seq=%u seat=%u board=%u run=%u sun=%d cards=%d",h.sequence,h.seat,h.boardBytes,h.runBytes,app->mBoard->mSunMoney,app->mBoard->mSeedBank->mNumPackets);
    Write(Root()/"ready",&loadedSequence,sizeof(loadedSequence));
    MoveWindow(app->mHWnd,0,0,800,600,TRUE);
    ShowWindow(app->mHWnd,SW_SHOW);SetFocus(app->mHWnd);return true;
}
}
bool IsObserver() {static bool value=!Env("PVZ_FULL_WATCH_PARENT").empty()&&!Env("PVZ_FULL_WATCH_DIR").empty();return value;}
std::string ObserverDataRoot(){return Env("PVZ_FULL_WATCH_DIR");}
bool ObserverPaused(){return remotePaused || !loaded || GetTickCount()-receivedAt>15000;}
void PrepareApp(LawnApp* value) {
    app=value;
    if(!IsObserver())return;
    app->mOnlyAllowOneCopyToRun=false;app->mIsWindowed=true;
    app->mRegKey+="\\FullWatch";app->mAutoEnable3D=false;
    app->mMuteCount++;app->mMusicVolume=0;app->mAllowAltEnter=false;
}
void AttachWindow(void* handle) {
    HWND window=static_cast<HWND>(handle);
    if(!IsObserver()) {
        previousProc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(ParentProc)));return;
    }
    SetParent(window,Parent());
    SetWindowLongPtrW(window,GWL_STYLE,WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
    SetWindowPos(window,HWND_TOP,0,0,800,600,SWP_FRAMECHANGED|SWP_NOACTIVATE);
}
void StopChild() {
    if(child.hProcess){TerminateProcess(child.hProcess,0);WaitForSingleObject(child.hProcess,1000);CloseHandle(child.hProcess);}
    if(childJob)CloseHandle(childJob);childJob=nullptr;child={};currentTarget=0;
    if(app && app->mHWnd)SetFocus(app->mHWnd);
}
void Close() {
    if(IsObserver()){PostMessageW(Parent(),Command,0,0);return;}
    if(app && app->mOnlineSession && app->mOnlineSession->IsWatching())app->mOnlineSession->EndWatch(true);
    if(app && app->mBoard){app->mBoard->mWatchKeyHeld=false;app->mBoard->mCoopWatchLatched=false;}
    StopChild();
}
void TickApp(LawnApp* value) {
    app=value;
    if(IsObserver()) {
        if(!IsWindow(Parent())){app->mShutdown=true;return;}
        if(app->mTitleScreen){if(app->mTitleScreen->mLoadingThreadComplete)app->LoadingCompleted();return;}
        if(!app->mLoadingThreadCompleted)return;
        DWORD now=GetTickCount();if(now-pollAt<200)return;pollAt=now;
        auto bytes=Read(Root()/"snapshot.dat");Bundle h{};
        if(Validate(bytes,h) && (!loaded || h.sequence!=loadedSequence)) {
            if(!Restore(bytes,h)){app->mShutdown=true;return;}
        }
        if(app->mBoard)app->mBoard->Pause(ObserverPaused());
        return;
    }
    auto* s=app->mOnlineSession;
    if(!app->mBoard || !s || !s->IsWatching()){
        if(child.hProcess)StopChild();
        if(app->mBoard){app->mBoard->mWatchKeyHeld=false;app->mBoard->mCoopWatchLatched=false;}
        return;
    }
    if(child.hProcess && currentTarget!=s->GetWatchTargetSeat())StopChild();
    if(child.hProcess && WaitForSingleObject(child.hProcess,0)==WAIT_OBJECT_0){Close();return;}
    if(child.hProcess && GetTickCount()-childStarted>30000) {
        std::error_code ec;if(!fs::exists(Root()/"ready",ec)){Close();return;}
    }
    std::vector<unsigned char> bytes;Bundle h{};
    if(!s->TakeFullWatch(bytes) || !Validate(bytes,h) || h.seat!=s->GetWatchTargetSeat())return;
    std::error_code ec;fs::create_directories(Root(),ec);if(ec)return;
    const auto temp=Root()/"snapshot.tmp",dest=Root()/"snapshot.dat";
    if(!Write(temp,bytes.data(),bytes.size()) || !MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING))return;
    if(!child.hProcess){if(!StartChild()){Close();return;}currentTarget=s->GetWatchTargetSeat();}
}
void TickBoard(Board* board) {
    if(IsObserver())return;
    auto* s=board->mApp->mOnlineSession;
    if(!s || !s->IsConnected() || !s->HasBoardWatchers())return;
    DWORD now=GetTickCount();if(captured && now-captured<5000)return;captured=now;
    std::error_code ec;fs::create_directories(Root(),ec);if(ec)return;
    auto file=Root()/"capture.dat";
    if(!LawnSaveGame(board,file.string()))return;
    auto rawBoard=Read(file);if(rawBoard.empty())return;
    auto* run=board->mApp->mRunState;
    auto rawRun=run?run->EncodeCheckpoint():std::vector<unsigned char>();
    Bundle h{BundleMagic,BundleVersion,sizeof(Board),++sequence,(DWORD)rawBoard.size(),(DWORD)rawRun.size(),
        (int)board->mApp->mGameMode,run?run->mMode:0,run?run->mEndlessScene:0,
        board->mPaused?1u:0u,s->GetRelayTargetSeat()?1u:0u,s->GetLocalSeat()};
    std::vector<unsigned char> bytes(sizeof(h)+rawBoard.size()+rawRun.size());
    memcpy(bytes.data(),&h,sizeof(h));memcpy(bytes.data()+sizeof(h),rawBoard.data(),rawBoard.size());
    if(!rawRun.empty())memcpy(bytes.data()+sizeof(h)+rawBoard.size(),rawRun.data(),rawRun.size());
    snapshot::Buffer packet{};
    if(s->mFullWatch->codec.encode(bytes.data(),(DWORD)bytes.size(),sequence,now,packet)) {
        TodLog("[watch] capture seq=%u raw=%u packed=%u",sequence,(unsigned)bytes.size(),packet.size);
        BYTE seats[NetProto::MAX_PLAYERS];int n=s->GetBoardWatcherSeats(seats,NetProto::MAX_PLAYERS);
        for(int i=0;i<n;++i)s->SendBoardSnapshot(seats[i],packet.data,packet.size);
    }
    snapshot::release(packet);
}
bool Key(int key) {
    if(!app)return false;
    if(IsObserver()) {
        if(key==VK_ESCAPE || key=='V' || key==VK_F10)Close();
        else if(key>='1' && key<='6')PostMessageW(Parent(),Command,key-'0',0);
        return true;
    }
    auto* s=app->mOnlineSession;auto* b=app->mBoard;
    if(!s || !b)return false;
    if(key=='V' || key==VK_F10) {
        if(s->IsWatching())Close();else Select(b->PickDefaultWatchSeat());return true;
    }
    if(s->IsWatching()) {
        if(key==VK_ESCAPE)Close();else if(key>='1' && key<='6')Select(key-'0');
        return true;
    }
    return false;
}
void Click(int x,int y){if(IsObserver() && x>=665 && x<790 && y>=8 && y<54)Close();}
void DrawControls(Sexy::Graphics* g) {
    if(IsObserver())CoopHud::SmallButton(g,Sexy::Rect(665,8,125,46),ModText::Tr("返回","Back"),true);
}
bool IsRelayedExit(Board*){return IsObserver() && remoteRelay;}
}
