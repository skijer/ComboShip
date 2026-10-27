#pragma once

#ifdef COMBO_BUILD
#include "../../../../combo/ComboGameMode.h"
#include "global.h"
#include "../../../../combo/entrances/LostWoodsEntrance.h"
#include <math.h>
#include <string.h>

// Inspect the active mesh so another pack winning load order cannot hide
// the fallback arch merely because a resource archive is mounted.
#define COMBO_OPENING_FOREST_NATIVE_PORTAL_ROOT "__OTR__custom/comboship/native_forest/portal_root"
#define COMBO_OPENING_FOREST_PACK_ROOT "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_v1"
#define COMBO_OPENING_FOREST_STUMP_PACK_ROOT "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_stump_v2"
#define COMBO_OPENING_FOREST_CORNER_PACK_ROOT "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_corner_v3"
#define COMBO_OPENING_FOREST_EDGE_PACK_ROOT "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_edge_v4"
#define COMBO_OPENING_FOREST_ALIGNED_PACK_ROOT \
    "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_aligned_v5"
#define COMBO_OPENING_FOREST_CENTERED_PACK_ROOT \
    "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_centered_v6"
#define COMBO_OPENING_FOREST_RECENTERED_PACK_ROOT \
    "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_centered_v7"
#define COMBO_OPENING_FOREST_NUDGED_PACK_ROOT \
    "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_centered_v8"

// Follow the large room-edge trunk face, turned 22.5 degrees from POC5.
// POC9 nudges right along that face; older packs retain their poses.
#define COMBO_OPENING_FOREST_STUMP_X (-695.0f)
#define COMBO_OPENING_FOREST_STUMP_Z (-313.0f)
#define COMBO_OPENING_FOREST_STUMP_YAW 12288

// A playable visit uses spawn zero/layer zero. Song of Time flashbacks use
// other spawns/layers and retain their native behavior. No story flag marks a visit.
static inline int ComboOpeningForest_IsExploration(const PlayState* play) {
    return Combo_IsCombinedGame() && play && play->sceneId == SCENE_LOST_WOODS &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) &&
           gSaveContext.sceneLayer == 0 && gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0) &&
           gSaveContext.save.cutsceneIndex == 0;
}

static inline void ComboOpeningForest_PrepareSpawn(PlayState* play) {
    if (!ComboOpeningForest_IsExploration(play) || !play->linkActorEntry)
        return;
    // Never edit the cached native/Alt resource or change the saved player form.
    static ActorEntry spawn;
    spawn = *play->linkActorEntry;
    spawn.params = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E);
    play->linkActorEntry = &spawn;
}

static inline void ComboOpeningForest_NormalizeEntry(void) {
    if (!Combo_IsCombinedGame() || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) ||
        (gSaveContext.save.entrance >> 9) != ENTR_SCENE_OPENING_DUNGEON)
        return;
    // The forward fall normally enters the curse/tutorial sequence. Continue
    // into the ordinary Clock Tower instead, before any prologue actor initializes.
    gSaveContext.save.entrance = ENTRANCE(CLOCK_TOWER_INTERIOR, 0);
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.respawnFlag = 0;
}

