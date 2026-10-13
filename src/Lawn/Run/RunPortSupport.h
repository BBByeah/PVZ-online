#pragma once
#include "RunBuffs.h"

// @pvz-online: only offer effects with live Android consumers during the staged port.
inline bool RunChoiceImplemented(int id)
{
    return id == RUN_BUFF_FIREPOWER || id == RUN_BUFF_ROOTED || id == RUN_BUFF_FASTSEED;
}

inline bool RunPlantImplemented(SeedType seed)
{
    // Upgrades, awake mushrooms and aquatic planting need their mod-specific rules first.
    return (seed >= SEED_PEASHOOTER && seed <= SEED_REPEATER) ||
        seed == SEED_SQUASH || seed == SEED_THREEPEATER ||
        (seed >= SEED_JALAPENO && seed <= SEED_TALLNUT) ||
        (seed >= SEED_PLANTERN && seed <= SEED_KERNELPULT) ||
        (seed >= SEED_GARLIC && seed <= SEED_MELONPULT);
}
