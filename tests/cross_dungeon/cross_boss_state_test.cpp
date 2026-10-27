#include <cassert>
#include <cstdint>
#if __has_include("combo/rando/CrossEntranceBoss.h")
#include "combo/rando/CrossEntranceBoss.h"
static int notified = -1;
static void Notify(int id) { notified = id; }
int main() {
    using ComboRando::CrossDungeon::BossCompletionState;
    BossCompletionState state;
    assert(state.LogicalClear(0, true, false));
    assert(!state.LogicalClear(0, true, true)); // Physical vanilla effect cannot leak into logical clear.
    state.SetCompletedMask(1u << 9);
    assert(state.LogicalClear(9, false, true));
    assert(!state.LogicalClear(8, true, true));
    state.SetCompletedMask(0xffff);
    assert(state.CompletedMask() == 0xfff);
    state.SetNotify(Notify);
    assert(!state.Warp(-1, true));
    assert(!state.Warp(12, true));
    assert(!state.Warp(0, false));
    assert(notified == -1);
    assert(state.Warp(11, true) && notified == 11);
    notified = -1;
    assert(state.Warp(11, true) && notified == 11); // Repeated cycle clear must reapply effects.
    state.SetCompletedMask(0);
    assert(!state.LogicalClear(9, true, true)); // Slot load replaces, does not merge stale progress.
}
#else
int main() { assert(false && "cross-boss logical state is not implemented"); }
#endif
