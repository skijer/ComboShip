#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <cstdarg>

static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
static int storySkip;
int CVarGetInteger(const char*, int) {
    return storySkip;
}
void Flags_SetEventChkInf(s32) {
}
#undef CVAR_ENHANCEMENT
#define CVAR_ENHANCEMENT(name) name
#define RAND_GET_OPTION(option) Randomizer_GetSettingValue(option)

/* BLUE_WARP_HOOK */

static void Invoke(GIVanillaBehavior id, bool* should, ...) {
    va_list args;
    va_start(args, should);
    interactor.ExecuteHooksForID<GameInteractor::OnVanillaBehavior>(id, id, should, args);
    va_end(args);
}

struct WarpCase {
    int dungeon, outside, warp, firstEntrance, firstCutscene, chamber;
};
static const WarpCase cases[] = {
    { ENTR_DEKU_TREE_ENTRANCE, ENTR_KOKIRI_FOREST_OUTSIDE_DEKU_TREE, ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,
      ENTR_KOKIRI_FOREST_0, 0xFFF1, 0 },
    { ENTR_DODONGOS_CAVERN_ENTRANCE, ENTR_DEATH_MOUNTAIN_TRAIL_OUTSIDE_DODONGOS_CAVERN,
      ENTR_DEATH_MOUNTAIN_TRAIL_DODONGO_BLUE_WARP, ENTR_DEATH_MOUNTAIN_TRAIL_BOTTOM_EXIT, 0xFFF1, 0 },
    { ENTR_JABU_JABU_ENTRANCE, ENTR_ZORAS_FOUNTAIN_OUTSIDE_JABU_JABU, ENTR_ZORAS_FOUNTAIN_JABU_JABU_BLUE_WARP,
      ENTR_ZORAS_FOUNTAIN_JABU_JABU_BLUE_WARP, 0xFFF0, 0 },
    { ENTR_FOREST_TEMPLE_ENTRANCE, ENTR_SACRED_FOREST_MEADOW_OUTSIDE_TEMPLE,
      ENTR_SACRED_FOREST_MEADOW_FOREST_TEMPLE_BLUE_WARP, ENTR_CHAMBER_OF_THE_SAGES_0, 0, CHAMBER_CS_FOREST },
    { ENTR_FIRE_TEMPLE_ENTRANCE, ENTR_DEATH_MOUNTAIN_CRATER_OUTSIDE_TEMPLE,
      ENTR_DEATH_MOUNTAIN_CRATER_FIRE_TEMPLE_BLUE_WARP, ENTR_KAKARIKO_VILLAGE_FRONT_GATE, 0xFFF3, 0 },
    { ENTR_WATER_TEMPLE_ENTRANCE, ENTR_LAKE_HYLIA_OUTSIDE_TEMPLE, ENTR_LAKE_HYLIA_WATER_TEMPLE_BLUE_WARP,
      ENTR_CHAMBER_OF_THE_SAGES_0, 0, CHAMBER_CS_WATER },
    { ENTR_SPIRIT_TEMPLE_ENTRANCE, ENTR_DESERT_COLOSSUS_OUTSIDE_TEMPLE, ENTR_DESERT_COLOSSUS_SPIRIT_TEMPLE_BLUE_WARP,
      ENTR_CHAMBER_OF_THE_SAGES_0, 0, CHAMBER_CS_SPIRIT },
    { ENTR_SHADOW_TEMPLE_ENTRANCE, ENTR_GRAVEYARD_OUTSIDE_TEMPLE, ENTR_GRAVEYARD_SHADOW_TEMPLE_BLUE_WARP,
      ENTR_CHAMBER_OF_THE_SAGES_0, 0, CHAMBER_CS_SHADOW },
};

static Dungeon Source(const WarpCase& warp) {
    Dungeon dungeon{ "source", { 0, warp.dungeon }, { 0, warp.outside }, { 0, warp.outside } };
    dungeon.escapeExits.push_back({ 0, warp.warp });
    return dungeon;
}

