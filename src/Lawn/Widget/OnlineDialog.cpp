#include "MobileCoopDraw.h"
#include "OnlineDialog.h"
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
    mServer = edit(31, 148, 228, 292, app->mOnlineServer, 253);
    mCode = edit(32, 580, 228, 128, app->mOnlineRoomCode, NetProto::ROOM_CODE_LEN);
    auto button = [&](int id, const char* text, int x, int y, int width) {
        auto b = std::make_unique<MobileStoneButton>(id, this);
        b->mLabel = text; b->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 24));
        b->SetLabelColor(Sexy::Color(245, 230, 165)); b->SetLabelHiliteColor(Sexy::Color::White); b->SetBkgColor(Sexy::Color(37, 83, 55));
        b->Resize(x, y, width, 46); AddWidget(b.get()); mButtons.push_back(std::move(b));
    };
    auto place=[&](int id,const char* label,Sexy::Rect r){button(id,label,r.mX,r.mY,r.mWidth);mButtons.back()->Resize(r);};
    using namespace OnlineRoomLayout;
    place(1,ModText::Tr("建房","Host"),DirectHost());
    place(2,ModText::Tr("加入","Join"),DirectJoin());
    place(3,ModText::Tr("创建房间","Create room"),Create());
    place(4,ModText::Tr("加入房间","Join room"),Join());
    place(5,ModText::Tr("断开连接","Disconnect"),Disconnect());
    place(8,ModText::Tr("席位","Seats"),Seats());
    place(6,ModText::Tr("返回","Back"),Close());
    place(7,ModText::Tr("房主开始","Start run"),Start());
}
OnlineDialog::~OnlineDialog()
{
    RemoveWidget(mAddress.get()); RemoveWidget(mServer.get()); RemoveWidget(mCode.get());
    for (auto& b : mButtons) RemoveWidget(b.get());
}
void OnlineDialog::Update()
{
    Dialog::Update(); const auto& session = *mApp->mOnlineSession;
    const bool active = session.IsActive() && session.GetState() != NetSession::State::DEAD;
    for (auto& b : mButtons) {
        if(b->mId<=4){b->SetVisible(!active);b->SetDisabled(active);}
        if(b->mId==5||b->mId==8||b->mId==7)b->SetVisible(active);
        if(b->mId==6)b->Resize(active?OnlineRoomLayout::Close():OnlineRoomLayout::Start());
        if(b->mId==7)b->SetDisabled(!(session.IsConnected()||session.GetState()==NetSession::State::LISTENING)
            ||!session.IsHostSeat()||mApp->mBoard||mApp->mOnlineWaitingStartAck);
    }
    for(auto* edit:{mAddress.get(),mServer.get(),mCode.get()}) {
        if(active&&edit->mHasFocus)mApp->mWidgetManager->SetFocus(this);
        edit->SetDisabled(active);edit->SetVisible(!active);
    }
    const auto& code=session.GetRoomCode();
    if(session.IsRelay()&&!code.empty()&&code!=mShownRoomCode){mCode->SetText(code);mShownRoomCode=code;}
    MarkDirty();
}
void OnlineDialog::Draw(Sexy::Graphics* g)
{
    auto* s=mApp->mOnlineSession.get();
    std::string detail=!mInputError.empty()?mInputError:s->GetStatusText();
    if(s->HasIncomingSwapRequest())detail=ModText::Tr("收到换位申请；打开席位查看并答复。","Swap request received; open Seats to reply.");
    else if(mApp->mOnlineWaitingStartAck)detail=RoomPresentation::WaitingSeats(s,true);
    else if(s->IsConnected()&&!s->IsHostSeat())detail=ModText::Tr("等待房主选择模式并开始；可先查看席位。","Waiting for the host to choose a mode; you can view Seats.");
    OnlineRoomLayout::Draw(g,mApp,detail,mLocalIp);
    if(!RoomPresentation::Active(s)) {
        for(auto* edit:{mAddress.get(),mServer.get(),mCode.get()}) {
            DrawEditBox(g,edit);
            if(edit->mHasFocus){g->SetColor(Sexy::Color(245,230,165));g->DrawRect(edit->mX-9,edit->mY-5,edit->mWidth+18,edit->mHeight+10);}
        }
    }
}

void OnlineDialog::ButtonDepress(int id)
{
    // Remember editable fields between openings, including after a failed connection.
    mApp->mOnlineAddress = mAddress->mString; mApp->mOnlineServer = mServer->mString; mApp->mOnlineRoomCode = mCode->mString;
    auto* app = mApp;
    if (id == 6) { app->KillDialog(DIALOG_ONLINE); return; }
    if (id == 7) {
        const auto& s=*app->mOnlineSession;
        if(!(s.IsConnected()||s.GetState()==NetSession::State::LISTENING)||!s.IsHostSeat()||app->mBoard||app->mOnlineWaitingStartAck)return;
        app->KillDialog(DIALOG_ONLINE);app->ShowRunMenu();return;
    }
    if (id == 5) { app->DisconnectOnline(); return; }
    if (id == 8)
    {
        // 2026-10-15：换位改开选人框（任选上座队友 + 换后预览）；请求/答复/超时在 NetSession。
        if (RoomPresentation::Active(app->mOnlineSession.get()))
            app->AddDialog(DIALOG_ONLINE_SWAP, new OnlineSeatDialog(app));
        return;
    }
    if (id < 1 || id > 4) return;
    if (app->mOnlineSession->IsActive() && app->mOnlineSession->GetState() != NetSession::State::DEAD) return;
    mInputError.clear(); auto& session = *app->mOnlineSession;
    const std::string host = mAddress->mString.empty() ? "127.0.0.1" : mAddress->mString;
    const std::string server = mServer->mString.empty() ? "127.0.0.1" : mServer->mString;
    if (id == 1) session.StartHost();
    if (id == 2) session.StartJoin(host.c_str());
    if (id == 3) session.StartRoomHost(server.c_str());
    if (id == 4) session.StartRoomJoin(server.c_str(), mCode->mString.c_str());
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
