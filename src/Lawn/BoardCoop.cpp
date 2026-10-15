#include "CoopPlay.h"
// PC Build 46 cooperation rules, with native touch and UTF-8 drawing.
#include "../LawnApp.h"
#include "../PvzpLib/PvzpCommon.h"
#include "../PvzpLib/PvzpStringFile.h"
#include "Board.h"
#include "Coin.h"
#include "Online/NetSession.h"
#include "Online/QuickChat.h"
#include "SeedPacket.h"
#include "Widget/MobileCoopDraw.h"
static const int kSunGiftAmount = 100, kSunGiftTax = 10, kSunGiftSelectFrames = 500,
                 kSunGiftCooldownFrames = 300;

bool Board::QuickChatAvailable()
{
    // 观看队友场地时不开面板：观看层盖住全屏，面板看不见却还在吞键（T/E 会被这里先接走）
    return mApp->IsOnlineGame() && mApp->mOnlineSession != nullptr && mApp->mOnlineSession->IsConnected() &&
           mApp->mGameScene == GameScenes::SCENE_PLAYING;
}

void Board::CloseQuickChatPanel()
{
    mChatPanelOpen = false;
    mChatPanelTimer = 0;
    // 关面板也吃一次冷却：Esc 关掉后同一下按键的自动重复不许立刻又把面板弹回去
    mChatInputCooldown = QUICK_CHAT_KEY_COOLDOWN;
}

bool Board::HandleQuickChatKey(KeyCode theKey)
{
    if (mChatPanelOpen)
    {
        // 面板开着：所有键一律吞掉（不落到 ESC 取消光标 / SPACE 暂停那条链），冷却中不产生动作
        if (mChatInputCooldown > 0)
            return true;
        mChatPanelTimer = QUICK_CHAT_PANEL_TIMEOUT; // 任意键都算"有人操作"，重置超时

        if (theKey == KeyCode::KEYCODE_ESCAPE)
        {
            CloseQuickChatPanel();
            return true;
        }
        if (theKey == 'T' || theKey == 't')
        {
            if (mChatEmotePage)
            {
                mChatEmotePage = false;
                mChatInputCooldown = QUICK_CHAT_KEY_COOLDOWN;
            }
            else
            {
                CloseQuickChatPanel(); // 已在短语页，再按 T = 关
            }
            return true;
        }
        if (theKey == 'E' || theKey == 'e')
        {
            if (!mChatEmotePage)
            {
                mChatEmotePage = true;
                mChatInputCooldown = QUICK_CHAT_KEY_COOLDOWN;
            }
            else
            {
                CloseQuickChatPanel(); // 已在表情页，再按 E = 关
            }
            return true;
        }
        if (theKey >= '1' && theKey <= '9')
        {
            // 按当前页选发：短语页 1-8 发 1..8，表情页 1-8 发 9..16
            if (theKey - '0' > (mChatEmotePage ? QuickChat::EMOTE_COUNT : QuickChat::PHRASE_COUNT)) return true;
            uint8_t anId = (uint8_t)((theKey - '1') + 1 + (mChatEmotePage ? QuickChat::PHRASE_COUNT : 0));
            CloseQuickChatPanel();
            if (mApp->SendCoopQuickChat(anId))
            {
                PushQuickChatBanner(mApp->mOnlineSession->GetLocalSeat(), anId); // 本地回显（我：…）
            }
            // 发送失败（掉线）就什么都不显示——掉线提示走既有路径
            return true;
        }
        return true;
    }

    // 面板关着：只认 T / E，其余原样放回（RETURN/SPACE/ESC 那条链不受影响）
    if (theKey != 'T' && theKey != 't' && theKey != 'E' && theKey != 'e')
        return false;
    if (mChatInputCooldown > 0)
        return true; // 冷却中吞掉，防自动重复把面板弹回去
    if (!QuickChatAvailable())
        return false; // 单机 / 过场 / 选卡：不响应（焦点本就不在棋盘）
    mChatPanelOpen = true;
    mChatEmotePage = (theKey == 'E' || theKey == 'e');
    mChatPanelTimer = QUICK_CHAT_PANEL_TIMEOUT;
    mChatInputCooldown = QUICK_CHAT_KEY_COOLDOWN;
    return true;
}