int main() {
    using namespace ComboRando::CrossDungeon;
    RegisterShouldPlayBlueWarp();
    PlayState play{};
    gPlayState = &play;
    Grotto_InitExitAndLoadLists();
    // Each alias runs through the registered hook, real native entrance dispatch
    // and real cross runtime. No direct Complete call is made by this fixture.
    for (const auto& warp : cases) {
        for (bool first : { true, false }) {
            for (int mode : { 0, 1, 2 }) { // foreign, same-game composed, native story skip
                gSaveContext = {};
                gSaveContext.fileNum = 1;
                gSaveContext.ship.quest.id = QUEST_RANDOMIZER;
                play = {};
                play.state.running = true;
                play.transitionTrigger = TRANS_TRIGGER_END;
                play.transitionMode = TRANS_MODE_SETUP;
                play.transitionType = TRANS_TYPE_FADE_WHITE;
                storySkip = mode == 2;
                queued = -1;
                queues = 0;
                Dungeon source = Source(warp);
                Dungeon peer = mode == 0 ? Dungeon{ "peer", { 1, 20 }, { 1, 21 }, { 1, 22 } }
                                         : Dungeon{ "peer",
                                                    { 0, ENTR_GERUDO_TRAINING_GROUND_ENTRANCE },
                                                    { 0, ENTR_GERUDOS_FORTRESS_OUTSIDE_GERUDO_TRAINING_GROUND },
                                                    { 0, ENTR_GERUDOS_FORTRESS_OUTSIDE_GERUDO_TRAINING_GROUND } };
                auto seed = Encode({ source, peer }, { 1, 0 });
                assert(SOH_SetCrossDungeonRuntimeSeed(mode == 2 ? "null" : seed.dump().c_str()));
                gSaveContext.entranceIndex = first ? warp.firstEntrance : warp.warp;
                gSaveContext.cutsceneIndex = first ? warp.firstCutscene : 0;
                gSaveContext.chamberCutsceneNum = warp.chamber;
                bool should = true;
                Invoke(VB_PLAY_BLUE_WARP_CS, &should);
                Invoke(VB_PLAY_TRANSITION_CS, &should);
                assert(!sEnteredBlueWarp && gSaveContext.cutsceneIndex == 0);
                assert(play.transitionTrigger == TRANS_TRIGGER_END && play.transitionMode == TRANS_MODE_SETUP &&
                       play.transitionType == TRANS_TYPE_FADE_WHITE && play.state.running);
                if (mode == 0) {
                    if (!ComboCrossOotRuntime::PrepareSwitch()) {
                        fprintf(stderr, "FAIL cold blue-warp 0x%04X (%s): pending=%d, peer not queued\n", warp.warp,
                                first ? "first" : "repeat", Combo_SOH_CrossDungeonTransitionPending());
                        return 1;
                    }
                    assert(queues == 1 && queued == 22);
                    assert(!ComboCrossOotRuntime::PrepareSwitch() && queues == 1);
                } else {
                    assert(!Combo_SOH_CrossDungeonTransitionPending() && !ComboCrossOotRuntime::PrepareSwitch());
                    assert(queues == 0 && queued == -1);
                    assert(gSaveContext.entranceIndex ==
                           (mode == 1 ? ENTR_GERUDOS_FORTRESS_OUTSIDE_GERUDO_TRAINING_GROUND : warp.warp));
                }
            }
        }
    }
    // The hook must not make an ordinary door's pending route ready merely
    // because Play_Init or another transition callback ran without a blue warp.
    auto seed = Encode({ Source(cases[0]), Dungeon{ "peer", { 1, 20 }, { 1, 21 }, { 1, 22 } } }, { 1, 0 });
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    queues = 0;
    storySkip = 0;
    Combo_SOH_CrossDungeonRoute(ENTR_DEKU_TREE_ENTRANCE, false);
    gSaveContext.entranceIndex = ENTR_MIDOS_HOUSE_0;
    bool should = true;
    Invoke(VB_PLAY_TRANSITION_CS, &should);
    assert(Combo_SOH_CrossDungeonTransitionPending() && !ComboCrossOotRuntime::PrepareSwitch() && queues == 0);
    puts("PASS cold OoT blue-warp registered hook: eight first/repeat aliases, foreign/local/native controls, "
         "single handoff and ordinary door fade gate");
}
