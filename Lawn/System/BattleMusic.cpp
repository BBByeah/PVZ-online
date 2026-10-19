#include "Music.h"
#include "BattleMusicState.h"
#include "PlayerInfo.h"
#include "../Board.h"
#include "../Zombie.h"
#include "../ModText.h"
#include "../../LawnApp.h"
#include "sound/BassMusicInterface.h"

namespace
{
static_assert(MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC == BattleMusicState::LAST_TUNE);

class BattleMusicChange
{
    BattleMusicState& mState;
    bool mPrevious;

public:
    explicit BattleMusicChange(BattleMusicState& state) : mState(state), mPrevious(state.mInternalChange)
    {
        mState.mInternalChange = true;
    }
    ~BattleMusicChange() { mState.mInternalChange = mPrevious; }
};

int ReadMusicSetting()
{
    int setting = BattleMusicState::DEFAULT_SETTING;
    ModText::RegReadDword("ModBattleMusic", &setting);
    return setting;
}

bool WriteMusicSetting(int setting)
{
    return ModText::RegWriteDword("ModBattleMusic", setting);
}
}

void Music::LoadBattleMusicSettings()
{
    auto& state = mApp->mBattleMusic;
    if (state.mSettingsLoaded) return;
    int setting = ReadMusicSetting();
    if (!BattleMusicState::ValidSetting(setting)) setting = BattleMusicState::DEFAULT_SETTING;
    state.mSelectedTune = setting & 0xf;
    state.mBossMusic = (setting & 0x100) != 0;
    state.mSettingsLoaded = true;
}

bool Music::SaveBattleMusicSettings(int tune, bool bossMusic)
{
    if (tune < 0 || tune > BattleMusicState::LAST_TUNE) return false;
    const int setting = tune | (bossMusic ? 0x100 : 0);
    if (!WriteMusicSetting(setting)) return false;
    StopMusicPreview();
    auto& state = mApp->mBattleMusic;
    state.mSettingsLoaded = true;
    state.mSelectedTune = tune;
    state.mBossMusic = bossMusic;
    RefreshBattleMusic();
    return true;
}

MusicTune Music::GetSceneMusicTune()
{
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN ||
        mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM)
        return MUSIC_TUNE_ZEN_GARDEN;
    if (mApp->IsFinalBossLevel()) return MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC;
    if (mApp->IsWallnutBowlingLevel() || mApp->IsWhackAZombieLevel() ||
        mApp->IsLittleTroubleLevel() || mApp->IsBungeeBlitzLevel() ||
        mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_SPEED)
        return MUSIC_TUNE_MINIGAME_LOONBOON;
    if ((mApp->IsAdventureMode() && !mApp->IsRunMode() &&
         (mApp->mPlayerInfo->GetLevel() == 10 || mApp->mPlayerInfo->GetLevel() == 20 ||
          mApp->mPlayerInfo->GetLevel() == 30)) ||
        mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_COLUMN)
        return MUSIC_TUNE_CONVEYER;
    if (mApp->IsStormyNightLevel()) return MUSIC_TUNE_NONE;
    if (mApp->IsScaryPotterLevel() || mApp->IsIZombieLevel()) return MUSIC_TUNE_PUZZLE_CEREBRAWL;
    if (mApp->mBoard->StageHasFog()) return MUSIC_TUNE_FOG_RIGORMORMIST;
    if (mApp->mBoard->StageIsNight()) return MUSIC_TUNE_NIGHT_MOONGRAINS;
    if (mApp->mBoard->StageHas6Rows()) return MUSIC_TUNE_POOL_WATERYGRAVES;
    if (mApp->mBoard->StageHasRoof()) return MUSIC_TUNE_ROOF_GRAZETHEROOF;
    return MUSIC_TUNE_DAY_GRASSWALK;
}

MusicTune Music::ResolveBattleMusic()
{
    LoadBattleMusicSettings();
    auto& state = mApp->mBattleMusic;
    if (state.mBossMusic && mApp->IsRunMode() && mApp->mBoard)
    {
        Zombie* zombie = nullptr;
        while (mApp->mBoard->IterateZombies(zombie))
        {
            if (mApp->mBoard->IsRunBossZombie(zombie) && !zombie->mDead &&
                zombie->mBodyHealth > 0 && !zombie->IsDeadOrDying() &&
                IsMusicTuneAvailable(MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC))
                return MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC;
        }
    }
    const MusicTune selected = static_cast<MusicTune>(state.mSelectedTune);
    if (state.mSelectedTune > 0 && IsMusicTuneAvailable(selected)) return selected;
    return static_cast<MusicTune>(state.mSceneTune);
}

MusicTune Music::PrepareGameMusic()
{
    StopMusicPreview();
    auto& state = mApp->mBattleMusic;
    state.mSceneTune = GetSceneMusicTune();
    state.mBoard = mApp->mBoard;
    state.mBattleActive = state.mSceneTune != MUSIC_TUNE_NONE &&
        !mApp->IsFinalBossLevel() &&
        mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN &&
        mApp->mGameMode != GameMode::GAMEMODE_TREE_OF_WISDOM &&
        mApp->mGameMode != GameMode::GAMEMODE_UPSELL &&
        mApp->mGameMode != GameMode::GAMEMODE_INTRO;
    return state.mBattleActive ? ResolveBattleMusic() :
        static_cast<MusicTune>(state.mSceneTune);
}