void Board::PushQuickChatBanner(uint8_t theSeat, uint8_t theId)
{
    // 横幅窗口 3 条，满了丢最旧保最新（与会话层收包队列同一条策略）
    if (mChatBannerCount >= QUICK_CHAT_BANNER_MAX)
    {
        for (int i = 1; i < mChatBannerCount; i++)
            mChatBanners[i - 1] = mChatBanners[i];
        mChatBannerCount--;
    }
    mChatBanners[mChatBannerCount].mSeat = theSeat;
    mChatBanners[mChatBannerCount].mId = theId;
    mChatBanners[mChatBannerCount].mFrames = QUICK_CHAT_BANNER_FRAMES;
    mChatBanners[mChatBannerCount].mCustom[0] = 0;
    mChatBannerCount++;
}

void Board::PushCustomBanner(const char *theTextUtf8)
{
    if (mChatBannerCount >= QUICK_CHAT_BANNER_MAX)
    {
        for (int i = 1; i < mChatBannerCount; i++)
            mChatBanners[i - 1] = mChatBanners[i];
        mChatBannerCount--;
    }
    mChatBanners[mChatBannerCount].mSeat = 0;
    mChatBanners[mChatBannerCount].mId = 0;
    mChatBanners[mChatBannerCount].mFrames = QUICK_CHAT_BANNER_FRAMES;
    snprintf(mChatBanners[mChatBannerCount].mCustom, sizeof(mChatBanners[mChatBannerCount].mCustom), "%s",
             theTextUtf8 ? theTextUtf8 : "");
    mChatBannerCount++;
}

void Board::UpdateQuickChat()
{
    // 发阳光选择态倒计时：超时自动退出（等待必须有出路）
    if (mSunGiftSelectFrames > 0)
        mSunGiftSelectFrames--;

    // 先收：队友的喊话不因我按了暂停就丢——横幅会冻着，继续了再轮播
    if (mApp->mOnlineSession != nullptr)
    {
        NetProto::MsgQuickChat aMsg;
        while (mApp->mOnlineSession->TakePendingQuickChat(aMsg))
        {
            PushQuickChatBanner(aMsg.mSrcSeat, aMsg.mId);
        }
    }

    if (!QuickChatAvailable())
    {
        // 掉线/收摊/不在局里：横幅与冷却一并清掉，别让上一局的话漂进新局面
        // （会话层收包队列由 DiscardLevelPackets/会话复位负责）
        if (mChatPanelOpen)
            CloseQuickChatPanel();
        mChatBannerCount = 0;
        mChatInputCooldown = 0;
        mSunGiftSelectFrames = 0;
        return;
    }

    if (mChatInputCooldown > 0)
        mChatInputCooldown--;
    if (mSunGiftCooldownFrames > 0)
        mSunGiftCooldownFrames--;
    if (mPaused || !mApp->mActive || mApp->mMinimized)
        mSunGiftSelectFrames = 0;

    if (mChatPanelOpen)
    {
        if (mPaused || !mApp->mActive || mApp->mMinimized)
        {
            CloseQuickChatPanel(); // 暂停菜单要弹出来了，面板压在底下没意义
        }
        else
        {
            mChatPanelTimer--;
            if (mChatPanelTimer <= 0)
                CloseQuickChatPanel(); // ≈10 秒无操作自动关（退路之一）
        }
    }

    // 横幅各自计时（2026-10-11 起三条同屏）：全体一起走，到期的原位摘除、保持顺序；
    // 暂停时冻着不推进
    if (!mPaused && mChatBannerCount > 0)
    {
        for (int i = 0; i < mChatBannerCount; i++)
            mChatBanners[i].mFrames--;
        int aWrite = 0;
        for (int i = 0; i < mChatBannerCount; i++)
        {
            if (mChatBanners[i].mFrames > 0)
            {
                if (aWrite != i)
                    mChatBanners[aWrite] = mChatBanners[i];
                aWrite++;
            }
        }
        mChatBannerCount = aWrite;
    }
}

