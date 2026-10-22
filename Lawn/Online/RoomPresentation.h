#pragma once
#include "NetSession.h"
#include "RelayNodes.h"
#include "PrivateEndpoint.h"
#include "../../LawnApp.h"
#include "../ModText.h"
#include <string>

// @pvz-online: presentation only; all roles, relay targets and tiers come from the live session.
namespace RoomPresentation {
inline NetSession* Session(LawnApp* app) {
#ifdef __ANDROID__
    return app->mOnlineSession.get();
#else
    return app->mOnlineSession;
#endif
}
inline bool Active(const NetSession* s){return s&&s->IsActive()&&(s->GetState()!=NetSession::State::DEAD||s->IsRelay());}
inline std::string State(const NetSession* s) {
    if(!s)return ModText::Tr("未连接","Offline");
    switch(s->GetState()) {
    case NetSession::State::LISTENING:return ModText::Tr("等待队友","Waiting for players");
    case NetSession::State::CONNECTING:return ModText::Tr("正在连接","Connecting");
    case NetSession::State::HANDSHAKING:return ModText::Tr("正在握手","Handshaking");
    case NetSession::State::CONNECTED:if(s->IsWaitingForNextRound(s->GetLocalSeat()))return ModText::Tr("已归队，等待下一关","Rejoined - next stage");return s->IsBuildDifferent()?ModText::Tr("已连接（版本不同）","Connected (builds differ)"):ModText::Tr("已连接","Connected");
    case NetSession::State::RECONNECTING:return ModText::Tr("正在重连","Reconnecting");
    case NetSession::State::DEAD:return ModText::Tr("连接中断 / 失败","Disconnected / failed");
    default:return ModText::Tr("未连接","Offline");
    }
}
inline std::string Identity(const NetSession* s) {
    if(!Active(s))return ModText::Tr("尚未加入房间","No room joined");
    std::string text=s->IsRelay()?ModText::Tr("房间 ","Room ")+(s->IsPrivateRoom()?s->GetRoomCode():RelayNodes::ShareCode({s->GetRelayHost(),s->GetRelayPort()},s->GetRoomCode())):ModText::Tr("局域网直连","Direct LAN");
    text+="  |  "+std::to_string(s->GetOccupiedSeatCount())+"/"+std::to_string(NetProto::MAX_PLAYERS)+ModText::Tr("席"," seats");
    if(s->GetLocalSeat())text+=ModText::Tr("  |  你 P","  |  You P")+std::to_string(s->GetLocalSeat());
    if(s->GetHostSeat())text+=ModText::Tr("  |  房主 P","  |  Host P")+std::to_string(s->GetHostSeat());
    return text;
}
// @pvz-online: the room header follows the actual host; every seat owns its build label.
inline uint16_t SeatBuild(const NetSession* s,int seat) {
    if(!s||!s->IsSeatOccupied((uint8_t)seat))return 0;
    return seat==s->GetLocalSeat()?NetProto::MOD_BUILD:s->GetSeatBuild((uint8_t)seat);
}
inline std::string BuildLabel(uint16_t build) {
    return build?"MOD"+std::to_string(build):ModText::Tr("待同步","Pending");
}
inline std::string HostVersion(const NetSession* s) {
    return ModText::Tr("房主版本 ","Host version ")+BuildLabel(SeatBuild(s,s?s->GetHostSeat():0));
}
inline std::string SeatVersion(const NetSession* s,int seat) {
    return BuildLabel(SeatBuild(s,seat));
}
inline std::string ReadySummary(const NetSession* s) {
    int ready=0,total=0;
    if(s)for(int seat=1;seat<=NetProto::MAX_PLAYERS;++seat)if(s->IsSeatOccupied((uint8_t)seat)) {
        ++total;if(s->IsSeatSeedsReady((uint8_t)seat))++ready;
    }
    return ModText::Tr("已准备 ","Ready ")+std::to_string(ready)+"/"+std::to_string(total);
}
inline std::string WaitingSeats(const NetSession* s,bool ack=false) {
    std::string seats;
    if(s)for(int seat=1;seat<=NetProto::MAX_PLAYERS;++seat) {
        if(!s->IsSeatOccupied((uint8_t)seat))continue;
        if(ack?!s->IsSeatAwaitingStartAck((uint8_t)seat):s->IsSeatSeedsReady((uint8_t)seat))continue;
        if(!seats.empty())seats+=", ";seats+="P"+std::to_string(seat);
        if(s->IsSeatOffline((uint8_t)seat))seats+=ModText::Tr("(断线)","(offline)");
    }
    return seats.empty()?ModText::Tr("等待同步","Synchronizing"):
        ModText::Tr("等待 ","Waiting: ")+seats;
}
inline bool ShowingPreparation(LawnApp* app) {
    return app->mBoard && (app->mOnlineSeedsHeld || app->mRunIntroHeld);
}
inline std::string SeatProgress(LawnApp* app,int seat) {
    auto* s=Session(app);
    if(!s||!s->IsSeatOccupied((uint8_t)seat))return ModText::Tr("空席","Empty");
    if(s->IsWaitingForNextRound((uint8_t)seat))return ModText::Tr("等待下一关","Next stage");
    if(s->IsSeatOffline((uint8_t)seat))return ModText::Tr("断线保留","Held offline");
    if(s->GetState()==NetSession::State::RECONNECTING)return ModText::Tr("重新连接中","Reconnecting");
    if(app->mOnlineWaitingStartAck && s->IsHostSeat())
        return s->IsSeatAwaitingStartAck((uint8_t)seat)?ModText::Tr("等待接受开局","Awaiting start reply"):ModText::Tr("已在房间","In room");
    if(ShowingPreparation(app))return s->IsSeatSeedsReady((uint8_t)seat)?
        ModText::Tr("已准备","Ready"):ModText::Tr("尚未准备","Not ready");
    return ModText::Tr("在线","Online");
}
inline std::string Summary(LawnApp* app) {
    auto* s=Session(app);
    if(!s||!s->IsConnected())return State(s);
    if(s->IsBuildDifferent())return State(s);
    if(app->mOnlineSeedsHeld)return ReadySummary(s)+" · "+WaitingSeats(s);
    if(app->mOnlineWaitingStartAck)return ModText::Tr("开局中 · 等待队友","Starting · waiting for players");
    if(s->HasIncomingSwapRequest())return ModText::Tr("收到换位申请 · 打开房间","Swap request · open room");
    if(s->IsSwapRequestPending())return ModText::Tr("换位申请已发出 · 等待答复","Swap sent · awaiting approval");
    return State(s);
}
inline std::string CompactRoster(const NetSession* s) {
    if(!Active(s))return ModText::Tr("点击查看连接设置","Open connection settings");
    if(!s->GetLocalSeat())return ModText::Tr("席位分配中","Waiting for a seat");
    return ModText::Tr("你 P","You P")+std::to_string(s->GetLocalSeat())+ModText::Tr(" · 房主 P"," · Host P")+std::to_string(s->GetHostSeat())+
        " · "+std::to_string(s->GetOccupiedSeatCount())+"/6"+ModText::Tr("人 · 点此查看"," · Open room");
}
inline std::string SeatName(const NetSession* s,int seat) {
    if(!Active(s)||!s->IsSeatOccupied((uint8_t)seat))return "P"+std::to_string(seat)+ModText::Tr("  空席","  Empty");
    std::string text="P"+std::to_string(seat)+"  "+s->GetSeatName((uint8_t)seat);
    if(seat==s->GetLocalSeat())text+=ModText::Tr(" [你]"," [You]");
    if(seat==s->GetHostSeat())text+=ModText::Tr(" [房主]"," [Host]");
    if(s->IsSeatOffline((uint8_t)seat))text+=ModText::Tr(" [断线保留]"," [Held offline]");
    return text;
}
inline std::string Order(const NetSession* s) {
    if(!Active(s))return ModText::Tr("加入房间后显示接力顺序","Join a room to see relay order");
    std::string line=ModText::Tr("漏怪接力：","Leak relay: ");int count=0;
    for(int seat=1;seat<=NetProto::MAX_PLAYERS;++seat)if(s->IsSeatOccupied((uint8_t)seat)) {
        if(count++)line+=ModText::Tr(" → "," -> ");line+="P"+std::to_string(seat);
    }
    if(count==0)return ModText::Tr("席位分配后显示接力顺序","Relay order appears once seats are assigned");
    if(count==1)return ModText::Tr("目前单人；队友加入后形成接力链","Solo for now; relay starts when teammates join");
    return line+ModText::Tr("（末席漏怪，全队失败）"," (last-seat leak loses the run)");
}
// @pvz-online: explain why preparation is held instead of showing an inert ready button.
inline std::string PreparationStatus(NetSession* s) {
    if(!s || !s->IsPreparationActive())return ReadySummary(s);
    switch(s->GetPreparationPhase()) {
    case PreparationWire::WAIT:return ModText::Tr("等待全员完成奖励并归队","Waiting for all rewards and reconnections");
    case PreparationWire::ASK:return ModText::Tr("换位协商中 · 准备已取消","Swap pending / everyone is unready");
    case PreparationWire::COMMIT:return ModText::Tr("正在同步最终席位，请稍候","Synchronizing the final seats");
    case PreparationWire::SEAL:return ModText::Tr("正在保存全队开局状态","Saving every player's starting state");
    case PreparationWire::GO:return ModText::Tr("全员确认，准备开始","Everyone confirmed / starting");
    default:return ModText::Tr("关间准备 · ","Preparation / ")+ReadySummary(s);
    }
}
inline bool CanSwap(LawnApp* app,int seat) {
    auto* s=Session(app);
    return s&&s->IsConnected()&&(!app->mBoard||s->CanChangePreparationSeats())&&!app->mOnlineWaitingStartAck&&
        !s->IsSwapRequestPending()&&!s->HasIncomingSwapRequest()&&
        seat!=s->GetLocalSeat()&&s->IsSeatOccupied((uint8_t)seat)&&!s->IsSeatOffline((uint8_t)seat);
}
}
