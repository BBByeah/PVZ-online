#pragma once
#include "RunBuffs.h"

// @pvz-online: validate stored IDs; the PC catalogue decides which effects enter the pool.
inline bool RunChoiceImplemented(int id)
{
    return id >= 0 && id < RUN_BUFF_COUNT + RUN_PLANT_UPGRADE_COUNT + RUN_PLANT_BUFF2_COUNT;
}

inline bool RunPlantImplemented(SeedType seed)
{
    // All 48 ordinary PC run plants now have their required consumers.
    return seed >= SEED_PEASHOOTER && seed <= SEED_COBCANNON;
}
