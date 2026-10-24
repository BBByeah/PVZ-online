#include "../Board.h"
#include "../Zombie.h"
#include "GameButton.h"
#include "../../LawnApp.h"
#include "ContinueDialog.h"
#include "../../Resources.h"
#include "../../Sexy.TodLib/TodFoley.h"
#include "../../Sexy.TodLib/TodStringFile.h"
#include "../ModText.h"
#include "graphics/Font.h"
#include <algorithm>

//0x4330D0
// GOTY @Patoke: 0x435E40
// @Patoke: these dialogs don't have localizations
ContinueDialog::ContinueDialog(LawnApp* theApp) : LawnDialog(
	theApp, 
	Dialogs::DIALOG_CONTINUE, 
	true, 
	ModText::Tr("继续游戏？", "CONTINUE GAME?"), 
	_S(""), 
	_S("[DIALOG_BUTTON_CANCEL]"), 
	Dialog::BUTTONS_FOOTER)
{
    if (theApp->IsAdventureMode())
    {
        mDialogLines = ModText::Tr("可以继续；重新开始会放弃本关进度。", "Continue, or restart and discard this level's progress.");
        mContinueButton = MakeButton(ContinueDialog::ContinueDialog_Continue, this, _S("[CONTINUE_BUTTON]"));
        mNewGameButton = MakeButton(ContinueDialog::ContinueDialog_NewGame, this, _S("[RESTART_BUTTON]"));
    }
    else
    {
        mDialogLines = ModText::Tr("可以继续；新游戏会替换当前进度。", "Continue, or start a new game and replace this progress.");
        mContinueButton = MakeButton(ContinueDialog::ContinueDialog_Continue, this, _S("[CONTINUE_BUTTON]"));
        mNewGameButton = MakeButton(ContinueDialog::ContinueDialog_NewGame, this, _S("[NEW_GAME_BUTTON]"));
    }

    mTallBottom = true;
    CalcSize(10, 60);
}

//0x4333D0,0x4333F0
ContinueDialog::~ContinueDialog()
{
    delete mContinueButton;
    delete mNewGameButton;
}

//0x433470
int ContinueDialog::GetPreferredHeight(int theWidth)
{
    return LawnDialog::GetPreferredHeight(theWidth) + 40;
}

//0x433480
void ContinueDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    int aBtnWidth = IMAGE_BUTTON_LEFT->mWidth + IMAGE_BUTTON_MIDDLE->mWidth * 3 + IMAGE_BUTTON_RIGHT->mWidth;
    const int aLabelWidth = std::max(FONT_DWARVENTODCRAFT18GREENINSET->StringWidth(mContinueButton->mLabel),
        FONT_DWARVENTODCRAFT18GREENINSET->StringWidth(mNewGameButton->mLabel)) + 24;
    while (aBtnWidth < aLabelWidth) aBtnWidth += IMAGE_BUTTON_MIDDLE->mWidth;
    const int aButtonGap = 12;
    const int aRowWidth = aBtnWidth * 2 + aButtonGap;
    const int aMinWidth = aRowWidth + mContentInsets.mLeft + mContentInsets.mRight +
        mBackgroundInsets.mLeft + mBackgroundInsets.mRight;
    if (theWidth < aMinWidth)
    {
        const int aFrameWidth = IMAGE_DIALOG_TOPLEFT->mWidth + IMAGE_DIALOG_TOPRIGHT->mWidth;
        const int aTileWidth = IMAGE_DIALOG_TOPMIDDLE->mWidth;
        theWidth = aFrameWidth + (aMinWidth - aFrameWidth + aTileWidth - 1) / aTileWidth * aTileWidth;
    }
    LawnDialog::Resize(theX, theY, theWidth, theHeight);
    const int aBtnHeight = mLawnYesButton->mHeight;
    const int aRowX = (mWidth - aRowWidth) / 2;
    const int aRowY = mLawnYesButton->mY - aBtnHeight - 8;
    mContinueButton->Resize(aRowX, aRowY, aBtnWidth, aBtnHeight);
    mNewGameButton->Resize(aRowX + aBtnWidth + aButtonGap, aRowY, aBtnWidth, aBtnHeight);
}

//0x433520
void ContinueDialog::AddedToManager(WidgetManager* theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);
    AddWidget(mContinueButton);
    AddWidget(mNewGameButton);
}

//0x433590
void ContinueDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    RemoveWidget(mContinueButton);
    RemoveWidget(mNewGameButton);
}

//0x4335D0
void ContinueDialog::RestartLoopingSounds()
{
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_RAINING_SEEDS || mApp->IsStormyNightLevel())
    {
        mApp->PlayFoley(FoleyType::FOLEY_RAIN);
    }

    Zombie* aZombie = nullptr;
    while (mApp->mBoard->IterateZombies(aZombie))
    {
        if (aZombie->mPlayingSong)
        {
            aZombie->StartZombieSound();
        }
    }
}

//0x4336C0
void ContinueDialog::ButtonDepress(int theId)
{
    if (theId == ContinueDialog::ContinueDialog_Continue)
    {
        if (mApp->mBoard->mNextSurvivalStageCounter != 1)
        {
            std::string aFileName = GetSavedGameName(mApp->mGameMode, mApp->mPlayerInfo->mId);
            mApp->EraseFile(aFileName);
        }

        RestartLoopingSounds();
        mApp->KillDialog(mId);
    }
    else if (theId == ContinueDialog::ContinueDialog_NewGame)
    {
        if (mApp->IsAdventureMode())
        {
            LawnDialog* aDialog = (LawnDialog*)mApp->DoDialog(
                Dialogs::DIALOG_RESTARTCONFIRM, 
                true, 
                _S("[RESTART_LEVEL_HEADER]"), 
                _S("[RESTART_LEVEL]"), 
                _S(""), 
                Dialog::BUTTONS_OK_CANCEL
            );
            aDialog->mLawnYesButton->mLabel = TodStringTranslate(_S("[RESTART_BUTTON]"));
            //aDialog->CalcSize(0, 0);
        }
        else
        {
            LawnDialog* aDialog = (LawnDialog*)mApp->DoDialog(
                Dialogs::DIALOG_RESTARTCONFIRM, 
                true, 
                _S("[NEW_GAME_HEADER]"), 
                _S("[NEW_GAME]"), 
                _S(""), 
                Dialog::BUTTONS_OK_CANCEL
            );
            aDialog->mLawnYesButton->mLabel = TodStringTranslate(_S("[NEW_GAME_BUTTON]"));
            //aDialog->CalcSize(0, 0);
        }
    }
    else
    {
        mApp->KillDialog(mId);
        if (mApp->IsAdventureMode())
        {
            mApp->ShowGameSelector();
        }
        else if (mApp->IsSurvivalMode())
        {
            mApp->KillBoard();
            mApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_SURVIVAL);
        }
        else if (mApp->IsPuzzleMode())
        {
            mApp->KillBoard();
            mApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_PUZZLE);
        }
        else
        {
            mApp->KillBoard();
            mApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_CHALLENGE);
        }
    }
}
