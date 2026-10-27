#include "soh/Enhancements/Gameplay/ComboForestActors.h"
#include "entrances/SoloForest.h"
#include "entrances/ForestActorPlacements.h"
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
using s16 = short;
using s32 = int;
struct Vec3f {
    float x, y, z;
};
struct Color_RGBA8 {
    unsigned char r, g, b, a;
};
struct Actor {
    int id = 0, params = 0, room = 0;
    void* update = (void*)1;
    Actor* next = nullptr;
    struct {
        Vec3f pos;
    } world = {};
};
struct Player {
    Actor actor;
    Actor* talkActor = nullptr;
};
struct CollisionPoly {};
struct PlayState {
    int sceneNum = COMBO_SOLO_FOREST_OOT_SCENE;
    struct {
        struct {
            int num = 0;
            void* segment = (void*)1;
        } curRoom;
        int status = 0;
    } roomCtx;
    struct {
        bool running = true;
    } state;
    int transitionTrigger = 0, transitionMode = 0;
    struct {
        int state = 0;
    } pauseCtx, gameOverCtx, csCtx;
    struct {
        struct List {
            Actor* head = nullptr;
        } actorLists[12];
    } actorCtx;
    int colCtx = 0, gameplayFrames = 1;
};
struct {
    int gameMode = 0, fileNum = 0, sceneLayer = 0, cutsceneIndex = 0, cutsceneTrigger = 0;
} gSaveContext;
int gComboGameMode = COMBO_GAME_OOT;
constexpr int TRANS_TRIGGER_OFF = 0, TRANS_MODE_OFF = 0, GAMEOVER_INACTIVE = 0, CS_STATE_IDLE = 0;
constexpr int ACTOR_EN_HORSE = 0x14;
constexpr float BGCHECK_Y_MIN = -32000;
Player player;
#define GET_PLAYER(play) (&::player)
bool cutscene = false, floorPresent = true;
static bool Player_InCsMode(PlayState*) {
    return cutscene;
}
PlayState play;
PlayState* gPlayState = &play;
ComboForestActors::Visit sVisit;
std::vector<Actor> spawned;
static float BgCheck_EntityRaycastFloor5(PlayState* p, int*, CollisionPoly** floor, int*, Actor*, Vec3f* probe) {
    assert(probe->y == (p->roomCtx.curRoom.num == 0 ? 120.0f : 500.0f));
    static CollisionPoly poly;
    *floor = floorPresent ? &poly : nullptr;
    return 12;
}
static Actor* Actor_Spawn(decltype(play.actorCtx)*, PlayState*, int id, float x, float y, float z, int, s16, int,
                          int params) {
    spawned.push_back({});
    auto* a = &spawned.back();
    a->id = id;
    a->params = params;
    a->world.pos = { x, y, z };
    return a;
}
struct CustomMessage {
    static inline std::string text;
    CustomMessage(const char* message) {
        text = message;
    }
    void AutoFormat() {
    }
    void LoadIntoFont() {
    }
};
/* PRODUCTION_ROUTING */
int main() {
    spawned.reserve(20);
    play.roomCtx.status = 1;
    Update();
    assert(spawned.empty() && sVisit.room == -1);
    play.roomCtx.status = 0;
    play.roomCtx.curRoom.segment = nullptr;
    Update();
    assert(spawned.empty());
    play.roomCtx.curRoom.segment = (void*)1;
    cutscene = true;
    Update();
    assert(spawned.empty());
    cutscene = false;
    floorPresent = false;
    Update();
    assert(spawned.empty() && sVisit.complete == 0);
    floorPresent = true;
    Update();
    assert(spawned.size() == 2);
    assert(spawned[0].params == 0x7F13 && spawned[0].world.pos.y == 12);
    // October 22's authored-rim follow-up moves both flanks from 230 to 120.
    // Keep independent world/local expectations for the shared native placement.
    assert(std::fabs(spawned[0].world.pos.x - (-630.056f)) < .01f);
    assert(std::fabs(spawned[0].world.pos.z - (-156.212f)) < .01f);
    const float angle = 12288 * (6.28318530718f / 65536.0f);
    const float dx = spawned[0].world.pos.x + 695.0f, dz = spawned[0].world.pos.z + 313.0f;
    assert(std::fabs(dx * std::cos(angle) - dz * std::sin(angle) - (-120.0f)) < .01f);
    assert(std::fabs(dx * std::sin(angle) + dz * std::cos(angle) - 120.0f) < .01f);
    assert(!ComboSoloForest_AtStump(spawned[0].world.pos.x, spawned[0].world.pos.y, spawned[0].world.pos.z));
    assert(std::hypot(dx, dz) > 68.0f + 35.0f); // Authored rim plus horse-footprint clearance.
    assert(spawned[1].params == 0x7E40 && spawned[1].world.pos.y == 12);
    assert(std::fabs((spawned[1].world.pos.x + 695.0f) * std::cos(angle) -
                     (spawned[1].world.pos.z + 313.0f) * std::sin(angle) - 120.0f) < .01f);
    assert(!ComboSoloForest_AtStump(spawned[1].world.pos.x, spawned[1].world.pos.y, spawned[1].world.pos.z));
    assert(std::hypot(spawned[1].world.pos.x + 695.0f, spawned[1].world.pos.z + 313.0f) > 68.0f + 35.0f);
    Update();
    assert(spawned.size() == 2);
    play.roomCtx.curRoom.num = 1;
    Update();
    assert(spawned.size() == 3);
    assert(spawned[2].params == 0x7E08 && spawned[2].world.pos.x == 2302 && spawned[2].world.pos.z == -185 &&
           spawned[2].world.pos.y == 47);
    sVisit.Reset();
    play.roomCtx.curRoom.num = 0;
    Actor authored;
    authored.id = 0x2A;
    authored.params = 0x7F13;
    authored.world.pos = { 777, 88, 999 };
    play.actorCtx.actorLists[0].head = &authored;
    Update();
    assert(spawned.size() == 4 && authored.world.pos.x == 777 && authored.world.pos.y == 88);
    bool fromTable = true;
    uint16_t textId = 0x1001;
    player.talkActor = &authored;
    OpenText(&textId, &fromTable);
    assert(!fromTable && CustomMessage::text.find("bridge") != std::string::npos);
    fromTable = true;
    play.sceneNum = 1;
    OpenText(&textId, &fromTable);
    assert(fromTable);
    Update();
    assert(sVisit.room == -1);
    play.sceneNum = COMBO_SOLO_FOREST_OOT_SCENE;
    for (int mode : { COMBO_GAME_BOTH, COMBO_GAME_MM }) {
        gComboGameMode = mode;
        Update();
        assert(spawned.size() == 4);
    }
    gComboGameMode = COMBO_GAME_OOT;
    gSaveContext.fileNum = 255;
    Update();
    assert(spawned.size() == 4);
}
