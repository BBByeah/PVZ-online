// Gardens are local lobbies for invitations, never unfinished combat lawns.
bool LawnApp::IsOnlineStartLobby() const
{
    return (mGameScene == GameScenes::SCENE_MENU && mBoard == nullptr) ||
        (mBoard != nullptr && (mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN ||
            mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM));
}

void LawnApp::LeaveLocalSceneForOnlineStart()
{
    if (mBoard == nullptr || (mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN &&
        mGameMode != GameMode::GAMEMODE_TREE_OF_WISDOM)) return;

    mMusic->StopAllMusic();
    KillNewOptionsDialog();
    // Dispose under the old mode so LeaveGarden/TreeOfWisdomLeave settle local state.
    KillBoard();
    WriteCurrentUserConfig();
    mBoardResult = BoardResult::BOARDRESULT_NONE;
    mGameScene = GameScenes::SCENE_MENU;
    mRunIntroHeld = false;
    mOnlineSeedsHeld = false;
    // Do not rebuild the menu: that would discard this START/ACK/RUN_GO exchange.
}
