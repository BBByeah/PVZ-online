// Local keyboard controls; planting and shovel rules stay in the normal click paths.
#include "BoardInclude.h"
#include "../LawnApp.h"
#include "widget/WidgetManager.h"

bool Board::HasPlantKeyboardControls()
{
    // Special minigames/tools and conveyors need their own slot semantics.
    return (mApp->IsAdventureMode() || mApp->IsSurvivalMode()) &&
        !HasConveyorBeltSeedBank() && !mApp->IsSlotMachineLevel() && mSeedBank->mNumPackets > 0;
}

bool Board::HandlePlantKeyboardKey(KeyCode key)
{
    if ((key < '0' || key > '9') && key != 'Q') return false;
    if (!HasPlantKeyboardControls()) return false;

    // Keep the click restrictions, but allow switching a held bank card/shovel.
    if (mApp->mGameScene != GameScenes::SCENE_PLAYING || mPaused || !mApp->mActive ||
        mApp->mMinimized || mApp->GetDialogCount() > 0 || mBoardFadeOutCounter >= 0 ||
        mTimeStopCounter > 0 || mApp->mCrazyDaveState != CrazyDaveState::CRAZY_DAVE_OFF ||
        mWidgetManager->mDownButtons != 0 || mWidgetManager->mActualDownButtons != 0 ||
        mWidgetManager->mKeyDown[KEYCODE_CONTROL] || mWidgetManager->mKeyDown[KEYCODE_MENU]) return true;
#ifdef __ANDROID__
    if (mTouchSeedDrag) return true;
#endif
    const auto cursor = mCursorObject->mCursorType;
    if (cursor != CursorType::CURSOR_TYPE_NORMAL && cursor != CursorType::CURSOR_TYPE_PLANT_FROM_BANK &&
        cursor != CursorType::CURSOR_TYPE_SHOVEL) return true;

    mKeyboardControlsUsed = true;
    if (key == 'Q')
    {
        if (!mShowShovel || cursor == CursorType::CURSOR_TYPE_SHOVEL) return true;
        if (cursor != CursorType::CURSOR_TYPE_NORMAL) RefreshSeedPacketFromCursor();
        PickUpTool(GameObjectType::OBJECT_TYPE_SHOVEL);
    }
    else
    {
        const int slot = key == '0' ? 9 : key - '1';
        if (slot >= mSeedBank->mNumPackets) return true;
        if (cursor == CursorType::CURSOR_TYPE_PLANT_FROM_BANK && mCursorObject->mSeedBankIndex == slot) return true;
        auto& packet = mSeedBank->mSeedPackets[slot];
        if (!packet.CanPickUp())
        {
            // MouseDown rejects unavailable cards before touching the current cursor.
            packet.MouseDown(0, 0, 1);
            return true;
        }
        if (cursor != CursorType::CURSOR_TYPE_NORMAL) RefreshSeedPacketFromCursor();
        packet.MouseDown(0, 0, 1);
    }
    UpdateCursor();
    MarkDirty();
    return true;
}

void Board::DrawKeyboardHint(Graphics* g, int slot, int x, int y)
{
    if (!mKeyboardControlsUsed || !HasPlantKeyboardControls() ||
        mApp->mGameScene != GameScenes::SCENE_PLAYING || slot >= 10) return;
    const char key = slot < 0 ? 'Q' : (slot == 9 ? '0' : '1' + slot);
    const std::string label(1, key);
    Sexy::Graphics hint(*g);
    hint.SetFont(Sexy::FONT_CONTINUUMBOLD14);
    hint.SetColor(Sexy::Color(35, 30, 20));
    hint.DrawString(label, x + 1, y + 1);
    hint.SetColor(Sexy::Color(255, 255, 190));
    hint.DrawString(label, x, y);
}
