#include "RunFlowScreen.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../Run/RunState.h"
#include "../Run/PortVersion.h"
#include "../Run/RunBuffs.h"
#include "../Run/RunPortSupport.h"
#include "../ModText.h"
#include "../System/PlayerInfo.h"
#include "widget/ButtonWidget.h"
#include "graphics/Graphics.h"
#include "graphics/Font.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include <format>

RunFlowScreen::RunFlowScreen(LawnApp* app, bool modePage) : mApp(app), mModePage(modePage)
{
    Resize(0, 0, 800, 600);
    auto addButton = [&](int id, const std::string& text, int x, int y, int width, int height) {
        auto* button = new Sexy::ButtonWidget(id, this);
        button->mLabel = text;
        button->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 28));
        button->SetLabelColor(Sexy::Color(245, 230, 165));
        button->SetLabelHiliteColor(Sexy::Color::White);
        button->SetBkgColor(Sexy::Color(37, 83, 55));
        button->Resize(x, y, width, height);
        AddWidget(button);
        return button;
    };
    if (mModePage)
    {
        addButton(10, ModText::Tr("完整：25 关", "Full: 25 levels"), 30, 170, 170, 58);
        addButton(11, ModText::Tr("普通：10 关", "Normal: 10 levels"), 220, 170, 170, 58);
        addButton(12, ModText::Tr("快速：5 关", "Quick: 5 levels"), 410, 170, 170, 58);
        addButton(15, ModText::Tr("无尽", "Endless"), 600, 170, 170, 58);
        for (int i = 0; i < 6; ++i)
            mOptionButtons[i] = addButton(20 + i, "", 35 + (i % 3) * 250, 255 + (i / 3) * 68, 230, 52);
        RefreshOptionLabels();
        const char* ladders[] = { "1.33", "1", "1.2", "1.5", "2" };
        mLadderButton = addButton(26, ModText::Format("场景阶梯：×{}", "Scene ladder: x{}", ladders[mLadder]), 35, 391, 230, 52);
        mBuffMultButton = addButton(14, ModText::Format("增益倍选：×{}", "Buff picks: x{}", RunState::BuffMultValueFor(mBuffMult)), 285, 391, 230, 52);
        addButton(13, ModText::Tr("继续闯关", "Continue run"), 535, 391, 230, 52)->SetDisabled(!RunState::HasCheckpoint(app->mPlayerInfo->mId));
    }
    else
    {
        RunState& run = *app->mRunState;
        if (!run.ChoicesRolled()) run.RollChoices();
        for (int i = 0; i < RunState::RUN_CHOICES; ++i)
        {
            const bool plant = run.IsPlantPick();
            const bool valid = plant ? run.mPlantChoices[i] != SEED_NONE : run.mBuffChoices[i] != RunState::RUN_BUFF_CHOICE_NONE;
            const char* label = !valid ? ModText::Tr("不可用", "Unavailable") : plant ? GetRunPlantName(run.mPlantChoices[i]) : GetRunChoiceDisplayName(run.mBuffChoices[i]);
            addButton(i, label ? label : "Plant", 48 + i * 240, 205, 224, 95)->SetDisabled(!valid);
        }
        const int cost = run.GetPaidRerollCost();
        addButton(4, ModText::Format("重摇 -{}", "Reroll -{}", cost), 90, 450, 250, 60)
            ->SetDisabled(app->mPlayerInfo->mCoins < cost);
        addButton(5, ModText::Tr("跳过", "Skip"), 460, 450, 250, 60);
    }
    addButton(9, ModText::Tr("返回主菜单", "Back to menu"), 290, 525, 220, 50);
}

RunFlowScreen::~RunFlowScreen() { RemoveAllWidgets(true); }

void RunFlowScreen::RefreshOptionLabels()
{
    const char* difficulties[] = { ModText::Tr("轻松", "Easy"), ModText::Tr("标准", "Standard"), ModText::Tr("高压", "High") };
    const char* scales[] = { ModText::Tr("少量", "Half"), ModText::Tr("标准", "Standard"), ModText::Tr("多量", "Double"), ModText::Tr("海量", "Quadruple") };
    const char* tempos[] = { ModText::Tr("快", "Fast"), ModText::Tr("标准", "Standard"), ModText::Tr("慢", "Slow") };
    const char* scenes[] = { ModText::Tr("白天", "Day"), ModText::Tr("夜晚", "Night"), ModText::Tr("泳池", "Pool"), ModText::Tr("迷雾", "Fog"), ModText::Tr("屋顶", "Roof") };
    mOptionButtons[0]->mLabel = ModText::Format("难度：{}", "Difficulty: {}", difficulties[mDifficulty]);
    mOptionButtons[1]->mLabel = ModText::Format("规模：{}", "Scale: {}", scales[mScale]);
    mOptionButtons[2]->mLabel = ModText::Format("节奏：{}", "Tempo: {}", tempos[mTempo]);
    mOptionButtons[3]->mLabel = ModText::Format("植物僵尸：{}", "Zombotany: {}", mZombotany ? ModText::Tr("开", "On") : ModText::Tr("关", "Off"));
    mOptionButtons[4]->mLabel = ModText::Format("关底首领：{}", "Final boss: {}", mBossFlag ? ModText::Tr("开", "On") : ModText::Tr("关", "Off"));
    mOptionButtons[5]->mLabel = ModText::Format("无尽场景：{}", "Endless scene: {}", scenes[mEndlessScene]);
    MarkDirty();
}

