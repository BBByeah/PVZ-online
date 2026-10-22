#include "../../LawnApp.h"
#include "RunState.h"
#include "RunHistory.h"
#include "../Online/NetSession.h"
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

void LawnApp::ShowRunMenu(int endlessScene)
{
    if (!IsOnlineStartAllowed() && !(mOnlineSession && mOnlineSession->GetRole() == NetSession::Role::HOST && mOnlineSession->GetState() == NetSession::State::LISTENING))
    { DoOnlineDialog(); return; }

    KillRunFlowScreen();
    KillBoard();
    KillGameSelector();
    mRunState.reset();
    mGameScene = SCENE_MENU;
    mRunFlowScreen = std::make_unique<RunFlowScreen>(this, true, endlessScene);
    mWidgetManager->AddWidget(mRunFlowScreen.get());
    mWidgetManager->SetFocus(mRunFlowScreen.get());
}

void LawnApp::StartRun(int mode, int buffMult, int difficulty, int scale, int tempo, int zombotany, int endlessScene, int bossFlag, int ladder, int zombieBuff,int speedTier, int firstWave, int endlessOptions)
{
    if (!IsOnlineStartAllowed() && !(mOnlineSession && mOnlineSession->GetRole() == NetSession::Role::HOST && mOnlineSession->GetState() == NetSession::State::LISTENING))
    { DoOnlineDialog(); return; }

    ResetZombieVote();
    KillRunFlowScreen();
    KillBoard();
    KillGameSelector();
    mRunState = std::make_unique<RunState>();
    mDamageStats.Reset();
    mRunState->StartNew(Sexy::Rand(), mode, difficulty, scale, tempo, zombotany, endlessScene, bossFlag, buffMult, ladder, zombieBuff,speedTier, firstWave, endlessOptions);
    mRunState->BeginStartPicks();
    mRunState->Save(mPlayerInfo->mId);
    EnterRunLevel();
}

void LawnApp::ResumeRun(int mode,int scene)
{
    ResetZombieVote();
    if (!IsOnlineStartAllowed() && !(mOnlineSession && mOnlineSession->GetRole() == NetSession::Role::HOST && mOnlineSession->GetState() == NetSession::State::LISTENING))
    { DoOnlineDialog(); return; }

    auto run = std::make_unique<RunState>();
    bool compatible = run->Load(mPlayerInfo->mId,mode,scene);
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
    if(mRunState->IsEndless() && !mRunState->IsNewEndless())
        LawnMessageBox(Dialogs::DIALOG_MESSAGE,ModText::Tr("旧版无尽存档","Legacy endless save"),
            ModText::Tr("这份旧档仅保存关数和增益，没有草坪阵型。继续时沿用旧规则并重新布阵；新开无尽才启用阶段主题和周期 BOSS。","This legacy save contains progress and buffs, without a lawn. Continuing keeps its old rules and starts a fresh lawn. New runs enable themes and boss cycles."),"OK","",Sexy::Dialog::BUTTONS_FOOTER);

    EnterRunLevel();
}

void LawnApp::ShowRunPicks()
{
    KillRunFlowScreen();
    if (!mRunState || !mRunState->HasPendingPick() || !mBoard || !mRunIntroHeld) return;
    mRunFlowScreen = std::make_unique<RunFlowScreen>(this, false);
    mRunState->Save(mPlayerInfo->mId);
    mWidgetManager->AddWidget(mRunFlowScreen.get());
    mWidgetManager->BringToFront(mRunFlowScreen.get());
    mWidgetManager->SetFocus(mRunFlowScreen.get());
}

void LawnApp::StartRunLevel() { EnterRunLevel(); }

void LawnApp::RetryRunLevel()
{
    if (!mRunState) return;
    if (IsOnlineGame()) { RetryOnlineLevel(); return; }
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
    mDamageStats.NoteFailure();
    // @pvz-online: endless levels share the same 25-slot failure counter ring as PC.
    PvzpLogLn("[run] failed index={} count={}", mRunState->mLevelIndex,
        mRunState->mFailCounts[mRunState->mLevelIndex % RunState::RUN_LEVEL_COUNT]);
    if (!mRunState->Save(mPlayerInfo->mId))
        PvzpLogLn("[run] could not save failure checkpoint");
    RunHistory::Capture(this,RunHistory::FAILED);
}

