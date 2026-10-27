#include "entrances/LostWoodsEntrance.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <utility>

static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() {
    for (int slot : {0, 1, 2})
        require(ComboLostWoods_IsPlayableSlot(slot), "every native save slot, including slot zero, must cross");
    for (int slot : {-1, 3, 0xFE, 0xFF})
        require(!ComboLostWoods_IsPlayableSlot(slot), "title, debug and invalid save slots must not cross");
    require(ComboLostWoods_InOotTunnel(0x5B, 0, 0, 0, -410), "straight-ahead OoT tunnel must cross");
    require(!ComboLostWoods_InOotTunnel(0x5B, 0, 0, 0, -280), "OoT arrival must not bounce back");
    require(!ComboLostWoods_InOotTunnel(0x5B, 8, 0, 0, -410), "another OoT room must not cross");
    require(!ComboLostWoods_InOotTunnel(0x55, 0, 0, 0, -410), "another OoT scene must not cross");
    require(!ComboLostWoods_InOotTunnel(0x5B, 0, 90, 0, -410), "OoT tunnel side wall must not cross");
    require(ComboLostWoods_InOotFairyTunnel(0x5B, 1, 800, 0, 340),
            "pink Fairy tunnel must enter the opening forest");
    require(!ComboLostWoods_InOotFairyTunnel(0x5B, 0, 800, 0, 340),
            "other rooms cannot use the Fairy route");
    require(!ComboLostWoods_InOotFairyTunnel(0x5B, 1, 800, 0, 160),
            "return from the opening must not bounce");
    require(!ComboLostWoods_InOotFairyTunnel(0x5B, 1, 800, 150, 340),
            "flying above the Fairy tunnel cannot cross");
    require(ComboLostWoods_InMmPassage(0x64, 1, 160, 0, -160), "MM side passage must return");
    require(!ComboLostWoods_InMmPassage(0x64, 1, 90, 0, -90), "MM arrival must not bounce back");
    require(!ComboLostWoods_InMmPassage(0x64, 0, 160, 0, -160), "another maze room must not return");
    require(!ComboLostWoods_InMmPassage(0x65, 1, 160, 0, -160), "MM intro woods must stay untouched");
    require(!ComboLostWoods_InMmPassage(0x64, 1, 160, 180, -160), "flying above the opening must not return");
    // Each day can start along a different cardinal path; the swamp exit is the fourth.
    for (auto pos : { std::pair{0, -350}, {0, 350}, {-350, 0}, {350, 0} })
        require(!ComboLostWoods_InMmPassage(0x64, 1, pos.first, 0, pos.second),
                "normal daily maze corridors and the swamp exit must not become Hyrule exits");
    std::puts("PASS: both crossings, safe arrivals, room/scene/height guards, and all four native MM corridors");
}
