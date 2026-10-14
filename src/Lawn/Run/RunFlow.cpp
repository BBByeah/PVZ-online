#include "../../LawnApp.h"
#include "RunState.h"
#include "RunBuffs.h"
#include "RunPortSupport.h"
#include "../ModText.h"
#include "../Widget/RunFlowScreen.h"
#include "../System/PlayerInfo.h"
#include "../System/Music.h"
#include "../../PvzpLib/PvzpDebug.h"
#include "../Widget/LawnDialog.h"
#include "../Board.h"
#include "../Cutscene.h"
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

void LawnApp::StartRun(int mode, int buffMult, int difficulty, int scale, int tempo, int zombotany, int endlessScene, int bossFlag)
{
    KillRunFlowScreen();
    KillBoard();
    KillGameSelector();
    mRunState = std::make_unique<RunState>();
    mRunState->StartNew(Sexy::Rand(), mode, difficulty, scale, tempo, zombotany, endlessScene, bossFlag, buffMult);
    mRunState->BeginStartPicks();
    ShowRunPicks();
}

void LawnApp::ResumeRun()
{
    auto run = std::make_unique<RunState>();
    bool compatible = run->Load(mPlayerInfo->mId);
    if (run->mMode < RunState::RUN_MODE_FULL || run->mMode > RunState::RUN_MODE_ENDLESS || run->IsComplete() ||
        run->mDiff < RunState::RUN_DIFF_EASY || run->mDiff > RunState::RUN_DIFF_HIGH ||
        run->mScale < RunState::RUN_SCALE_HALF || run->mScale > RunState::RUN_SCALE_QUAD ||
        run->mTempo < RunState::RUN_TEMPO_FAST || run->mTempo > RunState::RUN_TEMPO_SLOW) compatible = false;
    // Validate stored catalogue IDs before starting the board.
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
    if (mRunState->IsEndless() && mBoard && mBoardResult != BOARDRESULT_LOST &&
        (mBoard->mLevelComplete || mBoard->mLevelAwardSpawned))
    {
        mBoardResult = BOARDRESULT_NONE;
        mGameScene = SCENE_LEVEL_INTRO;
        mBoard->mPaused = false;
        mBoard->InitEndlessRunStage();
        ShowSeedChooserScreen();
        mBoard->mCutScene->StartLevelIntro();
        return;
    }
    NewGame();
}

void LawnApp::RetryRunLevel()
{
    if (!mRunState) return;
    KillDialog(DIALOG_GAME_OVER);
    KillDialog(DIALOG_CONFIRM_RESTART);
    KillNewOptionsDialog();
    mMusic->StopAllMusic();
    mSoundSystem->CancelPausedFoley();
    PvzpLogLn("[run] retry index={} seed={}", mRunState->mLevelIndex, mRunState->GetLevelSeed());
    StartRunLevel();
}

void LawnApp::NoteRunFailure()
{
    if (!mRunState) return;
    mRunState->NoteLevelFailed();
    PvzpLogLn("[run] failed index={} count={}", mRunState->mLevelIndex, mRunState->mFailCounts[mRunState->mLevelIndex]);
    if (!mRunState->Save(mPlayerInfo->mId))
        PvzpLogLn("[run] could not save failure checkpoint");
}

void LawnApp::RunPickPaidReroll()
{
    if (!mRunState || !mPlayerInfo || !mRunState->mPickReRolled) return;
    const int cost = mRunState->GetPaidRerollCost();
    if (mPlayerInfo->mCoins < cost) return;
    mPlayerInfo->AddCoins(-cost);
    mRunState->PaidReroll();
}



// Effect consumers ported from PC build 44 (61c37d6), CC0 source.
float LawnApp::RunBuffMul(int theBuffId) const
{
	if (mRunState == nullptr) return 1.0f;

	const RunBuffDef& aDef = GetRunBuffDef(theBuffId);
	int aStacks = mRunState->GetBuffCount(theBuffId);
	float aMul = 1.0f;
	if (aDef.mMultiplicative)
	{
		for (int i = 0; i < aStacks; i++) aMul *= 1.0f + aDef.mPerStackMul;
	}
	else
	{
		aMul = 1.0f + aDef.mPerStackMul * (float)aStacks;
	}
	return aMul < 0.1f ? 0.1f : aMul;
}

bool LawnApp::RunSkySunAtNight() const
{
	return mRunState != nullptr && mRunState->GetBuffCount(RUN_BUFF_SKYFALL) > 0;
}

float LawnApp::RunSkySunIntervalMul() const
{
	return RunSkySunAtNight() ? (1.0f / 2.0f) : 1.0f;
}

int LawnApp::RunMirrorStacks() const
{
	if (mRunState == nullptr) return 0;
	return mRunState->GetBuffCount(RUN_BUFF_MIRROR);
}

int LawnApp::RunBuffAdd(int theBuffId) const
{
	if (mRunState == nullptr) return 0;
	return GetRunBuffDef(theBuffId).mPerStackAdd * mRunState->GetBuffCount(theBuffId);
}

float LawnApp::RunPlantUpgradeMul(SeedType thePlant) const
{
	if (mRunState == nullptr) return 1.0f;

	int aIndex = RunPlantUpgradeIndexFor(thePlant);
	if (aIndex < 0) return 1.0f;

	const RunPlantUpgradeDef& aDef = GetRunPlantUpgradeDef(aIndex);
	int aStacks = mRunState->GetBuffCount(RUN_BUFF_COUNT + aIndex);
	float aMul = 1.0f;
	if (aDef.mMultiplicative)
	{
		for (int i = 0; i < aStacks; i++) aMul *= 1.0f + aDef.mPerStackMul;
	}
	else
	{
		aMul = 1.0f + aDef.mPerStackMul * (float)aStacks;
	}
	return aMul < 0.1f ? 0.1f : aMul;
}

