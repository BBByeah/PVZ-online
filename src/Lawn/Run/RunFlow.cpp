#include "../../LawnApp.h"
#include "RunState.h"
#include "RunBuffs.h"
#include "RunPortSupport.h"
#include "../ModText.h"
#include "../Widget/RunFlowScreen.h"
#include "../System/PlayerInfo.h"
#include "../Widget/LawnDialog.h"
#include "../Board.h"
#include "widget/WidgetManager.h"
#include "misc/MTRand.h"
#include <algorithm>
#include <cmath>

void LawnApp::KillRunFlowScreen()
{
    if (!mRunFlowScreen) return;
    mWidgetManager->RemoveWidget(mRunFlowScreen.get());
    SafeDeleteWidget(mRunFlowScreen.release());
}

void LawnApp::ShowRunMenu()
{
    KillRunFlowScreen();
    KillBoard();
    KillGameSelector();
    mRunState.reset();
    mGameScene = SCENE_MENU;
    mRunFlowScreen = std::make_unique<RunFlowScreen>(this, true);
    mWidgetManager->AddWidget(mRunFlowScreen.get());
    mWidgetManager->SetFocus(mRunFlowScreen.get());
}

void LawnApp::StartRun(int mode)
{
    KillRunFlowScreen();
    KillBoard();
    KillGameSelector();
    mRunState = std::make_unique<RunState>();
    mRunState->StartNew(Sexy::Rand(), mode, RunState::RUN_DIFF_STD);
    mRunState->BeginStartPicks();
    ShowRunPicks();
}

void LawnApp::ResumeRun()
{
    auto run = std::make_unique<RunState>();
    bool compatible = run->Load(mPlayerInfo->mId);
    // @pvz-online: a later port may have more effects; don't silently resume them without consumers.
    for (const auto& buff : run->mBuffs) if (!RunChoiceImplemented(buff.mId)) compatible = false;
    for (const auto seed : run->mPool) if (!RunPlantImplemented(seed)) compatible = false;
    if (!compatible)
    {
        LawnMessageBox(Dialogs::DIALOG_MESSAGE, ModText::Tr("无法继续", "Cannot continue"), ModText::Tr("闯关存档不存在或与此版本不兼容。", "This run save is missing or not compatible with this version."), "OK", "", Sexy::Dialog::BUTTONS_FOOTER);
        return;
    }
    mRunState = std::move(run);
    StartRunLevel();
}

void LawnApp::ShowRunPicks()
{
    KillRunFlowScreen();
    if (!mRunState) return;
    if (!mRunState->HasPendingPick())
    {
        mRunState->Save(mPlayerInfo->mId);
        StartRunLevel();
        return;
    }
    if (mBoard) mBoard->mPaused = true;
    mRunFlowScreen = std::make_unique<RunFlowScreen>(this, false);
    mWidgetManager->AddWidget(mRunFlowScreen.get());
    mWidgetManager->BringToFront(mRunFlowScreen.get());
    mWidgetManager->SetFocus(mRunFlowScreen.get());
}

void LawnApp::StartRunLevel()
{
    KillRunFlowScreen();
    KillGameSelector();
    mGameMode = GAMEMODE_ADVENTURE;
    mAppRandSeed = mRunState->GetLevelSeed();
    NewGame();
}

float LawnApp::RunBuffMul(int id) const
{
    if (!mRunState || !RunChoiceImplemented(id)) return 1.0f;
    const int count = mRunState->GetBuffCount(id);
    const auto& def = GetRunBuffDef(id);
    return std::max(0.05f, def.mMultiplicative ? (float)std::pow(1.0f + def.mPerStackMul, count) :
        1.0f + def.mPerStackMul * count);
}
