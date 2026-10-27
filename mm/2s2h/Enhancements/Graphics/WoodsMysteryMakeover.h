#pragma once

#ifdef COMBO_BUILD
#include "global.h"
#include <cstring>

extern "C" {
bool ResourceMgr_IsAltAssetsEnabled();
uint8_t ResourceMgr_FileAltExists(const char* path);
uint8_t ResourceMgr_FileExists(const char* path);
}

namespace WoodsMysteryMakeover {
constexpr char kBridgeMesh[] = "__OTR__custom/cor_woods_mystery/poc1/room_1/geometry";
constexpr float kMouthX = 110.0f;
constexpr float kMouthZ = -110.0f;
constexpr s16 kMouthYaw = -8192;
// The accepted sleeve's black cap is at its back, about 150 units beyond
// the mouth. Native baking retains this same cap; keep gameplay anchors intact.
constexpr float kBlackPortalX = 216.0f;
constexpr float kBlackPortalZ = -216.0f;

inline bool OwnsBridgeMesh(const PlayState* play) {
    if (!play || play->sceneId != SCENE_26SARUNOMORI || play->roomCtx.curRoom.num != 1)
        return false;
    const RoomShape* shape = play->roomCtx.curRoom.roomShape;
    if (!shape || shape->base.type != ROOM_SHAPE_TYPE_CULLABLE || !shape->cullable.entries ||
        shape->cullable.numEntries == 0)
        return false;
    // A mounted archive alone is insufficient: the loaded room must also use it.
    const auto* opa = shape->cullable.entries[shape->cullable.numEntries - 1].opa;
    return opa && std::strncmp(reinterpret_cast<const char*>(opa), "__OTR__", 7) == 0 &&
           std::strcmp(reinterpret_cast<const char*>(opa), kBridgeMesh) == 0;
}

inline bool Active(const PlayState* play) {
    // Alt can change without a room reload, leaving an old custom mesh pointer.
    return OwnsBridgeMesh(play) && ResourceMgr_IsAltAssetsEnabled() &&
           (ResourceMgr_FileExists(kBridgeMesh) || ResourceMgr_FileAltExists(kBridgeMesh));
}
} // namespace WoodsMysteryMakeover
#endif
