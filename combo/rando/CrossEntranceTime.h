#pragma once
#include <cstdint>

namespace ComboRando::CrossDungeon {
// Solver admission for cross-injected MM roots; runtime recalls/recovery must remain usable.
// A normal temple keeps MM's clock running, so replaying earlier time requires a playable
// Song of Time and, with Clock Shuffle, at least one real owned half-day (not a fallback slice).
// The boss exception requires a separately verified frozen boss-only arrival. The current
// five-temple catalogue has no such exception. This policy never modifies runtime time/speed.
inline bool MmForeignArrivalAllowed(bool canPlaySongOfTime, bool clockShuffle, uint8_t ownedClockMask,
                                    bool verifiedFrozenBossArrival = false) {
    return verifiedFrozenBossArrival || (canPlaySongOfTime && (!clockShuffle || (ownedClockMask & 0x3f) != 0));
}
} // namespace ComboRando::CrossDungeon
