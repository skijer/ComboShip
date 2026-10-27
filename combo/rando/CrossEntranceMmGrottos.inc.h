#pragma once
#include "CrossEntranceMmCatalogue.h"

// Include next to native MM runtime state after global/save/player declarations.
namespace ComboCrossMmCatalogue {
inline int32_t GrottoSeasonEntrance(int32_t entrance) {
    if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
        if ((entrance >> 9) == ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER)
            return ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 0) | (entrance & 0x1FF);
        if ((entrance >> 9) == ENTR_SCENE_PATH_TO_GORON_VILLAGE_WINTER)
            return ENTRANCE(PATH_TO_GORON_VILLAGE_SPRING, 0) | (entrance & 0x1FF);
        if ((entrance >> 9) == ENTR_SCENE_GORON_VILLAGE_WINTER)
            return ENTRANCE(GORON_VILLAGE_SPRING, 0) | (entrance & 0x1FF);
    }
    if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE) &&
        (entrance >> 9) == ENTR_SCENE_SOUTHERN_SWAMP_POISONED)
        return ENTRANCE(SOUTHERN_SWAMP_CLEARED, 0) | (entrance & 0x1FF);
    return entrance;
}
template <typename Respawn>
inline void SetGrottoRespawn(Respawn& destination, const RespawnDescriptor& source, int playerParams) {
    destination.entrance = GrottoSeasonEntrance(source.entrance);
    destination.roomIndex = source.room;
    destination.data = source.data;
    destination.yaw = source.yaw;
    destination.playerParams = playerParams;
    destination.pos.x = source.x;
    destination.pos.y = source.y;
    destination.pos.z = source.z;
    destination.tempSwitchFlags = 0;
    destination.unk_18 = 0;
    destination.tempCollectFlags = 0;
}
} // namespace ComboCrossMmCatalogue

inline int32_t ComboMmMaterializeGrotto(int32_t logical) {
    using namespace ComboCrossMmCatalogue;
    if (const auto* grotto = FindGrottoByEntry(logical)) {
        // Arrival must not inherit a source-world void or hole-exit respawn mode.
        gSaveContext.respawnFlag = 0;
        RespawnDescriptor hole = grotto->hole, down = grotto->down;
        if (grotto->logicalSpawn == 23 && !CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
            // Shuffling can enter this spring-only content before Snowhead is cleared.
            // Keep its content byte while placing every exterior recovery on winter's floor.
            // Winter fallback: ootmm-reference/mm-play.c, kGrottoExitMountainWinter.
            hole = { ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8), 0, hole.data, 0, 345, 8, -150 };
            down = { ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8), 0, 1, 0, 345, 8, -150 };
        }
        SetGrottoRespawn(gSaveContext.respawn[RESPAWN_MODE_DOWN], down, PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D));
        gSaveContext.respawn[RESPAWN_MODE_TOP] = gSaveContext.respawn[RESPAWN_MODE_DOWN];
        SetGrottoRespawn(gSaveContext.respawn[RESPAWN_MODE_UNK_3], hole, PLAYER_PARAMS(0xFF, PLAYER_START_MODE_GROTTO));
        return grotto->PhysicalEntry();
    }
    if (const auto* grotto = FindGrottoByExit(logical)) {
        if (!grotto->holeReturn) {
            gSaveContext.respawnFlag = 0;
            return GrottoSeasonEntrance(grotto->nativeReturn);
        }
        RespawnDescriptor exterior = grotto->hole;
        if (grotto->logicalSpawn == 23 && !CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
            // Spring's tunnel hole is absent in winter. Use the safe winter village floor.
            // Recovered reference: ootmm-reference/mm-play.c, kGrottoExitMountainWinter.
            exterior = { ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8), 0, 0, 0, 345, 8, -150 };
        }
        exterior.data = 0;
        SetGrottoRespawn(gSaveContext.respawn[RESPAWN_MODE_UNK_3], exterior, 0x04FF);
        gSaveContext.respawn[RESPAWN_MODE_DOWN] = gSaveContext.respawn[RESPAWN_MODE_UNK_3];
        gSaveContext.respawn[RESPAWN_MODE_TOP] = gSaveContext.respawn[RESPAWN_MODE_UNK_3];
        gSaveContext.respawnFlag = 4;
        return gSaveContext.respawn[RESPAWN_MODE_UNK_3].entrance;
    }
    return logical;
}
inline int32_t ComboMmCanonicalGrottoLoad(int32_t nativeIndex) {
    return ComboCrossMmCatalogue::CanonicalGrottoLoad(
        nativeIndex, static_cast<uint8_t>(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data));
}
inline int32_t ComboMmCanonicalGrottoExit(int32_t nativeIndex, int scene) {
    using namespace ComboCrossMmCatalogue;
    if (!gPlayState || scene != SCENE_KAKUSIANA)
        return nativeIndex;
    const int logical = LogicalGrottoForRoom(gPlayState->roomCtx.curRoom.num,
                                             static_cast<uint8_t>(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data));
    const auto* grotto = FindGrottoByEntry(logical);
    if (!grotto)
        return nativeIndex;
    if (grotto->holeReturn) {
        // Player's 0xFFFF exit restores UNK_3 and sets flag4. Other flags belong to death/void.
        if (gSaveContext.respawnFlag == 4 &&
            (nativeIndex == 0xFFFF || nativeIndex == gSaveContext.respawn[RESPAWN_MODE_UNK_3].entrance))
            return grotto->Exit();
    } else if (gSaveContext.respawnFlag == 0 &&
               (nativeIndex == grotto->nativeReturn || nativeIndex == GrottoSeasonEntrance(grotto->nativeReturn))) {
        return grotto->Exit();
    }
    return nativeIndex;
}
