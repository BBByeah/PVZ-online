#include "MobileCoopDraw.h"
#include "OnlineDialog.h"
#include "RoomDirectoryDialog.h"
#include "RunHistoryDialog.h"
#include "OnlineSeatDialog.h"
#include "OnlineRoomLayout.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../Online/NetSession.h"
#include "../ModText.h"
#include "../LawnCommon.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include "widget/EditWidget.h"
#include "widget/ButtonWidget.h"
#include "widget/WidgetManager.h"
#include "graphics/Graphics.h"

OnlineDialog::OnlineDialog(LawnApp* app)
    : Dialog(nullptr, nullptr, DIALOG_ONLINE, true, "", "", "", BUTTONS_NONE), mApp(app)
{
    Resize(0, 0, 760, 560);
    mLocalIp = NetLink::GetLocalIPv4Text();
    auto edit = [&](int id, int x, int y, int width, const std::string& text, int max) {
        auto e = std::make_unique<Sexy::EditWidget>(id, this);
        e->SetFont(ModText::FontFor(Sexy::FONT_BRIANNETOD16, 22));
        e->SetColors({Sexy::Color(0,0,0,0), Sexy::Color(0,0,0,0), Sexy::Color(240,240,255), Sexy::Color::White, Sexy::Color::Black});
        e->mMaxChars = max; e->SetText(text); e->Resize(x, y, width, 36);
        AddWidget(e.get()); return e;
    };
    // Match PC: direct host and relay server are independent; session defaults
    // select their respective ports instead of sharing an editable direct port.
    mAddress = edit(30, 148, 382, 292, app->mOnlineAddress, 253);
    mCode = edit(32, 580, 228, 128, app->mOnlineRoomCode, 7);
    auto button = [&](int id, const char* text, int x, int y, int width) {
        auto b = std::make_unique<MobileStoneButton>(id, this);
        b->mLabel = text; b->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 24));
        b->SetLabelColor(Sexy::Color(245, 230, 165)); b->SetLabelHiliteColor(Sexy::Color::White); b->SetBkgColor(Sexy::Color(37, 83, 55));
        b->Resize(x, y, width, 46); AddWidget(b.get()); mButtons.push_back(std::move(b));
    };
    auto place=[&](int id,const char* label,Sexy::Rect r){button(id,label,r.mX,r.mY,r.mWidth);mButtons.back()->Resize(r);};
    using namespace OnlineRoomLayout;
    if(RoomPresentation::Active(app->mOnlineSession.get()))mPage=app->mOnlineSession->IsRelay()?SERVER:LOCAL;
    place(1,ModText::Tr("建房","Host"),DirectHost());
    place(2,ModText::Tr("加入","Join"),DirectJoin());
    place(3,ModText::Tr("创建房间","Create room"),Create());
    place(4,ModText::Tr("加入房间","Join room"),Join());
    place(5,ModText::Tr("断开连接","Disconnect"),Disconnect());
    place(8,ModText::Tr("队友","Seats"),Seats());
    place(6,ModText::Tr("返回","Back"),Close());
    place(7,ModText::Tr("房主开始","Start run"),Start());
    place(9,ModText::Tr("公开房间","Public rooms"),Sexy::Rect(40,480,260,46));
    place(10,ModText::Tr("最近记录","Recent runs"),Sexy::Rect(552,34,168,42));
    place(11,ModText::Tr("公开房间：开","Public: on"),Sexy::Rect(480,440,232,32));
    place(12,ModText::Tr("单人与局域网游戏","Solo / LAN game"),LocalPage());
    place(13,ModText::Tr("在线房间","Online rooms"),ServerPage());
    place(14,ModText::Tr("单人游戏","Play solo"),Solo());
    const auto codeBox=Code();
    mCode->Resize(codeBox.mX,codeBox.mY,codeBox.mWidth,codeBox.mHeight);
    Update();
}
OnlineDialog::~OnlineDialog()
{
    RemoveWidget(mAddress.get()); RemoveWidget(mCode.get());
    for (auto& b : mButtons) RemoveWidget(b.get());
}
void OnlineDialog::Update()
{
    Dialog::Update(); const auto& session = *mApp->mOnlineSession;
    mAllocator.Update();
    if(mAllocator.state==RelayNodes::Allocator::READY){
        const auto endpoint=mAllocator.chosen;mAllocator.Cancel();
        if(mPage==OnlineRoomLayout::SERVER&&!RoomPresentation::Active(&session))mApp->mOnlineSession->StartRoomHost(endpoint.host.c_str(),endpoint.port);
    }else if(mAllocator.state==RelayNodes::Allocator::UNAVAILABLE){
        mAllocator.Cancel();mInputError=ModText::Tr("暂时无法创建房间，请稍后重试。","Could not create a room. Please retry shortly.");
    }
    const bool active=RoomPresentation::Active(&session);
    const bool allocating=mAllocator.state==RelayNodes::Allocator::LOADING;
    const bool local=!active&&mPage==OnlineRoomLayout::LOCAL;
    const bool server=!active&&mPage==OnlineRoomLayout::SERVER;
    for (auto& b : mButtons) {
        if(b->mId==3)b->mLabel=allocating?ModText::Tr("正在连接…","Connecting..."):ModText::Tr("创建房间","Create room");
        if(b->mId==9){b->SetVisible(server);b->SetDisabled(!server||allocating);}
        if(b->mId==12||b->mId==13)b->SetVisible(!active);
        if(b->mId==14){
            b->SetVisible(local||(active&&session.IsRelay()&&!mApp->mBoard));
            b->Resize(active?Sexy::Rect(40,440,420,32):OnlineRoomLayout::Solo());
            b->mLabel=mConfirmSolo?ModText::Tr("确认退房并单人开始","Confirm: leave and play solo"):
                active?ModText::Tr("返回单人游戏","Switch to solo"):ModText::Tr("单人游戏","Play solo");
        }
        if(b->mId==11){b->SetVisible(active&&session.IsRelay()&&session.IsHostSeat()&&!session.mDirectoryStarted);
            b->mLabel=!session.mDirectoryConfirmed?ModText::Tr("正在更新…","Updating..."):
                session.mDirectoryPublic?ModText::Tr("公开房间：开","Public: on"):ModText::Tr("公开房间：关","Public: off");}
        if(b->mId>=1&&b->mId<=4){const bool visible=b->mId<=2?local:server;b->SetVisible(visible);b->SetDisabled(!visible||allocating);}
        if(b->mId==5||b->mId==8||b->mId==7)b->SetVisible(active);
        if(b->mId==5)b->mLabel=mApp->mBoard?ModText::Tr("保存并退房","Save and leave"):ModText::Tr("退出房间","Leave room");
        if(b->mId==6)b->Resize(active?OnlineRoomLayout::Close():OnlineRoomLayout::Start());
        if(b->mId==7){b->SetDisabled(!OnlineRoomLayout::CanStart(mApp));
            b->mLabel=OnlineRoomLayout::NeedsPlayers(&session)?ModText::Tr("等待队友加入","Wait for players"):ModText::Tr("房主开始","Start run");}
    }
    for(auto* edit:{mAddress.get(),mCode.get()}) {
        const bool visible=edit==mAddress.get()?local:server;
        if(!visible&&edit->mHasFocus)mApp->mWidgetManager->SetFocus(this);
        edit->SetDisabled(!visible||allocating);edit->SetVisible(visible);
    }
    const auto& code=session.GetRoomCode();
    if(session.IsRelay()&&!code.empty()&&code!=mShownRoomCode){mCode->SetText(RelayNodes::ShareCode({session.GetRelayHost(),session.GetRelayPort()},code));mShownRoomCode=code;}
    MarkDirty();
}
void OnlineDialog::Draw(Sexy::Graphics* g)
{
    auto* s=mApp->mOnlineSession.get();
    std::string detail=!mInputError.empty()?mInputError:s->GetStatusText();
    if(s->HasIncomingSwapRequest())detail=ModText::Tr("收到换位申请；打开席位查看并答复。","Swap request received; open Seats to reply.");
    else if(mApp->mOnlineWaitingStartAck)detail=RoomPresentation::WaitingSeats(s,true);
    else if(s->IsConnected()&&!s->IsHostSeat())detail=ModText::Tr("等待房主选择模式并开始；可先查看席位。","Waiting for the host to choose a mode; you can view Seats.");
    OnlineRoomLayout::Draw(g,mApp,detail,mLocalIp,mPage);
    if(!mInputError.empty()&&!RoomPresentation::Active(s))SeatUi::Text(g,mInputError,48,465,664,15,Sexy::Color(255,175,120));
    if(mAllocator.state==RelayNodes::Allocator::LOADING)SeatUi::Text(g,ModText::Tr("正在寻找可用房间…","Finding an available room..."),48,465,664,15,Sexy::Color(210,220,195));
    if(!RoomPresentation::Active(s)) {
        for(auto* edit:{mAddress.get(),mCode.get()}) {
            if(!edit->mVisible)continue;
            DrawEditBox(g,edit);
            if(edit->mHasFocus){g->SetColor(Sexy::Color(245,230,165));g->DrawRect(edit->mX-9,edit->mY-5,edit->mWidth+18,edit->mHeight+10);}
        }
    }
}

