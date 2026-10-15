#include "MobileCoopDraw.h"
#include "OnlineDialog.h"
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

namespace
{
    // @pvz-online: 大厅换位选人框（2026-10-15，随 PC 协作批 UI 五项移植）：任选上座队友
    // 发换位请求，行内给出换后顺位预览（换位不改变上座席位集合，倍率/阳光按席位号查档）。
    // 对方同意才换；无回音由 NetSession 5 秒超时收摊。战斗中入口灰着（关间整备换位未做）。
    class OnlineSwapDialog : public Dialog
    {
      public:
        explicit OnlineSwapDialog(LawnApp *app) : Dialog(nullptr, nullptr, DIALOG_ONLINE_SWAP, true, "", "", "", BUTTONS_NONE), mApp(app), mDown(-1)
        {
            Resize(0, 0, 800, 600);
        }
        static Sexy::Rect SeatBox(int s) { return Sexy::Rect(40, 120 + (s - 1) * 56, 250, 44); }
        int Hit(int x, int y)
        {
            if (Sexy::Rect(620, 520, 130, 46).Contains(x, y)) return 90;
            auto *s = mApp->mOnlineSession.get();
            if (!s) return -1;
            for (int seat = 1; seat <= NetProto::MAX_PLAYERS; ++seat)
                if (seat != (int)s->GetLocalSeat() && s->IsSeatOccupied(seat) && SeatBox(seat).Contains(x, y)) return seat;
            return -1;
        }
        void Draw(Sexy::Graphics *g) override
        {
            DrawMobilePanel(g, Sexy::Rect(0, 0, mWidth, mHeight));
            auto *s = mApp->mOnlineSession.get();
            if (!s) { mApp->KillDialog(DIALOG_ONLINE_SWAP); return; }
            g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24, 30));
            g->SetColor(Sexy::Color(245, 230, 165));
            g->DrawString(ModText::Tr("换位 · 选择队友", "Swap - pick a teammate"), 40, 66);
            g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
            g->SetColor(Sexy::Color(205, 205, 205));
            g->DrawString(ModText::Tr("点队友申请换位；对方同意后生效，5秒无回应自动取消。",
                "Pick a teammate. Swaps need approval; requests expire in 5s."), 40, 96);

            uint8_t aMy = s->GetLocalSeat(), aLast = 0;
            for (uint8_t seat = 1; seat <= NetProto::MAX_PLAYERS; ++seat)
                if (s->IsSeatOccupied(seat)) aLast = seat;

