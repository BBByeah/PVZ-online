#pragma once
#include "OnlineUiDraw.h"
#include "../Online/RoomPresentation.h"

namespace OnlineRoomLayout {
using namespace Sexy;
enum Page { CHOOSE, LOCAL, SERVER };
inline Rect LocalPage(){return Rect(40,88,328,54);}
inline Rect ServerPage(){return Rect(392,88,328,54);}
inline Rect Solo(){return Rect(48,250,664,50);}
inline Rect Server(){return Rect(148,284,292,36);}
inline Rect Code(){return Rect(200,284,508,36);}
inline Rect Address(){return Rect(148,382,292,36);}
inline Rect Create(){return Rect(48,340,320,46);}
inline Rect Join(){return Rect(392,340,320,46);}
inline Rect DirectHost(){return Rect(464,376,116,46);}
inline Rect DirectJoin(){return Rect(596,376,116,46);}
inline Rect Seats(){return Rect(40,480,160,46);}
inline Rect Disconnect(){return Rect(216,480,160,46);}
inline Rect Close(){return Rect(392,480,152,46);}
inline Rect Start(){return Rect(560,480,160,46);}
inline bool CanStart(LawnApp* app) {
    auto* session=RoomPresentation::Session(app);
    return session && session->CanHostStart() && !app->mBoard && !app->mOnlineWaitingStartAck;
}
inline bool NeedsPlayers(const NetSession* session) {
    return session && session->IsRelay() && session->IsActive() && session->GetConnectedPlayerCount(true)<2;
}
inline void Draw(Graphics* g,LawnApp* app,const std::string& detail,const std::string& localIp,int page=CHOOSE) {
    SeatUi::Panel(g,Rect(0,0,760,560));
    auto* s=RoomPresentation::Session(app);const bool active=RoomPresentation::Active(s);
    SeatUi::Text(g,active?(s->IsRelay()?ModText::Tr("在线房间","Online room"):ModText::Tr("局域网房间","LAN room")):
        ModText::Tr("选择游戏方式","Choose how to play"),40,64,488,28,Color(245,230,165));
    if(!active) {
        SeatUi::Strip(g,Rect(40,156,680,70));
        const std::string explanation=page==LOCAL?
            ModText::Tr("自己玩选这里，无需服务器。局域网仅供同一网络内的朋友直连。","Play alone without a server, or connect friends on the same local network."):
            page==SERVER?ModText::Tr("远程组队专用：至少两名在线玩家才能开始。自己玩请选择左侧入口。","Remote co-op needs at least two online players. For solo play, choose the left option."):
            ModText::Tr("自己玩或同一网络内游玩选左侧；不同网络的朋友组队选右侧。","Choose left for solo or same-network play; choose right for friends on different networks.");
        SeatUi::Paragraph(g,explanation,Rect(56,166,648,52),19,Color(235,230,210));
        if(page==SERVER) {
            SeatUi::Text(g,ModText::Tr("自动连接 · 无需选服","Connect automatically"),48,260,664,18,Color(210,220,195));
            SeatUi::Text(g,ModText::Tr("房间码","Code"),48,310,136,20,Color(220,225,210));
            SeatUi::Text(g,ModText::Tr("建房自动分配；从大厅加入，或填写队友发来的完整房间码。","Rooms are assigned automatically. Browse rooms or enter the full room code."),48,420,664,18,Color(190,210,225));
        } else if(page==LOCAL) {
            SeatUi::Text(g,ModText::Tr("单人直接开始，不建房、不联网。","Solo starts directly: no room and no server."),48,332,664,18,Color(190,210,225));
            SeatUi::Text(g,ModText::Tr("局域网游戏 · 同一网络","LAN game - same network"),48,360,664,20,Color(245,230,165));
            SeatUi::Text(g,ModText::Tr("主机IP","Host IP"),48,409,88,20,Color(220,225,210));
            SeatUi::Text(g,ModText::Tr("本机地址：","Local IP: ")+localIp,48,448,664,18,Color(190,210,225));
        }
        if(!detail.empty() && s && s->GetState()!=NetSession::State::OFF)
            SeatUi::Text(g,detail,48,465,664,15,Color(255,175,120));
        return;
    }
    SeatUi::Strip(g,Rect(40,92,680,84));
    SeatUi::Text(g,RoomPresentation::Summary(app),56,121,active?412:648,23,Color(245,230,165));
    if(active)SeatUi::Text(g,RoomPresentation::HostVersion(s),488,121,216,14,Color(145,160,165));
    const std::string explanation=(!s||s->GetState()==NetSession::State::OFF)?
        ModText::Tr("浏览公开大厅，或填写房间码加入队友。","Browse public rooms, or enter a room code to join your team."):detail;
    SeatUi::Paragraph(g,s&&app->mBoard&&s->RelayBattleHeld()?
        ModText::Tr("等待队友或重连，本局已保留；服务器不能单人游玩。","Waiting for teammates or reconnection. Your lawn is held; server play needs two players."):
        s&&s->IsConnected()&&NeedsPlayers(s)?
            ModText::Tr("至少两名在线玩家才能开始；自己玩可点击返回单人游戏。","At least two online players are required. Choose Switch to solo to play alone."):
        explanation,Rect(56,130,648,42),17,Color(210,220,225));
    if(active) {
        SeatUi::Strip(g,Rect(40,188,680,54));
        SeatUi::Text(g,RoomPresentation::Identity(s),54,222,652,21,Color(245,230,165));
        for(int i=0;i<NetProto::MAX_PLAYERS;++i) {
            const int seat=i+1;Rect box(40+(i%2)*352,254+(i/2)*62,328,56);
            SeatUi::Strip(g,box);bool occupied=s->IsSeatOccupied((uint8_t)seat);
            SeatUi::Text(g,RoomPresentation::SeatName(s,seat),box.mX+10,box.mY+23,308,20,
                occupied?Color(245,230,165):Color(170,170,165));
            SeatUi::Text(g,occupied?RoomPresentation::SeatProgress(app,seat):ModText::Tr("可加入","Available"),
                box.mX+10,box.mY+47,occupied?176:308,18,s->IsSeatOffline((uint8_t)seat)?Color(255,175,120):Color(190,215,225));
            if(occupied)SeatUi::Text(g,RoomPresentation::SeatVersion(s,seat),box.mX+196,box.mY+47,122,14,Color(145,160,165));
        }
        if(!s->IsRelay()||app->mBoard)SeatUi::Text(g,RoomPresentation::Order(s),40,450,420,18,Color(215,215,195));
    }
}
}
