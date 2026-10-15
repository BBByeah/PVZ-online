#include "MobileCoopDraw.h"
#include "OnlineDialog.h"
#include "OnlineSeatDialog.h"
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
    Resize(0, 0, 750, 550);
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
    mAddress = edit(30, 128, 282, 574, app->mOnlineAddress, 253);
    mServer = edit(31, 128, 346, 340, app->mOnlineServer, 253);
    mCode = edit(32, 570, 346, 132, app->mOnlineRoomCode, NetProto::ROOM_CODE_LEN);
    auto button = [&](int id, const char* text, int x, int y, int width) {
        auto b = std::make_unique<MobileStoneButton>(id, this);
        b->mLabel = text; b->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 24));
        b->SetLabelColor(Sexy::Color(245, 230, 165)); b->SetLabelHiliteColor(Sexy::Color::White); b->SetBkgColor(Sexy::Color(37, 83, 55));
        b->Resize(x, y, width, 46); AddWidget(b.get()); mButtons.push_back(std::move(b));
    };
    button(1, ModText::Tr("直连建房", "Host direct"), 40, 475, 150);
    button(2, ModText::Tr("直连加入", "Join direct"), 200, 475, 150);
    button(3, ModText::Tr("创建房间", "Create room"), 40, 405, 320);
    button(4, ModText::Tr("加入房间", "Join room"), 390, 405, 320);
    button(5, ModText::Tr("断开连接", "Disconnect"), 40, 475, 150);
    button(8, ModText::Tr("席位", "Seats"), 200, 475, 150);
    button(6, ModText::Tr("关闭面板", "Close panel"), 360, 475, 150);
    button(7, ModText::Tr("房主开始", "Host starts"), 520, 475, 190);
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
    for (auto& b : mButtons)
    {
        if (b->mId <= 4) b->SetDisabled(active);
        if (b->mId == 1 || b->mId == 2) b->SetVisible(!active);
        if (b->mId == 5 || b->mId == 8) b->SetVisible(active);
        if (b->mId == 7) b->SetVisible(session.IsConnected());
        if (b->mId == 5) b->SetDisabled(!active);
        if (b->mId == 7) b->SetDisabled(!session.IsConnected() || !session.IsHostSeat() || mApp->mBoard != nullptr);
        if (b->mId == 8) b->SetDisabled(!active);
    }
    MarkDirty();
}
void OnlineDialog::Draw(Sexy::Graphics* g)
{
    DrawMobilePanel(g, Sexy::Rect(0,0,mWidth,mHeight));
    auto* titleFont=ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24,30);
    g->SetColor(Sexy::Color(245,230,165)); g->SetFont(titleFont);
    const std::string title=ModText::Tr("联机合作","Online co-op");
    g->DrawString(title,(mWidth-titleFont->StringWidth(title))/2,70);
    auto* font=ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,20);
    g->SetFont(font);
    const auto& s=*mApp->mOnlineSession;
    const std::string status=!mInputError.empty()?mInputError:RoomPresentation::State(&s)+"  |  "+s.GetStatusText();
    PvzpDrawStringWrapped(g,status,Sexy::Rect(40,90,670,48),font,Sexy::Color(245,230,165),DS_ALIGN_LEFT);
    g->DrawString(MobileTextLine(font,s.IsRelay()?RoomPresentation::Identity(&s):
        ModText::Format("本机地址：{}","Local IP: {}",mLocalIp),670),40,158);
    if(s.GetOccupiedSeatCount()>0) {
        for(int seat=1;seat<=NetProto::MAX_PLAYERS;++seat) {
            if(!s.IsSeatOccupied(seat))continue;
            const int x=seat<=3?40:390,y=190+((seat-1)%3)*28;
            std::string label=RoomPresentation::SeatName(&s,seat);
            g->DrawString(MobileTextLine(font,label,320),x,y);
        }
    } else {
        PvzpDrawStringWrapped(g,ModText::Tr("远程联机：填写服务器和房间码。\n同一局域网：填写主机 IP，使用下方直连按钮。",
            "Remote play: enter server and room code.\nLAN: enter the host IP and use the direct buttons below."),
            Sexy::Rect(40,180,670,76),font,Sexy::Color(225,220,195),DS_ALIGN_LEFT);
    }
    DrawEditBox(g,mAddress.get()); DrawEditBox(g,mServer.get()); DrawEditBox(g,mCode.get());
    for(auto* edit:{mAddress.get(),mServer.get(),mCode.get()})if(edit->mHasFocus) {
        g->SetColor(Sexy::Color(245,230,165)); g->DrawRect(edit->mX-9,edit->mY-5,edit->mWidth+18,edit->mHeight+10);
    }
    g->SetColor(Sexy::Color(245,230,165));
    g->DrawString(ModText::Tr("主机 IP","Host IP"),40,310);
    g->DrawString(ModText::Tr("服务器","Server"),40,374);
    g->DrawString(ModText::Tr("房间码","Code"),480,374);
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,18));
    g->DrawString(ModText::Tr("席位页可看接力与倍率；关闭保留连接。","Seats shows relay order and tiers; closing keeps the connection."),40,466);
}
void OnlineDialog::ButtonDepress(int id)
{
    // Remember editable fields between openings, including after a failed connection.
    mApp->mOnlineAddress = mAddress->mString; mApp->mOnlineServer = mServer->mString; mApp->mOnlineRoomCode = mCode->mString;
    auto* app = mApp;
    if (id == 6) { app->KillDialog(DIALOG_ONLINE); return; }
    if (id == 7) { app->KillDialog(DIALOG_ONLINE); app->ShowRunMenu(); return; }
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
