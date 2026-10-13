#pragma once
#include "RunBuffs.h"

// @pvz-online: only offer effects with live Android consumers during the staged port.
inline bool RunChoiceImplemented(int id)
{
    return id == RUN_BUFF_FIREPOWER || id == RUN_BUFF_ROOTED || id == RUN_BUFF_FASTSEED;
}

inline bool RunPlantImplemented(SeedType seed)
{
    // Coffee and upgrade plants still need their mod-specific rules; base mushrooms/aquatics are ready.
    return (seed >= SEED_PEASHOOTER && seed <= SEED_KERNELPULT) ||
        (seed >= SEED_GARLIC && seed <= SEED_MELONPULT);
}
