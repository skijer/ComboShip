#pragma once
#include "RegionalPassages.h"

namespace ComboMmRegional {
// Read-only native MM collision evidence: mm.o2r SHA256
// f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59.
// The fixed pose is two units in front of the intact wall. Native collision
// supports the shell/apron; no scene mesh, floor, water or exit is replaced.
// Rendering and mouth tests share this pose; native arrival uses the same
// profile's original entrance, keeping its camera and player-start machinery.
// Explicit Snowhead bridge arrivals substitute normal F start params in a
// private spawn copy; the native cached owl entry remains unchanged. The
// one-shot camera presentation then places Link on the dry exterior apron
// only after current floor and camera clearance both succeed.
struct Placement {
    int scene, route, entrance, room;
    ComboRegional::Pose mouth, nativeArrival;
    float armDistance;
};
// Road to Ikana: approach-zero north-west rock wall, polygon58.
inline constexpr Placement kIkana{
    0x53, COMBO_PORTAL_IKANA, 0, 0, { { -3132.08398f, 0, -165.22842f }, 25730 }, { { -3006, 0, -305 }, 14563 }, 110
};
// Coast: dry rock corridor beside the Termina Field approach, polygon321.
inline constexpr Placement kGreatBay{
    0x37, COMBO_PORTAL_GREAT_BAY, 0, 0, { { 3404.66382f, 80, 4431.17334f }, 25177 }, { { 3585, 80, 4394 }, -23484 }, 110
};
// Mountain Village: wall beside the native owl platform, polygons376/377
// (winter),319/413 (spring). The approach-zero snowbank melts into water;
// native owl entrance8 remains dry in both seasons and already has native
// winter-to-spring handling. Small native snowballs are left intact.
inline constexpr Placement kMountainWinter{
    0x50, COMBO_PORTAL_SNOWHEAD, 8, 0, { { -396.76031f, 8, -322.01703f }, -1362 }, { { -649, 8, -196 }, 20388 }, 110
};
// Outside the return arm radius (110), clear of native owl/snowballs, and
// inside the read-only native dry apron sampling in both seasonal scenes.
inline constexpr float kMountainPortalArrivalDepth = 140.0f;
inline constexpr Placement kMountainSpring{ 0x5A, COMBO_PORTAL_SNOWHEAD, 8,
                                            0,    kMountainWinter.mouth, kMountainWinter.nativeArrival,
                                            110 };
inline const Placement* ForScene(int scene) {
    switch (scene) {
        case 0x53:
            return &kIkana;
        case 0x37:
            return &kGreatBay;
        case 0x50:
            return &kMountainWinter;
        case 0x5A:
            return &kMountainSpring;
        default:
            return nullptr;
    }
}
inline const Placement* ForArrival(int route, bool spring) {
    switch (route) {
        case COMBO_PORTAL_IKANA:
            return &kIkana;
        case COMBO_PORTAL_GREAT_BAY:
            return &kGreatBay;
        case COMBO_PORTAL_SNOWHEAD:
            return spring ? &kMountainSpring : &kMountainWinter;
        default:
            return nullptr;
    }
}
} // namespace ComboMmRegional