static inline int ComboOpeningForest_ReturnPackVersion(const PlayState* play) {
    if (!ComboOpeningForest_IsExploration(play) || play->roomCtx.curRoom.num != 0)
        return 0;
    const RoomShape* shape = play->roomCtx.curRoom.roomShape;
    if (!shape)
        return 0;
    // The native scene keeps stock type-two room culling. Its portal is a
    // separate opaque entry, while legacy visual packs use one wrapper.
    if (shape->base.type == ROOM_SHAPE_TYPE_CULLABLE) {
        if (!shape->cullable.entries)
            return 0;
        for (unsigned i = 0; i < shape->cullable.numEntries; ++i) {
            const char* path = (const char*)shape->cullable.entries[i].opa;
            if (path && strcmp(path, COMBO_OPENING_FOREST_NATIVE_PORTAL_ROOT) == 0)
                return 9;
        }
        return 0;
    }
    if (shape->base.type != ROOM_SHAPE_TYPE_NORMAL || shape->normal.numEntries != 1 || !shape->normal.entries ||
        !shape->normal.entries[0].opa)
        return 0;
    const char* path = (const char*)shape->normal.entries[0].opa;
    if (strcmp(path, COMBO_OPENING_FOREST_NUDGED_PACK_ROOT) == 0)
        return 8;
    if (strcmp(path, COMBO_OPENING_FOREST_RECENTERED_PACK_ROOT) == 0)
        return 7;
    if (strcmp(path, COMBO_OPENING_FOREST_CENTERED_PACK_ROOT) == 0)
        return 6;
    if (strcmp(path, COMBO_OPENING_FOREST_ALIGNED_PACK_ROOT) == 0)
        return 5;
    if (strcmp(path, COMBO_OPENING_FOREST_EDGE_PACK_ROOT) == 0)
        return 4;
    if (strcmp(path, COMBO_OPENING_FOREST_CORNER_PACK_ROOT) == 0)
        return 3;
    if (strcmp(path, COMBO_OPENING_FOREST_STUMP_PACK_ROOT) == 0)
        return 2;
    return strcmp(path, COMBO_OPENING_FOREST_PACK_ROOT) == 0 ? 1 : 0;
}

static inline int ComboOpeningForest_UsesPackTunnel(const PlayState* play) {
    return ComboOpeningForest_ReturnPackVersion(play) != 0;
}

static inline Vec3f ComboOpeningForest_ReturnAnchor(const PlayState* play) {
    const int version = ComboOpeningForest_ReturnPackVersion(play);
    Vec3f anchor = { COMBO_OPENING_FOREST_STUMP_X, 0.0f, COMBO_OPENING_FOREST_STUMP_Z };
    if (version == 1 && play->linkActorEntry) {
        const Vec3s p = play->linkActorEntry->pos;
        anchor.x = p.x;
        anchor.y = p.y;
        anchor.z = p.z;
    } else if (version == 2) {
        anchor.x = 120.0f;
        anchor.z = 240.0f;
    } else if (version == 3) {
        anchor.x = -390.0f;
        anchor.z = 120.0f;
    } else if (version == 4) {
        anchor.x = -660.0f;
        anchor.z = -350.0f;
    } else if (version == 5) {
        anchor.x = -677.0f;
        anchor.z = -357.0f;
    } else if (version == 6) {
        anchor.x = -684.0f;
        anchor.z = -340.0f;
    } else if (version == 7) {
        anchor.x = -698.0f;
        anchor.z = -306.0f;
    }
    return anchor;
}

static inline s16 ComboOpeningForest_ReturnYaw(const PlayState* play) {
    const int version = ComboOpeningForest_ReturnPackVersion(play);
    if (version == 1 && play->linkActorEntry)
        return play->linkActorEntry->rot.y;
    if (version == 2)
        return -16384;
    return version == 3 || version == 4 ? 16384 : COMBO_OPENING_FOREST_STUMP_YAW;
}

static inline int ComboOpeningForest_AtReturn(const PlayState* play, const Vec3f* position) {
    if (!ComboOpeningForest_IsExploration(play) || play->roomCtx.curRoom.num != 0 || !play->linkActorEntry)
        return 0;
    const int version = ComboOpeningForest_ReturnPackVersion(play);
    // Keep the visible doorway and trigger together when an older pack wins
    // load order. Native assets use the temporary arch at the new stump.
    const Vec3f anchor = ComboOpeningForest_ReturnAnchor(play);
    const s16 yaw = ComboOpeningForest_ReturnYaw(play);
    const float dx = position->x - anchor.x, dz = position->z - anchor.z, dy = position->y - anchor.y;
    const float angle = yaw * (6.28318530718f / 65536.0f);
    const float lateral = dx * cosf(angle) - dz * sinf(angle);
    const float depth = dx * sinf(angle) + dz * cosf(angle);
    const float halfWidth = version >= 1 && version <= 8 ? 58.0f : 42.0f;
    // Trigger ahead of the black face (depth -24), before the existing stump
    // wall. The retained native arrival is well outside these bounds.
    return lateral > -halfWidth && lateral < halfWidth && depth > -62.0f && depth < 12.0f && dy > -40.0f && dy < 130.0f;
}
#endif
