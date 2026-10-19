#pragma once
#include "LawnDialog.h"
#include "../Run/RunState.h"
#include "../Online/RoomPresentation.h"
#include "../../Resources.h"
#include "graphics/Graphics.h"
#include "graphics/Font.h"
#include "graphics/Image.h"
#ifdef __ANDROID__
#include "MobileCoopDraw.h"
#endif

#include "OnlineUiDraw.h"

class OnlineSeatDialog : public LawnDialog {
    LawnApp* app;int selected=0,down=-1;bool fromWaiting=false;
    static Rect Card(int i){return Rect(48+(i%2)*360,182+(i/2)*100,344,90);}
    static Rect Back(){return Rect(48,528,160,46);}
    static Rect Apply(){return Rect(590,528,160,46);}
    static Rect Accept(){return Rect(420,528,150,46);}
    int Hit(int x,int y) {
        if(Back().Contains(x,y))return 100;
        auto* s=RoomPresentation::Session(app);if(!s)return -1;
        if(s->HasIncomingSwapRequest()) {
            if(Accept().Contains(x,y)&&(!app->mBoard||s->IsPreparationActive())&&!app->mOnlineWaitingStartAck)return 102;
            if(Apply().Contains(x,y))return 103;
        } else if(Apply().Contains(x,y)&&RoomPresentation::CanSwap(app,selected))return 101;
        for(int i=0;i<NetProto::MAX_PLAYERS;++i)
            if(RoomPresentation::Active(s)&&s->IsSeatOccupied((uint8_t)(i+1))&&Card(i).Contains(x,y))return i+1;
        return -1;
    }
public:
    explicit OnlineSeatDialog(LawnApp* a,bool waiting=false):LawnDialog(a,DIALOG_ONLINE_SWAP,true,"","","",Dialog::BUTTONS_NONE),app(a),fromWaiting(waiting) {
        Resize(0,0,800,600);auto* s=RoomPresentation::Session(app);if(s)selected=s->GetLocalSeat();
    }
    void Update() override {
        LawnDialog::Update();
        // The detail page must not keep the board modal after the ready gate releases.
        auto* session=RoomPresentation::Session(app);
        if(fromWaiting&&(!app->mOnlineSeedsHeld||!app->mBoard||!session||session->IsReconnecting())) {
            app->KillDialog(DIALOG_ONLINE_SWAP);return;
        }
        MarkDirty();
    }
    void Draw(Graphics* g) override {
        SeatUi::Panel(g,Rect(0,0,800,600));
        auto* s=RoomPresentation::Session(app);bool active=RoomPresentation::Active(s);
        SeatUi::Text(g,ModText::Tr("席位与接力","Seats & relay"),48,78,704,28,Color(245,230,165),true);
        SeatUi::Text(g,RoomPresentation::ShowingPreparation(app)?RoomPresentation::PreparationStatus(s):RoomPresentation::State(s),48,107,704,20,s&&s->IsConnected()&&!s->IsBuildDifferent()?Color(180,235,165):Color(230,190,140),true);
        SeatUi::Text(g,RoomPresentation::Identity(s),48,132,704,20,Color(210,225,235),true);
        SeatUi::Text(g,RoomPresentation::Order(s),48,160,704,19,Color(245,230,165),true);
        for(int i=0;i<NetProto::MAX_PLAYERS;++i) {
            int seat=i+1;auto box=Card(i);bool occupied=active&&s->IsSeatOccupied((uint8_t)seat);
            bool local=occupied&&seat==s->GetLocalSeat(),offline=occupied&&s->IsSeatOffline((uint8_t)seat);
            Graphics card(*g);card.SetColorizeImages(true);card.SetColor(occupied?Color(235,235,235):Color(145,145,145));
            card.DrawImage(IMAGE_DIALOG_CENTERMIDDLE,box,Rect(0,0,IMAGE_DIALOG_CENTERMIDDLE->mWidth,IMAGE_DIALOG_CENTERMIDDLE->mHeight));
            card.SetColor(local?Color(240,215,110):seat==selected?Color(125,205,245):Color(115,115,135));card.DrawRect(box.mX,box.mY,box.mWidth-1,box.mHeight-1);
            std::string name=occupied?s->GetSeatName((uint8_t)seat):ModText::Tr("空席","Empty");
            SeatUi::Text(g,"P"+std::to_string(seat)+"  "+name,box.mX+12,box.mY+24,210,21,occupied?Color(245,230,165):Color(175,175,170));
            std::string tag=local?ModText::Tr("你","You"):"";
            if(occupied&&seat==s->GetHostSeat())tag+=tag.empty()?ModText::Tr("房主","Host"):ModText::Tr(" · 房主"," · Host");
            SeatUi::Text(g,tag,box.mX+230,box.mY+24,104,18,Color(170,225,255));
            if(!occupied){SeatUi::Text(g,ModText::Tr("加入房间后自动分配","Assigned when a player joins"),box.mX+12,box.mY+58,320,18,Color(170,170,165));continue;}
            bool recovering=s->GetState()==NetSession::State::RECONNECTING;
            std::string tier=recovering?ModText::Tr("重连后确认倍率","Tiers resume after reconnect"):
                ModText::Tr("刷怪 ×","Spawn x")+std::to_string(app->OnlineSeatMultiplierForSeat((uint8_t)seat))+
                ((app->mRunState&&app->mRunState->IsEndless())?ModText::Tr("   阳光保留","   Sun retained"):
                ModText::Tr("   额外阳光 +","   Sun bonus +")+std::to_string(app->OnlineStartSunBonusForSeat((uint8_t)seat)));
            SeatUi::Text(g,tier,box.mX+12,box.mY+51,320,19,Color(225,225,210));
            int next=s->NextOccupiedSeat((uint8_t)seat);
            std::string flow=offline?ModText::Tr("断线保留 · 等待重连","Seat held · awaiting reconnect"):
                s->GetOccupiedSeatCount()<2?ModText::Tr("单人席位 · 尚无接力队友","Solo seat · no relay teammate"):
                next?ModText::Tr("漏怪送往 P","Leaks go to P")+std::to_string(next):ModText::Tr("末席 · 每行1车，整局一次","Last · one mower per row per run");
            if(RoomPresentation::ShowingPreparation(app)||app->mOnlineWaitingStartAck)
                flow=RoomPresentation::SeatProgress(app,seat);
            SeatUi::Text(g,flow,box.mX+12,box.mY+77,320,18,offline?Color(255,175,120):s->IsSeatSeedsReady((uint8_t)seat)?Color(180,235,165):Color(190,210,225));
        }
        std::string preview=ModText::Tr("点击队友席位，查看换位预览。","Select a teammate to preview a swap.");
        if(active&&selected>0&&s->IsSeatOccupied((uint8_t)selected)) {
            preview=RoomPresentation::SeatName(s,selected);
            if(RoomPresentation::CanSwap(app,selected))preview+=ModText::Tr("  换入后：×","  After swap: x")+std::to_string(app->OnlineSeatMultiplierForSeat((uint8_t)selected))+
                ((app->mRunState&&app->mRunState->IsEndless())?ModText::Tr(" / 阳光保留"," / sun retained"):
                ModText::Tr(" / 额外阳光 +"," / sun bonus +")+std::to_string(app->OnlineStartSunBonusForSeat((uint8_t)selected)));
        }
        if(s&&!s->GetNoticeText().empty())preview=s->GetNoticeText();
        if(s&&s->HasIncomingSwapRequest())preview=RoomPresentation::SeatName(s,s->GetSwapRequesterSeat())+
            ModText::Tr(" 申请与你换位；同意后全队重新准备。"," asks to swap; everyone must ready again.");
        SeatUi::Text(g,preview,48,492,704,20,Color(245,230,165));
        SeatUi::Button(g,Back(),fromWaiting?ModText::Tr("返回等待","Back to waiting"):app->mBoard?ModText::Tr("返回游戏","Back to game"):ModText::Tr("返回房间","Back to room"),true,down==100);
        if(s&&s->HasIncomingSwapRequest()) {
            SeatUi::Button(g,Accept(),ModText::Tr("同意换位","Accept"),(!app->mBoard||s->IsPreparationActive())&&!app->mOnlineWaitingStartAck,down==102);
            SeatUi::Button(g,Apply(),ModText::Tr("拒绝","Decline"),true,down==103);
        } else {
            SeatUi::Button(g,Apply(),ModText::Tr("申请换位","Request swap"),RoomPresentation::CanSwap(app,selected),down==101);
            bool pending=s&&s->IsSwapRequestPending(),readOnly=(app->mBoard&&!s->CanChangePreparationSeats())||app->mOnlineWaitingStartAck;
            const char* hint=pending?ModText::Tr("等待队友答复","Waiting for approval"):
                readOnly?ModText::Tr("战斗或开局期间","During battle / starting"):ModText::Tr("换位需要对方同意","Swaps need approval");
            const char* detail=pending?ModText::Tr("15秒无回应自动取消","Expires after 15 seconds"):
                readOnly?ModText::Tr("只可查看，不能换位","View only; swaps disabled"):s&&s->IsPreparationActive()?ModText::Tr("换位后全队重新准备，卡牌保留","Swap clears ready; chosen cards stay"):ModText::Tr("房主身份跟随玩家","Host role follows the player");
            SeatUi::Text(g,hint,220,544,356,18,Color(205,205,190),true);
            SeatUi::Text(g,detail,220,568,356,18,Color(205,205,190),true);
        }
    }
    void MouseDown(int x,int y,int clicks) override {down=clicks>0?Hit(x,y):-1;MarkDirty();}
    void MouseUp(int x,int y,int clicks) override {
        int hit=Hit(x,y),pressed=down;down=-1;if(clicks<=0||pressed<0||hit!=pressed)return;
        if(hit==100){app->KillDialog(DIALOG_ONLINE_SWAP);return;}
        auto* s=RoomPresentation::Session(app);if(!s)return;
        if(hit==101){s->SwapSeats((uint8_t)selected);return;}
        if(hit==102||hit==103){s->AnswerSwapRequest(hit==102);return;}
        if(hit>=1&&hit<=NetProto::MAX_PLAYERS)selected=hit;
    }
    void KeyDown(KeyCode key) override {if(key==KEYCODE_ESCAPE)app->KillDialog(DIALOG_ONLINE_SWAP);}
};