//0x45B770
void Music::StartGameMusic()
{
    if (!mApp->mBoard) return;
    const MusicTune target = PrepareGameMusic();
    auto& state = mApp->mBattleMusic;
    BattleMusicChange change(state);
    if (target == MUSIC_TUNE_NONE) StopAllMusic();
    else MakeSureMusicIsPlaying(target);
}

bool Music::RefreshBattleMusic(bool resuming)
{
    auto& state = mApp->mBattleMusic;
    Board* board = mApp->mBoard;
    if (!state.mBattleActive || state.mBoard != board || !board || state.mPreviewActive ||
        mApp->mGameScene != GameScenes::SCENE_PLAYING ||
        board->mLevelComplete || board->mBoardFadeOutCounter >= 0 ||
        board->mNextSurvivalStageCounter > 0 || mFadeOutCounter > 0 ||
        mCurMusicTune == MUSIC_TUNE_NONE || (!resuming && (mPaused || board->mPaused)))
        return false;
    const MusicTune target = ResolveBattleMusic();
    if (target == mCurMusicTune || !IsMusicTuneAvailable(target)) return false;
    BattleMusicChange change(state);
    StopAllMusic();
    PlayMusic(target);
    return true;
}

void Music::RestoreGameMusicAfterLoad()
{
    if (!mApp->mBoard || mCurMusicTune == MUSIC_TUNE_NONE || mFadeOutCounter > 0) return;
    const MusicTune savedTune = mCurMusicTune;
    const bool savedPaused = mPaused;
    const int offset = mPauseOffset;
    const int drumsOffset = mPauseOffsetDrums;
    const bool paused = mPaused || mApp->mBoard->mPaused;
    const MusicTune target = PrepareGameMusic();
    BattleMusicChange change(mApp->mBattleMusic);
    StopAllMusic();
    if (target == MUSIC_TUNE_NONE || !IsMusicTuneAvailable(target)) return;
    const bool restorePosition = savedPaused && target == savedTune;
    PlayMusic(target, restorePosition ? offset : -1, restorePosition ? drumsOffset : -1);
    if (paused) GameMusicPause(true);
}

bool Music::StartMusicPreview(MusicTune tune)
{
    if (!IsMusicTuneAvailable(tune)) return false;
    auto& state = mApp->mBattleMusic;
    if (state.mPreviewActive && mCurMusicTune == tune) return true;
    if (!state.mPreviewActive)
    {
        const bool paused = mPaused;
        GameMusicPause(true);
        state.mPreviewReturnTune = mCurMusicTune;
        state.mPreviewReturnOffset = mPauseOffset;
        state.mPreviewReturnDrumsOffset = mPauseOffsetDrums;
        state.mPreviewReturnPaused = paused;
        state.mPreviewReturnFade = mFadeOutCounter;
        state.mPreviewReturnFadeDuration = mFadeOutDuration;
        state.mPreviewActive = true;
    }
    BattleMusicChange change(state);
    StopAllMusic();
    PlayMusic(tune);
    return true;
}

void Music::StopMusicPreview()
{
    auto& state = mApp->mBattleMusic;
    if (!state.mPreviewActive) return;
    state.mPreviewActive = false;
    BattleMusicChange change(state);
    StopAllMusic();
    const MusicTune previous = static_cast<MusicTune>(state.mPreviewReturnTune);
    if (previous == MUSIC_TUNE_NONE || mMusicDisabled) return;
    PlayMusic(previous, state.mPreviewReturnOffset, state.mPreviewReturnDrumsOffset);
    if (state.mPreviewReturnPaused) GameMusicPause(true);
    mFadeOutCounter = state.mPreviewReturnFade;
    mFadeOutDuration = state.mPreviewReturnFadeDuration;
}

bool Music::IsMusicTuneAvailable(MusicTune tune)
{
    if (mMusicDisabled || !mMusicInterface || tune < MUSIC_TUNE_DAY_GRASSWALK ||
        tune > MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC) return false;
    auto* backend = static_cast<Sexy::BassMusicInterface*>(mMusicInterface);
    const auto loaded = [backend](MusicFile file) {
        const auto entry = backend->mMusicMap.find(file);
        return entry != backend->mMusicMap.end() && entry->second.mHMusic != 0;
    };
    if (!loaded(MUSIC_FILE_MAIN_MUSIC)) return false;
    if (tune == MUSIC_TUNE_NIGHT_MOONGRAINS) return loaded(MUSIC_FILE_DRUMS);
    if (tune == MUSIC_TUNE_DAY_GRASSWALK || tune == MUSIC_TUNE_POOL_WATERYGRAVES ||
        tune == MUSIC_TUNE_FOG_RIGORMORMIST || tune == MUSIC_TUNE_ROOF_GRAZETHEROOF)
        return loaded(MUSIC_FILE_DRUMS) && loaded(MUSIC_FILE_HIHATS);
    return true;
}