int LawnApp::RunPlantUpgradeCount(SeedType thePlant) const
{
	if (mRunState == nullptr) return 0;

	int aIndex = RunPlantUpgradeIndexFor(thePlant);
	if (aIndex < 0) return 0;
	return mRunState->GetBuffCount(RUN_BUFF_COUNT + aIndex);
}

float LawnApp::RunPlantUpgradeMulKind(SeedType thePlant, RunPlantUpgradeKind theKind) const
{
	if (mRunState == nullptr) return 1.0f;
	int aIndex = RunPlantUpgradeIndexFor(thePlant);
	if (aIndex < 0) return 1.0f;
	const RunPlantUpgradeDef& aDef = GetRunPlantUpgradeDef(aIndex);
	if (aDef.mKind != theKind) return 1.0f;
	int aStacks = mRunState->GetBuffCount(RUN_BUFF_COUNT + aIndex);
	if (aDef.mMaxStacks > 0 && aStacks > aDef.mMaxStacks) aStacks = aDef.mMaxStacks;
	float aMul = 1.0f;
	if (aDef.mMultiplicative)
	{
		for (int i = 0; i < aStacks; i++) aMul *= 1.0f + aDef.mPerStackMul;
	}
	else
	{
		aMul = 1.0f + aDef.mPerStackMul * (float)aStacks;
	}
	return aMul < 0.1f ? 0.1f : aMul;
}

int LawnApp::RunPlantUpgradeCountKind(SeedType thePlant, RunPlantUpgradeKind theKind) const
{
	if (mRunState == nullptr) return 0;
	int aIndex = RunPlantUpgradeIndexFor(thePlant);
	if (aIndex < 0) return 0;
	const RunPlantUpgradeDef& aDef = GetRunPlantUpgradeDef(aIndex);
	if (aDef.mKind != theKind) return 0;
	int aStacks = mRunState->GetBuffCount(RUN_BUFF_COUNT + aIndex);
	if (aDef.mMaxStacks > 0 && aStacks > aDef.mMaxStacks) aStacks = aDef.mMaxStacks;
	return aStacks;
}

float LawnApp::RunPlantBuff2Mul(SeedType thePlant) const
{
	if (mRunState == nullptr) return 1.0f;
	int aIndex = RunPlantBuff2IndexFor(thePlant);
	if (aIndex < 0) return 1.0f;
	const RunPlantUpgradeDef& aDef = GetRunPlantBuff2Def(aIndex);
	int aStacks = mRunState->GetBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
	float aMul = 1.0f;
	if (aDef.mMultiplicative)
	{
		for (int i = 0; i < aStacks; i++) aMul *= 1.0f + aDef.mPerStackMul;
	}
	else
	{
		aMul = 1.0f + aDef.mPerStackMul * (float)aStacks;
	}
	return aMul < 0.1f ? 0.1f : aMul;
}

int LawnApp::RunPlantBuff2Count(SeedType thePlant) const
{
	if (mRunState == nullptr) return 0;
	int aIndex = RunPlantBuff2IndexFor(thePlant);
	if (aIndex < 0) return 0;
	return mRunState->GetBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
}

float LawnApp::RunPlantBuff2MulKind(SeedType thePlant, RunPlantUpgradeKind theKind) const
{
	if (mRunState == nullptr) return 1.0f;
	int aIndex = RunPlantBuff2IndexFor(thePlant);
	if (aIndex < 0) return 1.0f;
	const RunPlantUpgradeDef& aDef = GetRunPlantBuff2Def(aIndex);
	if (aDef.mKind != theKind) return 1.0f;
	int aStacks = mRunState->GetBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
	if (aDef.mMaxStacks > 0 && aStacks > aDef.mMaxStacks) aStacks = aDef.mMaxStacks;
	float aMul = 1.0f;
	if (aDef.mMultiplicative)
	{
		for (int i = 0; i < aStacks; i++) aMul *= 1.0f + aDef.mPerStackMul;
	}
	else
	{
		aMul = 1.0f + aDef.mPerStackMul * (float)aStacks;
	}
	return aMul < 0.1f ? 0.1f : aMul;
}

int LawnApp::RunPlantBuff2CountKind(SeedType thePlant, RunPlantUpgradeKind theKind) const
{
	if (mRunState == nullptr) return 0;
	int aIndex = RunPlantBuff2IndexFor(thePlant);
	if (aIndex < 0) return 0;
	const RunPlantUpgradeDef& aDef = GetRunPlantBuff2Def(aIndex);
	if (aDef.mKind != theKind) return 0;
	int aStacks = mRunState->GetBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
	if (aDef.mMaxStacks > 0 && aStacks > aDef.mMaxStacks) aStacks = aDef.mMaxStacks;
	return aStacks;
}

float LawnApp::RunCoffeeBeanRefreshMul(SeedType thePlant, SeedType theImitaterType) const
{
	if (mRunState == nullptr) return 1.0f;
	if (thePlant == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE) thePlant = theImitaterType;
	return thePlant == SeedType::SEED_INSTANT_COFFEE ? 1.6f : 1.0f;
}
