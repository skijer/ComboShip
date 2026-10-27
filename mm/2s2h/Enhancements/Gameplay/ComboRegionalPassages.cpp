#ifdef COMBO_BUILD
#include "ComboRegionalPassages.h"
#include "ComboGameMode.h"
#include "2s2h/Enhancements/Camera/ComboForestArrivalCamera.h"
#include "entrances/MmRegionalBridgePlacements.h"

namespace {
using namespace ComboRegional;
using ComboMmRegional::Placement;
static_assert(SCENE_IKANAMAE == ComboMmRegional::kIkana.scene && SCENE_30GYOSON == ComboMmRegional::kGreatBay.scene &&
              SCENE_10YUKIYAMANOMURA == ComboMmRegional::kMountainWinter.scene &&
              SCENE_10YUKIYAMANOMURA2 == ComboMmRegional::kMountainSpring.scene);
struct Passage {
    PlayState* play = nullptr;
    const Placement* placement = nullptr;
    int slot = -1;
    u32 seed = 0;
    bool armed = false;
} sRegional;
PlayState* sEntrancePlay = nullptr;
unsigned sEntranceCount = 0;
struct Arrival {
    const Placement* placement = nullptr;
    int slot = -1, entrance = -1;
    u32 seed = 0;
} sArrival;
// Scene resources are cached across owl warps. Keep the bridge-only start
// params in a module-owned copy, alive through native player initialization.
ActorEntry sArrivalSpawn{};
} // namespace

extern "C" void ComboRegionalPassagesMm_Reset(void) {
    sRegional = {};
    sEntrancePlay = nullptr;
    sEntranceCount = 0;
    sArrival = {};
}
extern "C" void ComboRegionalPassagesMm_ReadEntrances(PlayState* play, unsigned count) {
    // Scene headers can reuse the same PlayState arena. The next spawn command
    // must recapture ownership before either drawing or testing a return.
    sRegional = {};
    sEntrancePlay = play;
    sEntranceCount = count;
}
extern "C" void ComboRegionalPassagesMm_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count) {
    sRegional = {};
    const auto arrival = sArrival;
    sArrival = {};
    if (!Combo_IsCombinedGame() || !play || !entries || !play->setupEntranceList || !count || sEntrancePlay != play)
        return;
    const auto* placement = ComboMmRegional::ForScene(play->sceneId);
    if (!placement || static_cast<unsigned>(placement->entrance) >= sEntranceCount)
        return;
    const auto native = play->setupEntranceList[placement->entrance];
    if (native.spawn >= count || native.room != placement->room)
        return;
    if (arrival.placement == placement && arrival.slot == gSaveContext.fileNum &&
        arrival.seed == gSaveContext.save.shipSaveInfo.rando.finalSeed &&
        arrival.entrance == gSaveContext.save.entrance && play->curSpawn == placement->entrance &&
        gSaveContext.gameMode == GAMEMODE_NORMAL && ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) &&
        gSaveContext.respawnFlag == 0 && gSaveContext.save.cutsceneIndex == 0 &&
        gSaveContext.save.shipSaveInfo.pauseSaveEntrance == -1 && !gSaveContext.save.isOwlSave &&
        play->linkActorEntry == &entries[native.spawn] &&
        PLAYER_GET_START_MODE(play->linkActorEntry) == PLAYER_START_MODE_OWL) {
        sArrivalSpawn = *play->linkActorEntry;
        sArrivalSpawn.params = PLAYER_PARAMS(sArrivalSpawn.params, PLAYER_START_MODE_F);
        play->linkActorEntry = &sArrivalSpawn;
        ComboForestArrivalCamera_Queue(COMBO_PORTAL_SNOWHEAD);
    }
    // Keep native player initialization and the cached owl pose intact. Only a
    // matched Snowhead handoff queues the transactional exterior reveal; its
    // floor/camera failure retains this dry native landing.
    sRegional.play = play;
    sRegional.placement = placement;
    sRegional.slot = gSaveContext.fileNum;
    sRegional.seed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
extern "C" int ComboRegionalPassagesMm_Get(PlayState* play, int* route, Vec3f* anchor, s16* yaw) {
    const auto* placement = sRegional.placement;
    if (!Combo_IsCombinedGame() || !play || play != sRegional.play || !placement ||
        ComboMmRegional::ForScene(play->sceneId) != placement || play->roomCtx.curRoom.num != placement->room ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) ||
        sRegional.slot != gSaveContext.fileNum || sRegional.seed != gSaveContext.save.shipSaveInfo.rando.finalSeed ||
        !play->colCtx.colHeader || !GET_PLAYER(play))
        return false;
    if (route)
        *route = placement->route;
    if (anchor)
        *anchor = { placement->mouth.pos.x, placement->mouth.pos.y, placement->mouth.pos.z };
    if (yaw)
        *yaw = placement->mouth.yaw;
    return true;
}
extern "C" int ComboRegionalPassagesMm_AtReturn(PlayState* play, const Vec3f* position) {
    if (!position || !ComboRegionalPassagesMm_Get(play, nullptr, nullptr, nullptr))
        return 0;
    const auto& placement = *sRegional.placement;
    const Point point{ position->x, position->y, position->z };
    if (Distance2(point, placement.mouth.pos) > placement.armDistance * placement.armDistance)
        sRegional.armed = true;
    return sRegional.armed && InMouth(placement.mouth, point) ? placement.route : 0;
}
extern "C" int ComboRegionalPassagesMm_ApplyArrival(int route) {
    if (!Combo_IsCombinedGame() || !ComboRegionalPassages_IsRoute(route) || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum))
        return 0;
    const auto* placement =
        ComboMmRegional::ForArrival(route, CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE));
    // These native approach records own floor, facing and camera. No respawn
    // coordinates or cycle/progression flags are rewritten.
    gSaveContext.save.entrance = placement->scene == SCENE_IKANAMAE   ? ENTRANCE(ROAD_TO_IKANA, placement->entrance)
                                 : placement->scene == SCENE_30GYOSON ? ENTRANCE(GREAT_BAY_COAST, placement->entrance)
                                 : placement->scene == SCENE_10YUKIYAMANOMURA2
                                     ? ENTRANCE(MOUNTAIN_VILLAGE_SPRING, placement->entrance)
                                     : ENTRANCE(MOUNTAIN_VILLAGE_WINTER, placement->entrance);
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.cutsceneTrigger = 0;
    // A portal checkpoint retains its ordinary resume marker. This explicit
    // destination replaces that resume, so use the bridge's normal start mode
    // rather than inheriting the saved-session marker's stationary mode D.
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = -1;
    gSaveContext.save.isOwlSave = false;
    gSaveContext.respawnFlag = 0;
    sArrival = route == COMBO_PORTAL_SNOWHEAD ? Arrival{ placement, gSaveContext.fileNum, gSaveContext.save.entrance,
                                                         gSaveContext.save.shipSaveInfo.rando.finalSeed }
                                              : Arrival{};
    return 1;
}
#endif
