#pragma once
#include "RegionalPassages.h"
namespace ComboRegional::IkanaGrave {
// Authored native/Alt spot02 room1: final +X grave of the z=-336 row,
// nearest the Royal Tomb. This is an existing Bg_Haka, never a spawned copy.
constexpr int kRoom = 1;
constexpr Pose kPose{ { 22, 120, -336 }, 0, { 0, 0, 1 } };
inline bool Matches(Point home, short yaw, short params, int room) {
    return room == kRoom && params == 0 && yaw == kPose.yaw && home.x == kPose.pos.x && home.y == kPose.pos.y &&
           home.z == kPose.pos.z;
}
} // namespace ComboRegional::IkanaGrave
