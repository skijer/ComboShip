#include "combo/rando/CrossEntranceTime.h"
#include <cassert>
#include <initializer_list>

int main() {
    using ComboRando::CrossDungeon::MmForeignArrivalAllowed;
    for (bool canPlay : {false, true}) {
        for (bool shuffled : {false, true}) {
            for (int mask = 0; mask < 256; ++mask) {
                const bool allowed = canPlay && (!shuffled || (mask & 0x3f) != 0);
                assert(MmForeignArrivalAllowed(canPlay, shuffled, mask) == allowed);
                assert(MmForeignArrivalAllowed(canPlay, shuffled, mask, false) == allowed);
                // Only explicitly verified frozen boss-only arrivals can bypass reset/clock access.
                assert(MmForeignArrivalAllowed(canPlay, shuffled, mask, true));
            }
        }
    }
}
