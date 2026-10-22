#pragma once
#include "../Board.h"
#include "../Zombie.h"
#include <vector>

// Only the zombie-update pass may batch pulses. Plant/projectile/direct calls
// retain an immediate sweep. No saved Board/Zombie layout or RNG is changed.
namespace FrostArmorBatch
{
inline void Refresh(Zombie* zombie)
{
    if (!zombie || zombie->mDead) return;
    const int frames = zombie->SoftControlDuration(2000);
    if (zombie->mFrostArmorCounter < frames)
    {
        zombie->mFrostArmorCounter = frames;
        zombie->UpdateAnimSpeed();
    }
}
inline void Sweep(Board* board)
{
    Zombie* zombie = nullptr;
    while (board->IterateZombies(zombie)) Refresh(zombie);
}
class Scope;
inline thread_local Scope* active = nullptr;
class Scope
{
    Board* board;
    Scope* previous;
    unsigned int allocationKey = 0;
    bool pulsed = false;
    std::vector<unsigned int> updated;
public:
    explicit Scope(Board* value) : board(value), previous(active) { active = this; }
    ~Scope() { active = previous; }
    void AfterUpdate(Zombie* zombie)
    {
        if (pulsed) updated.push_back(board->mZombies.DataArrayGetID(zombie));
    }
    bool Pulse(Board* value)
    {
        if (value != board) return false;
        // Unvisited zombies still have the previous pulse's full duration.
        // Allocations (including slot reuse and sled followers) invalidate that
        // invariant, so rescan immediately when the pool generation advances.
        if (!pulsed || allocationKey != board->mZombies.mNextKey)
            Sweep(board);
        else
            for (unsigned int id : updated) Refresh(board->mZombies.DataArrayTryToGet(id));
        updated.clear();
        pulsed = true;
        allocationKey = board->mZombies.mNextKey;
        return true;
    }
};
inline void Pulse(Board* board)
{
    if (!active || !active->Pulse(board)) Sweep(board);
}
}
