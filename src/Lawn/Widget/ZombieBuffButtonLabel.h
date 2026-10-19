#pragma once
#include "../ModText.h"
#include "../Run/RunState.h"

inline const char* ZombieBuffButtonLabel(int id)
{
    switch (id)
    {
    case RunState::ZOMBIE_BUFF_SWIFT:
        return ModText::Tr("快速", "Swift");
    case RunState::ZOMBIE_BUFF_STEADFAST:
        return ModText::Tr("坚定", "Steadfast");
    case RunState::ZOMBIE_BUFF_SOFTRESIST:
        return ModText::Tr("抗性", "Soft Resist");
    case RunState::ZOMBIE_BUFF_GHOSTWALK:
        return ModText::Tr("穿行", "Ghostwalk");
    default:
        return GetZombieBuffDisplayName(id);
    }
}