bool Board::TrySendSunGift(uint8_t theTargetSeat)
{
    if (!mApp->IsOnlineGame() || !mApp->IsRunMode() || mApp->mOnlineSession == nullptr)
        return false;
    if (theTargetSeat == mApp->mOnlineSession->GetLocalSeat() ||
        !mApp->mOnlineSession->IsSeatOccupied(theTargetSeat))
        return false;
    if (mSunGiftCooldownFrames > 0)
    {
        PushCustomBanner(ModText::Tr("发阳光冷却中", "Sun gift is on cooldown"));
        return false;
    }
    if (mSunMoney < kSunGiftAmount)
    {
        PushCustomBanner(ModText::Tr("阳光不足", "Not enough sun"));
        return false;
    }

    mSunMoney -= kSunGiftAmount;
    mSunGiftCooldownFrames = kSunGiftCooldownFrames;
    int aCredit = kSunGiftAmount - kSunGiftTax;
    if (mApp->mOnlineSession->SendSunGift((uint16_t)aCredit, theTargetSeat))
    {
        char aBuf[48];
        snprintf(aBuf, sizeof(aBuf), "%s%d%s", ModText::Tr("已发阳光（-", "Sent sun (-"), kSunGiftAmount,
                 ModText::Tr("）", ")"));
        PushCustomBanner(aBuf);
        return true;
    }
    // 没发出去（掉线等）：钱退回、不进冷却
    mSunMoney += kSunGiftAmount;
    mSunGiftCooldownFrames = 0;
    return false;
}

bool Board::HandleSunGiftKey(KeyCode theKey)
{
    if (!mApp->IsOnlineGame() || !mApp->IsRunMode())
        return false;
    if (mApp->mOnlineSession == nullptr)
        return false;
    bool aSelecting = mSunGiftSelectFrames > 0;

    if (theKey == 'G' || theKey == 'g')
    {
        if (aSelecting)
        {
            mSunGiftSelectFrames = 0;
            PushCustomBanner(ModText::Tr("已取消发阳光", "Sun gift cancelled"));
            return true;
        }
        if (mApp->mOnlineSession->GetOccupiedSeatCount() <= 1)
        {
            PushCustomBanner(ModText::Tr("没有可发阳光的队友", "No teammate to send sun to"));
            return true;
        }
        if (mSunGiftCooldownFrames > 0)
        {
            PushCustomBanner(ModText::Tr("发阳光冷却中", "Sun gift is on cooldown"));
            return true;
        }
        if (mSunMoney < kSunGiftAmount)
        {
            PushCustomBanner(ModText::Tr("阳光不足", "Not enough sun"));
            return true;
        }
        mSunGiftSelectFrames = kSunGiftSelectFrames;
        PushCustomBanner(
            ModText::Tr("发阳光：按 1-6 选队友（100，到账 90）", "Sun gift: press 1-6 to pick a teammate"));
        return true;
    }

    if (!aSelecting)
        return false;
    if (theKey == KEYCODE_ESCAPE)
    {
        mSunGiftSelectFrames = 0;
        return true;
    }
    if (theKey >= '1' && theKey <= '6')
    {
        uint8_t aSeat = (uint8_t)(theKey - '0');
        if (aSeat == mApp->mOnlineSession->GetLocalSeat())
        {
            PushCustomBanner(ModText::Tr("不能发给自己", "Can't send to yourself"));
            return true;
        }
        if (!mApp->mOnlineSession->IsSeatOccupied(aSeat))
        {
            PushCustomBanner(ModText::Tr("该席位没有人", "That seat is empty"));
            return true;
        }
        TrySendSunGift(aSeat);
        mSunGiftSelectFrames = 0; // 成败都退出选择态（冷却/钱不足的横幅已给原因）
        return true;
    }
    return false; // 其他键不吞、选择态继续（超时自动退）
}

