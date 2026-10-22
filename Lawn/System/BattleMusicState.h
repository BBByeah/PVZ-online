#pragma once

class Board;

struct BattleMusicState
{
    static constexpr int DEFAULT_SETTING = 0x100;
    static constexpr int LAST_TUNE = 12;

    bool mSettingsLoaded = false;
    int mSelectedTune = 0;
    bool mBossMusic = true;
    Board* mBoard = nullptr;
    int mSceneTune = -1;
    bool mBattleActive = false;
    bool mInternalChange = false;
    bool mPreviewActive = false;
    int mPreviewReturnTune = -1;
    int mPreviewReturnOffset = -1;
    int mPreviewReturnDrumsOffset = -1;
    bool mPreviewReturnPaused = false;
    int mPreviewReturnFade = 0;
    int mPreviewReturnFadeDuration = 0;

    static bool ValidSetting(int setting)
    {
        return setting >= 0 && (setting & ~0x10f) == 0 && (setting & 0xf) <= LAST_TUNE;
    }
};