void LawnApp::RunPickPaidReroll()
{
    if (!mRunState || !mPlayerInfo || !mRunState->HasPendingPick() || !mRunState->mChoicesRolled) return;
    const RunState before = *mRunState;
    const int coins = mPlayerInfo->mCoins;
    const int cost = mRunState->GetPaidRerollCost();
    if (mRunState->mRerollVouchers > 0) --mRunState->mRerollVouchers;
    else { if (coins < cost) return; mPlayerInfo->AddCoins(-cost); }
    mRunState->PaidReroll();
    // @pvz-online: neither new choices nor a debit become visible before the joint commit.
    if (!mRunState->SaveWithPlayer(*mPlayerInfo)) {
        *mRunState = before;
        mPlayerInfo->mCoins = coins;
        LawnMessageBox(Dialogs::DIALOG_MESSAGE, ModText::Tr("保存失败", "Save failed"),
            ModText::Tr("重摇未保存，未扣款；请检查可用空间。", "Reroll was not saved or charged. Check available storage."),
            "OK", "", Sexy::Dialog::BUTTONS_FOOTER);
        return;
    }
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
	int aStacks = mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + aIndex);
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
	return mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + aIndex);
}

float LawnApp::RunPlantUpgradeMulKind(SeedType thePlant, RunPlantUpgradeKind theKind) const
{
	if (mRunState == nullptr) return 1.0f;
	int aIndex = RunPlantUpgradeIndexFor(thePlant);
	if (aIndex < 0) return 1.0f;
	const RunPlantUpgradeDef& aDef = GetRunPlantUpgradeDef(aIndex);
	if (aDef.mKind != theKind) return 1.0f;
	int aStacks = mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + aIndex);
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
	int aStacks = mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + aIndex);
	if (aDef.mMaxStacks > 0 && aStacks > aDef.mMaxStacks) aStacks = aDef.mMaxStacks;
	return aStacks;
}

float LawnApp::RunPlantBuff2Mul(SeedType thePlant) const
{
	if (mRunState == nullptr) return 1.0f;
	int aIndex = RunPlantBuff2IndexFor(thePlant);
	if (aIndex < 0) return 1.0f;
	const RunPlantUpgradeDef& aDef = GetRunPlantBuff2Def(aIndex);
	int aStacks = mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
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
	return mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
}

float LawnApp::RunPlantBuff2MulKind(SeedType thePlant, RunPlantUpgradeKind theKind) const
{
	if (mRunState == nullptr) return 1.0f;
	int aIndex = RunPlantBuff2IndexFor(thePlant);
	if (aIndex < 0) return 1.0f;
	const RunPlantUpgradeDef& aDef = GetRunPlantBuff2Def(aIndex);
	if (aDef.mKind != theKind) return 1.0f;
	int aStacks = mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
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
	int aStacks = mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + aIndex);
	if (aDef.mMaxStacks > 0 && aStacks > aDef.mMaxStacks) aStacks = aDef.mMaxStacks;
	return aStacks;
}

float LawnApp::RunCoffeeBeanRefreshMul(SeedType thePlant, SeedType theImitaterType) const
{
	if (mRunState == nullptr) return 1.0f;
	if (thePlant == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE) thePlant = theImitaterType;
	return thePlant == SeedType::SEED_INSTANT_COFFEE ? 1.6f : 1.0f;
}

int LawnApp::RunExtraBuffCount(RunPlantBuff2Id theBuff) const
{
	if (theBuff < RUN_BUFF2_DISPATCH || theBuff >= RUN_PLANT_BUFF2_COUNT) return 0;
	return mRunState ? mRunState->GetEffectiveBuffCount(RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + theBuff) : 0;
}

void LawnApp::AwardRunRerollVoucher()
{
    if (!mRunState || !mBoard || mRunState->mRerollAwardedLevel == mRunState->mLevelIndex) return;
    mRunState->mRerollAwardedLevel = mRunState->mLevelIndex;
    if (RunExtraBuffCount(RUN_BUFF2_REROLL_FUND) == 0) return;
    Plant* aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant))
        if (aPlant->mSeedType == SeedType::SEED_MARIGOLD && !aPlant->NotOnGround() && aPlant->mRunAliveTicks >= 6000)
        {
            mRunState->mRerollVouchers = std::min(2, mRunState->mRerollVouchers + 1);
            break;
        }
}

void LawnApp::RefreshRunChoicePlants(int aId, int aBefore)
{
    auto* aRun=mRunState.get();
    if(!aRun) return;
		if (mBoard && aRun->GetBuffCount(aId) > 0)
		{
			Plant* aPlant = nullptr;
			while (mBoard->IteratePlants(aPlant))
			{
				aPlant->RefreshRunMaxHealth();
				if (aBefore == 0 && aId == RUN_BUFF_COUNT + RUN_UPGRADE_POTATOMINE && aPlant->mSeedType == SeedType::SEED_POTATOMINE &&
					aPlant->mState == PlantState::STATE_NOTREADY)
					aPlant->mStateCountdown = std::min(aPlant->mStateCountdown, 375);
			}
		}

}