void Board::GiveSunGift(int theAmount, uint8_t theFromSeat)
{
    mSunMoney = std::min(mSunMoney + theAmount, 9990);
    if(theAmount>0 && mApp->mCoopPlay)mApp->mCoopPlay->SunReceived();
    // @pvz-online: 收方横幅带发送者名字（2026-10-13 用户令「显示 XXX 送你阳光」）；
    // 席位名口径同聊天/掉线横幅，空名兜底 P<席位>。
    std::string aNameUtf8;
    if (mApp->mOnlineSession != nullptr)
        aNameUtf8 = mApp->mOnlineSession->GetSeatName(theFromSeat);
    if (aNameUtf8.empty())
        aNameUtf8 = "P" + std::to_string((unsigned)theFromSeat);
    PushCustomBanner((aNameUtf8 + ModText::Tr("送你阳光", " sent you sun")).c_str());
    // 天降演出：三枚面额币从天上落到场上（50+25+15 的视觉示意，落地自动飞向计数器）。
    // mVisualOnly 让它们收集时不二次入账、也不算进「在途阳光」种植预算——金额以横幅为准。
    int aBaseX = RandRangeInt(100, 529);
    Coin *aCoin = AddCoin(aBaseX, 60, CoinType::COIN_LARGESUN, CoinMotion::COIN_MOTION_FROM_SKY);
    if (aCoin != nullptr)
        aCoin->mVisualOnly = true;
    aCoin = AddCoin(aBaseX + 45, 60, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_SKY);
    if (aCoin != nullptr)
        aCoin->mVisualOnly = true;
    aCoin = AddCoin(aBaseX + 90, 60, CoinType::COIN_SMALLSUN, CoinMotion::COIN_MOTION_FROM_SKY);
    if (aCoin != nullptr)
        aCoin->mVisualOnly = true;
}
namespace
{
Sexy::Rect CoopButtonRect(int i)
{
    return Sexy::Rect(4, 146 + i * 52, 72, 44);
}
Sexy::Rect CoopChoiceRect(int i, bool emote, bool sun)
{
    if (emote)
        return Sexy::Rect(154 + (i % 4) * 124, 220 + (i / 4) * 124, 118, 115);
    if (!sun) return Sexy::Rect(154, 145 + i * 44, 492, 40);
    return Sexy::Rect(154 + (i % 2) * 248, 230 + (i / 2) * 68, 240, 54);
}
void CoopEmote(Sexy::Graphics *g, int x, int y, SeedType seed, float scale)
{
    Sexy::Graphics icon(*g);
    icon.mTransX += x * icon.mScaleX;
    icon.mTransY += y * icon.mScaleY;
    icon.mScaleX *= scale;
    icon.mScaleY *= scale;
    SeedPacketDrawSeed(&icon, 0, 0, seed, SEED_NONE, 0, 0, 1);
}
} // namespace
int Board::CoopHit(int x, int y)
{
    if (mMobileWatchOpen) {
        if (Sexy::Rect(530,8,125,46).Contains(x,y)) return 5;
        if (Sexy::Rect(665,8,125,46).Contains(x,y)) return 6;
        return 0; // consume every lawn tap while viewing a remote snapshot
    }
    if (CoopButtonRect(4).Contains(x,y)) return 7;
    if (CoopButtonRect(3).Contains(x,y)) return 5;
    for (int i = 0; i < 3; ++i)
        if (CoopButtonRect(i).Contains(x, y))
            return 1 + i;
    if (!(mChatPanelOpen || mSunGiftSelectFrames > 0))
        return -1;
    if (Sexy::Rect(604, (mChatPanelOpen && !mChatEmotePage) ? 99 : 143, 42, 40).Contains(x, y))
        return 4;
    if (mSunGiftSelectFrames > 0)
    {
        for (int i = 0; i < 6; ++i)
            if (CoopChoiceRect(i, false, true).Contains(x, y))
                return 101 + i;
    }
    else
    {
        for (int i = 0; i < (mChatEmotePage ? QuickChat::EMOTE_COUNT : QuickChat::PHRASE_COUNT); ++i)
            if (CoopChoiceRect(i, mChatEmotePage, false).Contains(x, y))
                return 201 + i + (mChatEmotePage ? QuickChat::PHRASE_COUNT : 0);
    }
    return 0; // Dismissal is consumed and never lands on the lawn.
}
bool Board::CoopMouseDown(int x, int y, int clicks)
{
    if (!QuickChatAvailable() || mPaused || !mApp->mActive || mApp->mMinimized || clicks <= 0)
        return false;
    int hit = CoopHit(x, y);
    if (hit < 0)
        return false;
    mCoopTouchCaptured = true;
    mCoopTouchAction = hit;
    return true;
}
bool Board::CoopMouseUp(int x, int y, int clicks)
{
    if (!mCoopTouchCaptured)
        return false;
    mCoopTouchCaptured = false;
    int down = mCoopTouchAction;
    mCoopTouchAction = -1;
    if (clicks > 0 && QuickChatAvailable() && !mPaused && mApp->mActive && !mApp->mMinimized &&
        CoopHit(x, y) == down)
        CoopAction(down);
    return true;
}
void Board::CoopAction(int action)
{
    if (action == 7) { mApp->ShowCoopPlay(); return; }
    if (action == 5) { CycleMobileWatch(); return; }
    if (action == 6) { StopMobileWatch(); return; }
    if (mMobileWatchOpen) return;
    if (action == 0 || action == 4)
    {
        CloseQuickChatPanel();
        mSunGiftSelectFrames = 0;
        return;
    }
    if (action == 1 || action == 2)
    {
        mSunGiftSelectFrames = 0;
        HandleQuickChatKey(static_cast<KeyCode>(action == 1 ? 'T' : 'E'));
        return;
    }
    if (action == 3)
    {
        CloseQuickChatPanel();
        HandleSunGiftKey(static_cast<KeyCode>('G'));
        if (mSunGiftSelectFrames > 0 && mChatBannerCount > 0)
            snprintf(mChatBanners[mChatBannerCount-1].mCustom, sizeof(mChatBanners[0].mCustom), "%s", ModText::Tr("点击队友：扣 100，到账 90", "Tap a teammate: pay 100, receive 90"));
        return;
    }
    if (action >= 101 && action <= 106 && mSunGiftSelectFrames > 0)
    {
        HandleSunGiftKey(static_cast<KeyCode>('0' + action - 100));
        return;
    }
    if (action >= 201 && action <= 200 + QuickChat::TOTAL_COUNT && mChatPanelOpen && mChatInputCooldown == 0)
    {
        uint8_t id = static_cast<uint8_t>(action - 200);
        CloseQuickChatPanel();
        if (mApp->SendCoopQuickChat(id))
            PushQuickChatBanner(mApp->mOnlineSession->GetLocalSeat(), id);
    }
}
void Board::DrawCoopUI(Sexy::Graphics *g)
{
    if (!QuickChatAvailable() || mMobileWatchOpen)
        return;
    auto *session = mApp->mOnlineSession.get();
    bool interactive = !mPaused && mApp->mActive && !mApp->mMinimized;
    const char *labels[] = {ModText::Tr("消息", "Chat"), ModText::Tr("表情", "Emote"),
                            ModText::Tr("阳光", "Sun"), ModText::Tr("观战", "Watch"), ModText::Tr("协作", "Team")};
    for (int i = 0; i < 5; ++i)
        DrawMobileStone(g, CoopButtonRect(i), labels[i], mCoopTouchCaptured && mCoopTouchAction == i + 1,
                        interactive && (i != 2 || mApp->IsRunMode()), 22);
    if (interactive && (mChatPanelOpen || mSunGiftSelectFrames > 0))
    {
        bool sun = mSunGiftSelectFrames > 0;
        bool phrase = !sun && !mChatEmotePage;
        DrawMobilePanel(g, Sexy::Rect(140, phrase ? 86 : 130, 520, phrase ? 468 : 390));
        g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24, 28));
        g->SetColor(Sexy::Color(245, 230, 165));
        g->DrawString(sun              ? ModText::Tr("送阳光", "Send sun")
                      : mChatEmotePage ? ModText::Tr("植物表情", "Plant emotes")
                                       : ModText::Tr("快捷消息", "Quick messages"),
                      158, phrase ? 126 : 176);
        DrawMobileStone(g, Sexy::Rect(604, phrase ? 99 : 143, 42, 40), "X", false, true, 20);
        if (sun)
        {
            g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 21));
            g->DrawString(ModText::Tr("选择队友：扣 100，到账 90", "Pick a teammate: pay 100, receive 90"),
                          158, 208);
            for (int i = 0; i < 6; ++i)
            {
                uint8_t seat = static_cast<uint8_t>(i + 1);
                bool available = session->IsSeatOccupied(seat) && seat != session->GetLocalSeat();
                std::string name =
                    session->IsSeatOccupied(seat) ? session->GetSeatName(seat) : ModText::Tr("空位", "Empty");
                DrawMobileStone(g, CoopChoiceRect(i, false, true), std::to_string(seat) + ": " + name, false,
                                available, 21);
            }
        }
        else
            for (int i = 0; i < (mChatEmotePage ? QuickChat::EMOTE_COUNT : QuickChat::PHRASE_COUNT); ++i)
            {
                Sexy::Rect tile = CoopChoiceRect(i, mChatEmotePage, false);
                if (mChatEmotePage)
                {
                    DrawMobilePanel(g, tile);
                    CoopEmote(g, tile.mX + 25, tile.mY + 3, QuickChat::EMOTE_SEEDS[i], 0.68f);
                    auto *font = ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 18);
                    const char *name = ModText::Tr(QuickChat::EMOTE_NAMES[i], QuickChat::EMOTE_NAMES_EN[i]);
                    g->SetFont(font);
                    g->SetColor(Sexy::Color(245, 230, 165));
                    g->DrawString(name, tile.mX + (tile.mWidth - font->StringWidth(name)) / 2, tile.mY + 101);
                }
                else
                    DrawMobileStone(g, tile, ModText::Tr(QuickChat::PHRASES[i], QuickChat::PHRASES_EN[i]),
                                    false, true, 18);
            }
    }
    if (mChatBannerCount <= 0)
        return;
    const bool panelOpen = interactive && (mChatPanelOpen || mSunGiftSelectFrames > 0);
    int y = panelOpen ? 92 : 95;
    const int first = panelOpen ? mChatBannerCount - 1 : 0;
    for (int i = mChatBannerCount - 1; i >= first; --i)
    {
        const auto &b = mChatBanners[i];
        bool emote = QuickChat::IsEmoteId(b.mId);
        int height = panelOpen ? 36 : emote ? 54 : 38;
        DrawMobilePanel(g, Sexy::Rect(280, y, 505, height));
        std::string text;
        if (b.mId == 0)
            text = b.mCustom;
        else
        {
            text =
                b.mSeat == session->GetLocalSeat() ? ModText::Tr("我", "Me") : session->GetSeatName(b.mSeat);
            if (text.empty())
                text = "P" + std::to_string(b.mSeat);
            text += " [" + std::to_string(b.mSeat) + "] ";
            if (!emote)
                text += ModText::Tr(QuickChat::PHRASES[b.mId - 1], QuickChat::PHRASES_EN[b.mId - 1]);
        }
        PvzpDrawStringWrapped(g, text, Sexy::Rect(290, y + 5, emote ? 410 : 485, height - 7),
                              ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 19), Sexy::Color(245, 230, 165),
                              DrawStringJustification::DS_ALIGN_LEFT);
        if (emote)
            CoopEmote(g, 719, y + 1, QuickChat::EMOTE_SEEDS[b.mId - QuickChat::PHRASE_COUNT - 1], panelOpen ? 0.32f : 0.42f);
        y += height + 3;
    }
}
