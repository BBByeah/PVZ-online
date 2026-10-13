#include "RunFlowScreen.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../Run/RunState.h"
#include "../Run/RunBuffs.h"
#include "../Run/RunPortSupport.h"
#include "../ModText.h"
#include "../System/PlayerInfo.h"
#include "widget/ButtonWidget.h"
#include "graphics/Graphics.h"
#include "graphics/Font.h"
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
        addButton(10, ModText::Tr("完整：25 关", "Full: 25 levels"), 60, 180, 210, 70);
        addButton(11, ModText::Tr("普通：10 关", "Normal: 10 levels"), 295, 180, 210, 70);
        addButton(12, ModText::Tr("快速：5 关", "Quick: 5 levels"), 530, 180, 210, 70);
        addButton(13, ModText::Tr("继续闯关", "Continue run"), 240, 290, 320, 65)->SetDisabled(!RunState::HasCheckpoint(app->mPlayerInfo->mId));
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
        addButton(4, ModText::Tr("刷新选项", "Refresh choices"), 90, 430, 250, 60)->SetDisabled(run.mPickReRolled);
        addButton(5, ModText::Tr("跳过", "Skip"), 460, 430, 250, 60);
    }
    addButton(9, ModText::Tr("返回主菜单", "Back to menu"), 290, 525, 220, 50);
}

RunFlowScreen::~RunFlowScreen() { RemoveAllWidgets(true); }

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
        g->DrawString(ModText::Tr("通过五个场景，逐步收集植物和增益。", "Build your collection as you progress through five scenes."), 60, 135);
    }
    else
    {
        const RunState& run = *mApp->mRunState;
        g->DrawString(ModText::Format("第 {} / {} 关   植物：{}", "Level {} / {}   Plants: {}", run.mLevelIndex + 1, run.GetLevelCount(), run.mPool.size()), 70, 135);
        if (!run.IsPlantPick())
        {
            for (int i = 0; i < RunState::RUN_CHOICES; ++i)
            {
                const int id = run.mBuffChoices[i];
                const char* desc = id == RUN_BUFF_FIREPOWER ? ModText::Tr("弹体伤害", "Projectile damage") :
                    id == RUN_BUFF_ROOTED ? ModText::Tr("植物血量", "Plant health") : ModText::Tr("种子冷却", "Seed cooldown");
                const char* value = id == RUN_BUFF_FIREPOWER ? "+30%" : id == RUN_BUFF_ROOTED ? "+75%" : "x0.8";
                g->DrawString(desc, 55 + i * 240, 345);
                g->DrawString(value, 55 + i * 240, 373);
                g->DrawString(ModText::Format("已拥有：{}", "Owned: {}", run.GetBuffCount(id)), 55 + i * 240, 405);
            }
        }
    }
}

void RunFlowScreen::ButtonDepress(int id)
{
    if (id == 9) { mApp->ShowGameSelector(); return; }
    if (mModePage)
    {
        if (id == 13) mApp->ResumeRun();
        else if (id >= 10 && id <= 12) mApp->StartRun(id - 10);
        return;
    }
    RunState& run = *mApp->mRunState;
    if (id == 4) run.RerollChoices();
    else if (id == 5) run.SkipPendingPick();
    else if (id >= 0 && id < RunState::RUN_CHOICES)
    {
        if (run.IsPlantPick()) run.TakePlantChoice(id);
        else run.TakeBuffChoice(id);
    }
    mApp->ShowRunPicks();
}
