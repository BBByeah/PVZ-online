#include "MobileCoopDraw.h"
#include "OnlineDialog.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../Online/NetSession.h"
#include "../ModText.h"
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
        e->mMaxChars = max; e->SetText(text); e->Resize(x, y, width, 36);
        AddWidget(e.get()); return e;
    };
    // Match PC: direct host and relay server are independent; session defaults
    // select their respective ports instead of sharing an editable direct port.
    mAddress = edit(30, 110, 290, 595, app->mOnlineAddress, 253);
    mServer = edit(31, 110, 340, 385, app->mOnlineServer, 253);
    mCode = edit(32, 580, 340, 125, app->mOnlineRoomCode, NetProto::ROOM_CODE_LEN);
    auto button = [&](int id, const char* text, int x, int y, int width) {
        auto b = std::make_unique<MobileStoneButton>(id, this);
        b->mLabel = text; b->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 24));
        b->SetLabelColor(Sexy::Color(245, 230, 165)); b->SetLabelHiliteColor(Sexy::Color::White); b->SetBkgColor(Sexy::Color(37, 83, 55));
        b->Resize(x, y, width, 46); AddWidget(b.get()); mButtons.push_back(std::move(b));
    };
    button(1, ModText::Tr("直连建房", "Host direct"), 35, 394, 155);
    button(2, ModText::Tr("直连加入", "Join direct"), 210, 394, 155);
    button(3, ModText::Tr("创建房间", "Create room"), 385, 394, 155);
    button(4, ModText::Tr("加入房间", "Join room"), 560, 394, 155);
    button(5, ModText::Tr("断开连接", "Disconnect"), 35, 475, 200);
    button(6, ModText::Tr("关闭面板", "Close panel"), 275, 475, 200);
    button(7, ModText::Tr("房主开始", "Host starts"), 515, 475, 200);
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
        if (b->mId == 5) b->SetDisabled(!active);
        if (b->mId == 7) b->SetDisabled(!session.IsConnected() || !session.IsHostSeat() || mApp->mBoard != nullptr);
    }
    MarkDirty();
}
void OnlineDialog::Draw(Sexy::Graphics* g)
{
    DrawMobilePanel(g, Sexy::Rect(0,0,mWidth,mHeight));
    g->SetColor(Sexy::Color(245, 230, 165));
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24, 32));
    g->DrawString(ModText::Tr("联机房间", "Online room"), 30, 42);
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
    const auto& s = *mApp->mOnlineSession;
    g->DrawString(!mInputError.empty() ? mInputError : s.GetState() == NetSession::State::OFF ? ModText::Tr("未连接。", "Not connected.") : s.GetStatusText(), 30, 73);
    g->DrawString(s.IsRelay() ? ModText::Format("房间码：{}", "Room code: {}", s.GetRoomCode()) :
        ModText::Format("本机地址：{}", "Local IP: {}", mLocalIp), 30, 101);
    for (int seat = 1; seat <= NetProto::MAX_PLAYERS; ++seat)
    {
        if (!s.IsSeatOccupied(seat)) continue;
        const int x = seat <= 3 ? 35 : 390, y = 140 + ((seat - 1) % 3) * 28;
        g->DrawString(ModText::Format("{}号：{}{}", "Seat {}: {}{}", seat, s.GetSeatName(seat),
            seat == s.GetHostSeat() ? ModText::Tr("（房主）", " (host)") : ""), x, y);
    }
    g->DrawString(ModText::Tr("主机 IP", "Host IP"), 30, 316);
    g->DrawString(ModText::Tr("服务器", "Server"), 30, 366);
    g->DrawString(ModText::Tr("房间码", "Code"), 510, 366);
    g->DrawString(ModText::Tr("关闭面板保留连接；进入闯关由房主发起。", "Closing the panel keeps the connection; the host starts the run."), 35, 461);
}
void OnlineDialog::ButtonDepress(int id)
{
    // Remember editable fields between openings, including after a failed connection.
    mApp->mOnlineAddress = mAddress->mString; mApp->mOnlineServer = mServer->mString; mApp->mOnlineRoomCode = mCode->mString;
    auto* app = mApp;
    if (id == 6) { app->KillDialog(DIALOG_ONLINE); return; }
    if (id == 7) { app->KillDialog(DIALOG_ONLINE); app->ShowRunMenu(); return; }
    if (id == 5) { app->DisconnectOnline(); return; }
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