            for (int seat = 1; seat <= NetProto::MAX_PLAYERS; ++seat)
            {
                std::string name = s->GetSeatName(seat);
                if (name.empty()) name = "P" + std::to_string(seat);
                auto box = SeatBox(seat);
                g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
                if (seat == (int)aMy)
                {
                    g->SetColor(Sexy::Color(150, 150, 150));
                    g->DrawString(ModText::Format("{}号：你自己", "Seat {}: you", seat), box.mX + 12, box.mY + 30);
                    continue;
                }
                if (!s->IsSeatOccupied(seat))
                {
                    g->SetColor(Sexy::Color(120, 120, 120));
                    g->DrawString(ModText::Format("{}号：空席", "Seat {}: empty", seat), box.mX + 12, box.mY + 30);
                    continue;
                }
                DrawMobileStone(g, box, ModText::Format("跟 {}", "With {}", name), false, true);
                int newMult = mApp->OnlineSeatMultiplierForSeat(seat), oldMult = mApp->OnlineSeatMultiplierForSeat(aMy),
                    newSun = mApp->OnlineStartSunBonusForSeat(seat);
                g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
                g->SetColor(Sexy::Color(255, 230, 150));
                g->DrawString(ModText::Format("P{}：刷怪 x{} → x{} · 阳光 +{}",
                    "P{}: spawn x{} -> x{} / sun +{}", seat, oldMult, newMult, newSun), 320, box.mY + 18);
                g->SetColor(Sexy::Color(180, 180, 180));
                if (aLast == seat)
                    g->DrawString(ModText::Tr("换后你是末席：漏怪将全队失败。",
                        "Last seat: a leak loses the run."), 320, box.mY + 40);
                else if (aLast == aMy)
                    g->DrawString(MobileTextLine(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,20),ModText::Format("换后末席是 {}。", "Last seat: {}.", name),420), 320, box.mY + 40);
                else
                    g->DrawString(ModText::Format("末席不变（P{}）。", "Last seat unchanged (P{}).", (unsigned)aLast), 320, box.mY + 40);
            }
            g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
            g->SetColor(Sexy::Color(160, 160, 160));
            PvzpDrawStringWrapped(g,ModText::Tr("末席每行一台推车（整局一次性）。\n换位只交换玩家，席位倍率不变。",
                "Last seat: one mower per row per run.\nSeat tiers stay fixed when players swap."),Sexy::Rect(40,510,550,60),
                ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18,20),Sexy::Color(205,205,190),DS_ALIGN_LEFT);
            DrawMobileStone(g, Sexy::Rect(620, 520, 130, 46), ModText::Tr("返回", "Back"), false, true);
        }
        void MouseDown(int x, int y, int clicks) override { mDown = clicks > 0 ? Hit(x, y) : -1; }
        void MouseUp(int x, int y, int clicks) override
        {
            int a = Hit(x, y);
            if (clicks <= 0 || mDown < 0 || a != mDown) { mDown = -1; return; }
            mDown = -1;
            if (a == 90) { mApp->KillDialog(DIALOG_ONLINE_SWAP); return; }
            auto *s = mApp->mOnlineSession.get();
            if (s && s->SwapSeats((uint8_t)a)) mApp->KillDialog(DIALOG_ONLINE_SWAP);
        }
        void KeyDown(Sexy::KeyCode key) override
        {
            if (key == Sexy::KEYCODE_ESCAPE) mApp->KillDialog(DIALOG_ONLINE_SWAP);
        }

      private:
        LawnApp *mApp;
        int mDown;
    };
}

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
    button(8, ModText::Tr("换位", "Swap"), 200, 475, 150);
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
        if (b->mId == 8) b->SetDisabled(!session.IsConnected() || mApp->mBoard != nullptr || session.GetOccupiedSeatCount() < 2);
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
    const std::string status=!mInputError.empty()?mInputError:s.GetState()==NetSession::State::OFF?
        ModText::Tr("未连接。填写服务器创建或加入房间。","Not connected. Enter a server to create or join a room."):s.GetStatusText();
    PvzpDrawStringWrapped(g,status,Sexy::Rect(40,90,670,48),font,Sexy::Color(245,230,165),DS_ALIGN_LEFT);
    g->DrawString(MobileTextLine(font,s.IsRelay()?ModText::Format("房间码：{}","Room code: {}",s.GetRoomCode()):
        ModText::Format("本机地址：{}","Local IP: {}",mLocalIp),670),40,158);
    if(s.GetOccupiedSeatCount()>0) {
        for(int seat=1;seat<=NetProto::MAX_PLAYERS;++seat) {
            if(!s.IsSeatOccupied(seat))continue;
            const int x=seat<=3?40:390,y=190+((seat-1)%3)*28;
            std::string label="P"+std::to_string(seat)+(seat==s.GetHostSeat()?ModText::Tr(" 房主："," Host: "):": ")+s.GetSeatName(seat);
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
    g->DrawString(ModText::Tr("关闭保留连接；闯关由房主发起。","Closing keeps the connection; the host starts the run."),40,466);
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
        if (app->mOnlineSession && app->mOnlineSession->IsConnected() && app->mBoard == nullptr)
            app->AddDialog(DIALOG_ONLINE_SWAP, new OnlineSwapDialog(app));
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
