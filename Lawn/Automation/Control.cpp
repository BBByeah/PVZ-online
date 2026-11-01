#include "../Run/RunState.h"
#include "../Online/FullWatch.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Control.h"
#include "Diagnostics.h"
#include "../../LawnApp.h"
#include "../Board.h"
#include "../TeamPlay.h"
#include "../Widget/TeamProgressionDialog.h"
#include "../Plant.h"
#include "../Zombie.h"
#include "../Projectile.h"
#include "../Coin.h"
#include "../SeedPacket.h"
#include "../Online/NetSession.h"
#include "../../SexyAppFramework/widget/WidgetManager.h"
#include "../../SexyAppFramework/widget/ButtonWidget.h"
#include "../../SexyAppFramework/widget/EditWidget.h"
#include "../../SexyAppFramework/widget/Dialog.h"
#include "../../third_party/nlohmann/json.hpp"
#include <sddl.h>
#include <filesystem>
#include <deque>
#include <chrono>
#include <stdexcept>
using nlohmann::json;
namespace MachineControl {
namespace {
bool localWatch=true,edgeKill=false;
std::string instance;HANDLE pipe=INVALID_HANDLE_VALUE;bool connected=false,closeQueued=false;std::string input,output;size_t written=0;
json queuedAction;std::map<std::string,json> completed;int dispatchDepth=0;
ULONGLONG connectedAt=0,lastStats=0;int lastScene=-1,lastLevel=-1,lastWatch=-1;bool lastComplete=false;std::string lastStatus;
void Require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void Click(LawnApp*a,int x,int y,int button=1){Require(x>=0&&x<800&&y>=0&&y<600,"outside_game_canvas");a->mWidgetManager->MouseMove(x,y);a->mWidgetManager->MouseDown(x,y,button);a->mWidgetManager->MouseUp(x,y,button);}
void Key(LawnApp*a,int k){Require(k>=0&&k<255,"invalid_key");a->mWidgetManager->KeyDown(static_cast<Sexy::KeyCode>(k));a->mWidgetManager->KeyUp(static_cast<Sexy::KeyCode>(k));}
void Widgets(Sexy::WidgetContainer*c,json&out,int ox=0,int oy=0,int depth=0){
 if(depth>20)return;for(auto*w:c->mWidgets){if(!w->mVisible)continue;int x=ox+w->mX,y=oy+w->mY;
  json j={{"x",x},{"y",y},{"w",w->mWidth},{"h",w->mHeight},{"enabled",!w->mDisabled}};
  if(auto*b=dynamic_cast<Sexy::ButtonWidget*>(w)){j["kind"]="button";j["id"]=b->mId;j["label"]=Sexy::SexyStringToString(b->mLabel);out.push_back(j);}
  if(auto*e=dynamic_cast<Sexy::EditWidget*>(w)){j["kind"]="edit";j["text"]=Sexy::SexyStringToString(e->mString);out.push_back(j);}
  if(auto*d=dynamic_cast<Sexy::Dialog*>(w)){j["kind"]="dialog";j["id"]=d->mId;j["title"]=Sexy::SexyStringToString(d->mDialogHeader);j["text"]=Sexy::SexyStringToString(d->mDialogLines);out.push_back(j);}
  if(auto*t=dynamic_cast<TeamProgressionDialog*>(w))for(auto&v:t->Controls())out.push_back({{"kind","team_control"},{"label",v.label},{"action",int(v.kind)},{"value",v.value},{"enabled",v.enabled},{"x",x+v.rect.mX},{"y",y+v.rect.mY},{"w",v.rect.mWidth},{"h",v.rect.mHeight}});
  Widgets(w,out,x,y,depth+1);
 }
}
json State(LawnApp*a){
 json j={{"watch_engine",LocalWatch()?"local":"author"},{"edge_kill",EdgeKill()},{"observer_pid",FullWatch::ChildPid()},{"observer_restored_sequence",FullWatch::RestoredSequence()},{"instance",instance},{"pid",GetCurrentProcessId()},{"scene",int(a->mGameScene)},{"loading",!a->mLoadingThreadCompleted},{"data_dir",Sexy::GetAppDataFolder()},{"widgets",json::array()}};Widgets(a->mWidgetManager,j["widgets"]);
 if(a->mTeamProgressionController){auto t=a->mTeamProgressionController->Read();json choices=json::array();for(auto&v:t.choices)choices.push_back({{"id",v.id},{"name",v.name},{"description",v.description},{"enabled",v.enabled}});j["team"]={{"can_vote",t.canVote},{"vote_open",t.voteOpen},{"status",t.status},{"choices",choices}};}
 if(a->mRunState)j["run"]={{"seed",a->mRunState->mRunSeed},{"mode",a->mRunState->mMode},{"difficulty",a->mRunState->mDiff}};
 auto*s=a->mOnlineSession;if(s){j["seats"]=json::array();for(int seat=1;seat<=6;++seat)if(s->IsSeatOccupied(seat))j["seats"].push_back({{"seat",seat},{"name",s->GetSeatName(seat)}});j["room_peers"]=json::array();for(int peer=1;peer<=6;++peer)if(peer!=s->GetLocalSeat()&&s->IsSeatOccupied(peer))j["room_peers"].push_back({{"seat",peer},{"ready",s->RoomPeerReady(peer)},{"failed",s->RoomPeerFailed(peer)}});j["network"]={{"shared_paused",s->IsSharedPaused()},{"connected",s->IsConnected()},{"room",s->GetRoomCode()},{"seat",s->GetLocalSeat()},{"watch_target",s->GetWatchTargetSeat()},{"p2p",s->ObserverDiagnosticState()},{"p2p_enabled",s->ObserverP2PEnabled()},{"rtt_ms",s->ObserverRtt()},{"picture_age_ms",s->ObserverPictureAge()}};}
 if(auto*b=a->mBoard){
  json board={{"main_counter",b->mMainCounter},{"level",b->mLevel},{"wave",b->mCurrentWave},{"num_waves",b->mNumWaves},{"sun",b->mSunMoney},{"paused",b->mPaused},{"complete",b->mLevelComplete},{"plants",json::array()},{"zombies",json::array()},{"coins",json::array()},{"cards",json::array()},{"projectiles",json::array()}};
  Plant*p=nullptr;while(b->IteratePlants(p))board["plants"].push_back({{"id",b->mPlants.DataArrayGetID(p)},{"type",int(p->mSeedType)},{"row",p->mRow},{"x",p->mX},{"y",p->mY},{"hp",p->mPlantHealth}});
  Zombie*z=nullptr;while(b->IterateZombies(z))board["zombies"].push_back({{"id",b->mZombies.DataArrayGetID(z)},{"type",int(z->mZombieType)},{"row",z->mRow},{"x",z->mPosX},{"y",z->mPosY},{"hp",z->mBodyHealth}});
  Coin*c=nullptr;while(b->IterateCoins(c))board["coins"].push_back({{"id",b->mCoins.DataArrayGetID(c)},{"type",int(c->mType)},{"sun",c->IsSun()},{"collecting",c->mIsBeingCollected},{"x",c->mPosX},{"y",c->mPosY}});
  Projectile*p2=nullptr;while(b->IterateProjectiles(p2))board["projectiles"].push_back({{"id",b->mProjectiles.DataArrayGetID(p2)},{"x",p2->mPosX},{"y",p2->mPosY}});
  if(b->mSeedBank)for(int n=0;n<b->mSeedBank->mNumPackets;n++){auto&c=b->mSeedBank->mSeedPackets[n];board["cards"].push_back({{"slot",n},{"type",int(c.mPacketType)},{"ready",c.CanPickUp()},{"cooldown",c.mRefreshCounter},{"cooldown_total",c.mRefreshTime},{"cost",b->GetCurrentPlantCost(c.mPacketType,c.mImitaterType)}});}
  j["board"]=std::move(board);
 }return j;
}
json Execute(LawnApp*a,const json&r){
 // Enable widget input routing for this dispatch only; never change OS focus.
 struct InputScope{Sexy::WidgetManager*m;bool old;InputScope(Sexy::WidgetManager*v):m(v),old(v->mHasFocus){m->mHasFocus=true;}~InputScope(){m->mHasFocus=old;}} inputScope(a->mWidgetManager);
 std::string op=r.at("op").get<std::string>();if(op=="state")return State(a);
 if(op=="metrics"){auto&c=TestDiagnostics::counters;return {{"time_ms",GetTickCount()},{"raw",c.raw.load()},{"packed",c.packed.load()},{"captures",c.captures.load()},{"queued",c.queued.load()},{"tcp_tx",c.tcpTx.load()},{"tcp_rx",c.tcpRx.load()},{"udp_tx",c.udpTx.load()},{"udp_rx",c.udpRx.load()},{"udp_watch_tx",c.udpWatchTx.load()},{"udp_watch_rx",c.udpWatchRx.load()},{"tcp_observer_tx",c.tcpObserverTx.load()},{"tcp_local_tx",c.tcpLocalTx.load()},{"tcp_local_rx",c.tcpLocalRx.load()},{"complete",c.complete.load()},{"displayed",c.displayed.load()}};}
 if(op=="command.result"){auto id=r.at("command_id").dump();auto it=completed.find(id);if(it==completed.end())return {{"pending",true}};return it->second;}
 Require(a->mLoadingThreadCompleted,"loading_in_progress");
 if(r.contains("scene"))Require(int(a->mGameScene)==r["scene"].get<int>(),"scene_changed");
 if(op=="ui.click")Click(a,r.at("x"),r.at("y"),r.value("button",1));
 else if(op=="ui.button"){
  json w=json::array();Widgets(a->mWidgetManager,w);std::vector<json> matches;
  for(auto&v:w)if(v.value("kind","")=="button"&&v["enabled"].get<bool>()&&v.value("label","")==r.at("label").get<std::string>())matches.push_back(v);
  Require(matches.size()==1,"button_missing_disabled_or_ambiguous");auto&v=matches[0];Click(a,v["x"].get<int>()+v["w"].get<int>()/2,v["y"].get<int>()+v["h"].get<int>()/2);
 }
 else if(op=="ui.key")Key(a,r.at("key"));
 else if(op=="ui.text"){
  auto text=r.at("text").get<std::string>();Require(text.size()<=256,"text_too_long");int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);Require(n>0,"invalid_utf8");
  std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),w.data(),n);for(auto ch:w)a->mWidgetManager->KeyChar(ch);
 }
 else if(op=="team.vote"){
  Require(a->mTeamProgressionController!=nullptr,"no_team_vote");auto t=a->mTeamProgressionController->Read();Require(t.canVote,"vote_not_ready");int choice=r.value("choice",-1);Require(choice==-1||std::any_of(t.choices.begin(),t.choices.end(),[&](auto&v){return v.id==choice&&v.enabled;}),"invalid_choice");TeamProgressionView::Intent i;i.action=TeamProgressionView::Action::VOTE;i.context=t.context;i.choice=choice;i.value=choice==-1?-1:int(std::find_if(t.choices.begin(),t.choices.end(),[&](auto&v){return v.id==choice;})-t.choices.begin());a->mTeamProgressionController->Submit(i);
 }
 else if(op=="watch.test_hold_frames"){TestDiagnostics::holdTestWatchFrames=r.at("enabled").get<bool>();}
 else if(op=="p2p.test_block_udp"){TestDiagnostics::blockTestUdp=r.at("enabled").get<bool>();}
 else if(op=="p2p.set"){a->mObserverP2P=r.at("enabled").get<bool>();if(a->mOnlineSession)a->mOnlineSession->SetObserverP2PEnabled(a->mObserverP2P);}
 else if(op=="watch.start"||op=="watch.refresh"){auto*s=a->mOnlineSession;Require(s&&s->IsConnected(),"not_connected");int seat=op=="watch.refresh"?s->GetWatchTargetSeat():r.at("seat").get<int>();Require(seat>=1&&seat<=6,"invalid_seat");Require(s->BeginWatch(uint8_t(seat)),"watch_rejected_or_cooldown");if(a->mBoard){a->mBoard->mWatchKeyHeld=true;a->mBoard->mWatchLastSeat=seat;a->mBoard->mCoopWatchLatched=true;}}
 else if(op=="watch.stop"){Require(a->mOnlineSession!=nullptr,"not_connected");a->mOnlineSession->EndWatch(true);}
 else if(op=="plant"||op=="shovel"||op=="collect"){
  auto*b=a->mBoard;Require(b&&a->mGameScene==GameScenes::SCENE_PLAYING&&!b->mPaused,"not_playing");Require(a->GetDialogCount()==0,"dialog_open");Require(!a->mOnlineSession||!a->mOnlineSession->IsWatching(),"watching_read_only");
  if(op=="collect"){
   Coin*c=nullptr;bool hit=false;while(b->IterateCoins(c))if(b->mCoins.DataArrayGetID(c)==r.at("coin_id").get<unsigned>()&&!c->mIsBeingCollected){Click(a,int(c->mPosX)+15,int(c->mPosY)+15);hit=true;break;}Require(hit,"coin_missing_or_collecting");
  }else{
   int col=r.at("col"),row=r.at("row");Require(col>=0&&col<9&&row>=0&&row<6,"invalid_cell");int x=b->GridToPixelX(col,row)+40,y=b->GridToPixelY(col,row)+40;
   int before=b->mPlants.mSize;
   if(op=="plant") {int slot=r.at("slot");Require(b->mSeedBank&&slot>=0&&slot<b->mSeedBank->mNumPackets,"invalid_slot");auto&c=b->mSeedBank->mSeedPackets[slot];Require(c.CanPickUp(),"card_unavailable_sun_or_cooldown");Click(a,c.mX+b->mSeedBank->mX+20,c.mY+b->mSeedBank->mY+20);Click(a,x,y);if(b->mPlants.mSize<=before){Click(a,x,y,-1);throw std::runtime_error("plant_rejected");}}
   else{int count=b->mPlantsShoveled;Click(a,640,40);Click(a,x,y);Require(b->mPlantsShoveled>count,"shovel_rejected");}
  }
 }
 else if(op=="pause"){Require(a->UsesManualOnlinePause(),"shared_pause_not_available");bool enabled=r.at("enabled");if(a->mOnlineSession->IsSharedPaused()!=enabled)Require(a->HandleOnlinePauseKey('P'),"pause_rejected");}
 else if(op=="menu.open"){Require(a->mBoard!=nullptr,"no_board");if(!a->GetDialog(Dialogs::DIALOG_NEWOPTIONS)){Require(a->CanPauseNow(),"menu_not_available");a->DoNewOptions(false);}}
 else if(op=="menu.close"){if(a->GetDialog(Dialogs::DIALOG_NEWOPTIONS))Key(a,27);}
 else if(op=="window.position"){
  int x=r.at("x"),y=r.at("y");SetWindowPos(a->mHWnd,nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
 }
 else if(op=="close")closeQueued=true;
 else throw std::runtime_error("unknown_operation");
 return {{"accepted",true},{"scene",int(a->mGameScene)}};
}
void Disconnect(){DisconnectNamedPipe(pipe);connected=false;input.clear();output.clear();written=0;queuedAction=nullptr;}
void OpenPipe(){
 HANDLE token=nullptr;Require(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)!=0,"token_open_failed");DWORD size=0;GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<char> data(size);
 bool ok=GetTokenInformation(token,TokenUser,data.data(),size,&size)!=0;CloseHandle(token);Require(ok,"token_query_failed");LPSTR sid=nullptr;Require(ConvertSidToStringSidA(reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid,&sid)!=0,"sid_failed");
 std::string acl="D:P(A;;GA;;;"+std::string(sid)+")";LocalFree(sid);PSECURITY_DESCRIPTOR sd=nullptr;Require(ConvertStringSecurityDescriptorToSecurityDescriptorA(acl.c_str(),SDDL_REVISION_1,&sd,nullptr)!=0,"acl_failed");SECURITY_ATTRIBUTES sa{sizeof(sa),sd,FALSE};
 std::string name="\\\\.\\pipe\\PVZ-MOD104-test-"+instance;
 pipe=CreateNamedPipeA(name.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT|0x8,1,65536,65536,0,&sa);LocalFree(sd);Require(pipe!=INVALID_HANDLE_VALUE,"pipe_create_failed");
 TestDiagnostics::Line("control_listening",name);
}
}
bool LocalWatch(){return localWatch;}
bool EdgeKill(){return edgeKill;}
bool Enabled(){return !instance.empty();}const std::string& Instance(){return instance;}
bool Parameter(const std::string&name,const std::string&value){if(name=="-watchengine"){Require(value=="local"||value=="author","invalid_engine");localWatch=value=="local";return true;}if(name=="-bench-edge-kill"){edgeKill=value=="1";return true;}if(name!="-control")return false;Require(!value.empty()&&value.size()<=32&&value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==std::string::npos,"invalid_instance");instance=value;return true;}
void Configure(LawnApp*a){if(!Enabled())return;a->mProdName="PVZ-MOD104-test-"+instance;a->mStringProperties["RegistryKey"]=Sexy::StringToWString("PVZ-MOD104-test\\"+instance);a->mTitle="PVZ MOD104 test - "+instance;}
void Tick(LawnApp*a){
 if(!lastStats){TestDiagnostics::Initialize(FullWatch::IsObserver()?FullWatch::ObserverLogInstance():instance);TestDiagnostics::Line("startup","MOD104 52d7ebc room-p2p PWV2 test v2 instance="+instance);lastStats=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
 auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();auto*s=a->mOnlineSession;std::string status=s?s->ObserverDiagnosticState():"idle";
 int level=a->mBoard?a->mBoard->mLevel:-1,watch=s?s->GetWatchTargetSeat():0;bool complete=a->mBoard&&a->mBoard->mLevelComplete;
 if(level!=lastLevel||watch!=lastWatch||complete!=lastComplete){TestDiagnostics::Line("progress",json({{"level",level},{"complete",complete},{"watch_target",watch}}).dump());lastLevel=level;lastWatch=watch;lastComplete=complete;}
 if(lastScene!=int(a->mGameScene)||lastStatus!=status){TestDiagnostics::Line("transition",json({{"scene",int(a->mGameScene)},{"p2p",status}}).dump());lastScene=int(a->mGameScene);lastStatus=status;}
 if(now-lastStats>=5000){auto&c=TestDiagnostics::counters;json j={{"raw",c.raw.load()},{"packed",c.packed.load()},{"captures",c.captures.load()},{"capture_failed",c.captureFailed.load()},{"queued",c.queued.load()},{"udp_tx",c.udpTx.load()},{"udp_rx",c.udpRx.load()},{"udp_watch_tx",c.udpWatchTx.load()},{"udp_watch_rx",c.udpWatchRx.load()},{"tcp_tx_all",c.tcpTx.load()},{"tcp_rx_all",c.tcpRx.load()},{"tcp_observer_tx",c.tcpObserverTx.load()},{"tcp_local_tx",c.tcpLocalTx.load()},{"tcp_local_rx",c.tcpLocalRx.load()},{"complete",c.complete.load()},{"displayed",c.displayed.load()},{"dropped",c.dropped.load()},{"invalid",c.invalid.load()},{"crc_failed",c.crcFailed.load()},{"p2p",status},{"interval_ms",now-lastStats}};if(s){j["rtt_ms"]=s->ObserverRtt();j["picture_age_ms"]=s->ObserverPictureAge();}TestDiagnostics::Line("observer_totals",j.dump());lastStats=now;}
 if(!Enabled())return;
 try{
  if(pipe==INVALID_HANDLE_VALUE)OpenPipe();
  if(!connected){BOOL ok=ConnectNamedPipe(pipe,nullptr);auto e=GetLastError();if(!ok&&e!=ERROR_PIPE_CONNECTED)return;connected=true;connectedAt=now;}
  if(now-connectedAt>10000){Disconnect();if(closeQueued)a->mCloseRequest=true;return;}
  if(!output.empty()){
   if(written<output.size()){DWORD n=0;BOOL ok=WriteFile(pipe,output.data()+written,DWORD(std::min<size_t>(4096,output.size()-written)),&n,nullptr);if(!ok&&GetLastError()!=ERROR_NO_DATA){Disconnect();return;}written+=n;}
   // Client sends a single ACK byte after reading the entire line; don't discard buffered output early.
   else{char ack;DWORD n=0;if(ReadFile(pipe,&ack,1,&n,nullptr)&&n){
    auto action=std::move(queuedAction);queuedAction=nullptr;Disconnect();
    if(!action.is_null()){
      auto id=action.at("id");json result;dispatchDepth++;
      try{result={{"pending",false},{"ok",true},{"result",Execute(a,action)}};}catch(const std::exception&e){result={{"pending",false},{"ok",false},{"error",e.what()}};}
      dispatchDepth--;if(completed.size()>=128)completed.erase(completed.begin());completed[id.dump()]=result;TestDiagnostics::Line("command_complete",json({{"id",id},{"op",action["op"]},{"outcome",result}}).dump());
    }
    if(closeQueued)a->mCloseRequest=true;
   }}return;
  }
  char data[4096];DWORD n=0;BOOL ok=ReadFile(pipe,data,sizeof(data),&n,nullptr);if(!ok&&GetLastError()!=ERROR_NO_DATA){Disconnect();return;}input.append(data,n);if(input.size()>65536){Disconnect();return;}
  if(input.find('\n')==std::string::npos)return;
  json response,id=nullptr;std::string op;
  try{auto r=json::parse(input.substr(0,input.find('\n')));id=r.at("id");Require(id.is_string()||id.is_number_integer(),"invalid_id");op=r.at("op");json result;
   if(op=="ui.click"||op=="ui.button"||op=="ui.key"||op=="menu.open"||op=="menu.close"||op=="pause"){
    Require(dispatchDepth<8,"nested_command_limit");queuedAction=r;result={{"queued",true},{"command_id",id}};
   }else result=Execute(a,r);response={{"id",id},{"ok",true},{"result",result}};}
  catch(const std::exception&e){response={{"id",id},{"ok",false},{"error",e.what()}};}
  output=response.dump(-1,' ',false,json::error_handler_t::replace)+"\n";
  if(op!="state"&&op!="metrics")TestDiagnostics::Line("command",json({{"id",id},{"op",op},{"ok",response["ok"]},{"error",response.value("error","")}}).dump());
 }catch(const std::exception&e){TestDiagnostics::Line("control_error",e.what());if(pipe!=INVALID_HANDLE_VALUE)Disconnect();}
}
void Stop(){if(pipe!=INVALID_HANDLE_VALUE){DisconnectNamedPipe(pipe);CloseHandle(pipe);pipe=INVALID_HANDLE_VALUE;}TestDiagnostics::Close();}
}
