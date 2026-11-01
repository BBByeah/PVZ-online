#include "../Automation/Control.h"
#include "../Automation/Diagnostics.h"
#include "FullWatch.h"
#include "WatchState.h"
#include "NetSession.h"
#include "../../LawnApp.h"
#include "../Board.h"
#include "../Run/RunState.h"
#include "../Widget/TitleScreen.h"
#include "../Widget/CoopHud.h"
#include "../Widget/GameButton.h"
#include "../../SexyAppFramework/widget/ButtonWidget.h"
#include "../../SexyAppFramework/widget/WidgetManager.h"
#include "../../SexyAppFramework/Common.h"
#include "../../Sexy.TodLib/TodDebug.h"
#include "../SeedPacket.h"
#include "../MessageWidget.h"
#include "../../SexyAppFramework/graphics/Graphics.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <chrono>

// @pvz-online: PWV2 field-based spectator state. Process-global effects stay
// isolated in the child; native saves and RUN1 checkpoints are no longer sent.
namespace FullWatch {
namespace {
namespace fs=std::filesystem;
constexpr UINT Command=WM_APP+0x331;
constexpr UINT GiftCommand=WM_APP+0x332;
constexpr UINT PauseCommand=WM_APP+0x333;
using Bundle=WatchWire::Header;
WatchWire::Schedule schedule;
DWORD epoch=GetTickCount()|1u,revision=0;
WatchWire::Bytes lastConfig;
bool lastPaused=false,lastComplete=false;
LawnApp* app=nullptr;
PROCESS_INFORMATION child{};
HANDLE childJob=nullptr;
WNDPROC previousProc=nullptr;
DWORD sequence=0,receivedAt=0,loadedSequence=0,pollAt=0,childStarted=0;
BYTE currentTarget=0;
DWORD activeSubscription=0;
bool childVisible=false;
DWORD lastIndicator=~0u;
DWORD childRetryAt=0;unsigned childFailures=0;
bool remotePaused=true,remoteRelay=false,loaded=false;
bool incrementalUpdates=true,forceFullUpdate=false;
Bundle loadedHeader{};
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
    return WatchWire::Parse(bytes,h) && h.configBytes && h.mode<NUM_GAME_MODES;
}
void Select(int seat) {
    if(!app || !app->mBoard || !app->mOnlineSession)return;
    auto* s=app->mOnlineSession;auto* b=app->mBoard;
    if(seat==0){Close();return;}
    if(seat==7){for(int i=1;i<=6;++i){int next=(s->GetWatchTargetSeat()+i-1)%6+1;if(s->CanWatchSeat((BYTE)next)&&next!=s->GetWatchTargetSeat()){seat=next;break;}}}
    if(seat<1 || seat>NetProto::MAX_PLAYERS || seat==s->GetLocalSeat() || !s->IsSeatOccupied((BYTE)seat))return;
    if(s->IsWatching())s->EndWatch(true);
    if(s->BeginWatch((BYTE)seat)){b->mWatchKeyHeld=true;b->mCoopWatchLatched=true;b->mWatchLastSeat=(BYTE)seat;}
}
LRESULT CALLBACK ParentProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    // @pvz-online: gifts during spectating spend the player's real sun, never the mirrored board's.
    if(message==PauseCommand){if(app)app->HandleOnlinePauseKey('P');return 0;}
    if(message==GiftCommand) {
        if(app && app->mBoard && app->mOnlineSession && app->mOnlineSession->IsWatching() &&
            w>=VK_F1 && w<=VK_F6)app->mBoard->HandleSunGiftKey(static_cast<Sexy::KeyCode>(w));
        return 0;
    }
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
    std::wstring command=L"\""+std::wstring(exe)+L"\" -watchengine=local";
    if(MachineControl::EdgeKill())command+=L" -bench-edge-kill=1";
    const auto oldParent=Env("PVZ_FULL_WATCH_PARENT"),oldDir=Env("PVZ_FULL_WATCH_DIR"),oldLog=Env("PVZ_FULL_WATCH_LOG");
    SetEnvironmentVariableA("PVZ_FULL_WATCH_LOG",(MachineControl::Instance().empty()?"player":MachineControl::Instance()).c_str());
    SetEnvironmentVariableA("PVZ_FULL_WATCH_PARENT",std::to_string(reinterpret_cast<uintptr_t>(app->mHWnd)).c_str());
    SetEnvironmentVariableA("PVZ_FULL_WATCH_DIR",(Root().string()+"\\").c_str());
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    bool ok=CreateProcessW(exe,command.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED,nullptr,nullptr,&startup,&child)!=FALSE;
    SetEnvironmentVariableA("PVZ_FULL_WATCH_PARENT",oldParent.empty()?nullptr:oldParent.c_str());
    SetEnvironmentVariableA("PVZ_FULL_WATCH_DIR",oldDir.empty()?nullptr:oldDir.c_str());
    SetEnvironmentVariableA("PVZ_FULL_WATCH_LOG",oldLog.empty()?nullptr:oldLog.c_str());
    if(!ok)return false;
    childJob=CreateJobObjectW(nullptr,nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(childJob){SetInformationJobObject(childJob,JobObjectExtendedLimitInformation,&limits,sizeof(limits));AssignProcessToJobObject(childJob,child.hProcess);}
    ResumeThread(child.hThread);CloseHandle(child.hThread);child.hThread=nullptr;childStarted=GetTickCount();return true;
}
bool Restore(const std::vector<unsigned char>& bytes,const Bundle& h) {
    const bool firstDisplay=!loaded;
    const bool incremental=incrementalUpdates && !forceFullUpdate && loaded &&
        h.subscription==loadedHeader.subscription && h.epoch==loadedHeader.epoch && h.revision==loadedHeader.revision &&
        h.seat==loadedHeader.seat && h.mode==loadedHeader.mode && h.paused==loadedHeader.paused &&
        GetTickCount()-receivedAt<=15000;
    if(!WatchState::Restore(app,bytes,h,incremental))return false;
    loadedHeader=h;forceFullUpdate=false;
    app->mBoard->mMenuButton->mBtnNoDraw=true;
    app->mBoard->mMenuButton->mDisabled=true;
    remotePaused=h.paused!=0;remoteRelay=h.relay!=0;currentTarget=(BYTE)h.seat;
    loadedSequence=h.sequence;receivedAt=GetTickCount();loaded=true;
    app->mBoard->Pause(remotePaused);
    TodLog("[watch] restore seq=%u seat=%u state=%u config=%u sun=%d cards=%d",h.sequence,h.seat,h.stateBytes,h.configBytes,app->mBoard->mSunMoney,app->mBoard->mSeedBank->mNumPackets);
    TodLog("[watch] update=%s",incremental?"incremental":"full");
    DWORD ready[2]{loadedSequence,h.subscription};Write(Root()/"ready",ready,sizeof(ready));
    // AttachWindow already sizes the child. Repeating window/focus operations
    // on every snapshot synchronizes UI threads and disturbs other windows.
    if(firstDisplay) {
        // Parent shows this window only after the matching subscription is ready.
        // Background control never takes foreground focus.
    }
    return true;
}
}
unsigned RestoredSequence(){auto b=Read(Root()/"ready");return b.size()==8&&WatchWire::Get(b.data()+4)==activeSubscription?WatchWire::Get(b.data()):0;}
unsigned ChildPid(){return child.dwProcessId;}
bool Enabled(){return MachineControl::LocalWatch() || IsObserver();}
bool IsObserver() {static bool value=!Env("PVZ_FULL_WATCH_PARENT").empty()&&!Env("PVZ_FULL_WATCH_DIR").empty();return value;}
bool CoversParent() {
    if(IsObserver() || !app || !child.hProcess || !app->mBoard ||
        app->GetDialogCount()>0 || !app->mOnlineSession || !app->mOnlineSession->IsWatching())return false;
    RECT parent{};GetClientRect(app->mHWnd,&parent);
    if(parent.right<=0 || parent.bottom<=0)return false;
    MapWindowPoints(app->mHWnd,nullptr,reinterpret_cast<POINT*>(&parent),2);
    for(HWND window=GetWindow(app->mHWnd,GW_CHILD);window;window=GetWindow(window,GW_HWNDNEXT)) {
        DWORD pid=0;GetWindowThreadProcessId(window,&pid);
        if(pid!=child.dwProcessId || !IsWindowVisible(window))continue;
        RECT bounds{};if(!GetWindowRect(window,&bounds))continue;
        return bounds.left<=parent.left && bounds.top<=parent.top &&
            bounds.right>=parent.right && bounds.bottom>=parent.bottom;
    }
    return false;
}
std::string ObserverLogInstance(){return "observer-"+Env("PVZ_FULL_WATCH_LOG");}
std::string ObserverDataRoot(){return Env("PVZ_FULL_WATCH_DIR");}
bool ObserverPaused(){return remotePaused || !loaded || GetTickCount()-receivedAt>15000;}
void PrepareApp(LawnApp* value) {
    app=value;
    if(!IsObserver())return;
    app->mOnlyAllowOneCopyToRun=false;app->mIsWindowed=true;
    app->mProdName+="-Watch-"+std::to_string(GetCurrentProcessId());
    app->mRegKey+="\\FullWatch";app->mAutoEnable3D=false;
    app->mMuteCount++;app->mMusicVolume=0;app->mAllowAltEnter=false;
}
void AttachWindow(void* handle) {
    if(!Enabled())return;
    HWND window=static_cast<HWND>(handle);
    if(!IsObserver()) {
        previousProc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(ParentProc)));return;
    }
    SetWindowLongPtrW(window,GWL_EXSTYLE,GetWindowLongPtrW(window,GWL_EXSTYLE)|WS_EX_NOACTIVATE);
    SetParent(window,Parent());
    SetWindowLongPtrW(window,GWL_STYLE,WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
    SetWindowPos(window,HWND_TOP,0,0,800,600,SWP_FRAMECHANGED|SWP_NOACTIVATE);
}
void StopChild() {
    if(child.hProcess){TerminateProcess(child.hProcess,0);WaitForSingleObject(child.hProcess,1000);CloseHandle(child.hProcess);}
    if(childJob)CloseHandle(childJob);childJob=nullptr;child={};currentTarget=0;
    // Keep the desktop focus unchanged.
}
void Close() {
    if(IsObserver()){PostMessageW(Parent(),Command,0,0);return;}
    if(app && app->mOnlineSession && app->mOnlineSession->IsWatching())app->mOnlineSession->EndWatch(true);
    if(app && app->mBoard){app->mBoard->mWatchKeyHeld=false;app->mBoard->mCoopWatchLatched=false;}
}
void TickApp(LawnApp* value) {
    app=value;
    if(!Enabled())return;
    if(IsObserver()) {
        if(!IsWindow(Parent())){app->mShutdown=true;return;}
        if(app->mTitleScreen){if(app->mTitleScreen->mLoadingThreadComplete)app->LoadingCompleted();return;}
        if(!app->mLoadingThreadCompleted)return;
        DWORD now=GetTickCount();if(now-pollAt<50)return;pollAt=now;
        auto active=Read(Root()/"active");DWORD subscription=active.size()==4?WatchWire::Get(active.data()):0;
        if(!subscription){loaded=false;remotePaused=true;if(app->mBoard)app->mBoard->Pause(true);ShowWindow(app->mHWnd,SW_HIDE);return;}
        auto bytes=Read(Root()/"snapshot.dat");Bundle h{};
        if(Validate(bytes,h) && h.subscription==subscription && (!loaded || h.subscription!=loadedHeader.subscription || h.sequence!=loadedSequence)) {
            if(!Restore(bytes,h)){TodLog("[watch] restore_failed subscription=%u seq=%u",h.subscription,h.sequence);return;}
        }
        if(app->mBoard)app->mBoard->Pause(ObserverPaused());
        return;
    }
    auto* s=app->mOnlineSession;
    if(!s || !s->IsConnected()){if(child.hProcess)StopChild();activeSubscription=0;return;}
    if(child.hProcess&&WaitForSingleObject(child.hProcess,0)==WAIT_OBJECT_0){DWORD code=0;GetExitCodeProcess(child.hProcess,&code);TodLog("[watch] child_exit code=%u",code);StopChild();childRetryAt=GetTickCount();++childFailures;}
    if(s->IsWatching()&&s->mFullWatch->subscription!=activeSubscription){childFailures=0;childRetryAt=0;}
    if(!child.hProcess&&(childFailures>=3||(childRetryAt&&GetTickCount()-childRetryAt<5000)))return;
    if(!child.hProcess){
        std::error_code ec;fs::create_directories(Root(),ec);DWORD zero=0;Write(Root()/"active",&zero,4);
        fs::remove(Root()/"snapshot.dat",ec);if(!StartChild()){childRetryAt=GetTickCount();++childFailures;activeSubscription=s->IsWatching()?s->mFullWatch->subscription:0;return;}
        activeSubscription=0;childVisible=false;lastIndicator=~0u;TodLog("[watch] prewarm child=%u",child.dwProcessId);
    }
    DWORD wanted=app->mBoard&&s->IsWatching()?s->mFullWatch->subscription:0;
    HWND childWindow=nullptr;
    for(HWND w=GetWindow(app->mHWnd,GW_CHILD);w;w=GetWindow(w,GW_HWNDNEXT)){DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==child.dwProcessId){childWindow=w;break;}}
    if(wanted!=activeSubscription){
        activeSubscription=wanted;childVisible=false;Write(Root()/"active",&wanted,4);
        if(childWindow)ShowWindow(childWindow,SW_HIDE);
        TodLog("[watch] activate subscription=%u",wanted);
    }
    if(!wanted){if(childWindow)ShowWindow(childWindow,SW_HIDE);return;}
    if(childWindow&&!childVisible&&RestoredSequence()){
        ShowWindow(childWindow,SW_SHOWNOACTIVATE);childVisible=true;
        TestDiagnostics::counters.displayed++;
        TodLog("[watch] first_display subscription=%u elapsed=%llu",wanted,std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()-s->mFullWatch->requestedAt);
    }
    DWORD indicator=strcmp(s->ObserverDiagnosticState(),"direct")==0?2:strcmp(s->ObserverDiagnosticState(),"fallback")==0?3:s->ObserverP2PEnabled()?1:0;
    if(indicator!=lastIndicator){Write(Root()/"indicator",&indicator,4);lastIndicator=indicator;}
    std::vector<unsigned char> bytes;Bundle h{};
    if(!s->TakeFullWatch(bytes) || !Validate(bytes,h) || h.seat!=s->GetWatchTargetSeat())return;
    std::error_code ec;fs::create_directories(Root(),ec);if(ec)return;
    const auto temp=Root()/"snapshot.tmp",dest=Root()/"snapshot.dat";
    if(!Write(temp,bytes.data(),bytes.size()) || !MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING))return;
    currentTarget=s->GetWatchTargetSeat();
}
void BoardCreated(Board*) {
    if(!Enabled())return;
    if(IsObserver())return;
    if(!++epoch)++epoch;revision=0;lastConfig.clear();schedule={};
}
void BoardEdited(Board*) {if(Enabled()&&!IsObserver())schedule.Edit();}
void TickBoard(Board* board) {
    if(IsObserver()||!Enabled()||TestDiagnostics::holdTestWatchFrames)return;
    auto* s=board->mApp->mOnlineSession;
    if(!s || !s->IsConnected() || !s->HasBoardWatchers())return;
    // Never publish a provisional loadout while the seed-selection gate is still up.
    if(board->mApp->mGameScene!=SCENE_PLAYING || board->mApp->mRunIntroHeld || board->mApp->mOnlineSeedsHeld || s->RelayBattleHeld())return;
    BYTE seats[NetProto::MAX_PLAYERS];int n=s->GetBoardWatcherSeats(seats,NetProto::MAX_PLAYERS);
    bool newcomer=false;for(int i=0;i<n;++i)if(s->mFullWatch->ackEpoch[seats[i]]!=epoch || s->mFullWatch->ackRevision[seats[i]]!=revision)newcomer=true;
    if(lastPaused!=board->mPaused || lastComplete!=board->mLevelComplete){schedule.Edit();lastPaused=board->mPaused;lastComplete=board->mLevelComplete;}
    bool urgent=false;for(int i=0;i<n;++i)urgent=urgent||s->mFullWatch->urgent[seats[i]];
    DWORD now=GetTickCount();if(!urgent&&!schedule.Due(now,newcomer))return;
    // Failed capture/compression/queueing must not retry on every simulation frame.
    schedule.Attempt(now);
    const auto captureStart=std::chrono::steady_clock::now();
    WatchWire::Bytes config,state;
    if(!WatchState::Capture(board,config,state))return;
    const auto captureEnd=std::chrono::steady_clock::now();
    if(config!=lastConfig){lastConfig=config;if(!++revision)++revision;}
    TestDiagnostics::counters.captures++;
    Bundle h;h.epoch=epoch;h.revision=revision;h.sequence=++sequence;h.seat=s->GetLocalSeat();
    h.mode=board->mApp->mGameMode;h.paused=board->mPaused?1:0;h.relay=s->GetRelayTargetSeat()?1:0;
    bool sent=false;
    for(int i=0;i<n;++i){BYTE seat=seats[i];bool init=s->mFullWatch->ackEpoch[seat]!=epoch || s->mFullWatch->ackRevision[seat]!=revision;
        h.subscription=s->mFullWatch->viewerSubscription[seat];if(!h.subscription)continue;
        auto bytes=WatchWire::Pack(h,init?config:WatchWire::Bytes{},state);
        TestDiagnostics::counters.raw+=bytes.size();
        snapshot::Buffer packet{};
        if(s->mFullWatch->codec.encode(bytes.data(),static_cast<DWORD>(bytes.size()),sequence,now,packet)){
            TestDiagnostics::counters.packed+=packet.size;
            if(s->SendBoardSnapshot(seat,packet.data,packet.size)){sent=true;s->mFullWatch->urgent[seat]=false;TestDiagnostics::counters.queued+=packet.size;}
            TodLog("[watch] send seq=%u epoch=%u rev=%u to=%u tick=%u init=%u state=%u packed=%u",sequence,epoch,revision,seat,now,init?(unsigned)config.size():0,(unsigned)state.size(),packet.size);
        }
        snapshot::release(packet);
    }
    if(sent)schedule.Sent(now);
    const auto sendEnd=std::chrono::steady_clock::now();
    TodLog("[watch] cost capture=%.3fms encode_queue=%.3fms viewers=%d state=%u",
        std::chrono::duration<double,std::milli>(captureEnd-captureStart).count(),
        std::chrono::duration<double,std::milli>(sendEnd-captureEnd).count(),n,(unsigned)state.size());
}
bool Key(int key) {
    if(!Enabled())return false;
    if(!app)return false;
    if(IsObserver()) {
        if(key=='P'){if(!app->mWidgetManager->mIsKeyRepeat)PostMessageW(Parent(),PauseCommand,0,0);return true;}
        if(key>=VK_F1 && key<=VK_F6) {
            if(!app->mWidgetManager->mIsKeyRepeat)PostMessageW(Parent(),GiftCommand,key,0);
            return true;
        }
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
void Click(int x,int y){
    if(!IsObserver())return;
    if(y>=8&&y<54&&x>=465&&x<555){PostMessageW(Parent(),Command,currentTarget,0);return;}
    if(y>=8&&y<54&&x>=565&&x<655){PostMessageW(Parent(),Command,7,0);return;}
    if(x<665 || x>=790)return;
    if(y>=8 && y<54)Close();
    else if(y>=58 && y<100){
        incrementalUpdates=!incrementalUpdates;
        if(!incrementalUpdates)forceFullUpdate=true;
        TodLog("[watch] mode selected=%s",incrementalUpdates?"incremental":"full");
    }
}
bool DrawLoading(Board* board,Sexy::Graphics* g){
    if(IsObserver()||!Enabled()||!board->mApp->mOnlineSession||!board->mApp->mOnlineSession->IsWatching())return false;
    board->DrawBackdrop(g);
    auto* s=board->mApp->mOnlineSession;
    MessageWidget message(board->mApp);
    message.SetLabel(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()-s->mFullWatch->requestedAt>=10000?
        ModText::Tr("加载失败，请刷新或返回","Loading failed. Refresh or return"):
        ModText::Tr("加载中……","Loading..."),MessageStyle::MESSAGE_STYLE_HINT_STAY);
    message.Draw(g);CoopHud::WatchControls(g,board);return true;
}
void DrawControls(Sexy::Graphics* g) {
    if(IsObserver()){
        auto status=Read(Root()/"indicator");DWORD light=status.size()==4?WatchWire::Get(status.data()):0;
        g->SetColor(light==2?Sexy::Color(70,220,70):light==3?Sexy::Color(255,155,50):light==1?Sexy::Color(245,220,65):Sexy::Color(150,150,150));
        for(int y=-5;y<=5;++y){int w=int(sqrt(double(25-y*y)));g->FillRect(540-w,78+y,2*w+1,1);}
        SeatUi::Text(g,light==2?"P2P":light==3?ModText::Tr("服务器","Server"):light==1?ModText::Tr("连接中","Connecting"):ModText::Tr("已关闭","Off"),552,85,105,16,Sexy::Color(240,230,195));
        CoopHud::SmallButton(g,Sexy::Rect(465,8,90,46),ModText::Tr("刷新","Refresh"),true);
        CoopHud::SmallButton(g,Sexy::Rect(565,8,90,46),ModText::Tr("换人","Next"),true);
        CoopHud::SmallButton(g,Sexy::Rect(665,8,125,46),ModText::Tr("返回","Back"),true);
        CoopHud::SmallButton(g,Sexy::Rect(665,58,125,42),incrementalUpdates?ModText::Tr("增量更新","Incremental"):ModText::Tr("完全更新","Full update"),true);
    }
}
bool IsRelayedExit(Board*){return IsObserver() && (remoteRelay || MachineControl::EdgeKill());}
}
