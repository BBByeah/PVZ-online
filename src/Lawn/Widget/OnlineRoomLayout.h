#pragma once
#include "OnlineUiDraw.h"
#include "../Online/RoomPresentation.h"

namespace OnlineRoomLayout {
using namespace Sexy;
inline Rect Server(){return Rect(148,228,292,36);}
inline Rect Code(){return Rect(580,228,128,36);}
inline Rect Address(){return Rect(148,382,292,36);}
inline Rect Create(){return Rect(48,284,320,46);}
inline Rect Join(){return Rect(392,284,320,46);}
inline Rect DirectHost(){return Rect(464,376,116,46);}
inline Rect DirectJoin(){return Rect(596,376,116,46);}
inline Rect Seats(){return Rect(40,480,160,46);}
inline Rect Disconnect(){return Rect(216,480,160,46);}
inline Rect Close(){return Rect(392,480,152,46);}
inline Rect Start(){return Rect(560,480,160,46);}
inline void Draw(Graphics* g,LawnApp* app,const std::string& detail,const std::string& localIp) {
    SeatUi::Panel(g,Rect(0,0,760,560));
    auto* s=RoomPresentation::Session(app);const bool active=RoomPresentation::Active(s);
    SeatUi::Text(g,ModText::Tr("联机合作","Online co-op"),40,74,680,30,Color(245,230,165),true);
    SeatUi::Strip(g,Rect(40,92,680,84));
    SeatUi::Text(g,RoomPresentation::Summary(app),56,121,648,23,Color(245,230,165));
    SeatUi::Paragraph(g,detail,Rect(56,130,648,42),17,Color(210,220,225));
    if(active) {
        SeatUi::Strip(g,Rect(40,188,680,54));
        SeatUi::Text(g,RoomPresentation::Identity(s),54,222,652,21,Color(245,230,165));
        for(int i=0;i<NetProto::MAX_PLAYERS;++i) {
            const int seat=i+1;Rect box(40+(i%2)*352,254+(i/2)*62,328,56);
            SeatUi::Strip(g,box);bool occupied=s->IsSeatOccupied((uint8_t)seat);
            SeatUi::Text(g,RoomPresentation::SeatName(s,seat),box.mX+10,box.mY+23,308,20,
                occupied?Color(245,230,165):Color(170,170,165));
            SeatUi::Text(g,occupied?RoomPresentation::SeatProgress(app,seat):ModText::Tr("可加入","Available"),
                box.mX+10,box.mY+47,308,18,s->IsSeatOffline((uint8_t)seat)?Color(255,175,120):Color(190,215,225));
        }
        SeatUi::Text(g,RoomPresentation::Order(s),40,450,680,18,Color(215,215,195));
    } else {
        SeatUi::Text(g,ModText::Tr("云中继 · 远程联机 · 端口97","Cloud relay · remote play · port 97"),48,211,664,22,Color(245,230,165));
        SeatUi::Text(g,ModText::Tr("服务器","Server"),48,254,88,20,Color(220,225,210));
        SeatUi::Text(g,ModText::Tr("房间码","Code"),480,254,88,20,Color(220,225,210));
        SeatUi::Text(g,ModText::Tr("局域网直连 · 端口27777","Direct LAN · port 27777"),48,360,664,22,Color(245,230,165));
        SeatUi::Text(g,ModText::Tr("主机IP","Host IP"),48,409,88,20,Color(220,225,210));
        SeatUi::Text(g,ModText::Tr("本机地址：","Local IP: ")+localIp,48,448,664,18,Color(190,210,225));
    }
}
}