void OnlineDialog::ButtonDepress(int id)
{
    // Remember editable fields between openings, including after a failed connection.
    mApp->mOnlineAddress = mAddress->mString; mApp->mOnlineRoomCode = mCode->mString;
    auto* app = mApp;
    if(id==12||id==13) {
        if(RoomPresentation::Active(app->mOnlineSession.get()))return;
        mAllocator.Cancel();mInputError.clear();
        mPage=id==12?OnlineRoomLayout::LOCAL:OnlineRoomLayout::SERVER;
        mConfirmSolo=false;Update();app->mWidgetManager->SetFocus(this);return;
    }
    if(id==14) {
        if(app->mBoard||(!RoomPresentation::Active(app->mOnlineSession.get())&&mPage!=OnlineRoomLayout::LOCAL))return;
        if(RoomPresentation::Active(app->mOnlineSession.get())&&!mConfirmSolo){mConfirmSolo=true;Update();return;}
        app->mOnlineSession->Close();app->KillDialog(DIALOG_ONLINE);app->ShowRunMenu();return;
    }
    mConfirmSolo=false;
    if(id==9){if(!RoomPresentation::Active(app->mOnlineSession.get())&&mPage==OnlineRoomLayout::SERVER&&mAllocator.state!=RelayNodes::Allocator::LOADING)
        app->AddDialog(DIALOG_ROOM_DIRECTORY,new RoomDirectoryDialog(app));
        return;}
    if(id==10){app->AddDialog(DIALOG_RUN_HISTORY,new RunHistoryDialog(app));return;}
    if(id==11){auto& s=*app->mOnlineSession;if(s.IsRelay()&&s.IsHostSeat()&&!s.mDirectoryStarted)s.SetDirectoryPublic(!s.mDirectoryPublic);return;}
    if (id == 6) { app->KillDialog(DIALOG_ONLINE); return; }
    if (id == 7) {
        if(!OnlineRoomLayout::CanStart(app))return;
        app->KillDialog(DIALOG_ONLINE);app->ShowRunMenu();return;
    }
    if (id == 5) {
        if(app->mBoard){
            app->DoBackToMain(false);
            if(!app->mBoard){app->mOnlineSession->Close();app->KillDialog(DIALOG_ONLINE);}
            return;
        }
        app->DisconnectOnline();return;
    }
    if (id == 8)
    {
        // 2026-10-15：换位改开选人框（任选上座队友 + 换后预览）；请求/答复/超时在 NetSession。
        if (RoomPresentation::Active(app->mOnlineSession.get()))
            app->AddDialog(DIALOG_ONLINE_SWAP, new OnlineSeatDialog(app));
        return;
    }
    if (id < 1 || id > 4) return;
    if((id<=2&&mPage!=OnlineRoomLayout::LOCAL)||(id>=3&&mPage!=OnlineRoomLayout::SERVER))return;
    if (RoomPresentation::Active(app->mOnlineSession.get())||mAllocator.state==RelayNodes::Allocator::LOADING) return;
    mInputError.clear(); auto& session = *app->mOnlineSession;
    const std::string host = mAddress->mString.empty() ? "127.0.0.1" : mAddress->mString;
    if (id == 1) session.StartHost();
    if (id == 2) session.StartJoin(host.c_str());
    if (id == 3) {mShownRoomCode.clear();mAllocator.Start();Update();}
    if (id == 4) {
        RelayNodes::Endpoint target;std::string code;
        if(!RelayNodes::ResolveCode(mCode->mString,RelayNodes::Catalog().front().endpoint,target,code)){mInputError=ModText::Tr("请填写队友显示的完整房间码，例如 2-ABCD。","Enter your teammate's full room code, for example 2-ABCD.");return;}
        mShownRoomCode.clear();session.StartRoomJoin(target.host.c_str(),code.c_str(),target.port);
    }
    // Stop the IME when leaving the edit fields without closing the live room panel.
    app->mWidgetManager->SetFocus(this);
}
void OnlineDialog::EditWidgetText(int id, const std::string&)
{
    // PC Enter flow: host IP joins directly; server/code joins the relay room.
    ButtonDepress(id == 30 ? 2 : 4);
}
void OnlineDialog::KeyDown(Sexy::KeyCode key)
{
    if (key == Sexy::KEYCODE_ESCAPE) ButtonDepress(6);
}
