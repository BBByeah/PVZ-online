#pragma once
#include "OnlineSeatDialog.h"

// @pvz-online: the existing ready gate stays modal; its presentation no longer hides the lawn.
class OnlineReadyDialog : public LawnDialog {
    LawnApp* app;
    bool down=false;
    bool readyDown=false,inviteDown=false;
    static Rect Invite(){return Rect(486,138,166,40);}
    bool Endless()const{return app->GetRunState() && app->GetRunState()->IsNewEndless();}
    static Rect Ready(){return Rect(486,74,166,46);}
    static Rect Details(){return Rect(486,14,166,46);}
public:
    explicit OnlineReadyDialog(LawnApp* a):LawnDialog(a,DIALOG_ONLINE_START,true,"","","",Dialog::BUTTONS_NONE),app(a) {
        Resize(64,92,672,Endless()?192:140);
    }
    void Update() override {LawnDialog::Update();MarkDirty();}
    void Draw(Graphics* g) override {
        SeatUi::Strip(g,Rect(0,0,mWidth,mHeight));
        auto* s=RoomPresentation::Session(app);
        SeatUi::Text(g,(s&&s->IsPreparationActive()?ModText::Tr("关间准备 · ","Preparation · "):ModText::Tr("等待队友 · ","Waiting · "))+RoomPresentation::ReadySummary(s),18,35,450,24,Color(245,230,165));
        SeatUi::Paragraph(g,(s&&s->IsPreparationActive()&&s->GetPreparationPhase()!=PreparationWire::OPEN?RoomPresentation::PreparationStatus(s):RoomPresentation::WaitingSeats(s)),Rect(18,54,450,48),20,Color(220,225,210));
        SeatUi::Text(g,ModText::Tr("全员准备后自动开始，无需再次点击。","Starts automatically when everyone is ready."),18,124,634,18,Color(190,210,225));
        SeatUi::Button(g,Details(),ModText::Tr("席位与换位","Seats / swap"),true,down);
        if(Endless() && s) {
            auto* run=app->GetRunState();const bool host=s->GetRole()==NetSession::Role::HOST;
            std::string status=ModText::Tr("房间码 ","Room code ")+s->GetRoomCode()+" · ";
            status+=!s->SupportsAdmission()?ModText::Tr("服务器需更新","Server update needed"):
                s->mAdmissionNewMask?ModText::Tr("补位玩家准备中","New player preparing"):
                s->mGateWanted==1 && (!host || s->mGateAck==s->mGateRequest)?ModText::Tr("可凭码补位","Code invitations open"):
                ModText::Tr("补位暂未开放","Invitations closed");
            SeatUi::Text(g,status,18,166,450,18,Color(220,225,210));
            if(host)SeatUi::Button(g,Invite(),s->mAdmissionNewMask?ModText::Tr("取消补位","Cancel joining"):
                run->mAdmission.allow?ModText::Tr("关闭补位","Close invitations"):ModText::Tr("开放补位","Open invitations"),
                s->SupportsAdmission() && s->GetPreparationPhase()!=PreparationWire::SEAL && s->GetPreparationPhase()!=PreparationWire::GO,inviteDown);
        }
        const bool ready=s&&s->IsSeatSeedsReady(s->GetLocalSeat());
        SeatUi::Button(g,Ready(),ready?ModText::Tr("取消准备","Not ready"):ModText::Tr("准备","Ready"),s&&s->CanReadyPreparation(),readyDown);
    }
    void MouseDown(int x,int y,int clicks) override {down=clicks>0&&Details().Contains(x,y);readyDown=clicks>0&&Ready().Contains(x,y);inviteDown=clicks>0&&Endless()&&Invite().Contains(x,y);MarkDirty();}
    void MouseUp(int x,int y,int clicks) override {
        const bool open=down&&clicks>0&&Details().Contains(x,y);
        const bool toggle=readyDown&&clicks>0&&Ready().Contains(x,y);
        const bool invitation=inviteDown&&clicks>0&&Invite().Contains(x,y);
        down=readyDown=inviteDown=false;MarkDirty();
        if(invitation) {
            auto* s=RoomPresentation::Session(app);auto* run=app->GetRunState();
            if(s && run && s->SupportsAdmission() && s->GetRole()==NetSession::Role::HOST && s->GetPreparationPhase()!=PreparationWire::SEAL && s->GetPreparationPhase()!=PreparationWire::GO) {
                if(s->mAdmissionNewMask)s->CancelPendingAdmissions();
                else {run->mAdmission.allow=!run->mAdmission.allow;app->mAdmissionDirty=true;s->RequestRoomGate(false);}
            }
            return;
        }
        if(toggle){app->ToggleOnlineSeedsReady();return;}
        if(open&&!app->GetDialog(DIALOG_ONLINE_SWAP))app->AddDialog(DIALOG_ONLINE_SWAP,new OnlineSeatDialog(app,true));
    }
    void KeyDown(KeyCode key) override {
        if(key==KEYCODE_ESCAPE){app->ToggleOnlineSeedsReady();return;}
        if(key==KEYCODE_RETURN&&!app->GetDialog(DIALOG_ONLINE_SWAP))
            app->AddDialog(DIALOG_ONLINE_SWAP,new OnlineSeatDialog(app,true));
    }
};

// Same reconnect pause/cancel semantics, with a live compact status instead of a large notice.
class OnlineReconnectDialog : public LawnDialog {
    LawnApp* app;
    bool down=false;
    static Rect Cancel(){return Rect(486,14,166,46);}
public:
    explicit OnlineReconnectDialog(LawnApp* a):LawnDialog(a,DIALOG_ONLINE_RECONNECT,true,"","","",Dialog::BUTTONS_NONE),app(a) {
        Resize(64,92,672,140);
    }
    void Update() override {LawnDialog::Update();MarkDirty();}
    void Draw(Graphics* g) override {
        auto* s=RoomPresentation::Session(app);if(!s)return;
        SeatUi::Strip(g,Rect(0,0,mWidth,mHeight));
        SeatUi::Text(g,ModText::Tr("正在重连 · 房间 ","Reconnecting · Room ")+s->GetRoomCode(),18,35,450,24,Color(245,230,165));
        const auto progress=ModText::Tr("尝试 ","Attempt ")+std::to_string(s->GetReconnectAttempt())+
            ModText::Tr(" · 剩余 "," · ")+std::to_string(s->GetReconnectSecondsLeft())+ModText::Tr("秒","s left");
        SeatUi::Text(g,progress,18,80,634,22,Color(220,225,210));
        SeatUi::Text(g,ModText::Tr("席位保留中，连接恢复后继续。","Your seat is held while the link recovers."),18,124,634,18,Color(190,210,225));
        SeatUi::Button(g,Cancel(),ModText::Tr("返回主菜单","Leave game"),true,down);
    }
    void MouseDown(int x,int y,int clicks) override {down=clicks>0&&Cancel().Contains(x,y);MarkDirty();}
    void MouseUp(int x,int y,int clicks) override {
        bool cancel=down&&clicks>0&&Cancel().Contains(x,y);down=false;MarkDirty();
        if(cancel)app->OnlineReconnectCancelled();
    }
    void KeyDown(KeyCode key) override {if(key==KEYCODE_ESCAPE)app->OnlineReconnectCancelled();}
};
