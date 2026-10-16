#pragma once
#include "OnlineSeatDialog.h"

// @pvz-online: the existing ready gate stays modal; its presentation no longer hides the lawn.
class OnlineReadyDialog : public LawnDialog {
    LawnApp* app;
    bool down=false;
    static Rect Details(){return Rect(486,14,166,46);}
public:
    explicit OnlineReadyDialog(LawnApp* a):LawnDialog(a,DIALOG_ONLINE_START,true,"","","",Dialog::BUTTONS_NONE),app(a) {
        Resize(64,92,672,140);
    }
    void Update() override {LawnDialog::Update();MarkDirty();}
    void Draw(Graphics* g) override {
        SeatUi::Strip(g,Rect(0,0,mWidth,mHeight));
        auto* s=RoomPresentation::Session(app);
        SeatUi::Text(g,ModText::Tr("等待队友 · ","Waiting · ")+RoomPresentation::ReadySummary(s),18,35,450,24,Color(245,230,165));
        SeatUi::Paragraph(g,RoomPresentation::WaitingSeats(s),Rect(18,54,634,54),20,Color(220,225,210));
        SeatUi::Text(g,ModText::Tr("全员准备后自动开始，无需再次点击。","Starts automatically when everyone is ready."),18,124,634,18,Color(190,210,225));
        SeatUi::Button(g,Details(),ModText::Tr("查看席位","View seats"),true,down);
    }
    void MouseDown(int x,int y,int clicks) override {down=clicks>0&&Details().Contains(x,y);MarkDirty();}
    void MouseUp(int x,int y,int clicks) override {
        const bool open=down&&clicks>0&&Details().Contains(x,y);down=false;MarkDirty();
        if(open&&!app->GetDialog(DIALOG_ONLINE_SWAP))app->AddDialog(DIALOG_ONLINE_SWAP,new OnlineSeatDialog(app,true));
    }
    void KeyDown(KeyCode key) override {
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
        SeatUi::Button(g,Cancel(),ModText::Tr("退出本局","Leave game"),true,down);
    }
    void MouseDown(int x,int y,int clicks) override {down=clicks>0&&Cancel().Contains(x,y);MarkDirty();}
    void MouseUp(int x,int y,int clicks) override {
        bool cancel=down&&clicks>0&&Cancel().Contains(x,y);down=false;MarkDirty();
        if(cancel)app->OnlineReconnectCancelled();
    }
    void KeyDown(KeyCode key) override {if(key==KEYCODE_ESCAPE)app->OnlineReconnectCancelled();}
};
