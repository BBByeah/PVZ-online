#ifndef __RUNMATH_H__
#define __RUNMATH_H__

#include <cstdint>
#include <climits>

namespace RunMath
{
// @pvz-online: large run multipliers must divide in 64 bits before narrowing; keep absent-armor sentinels.
inline int ScaleHealth(int theHealth, int theNumerator, int theDenominator = 1000)
{
    if (theHealth <= 0) return theHealth;
    const int64_t aScaled = static_cast<int64_t>(theHealth) * theNumerator / theDenominator;
    return aScaled > INT_MAX ? INT_MAX : static_cast<int>(aScaled);
}

// @pvz-online: charm/imp scaling also needs saturation before a floating result becomes an int.
inline int ScaleHealthFloat(int theHealth, float theFactor, bool theRound = false)
{
    if (theHealth <= 0) return theHealth;
    const double aScaled = static_cast<double>(theHealth) * theFactor + (theRound ? 0.5 : 0.0);
    return aScaled >= INT_MAX ? INT_MAX : static_cast<int>(aScaled);
}
}

#endif
