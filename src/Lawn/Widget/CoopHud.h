#pragma once
#include "OnlineUiDraw.h"
#include "../Online/RoomPresentation.h"
#include "../Online/QuickChat.h"
#include "../CoopPlay.h"
#include "../Board.h"
#include "../SeedPacket.h"

// @pvz-online: one non-modal panel; blank lawn outside it keeps its normal input route.
namespace CoopHud {
using namespace Sexy;
inline bool Watching(Board* b) {
#ifdef __ANDROID__
    return b->mMobileWatchOpen;
#else
    return b->mWatchKeyHeld;
#endif
}
inline Rect Shortcut(int i){return Rect(660+i*46,40,44,44);}
inline Rect Panel(Board* b){return Rect(316,90,480,b->mSunGiftSelectFrames>0?302:b->mChatEmotePage?284:336);}
inline Rect Close(){return Rect(746,96,44,44);}
inline Rect Choice(int i,bool emote,bool sun) {
    if(sun)return Rect(328+(i%2)*232,166+(i/2)*50,224,44);
    if(emote)return Rect(328+(i%4)*116,138+(i/4)*90,108,84);
    return Rect(328+(i%3)*156,138+(i/3)*76,148,70);
}
inline int FooterY(Board* b){return b->mSunGiftSelectFrames>0?320:b->mChatEmotePage?320:372;}
inline Rect Request(Board* b){return Rect(328,FooterY(b),224,44);}
inline Rect Team(Board* b){return Rect(b->mSunGiftSelectFrames>0?560:328,FooterY(b),b->mSunGiftSelectFrames>0?108:224,44);}
inline Rect Watch(Board* b){return Rect(b->mSunGiftSelectFrames>0?676:560,FooterY(b),b->mSunGiftSelectFrames>0?108:224,44);}
inline void SmallButton(Graphics* g,Rect r,const std::string& label,bool enabled,bool down=false,int size=17) {
    SeatUi::Button(g,r,"",enabled,down);
    SeatUi::Text(g,label,r.mX+3,r.mY+(r.mHeight+size)/2-2,r.mWidth-6,size,enabled?Color(245,230,165):Color(175,175,165),true);
}
inline void Emote(Graphics* g,int x,int y,SeedType seed,float scale) {
    Graphics icon(*g);icon.mTransX+=x*icon.mScaleX;icon.mTransY+=y*icon.mScaleY;
    icon.mScaleX*=scale;icon.mScaleY*=scale;SeedPacketDrawSeed(&icon,0,0,seed,SEED_NONE,0,0,1);
}
inline bool GiftEnabled(Board* b,int seat) {
    auto* s=RoomPresentation::Session(b->mApp);auto* c=b->mApp->mCoopPlay.get();
    return s&&s->IsSeatOccupied((uint8_t)seat)&&seat!=s->GetLocalSeat()&&!s->IsSeatOffline((uint8_t)seat)
        &&s->GetSeatBuild((uint8_t)seat)>=55&&s->mCoopPeers[seat]&&b->mApp->IsRunMode()
        &&b->mSunMoney>=100&&!b->mSunGiftCooldownFrames&&c&&!c->sunCooldown&&!c->giftWaiting;
}
inline std::string SunStatus(Board* b) {
    auto* c=b->mApp->mCoopPlay.get();auto* s=RoomPresentation::Session(b->mApp);
    if(c&&c->giftWaiting)return ModText::Tr("等待P","Awaiting P")+std::to_string(s->FindCoopSeat(c->gift.target))+ModText::Tr("到账回执"," receipt");
    if(b->mSunMoney<100)return ModText::Tr("余额不足100；仍可向队友求援。","Below 100 sun; you can still request aid.");
    int ticks=std::max(b->mSunGiftCooldownFrames,c?c->sunCooldown:0);
    if(ticks>0)return ModText::Tr("赠送冷却 ","Gift cooldown ")+std::to_string((ticks+99)/100)+ModText::Tr("秒","s");
    if(c&&c->requestTicks>0)return ModText::Tr("求援已发出，等待队友响应。","Aid requested; waiting for a teammate.");
    return ModText::Tr("点击队友赠送；标有求援的优先响应。","Choose a teammate; Aid marks a pending request.");
}
inline int Hit(Board* b,int x,int y) {
    if(Watching(b)) {
        if(Rect(530,8,125,46).Contains(x,y))return 5;
        if(Rect(665,8,125,46).Contains(x,y))return 6;
        return 0;
    }
    for(int i=0;i<3;++i)if(Shortcut(i).Contains(x,y))return i+1;
    if(!b->mChatPanelOpen&&b->mSunGiftSelectFrames<=0)return -1;
    if(!Panel(b).Contains(x,y))return -1;
    if(Close().Contains(x,y))return 4;
    if(Team(b).Contains(x,y))return 7;
    if(Watch(b).Contains(x,y))return 5;
    bool sun=b->mSunGiftSelectFrames>0;auto* c=b->mApp->mCoopPlay.get();auto* s=RoomPresentation::Session(b->mApp);
    if(sun) {
        if(Request(b).Contains(x,y)&&c&&!c->requestCooldown&&!c->requestTicks&&s->HasOtherSeats())return 9;
        for(int i=0;i<NetProto::MAX_PLAYERS;++i)if(Choice(i,false,true).Contains(x,y)&&GiftEnabled(b,i+1))
            return (c->requests.count(s->mCoopPeers[i+1])?301:101)+i;
    } else {
        int count=b->mChatEmotePage?QuickChat::EMOTE_COUNT:QuickChat::PHRASE_COUNT;
        for(int i=0;i<count;++i)if(Choice(i,b->mChatEmotePage,false).Contains(x,y))
            return 201+i+(b->mChatEmotePage?QuickChat::PHRASE_COUNT:0);
    }
    return 0;
}
inline void StopWatch(Board* b) {
#ifdef __ANDROID__
    b->StopMobileWatch();
#else
    auto* s=RoomPresentation::Session(b->mApp);if(s)s->EndWatch(true);
    b->mCoopWatchLatched=false;b->mWatchKeyHeld=false;
#endif
}
inline void WatchNext(Board* b) {
#ifdef __ANDROID__
    b->CycleMobileWatch();
#else
    auto* s=RoomPresentation::Session(b->mApp);if(!s)return;
    int start=Watching(b)?b->mWatchLastSeat:s->GetLocalSeat();
    for(int n=1;n<=NetProto::MAX_PLAYERS;++n){uint8_t seat=(uint8_t)((start+n-1)%NetProto::MAX_PLAYERS+1);
        if(seat==s->GetLocalSeat()||!s->IsSeatOccupied(seat)||s->IsSeatOffline(seat))continue;
        if(s->IsWatching())s->EndWatch(true);
        if(s->BeginWatch(seat)){b->mWatchKeyHeld=true;b->mWatchLastSeat=seat;b->mCoopWatchLatched=true;return;}
    }
    StopWatch(b);
#endif
}
inline void Action(Board* b,int action) {
    auto* app=b->mApp;auto* s=RoomPresentation::Session(app);auto* c=app->mCoopPlay.get();
    if(action==6){StopWatch(b);return;}
    if(action==5){b->CloseQuickChatPanel();b->mSunGiftSelectFrames=0;WatchNext(b);return;}
    if(Watching(b))return;
    if(action==0)return; // A blank area inside the panel consumes the tap without changing pages.
    if(action==4){b->CloseQuickChatPanel();b->mSunGiftSelectFrames=0;return;}
    if(action==7){b->CloseQuickChatPanel();b->mSunGiftSelectFrames=0;app->ShowCoopPlay();return;}
    if(action==1||action==2){bool close=b->mChatPanelOpen&&b->mChatEmotePage==(action==2);
        b->mSunGiftSelectFrames=0;b->CloseQuickChatPanel();
        if(!close){b->mChatPanelOpen=true;b->mChatEmotePage=action==2;b->mChatPanelTimer=Board::QUICK_CHAT_PANEL_TIMEOUT;}
        return;
    }
    if(action==3){b->CloseQuickChatPanel();if(app->IsRunMode())b->mSunGiftSelectFrames=b->mSunGiftSelectFrames>0?0:500;return;}
    if(action==9&&c){if(c->RequestSun()){b->mSunGiftSelectFrames=0;b->PushCustomBanner(c->notice.c_str());}return;}
    if((action>=101&&action<=106)||(action>=301&&action<=306)) {
        bool aid=action>=301;int seat=action-(aid?300:100);if(!GiftEnabled(b,seat))return;
        bool sent=aid?c->Aid(s->mCoopPeers[seat]):b->TrySendSunGift((uint8_t)seat);
        if(aid)b->PushCustomBanner(c->notice.c_str());if(sent)b->mSunGiftSelectFrames=0;return;
    }
    if(action>=201&&action<=200+QuickChat::TOTAL_COUNT&&b->mChatPanelOpen) {
        uint8_t id=(uint8_t)(action-200);b->CloseQuickChatPanel();
        if(app->SendCoopQuickChat(id))b->PushQuickChatBanner(s->GetLocalSeat(),id);return;
    }
}
inline std::string Banner(Board* b,int index) {
    const auto& notice=b->mChatBanners[index];if(!notice.mId)return notice.mCustom;
    auto* s=RoomPresentation::Session(b->mApp);std::string name=s?s->GetSeatName(notice.mSeat):"";
    if(name.empty())name="P"+std::to_string(notice.mSeat);
    return name+": "+(QuickChat::IsEmoteId(notice.mId)?ModText::Tr(QuickChat::EMOTE_NAMES[notice.mId-QuickChat::PHRASE_COUNT-1],QuickChat::EMOTE_NAMES_EN[notice.mId-QuickChat::PHRASE_COUNT-1]):ModText::Tr(QuickChat::PHRASES[notice.mId-1],QuickChat::PHRASES_EN[notice.mId-1]));
}
inline void Draw(Board* b,Graphics* g) {
    if(!b->QuickChatAvailable()||Watching(b))return;
    bool interactive=!b->mPaused&&b->mApp->mActive&&!b->mApp->mMinimized;
    const char* labels[]={ModText::Tr("消息","Chat"),ModText::Tr("表情","Face"),ModText::Tr("阳光","Sun")};
    for(int i=0;i<3;++i)SmallButton(g,Shortcut(i),labels[i],interactive&&(i!=2||b->mApp->IsRunMode()),
        b->mCoopTouchCaptured&&b->mCoopTouchAction==i+1,16);
    auto* s=RoomPresentation::Session(b->mApp);auto* c=b->mApp->mCoopPlay.get();
    if(interactive&&(b->mChatPanelOpen||b->mSunGiftSelectFrames>0)) {
        bool sun=b->mSunGiftSelectFrames>0,emote=!sun&&b->mChatEmotePage;SeatUi::Strip(g,Panel(b));
        SeatUi::Text(g,sun?ModText::Tr("阳光：扣100 → 到账90","Sun: spend 100 -> receive 90"):emote?ModText::Tr("植物表情","Plant emotes"):ModText::Tr("快捷消息","Quick messages"),328,123,408,22,Color(245,230,165));
        SmallButton(g,Close(),"X",true,b->mCoopTouchCaptured&&b->mCoopTouchAction==4);
        if(sun) {
            SeatUi::Text(g,SunStatus(b),328,156,456,17,Color(210,225,230));
            for(int i=0;i<NetProto::MAX_PLAYERS;++i){int seat=i+1;auto box=Choice(i,false,true);bool occupied=s->IsSeatOccupied((uint8_t)seat);
                std::string label="P"+std::to_string(seat)+" "+(occupied?s->GetSeatName((uint8_t)seat):ModText::Tr("空席","Empty"));
                if(seat==s->GetLocalSeat())label+=ModText::Tr("(你)"," (you)");
                else if(s->IsSeatOffline((uint8_t)seat))label+=ModText::Tr("(离线)"," (offline)");
                else if(c&&c->requests.count(s->mCoopPeers[seat]))label=ModText::Tr("求援 ","Aid ")+label;
                SmallButton(g,box,label,GiftEnabled(b,seat),b->mCoopTouchCaptured&&(b->mCoopTouchAction==101+i||b->mCoopTouchAction==301+i),18);
            }
            SmallButton(g,Request(b),c&&c->requestTicks>0?ModText::Tr("已求援","Aid requested"):c&&c->requestCooldown>0?ModText::Tr("求援冷却中","Aid cooldown"):ModText::Tr("请求阳光","Request sun"),c&&!c->requestCooldown&&!c->requestTicks&&s->HasOtherSeats(),false,18);
        } else {
            int count=emote?QuickChat::EMOTE_COUNT:QuickChat::PHRASE_COUNT;
            for(int i=0;i<count;++i){Rect box=Choice(i,emote,false);int action=201+i+(emote?QuickChat::PHRASE_COUNT:0);
                SeatUi::Button(g,box,"",true,b->mCoopTouchCaptured&&b->mCoopTouchAction==action);
                if(emote){Emote(g,box.mX+35,box.mY+4,QuickChat::EMOTE_SEEDS[i],0.42f);
                    SeatUi::Text(g,std::to_string(i+1)+" "+ModText::Tr(QuickChat::EMOTE_NAMES[i],QuickChat::EMOTE_NAMES_EN[i]),box.mX+4,box.mY+75,box.mWidth-8,15,Color(245,230,165),true);
                } else SeatUi::Paragraph(g,std::to_string(i+1)+" "+ModText::Tr(QuickChat::PHRASES[i],QuickChat::PHRASES_EN[i]),Rect(box.mX+8,box.mY+3,box.mWidth-16,66),16,Color(245,230,165));
            }
        }
        SmallButton(g,Team(b),ModText::Tr("借用 / 详情","Team"),b->mApp->GetRunState()!=nullptr,false,18);
        SmallButton(g,Watch(b),ModText::Tr("观战","Watch"),s->HasOtherSeats(),false,18);
    }
    std::vector<std::string> notices;
    if(c&&c->giftWaiting)notices.push_back(SunStatus(b));
    for(int i=b->mChatBannerCount-1;i>=0&&(int)notices.size()<2;--i){auto text=Banner(b,i);if(notices.empty()||notices.front()!=text)notices.push_back(text);}
    if(!notices.empty()) {
        // While a panel is open its own content wins; notifications never cover its buttons.
        int width=(b->mChatPanelOpen||b->mSunGiftSelectFrames>0)?302:640;
        SeatUi::Strip(g,Rect(4,88,width,(int)notices.size()*23+6));
        for(int i=0;i<(int)notices.size();++i)SeatUi::Text(g,notices[i],10,108+i*23,width-12,17,Color(240,230,195));
    }
}
inline void WatchControls(Graphics* g) {
    SmallButton(g,Rect(530,8,125,46),ModText::Tr("换人","Next"),true);
    SmallButton(g,Rect(665,8,125,46),ModText::Tr("返回","Back"),true);
}
}
