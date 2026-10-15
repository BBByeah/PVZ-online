#pragma once
#include "NetSession.h"
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
inline bool Active(const NetSession* s){return s&&s->IsActive()&&s->GetState()!=NetSession::State::DEAD;}
inline std::string State(const NetSession* s) {
    if(!s)return ModText::Tr("未连接","Offline");
    switch(s->GetState()) {
    case NetSession::State::LISTENING:return ModText::Tr("等待队友","Waiting for players");
    case NetSession::State::CONNECTING:return ModText::Tr("正在连接","Connecting");
    case NetSession::State::HANDSHAKING:return ModText::Tr("正在握手","Handshaking");
    case NetSession::State::CONNECTED:return s->IsBuildDifferent()?ModText::Tr("已连接（版本不同）","Connected (builds differ)"):ModText::Tr("已连接","Connected");
    case NetSession::State::RECONNECTING:return ModText::Tr("正在重连","Reconnecting");
    case NetSession::State::DEAD:return ModText::Tr("连接中断 / 失败","Disconnected / failed");
    default:return ModText::Tr("未连接","Offline");
    }
}
inline std::string Identity(const NetSession* s) {
    if(!Active(s))return ModText::Tr("尚未加入房间","No room joined");
    std::string text=s->IsRelay()?ModText::Tr("中继房间 ","Relay room ")+s->GetRoomCode():ModText::Tr("局域网直连","Direct LAN");
    text+="  |  "+std::to_string(s->GetOccupiedSeatCount())+"/"+std::to_string(NetProto::MAX_PLAYERS)+ModText::Tr("席"," seats");
    if(s->GetLocalSeat())text+=ModText::Tr("  |  你 P","  |  You P")+std::to_string(s->GetLocalSeat());
    if(s->GetHostSeat())text+=ModText::Tr("  |  房主 P","  |  Host P")+std::to_string(s->GetHostSeat());
    return text;
}
inline std::string Summary(LawnApp* app) {
    auto* s=Session(app);
    if(!s||!s->IsConnected())return State(s);
    if(s->IsBuildDifferent())return State(s);
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
    if(count<2)return ModText::Tr("目前单人；队友加入后形成接力链","Solo for now; relay starts when teammates join");
    return line+ModText::Tr("（末席漏怪，全队失败）"," (last-seat leak loses the run)");
}
inline bool CanSwap(LawnApp* app,int seat) {
    auto* s=Session(app);
    return s&&s->IsConnected()&&!app->mBoard&&!app->mOnlineWaitingStartAck&&
        !s->IsSwapRequestPending()&&!s->HasIncomingSwapRequest()&&
        seat!=s->GetLocalSeat()&&s->IsSeatOccupied((uint8_t)seat)&&!s->IsSeatOffline((uint8_t)seat);
}
}