void RunFlowScreen::Draw(Sexy::Graphics* g)
{
    g->SetColor(Sexy::Color(23, 57, 38));
    g->FillRect(0, 0, 800, 600);
    g->SetColor(Sexy::Color(245, 230, 165));
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT24, 38));
    const std::string title = mModePage ? ModText::Tr("闯关模式", "Roguelike Run") : mApp->mRunState->IsPlantPick() ? ModText::Tr("选择植物", "Choose a plant") : ModText::Tr("选择增益", "Choose a bonus");
    g->DrawString(title, 70, 85);
    g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 28));
    if (mModePage)
    {
        g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
        g->DrawString(ModText::Format("PC 规则 Build {}", "PC rules Build {}", PVZ_ANDROID_PC_MOD_BUILD), 550, 80);
        g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 28));
        g->DrawString(ModText::Tr("通过五个场景，逐步收集植物和增益。", "Build your collection as you progress through five scenes."), 60, 135);
        g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20));
        g->DrawString(ModText::Tr("无尽锁定场景；场景阶梯仅影响有限关卡。", "Endless locks its scene; scene ladder applies to finite runs."), 70, 475);
        g->DrawString(ModText::Tr("僵尸强化投票将在联机阶段开放。", "Zombie buff voting will open with multiplayer."), 70, 501);
    }
    else
    {
        const RunState& run = *mApp->mRunState;
        g->DrawString(run.IsEndless() ? ModText::Format("无尽第 {} 关   植物：{}", "Endless level {}   Plants: {}", run.mLevelIndex + 1, run.mPool.size()) :
            ModText::Format("第 {} / {} 关   植物：{}", "Level {} / {}   Plants: {}", run.mLevelIndex + 1, run.GetLevelCount(), run.mPool.size()), 70, 135);
        g->SetFont(ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 22));
        g->DrawString(ModText::Format("金币 {}   增益倍选 ×{}", "Coins {}   Buff picks x{}", mApp->mPlayerInfo->mCoins, RunState::BuffMultValueFor(run.mBuffMult)), 70, 170);
        if (!run.IsPlantPick())
        {
            for (int i = 0; i < RunState::RUN_CHOICES; ++i)
            {
                const int id = run.mBuffChoices[i];
                if (id == RunState::RUN_BUFF_CHOICE_NONE) continue;
                const int x = 55 + i * 240;
                const SeedType plant = GetRunChoicePlant(id);
                auto* font = ModText::FontFor(Sexy::FONT_DWARVENTODCRAFT18, 20);
                if (plant != SEED_NONE) { g->SetFont(font); g->DrawString(GetRunPlantName(plant), x, 325); }
                PvzpDrawStringWrapped(g, GetRunChoiceDesc(id), Sexy::Rect(x, 337, 218, 85), font,
                    Sexy::Color(245, 230, 165), DrawStringJustification::DS_ALIGN_LEFT);
                g->SetFont(font);
                const int cap = GetRunChoiceMaxStacks(id);
                g->DrawString(cap > 0 ? ModText::Format("已拥有：{} / {}", "Owned: {} / {}", run.GetBuffCount(id), cap) :
                    ModText::Format("已拥有：{}", "Owned: {}", run.GetBuffCount(id)), x, 436);
            }
        }
    }
}

void RunFlowScreen::ButtonDepress(int id)
{
    if (id == 9) { mApp->ShowGameSelector(); return; }
    if (mModePage)
    {
        if (id >= 20 && id <= 25)
        {
            if (id == 20) mDifficulty = (mDifficulty + 1) % 3;
            if (id == 21) mScale = (mScale + 1) % 4;
            if (id == 22) mTempo = (mTempo + 1) % 3;
            if (id == 23) mZombotany ^= 1;
            if (id == 24) mBossFlag ^= 1;
            if (id == 25) mEndlessScene = (mEndlessScene + 1) % 5;
            RefreshOptionLabels();
            return;
        }
        if (id == 26)
        {
            mLadder = (mLadder + 1) % 5;
            const char* ladders[] = { "1.33", "1", "1.2", "1.5", "2" };
            mLadderButton->mLabel = ModText::Format("场景阶梯：×{}", "Scene ladder: x{}", ladders[mLadder]);
            MarkDirty();
            return;
        }
        if (id == 14)
        {
            mBuffMult = (mBuffMult + 1) % 3;
            mBuffMultButton->mLabel = ModText::Format("增益倍选：×{}", "Buff picks: x{}", RunState::BuffMultValueFor(mBuffMult));
            MarkDirty();
            return;
        }
        if (id == 13) mApp->ResumeRun();
        else if ((id >= 10 && id <= 12) || id == 15)
            mApp->StartRun(id == 15 ? RunState::RUN_MODE_ENDLESS : id - 10, mBuffMult,
                mDifficulty, mScale, mTempo, mZombotany, mEndlessScene, mBossFlag, mLadder);
        return;
    }
    RunState& run = *mApp->mRunState;
    if (id == 4) mApp->RunPickPaidReroll();
    else if (id == 5) run.SkipPendingPick();
    else if (id >= 0 && id < RunState::RUN_CHOICES)
    {
        if (run.IsPlantPick()) run.TakePlantChoice(id);
        else run.TakeBuffChoice(id);
    }
    mApp->ShowRunPicks();
}
