#include "soh/Enhancements/Gameplay/ComboForestActors.h"
#include <cassert>
int main() {
    ComboForestActors::Visit visit;
    int roomData = 0, otherRoom = 0;
    assert(!visit.Ready(0, &roomData, 1));
    assert(!visit.Ready(0, nullptr, 0));
    assert(visit.room == -1 && visit.complete == 0);
    assert(visit.Ready(0, &roomData, 0));
    int scans = 0, spawns = 0;
    auto ensure = [&](bool authored, bool success) {
        visit.Ensure(
            1,
            [&] {
                ++scans;
                return authored;
            },
            [&] {
                ++spawns;
                return success;
            });
    };
    ensure(true, true);
    ensure(false, true);
    assert(scans == 1 && spawns == 0); // authored placement wins, then latches
    assert(visit.Ready(1, &otherRoom, 0));
    ensure(false, false);
    ensure(false, true);
    ensure(false, true);
    assert(scans == 3 && spawns == 2); // failed spawn retries, successful spawn only once
    visit.Reset();
    assert(visit.Ready(1, &otherRoom, 0));
    ensure(false, true);
    assert(spawns == 3);
}
