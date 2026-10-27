#include "global.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "entrances/BlackMarketEntrance.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#ifndef COUNTER_BARRIER_DISABLED
#include "mm/2s2h/Enhancements/Gameplay/BlackMarketCounterBarrier.h"
static BlackMarketCounterBarrier::Fixture sCounterBarrier;
/* PRODUCTION BARRIER GUARD */
#else
extern "C" int ComboBlackMarket_IsCounterBarrier(const CollisionContext*, int) { return false; }
#endif

// Native internal entry point: the same dynamic wall resolver used by MM's
// entity sphere-vs-wall path. Collision math and surface lookups remain real.
extern "C" s32 BgCheck_SphVsDynaWall(CollisionContext*, u16, f32*, f32*, Vec3f*, f32,
                                     CollisionPoly**, s32*, Actor*);
static std::vector<void*> allocations;
static int talkQueues;
static bool climbCheat;
static s32 climbableSlateBg = -1;
extern "C" {
uintptr_t gSegments[16]{};
u8 gMogmaMittsClimbActive = 0;
u8 Slate_IsClimbableBgId(s32 bg) { return bg == climbableSlateBg; }
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) {
    return behavior == VB_BE_CLIMBABLE_SURFACE ? climbCheat : result;
}
bool Player_InCsMode(PlayState*) { return false; }
void CutsceneManager_Queue(s16 id) { assert(id == CS_ID_GLOBAL_TALK); ++talkQueues; }
void* THA_AllocTailAlign(TwoHeadArena*, size_t size, uintptr_t) {
    void* result = std::calloc(1, size);
    assert(result);
    allocations.push_back(result);
    return result;
}
void* THA_AllocTailAlign16(TwoHeadArena* arena, size_t size) { return THA_AllocTailAlign(arena, size, 0); }
void Fault_AddHungupAndCrash(const char* file, s32 line) {
    std::fprintf(stderr, "Unexpected native collision fault: %s:%d\n", file, line);
    std::abort();
}
void Fault_AddHungupAndCrashImpl(const char* first, const char* second) {
    std::fprintf(stderr, "Unexpected native collision fault: %s %s\n", first, second);
    std::abort();
}
}

static u32 Read32(FILE* file) {
    u8 bytes[4];
    assert(std::fread(bytes, 1, 4, file) == 4);
    return bytes[0] | (u32(bytes[1]) << 8) | (u32(bytes[2]) << 16) | (u32(bytes[3]) << 24);
}
struct NativeRoom {
    CollisionHeader header{};
    std::vector<Vec3s> vertices;
    std::vector<CollisionPoly> polygons;
    std::vector<SurfaceType> surfaces;
    explicit NativeRoom(const char* path) {
        FILE* file = std::fopen(path, "rb");
        assert(file && std::fseek(file, 64, SEEK_SET) == 0);
        assert(std::fread(&header.minBounds, sizeof(Vec3s), 1, file) == 1);
        assert(std::fread(&header.maxBounds, sizeof(Vec3s), 1, file) == 1);
        header.numVertices = Read32(file);
        vertices.resize(header.numVertices);
        assert(std::fread(vertices.data(), sizeof(Vec3s), vertices.size(), file) == vertices.size());
        header.numPolygons = Read32(file);
        polygons.resize(header.numPolygons);
        assert(std::fread(polygons.data(), sizeof(CollisionPoly), polygons.size(), file) == polygons.size());
        surfaces.resize(Read32(file));
        assert(std::fread(surfaces.data(), sizeof(SurfaceType), surfaces.size(), file) == surfaces.size());
        std::fclose(file);
        header.vtxList = vertices.data();
        header.polyList = polygons.data();
        header.surfaceTypeList = surfaces.data();
    }
};

struct Hit {
    bool hit;
    Vec3f point{};
    CollisionPoly* poly = nullptr;
    s32 bg = BGCHECK_SCENE;
};
static Hit Line(PlayState& play, Vec3f from, Vec3f to, bool camera = false) {
    Hit result{};
    result.bg = BGCHECK_SCENE;
    if (camera)
        result.hit = BgCheck_CameraLineTest1(&play.colCtx, &from, &to, &result.point, &result.poly,
                                           true, true, true, true, &result.bg);
    else
        result.hit = BgCheck_EntityLineTest1(&play.colCtx, &from, &to, &result.point, &result.poly,
                                           true, true, true, true, &result.bg);
    return result;
}
static float Floor(PlayState& play, Vec3f point) {
    CollisionPoly* poly = nullptr;
    s32 bg = BGCHECK_SCENE;
    const float y = BgCheck_EntityRaycastFloor3(&play.colCtx, &poly, &bg, &point);
    assert(poly && bg == BGCHECK_SCENE && "barrier must not become a new floor/ledge");
    return y;
}
static void ClearArena(PlayState& play) {
    for (void* allocation : allocations) std::free(allocation);
    allocations.clear();
    play.colCtx = {};
}
static void InitRoom(PlayState& play, NativeRoom& room) {
    play = {};
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    play.roomCtx.curRoom.num = 0;
    BgCheck_Allocate(&play.colCtx, &play, &room.header);
}
static int ActiveActors(const PlayState& play) {
    int result = 0;
    for (auto flags : play.colCtx.dyna.bgActorFlags)
        result += (flags & BGACTOR_IN_USE) && !(flags & BGACTOR_1);
    return result;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    NativeRoom room(argv[1]);
    const auto originalVertices = room.vertices;
    const auto originalPolygons = room.polygons;
    static PlayState play;
    InitRoom(play, room);
    const auto cameraBefore = Line(play, { 0, 70, 180 }, { 0, 70, 90 }, true);
    assert(!cameraBefore.hit);
    assert(!Line(play, { 0, 70, 180 }, { 0, 70, 90 }).hit && "negative control starts above native counter");
    const std::array<Vec3f, 6> floorPoints{{ { 0, 20, 150 }, { 0, 20, 170 }, { 0, 20, 250 },
                                          { -11, 20, 381 }, { -100, 20, 200 }, { 100, 20, 200 } }};
    std::array<float, 6> floorBefore{};
    for (size_t i = 0; i < floorPoints.size(); ++i) floorBefore[i] = Floor(play, floorPoints[i]);
#ifndef COUNTER_BARRIER_DISABLED
    auto& barrier = sCounterBarrier;
    assert(!barrier.Ensure(nullptr));
    const auto arenaAllocations = allocations.size();
    const auto bgActorCount = play.actorCtx.actorLists[ACTORCAT_BG].length;
    assert(barrier.Ensure(&play));
    assert(allocations.size() == arenaAllocations && play.actorCtx.actorLists[ACTORCAT_BG].length == bgActorCount);
#endif
    DynaPoly_UpdateContext(&play, &play.colCtx.dyna);
    // Fails on the actual unchanged room when --without-barrier is selected.
    const auto front = Line(play, { 0, 70, 180 }, { 0, 70, 90 });
    assert(front.hit && front.bg != BGCHECK_SCENE && "Link must be stopped above the counter top");
#ifndef COUNTER_BARRIER_DISABLED
    assert(front.bg == barrier.OwnedBgId(&play));
    assert(std::abs(front.point.z - 130) < .01f);
    assert(ActiveActors(play) == 1 && barrier.Ensure(&play) && ActiveActors(play) == 1);
    barrier.Reset();
    assert(barrier.Ensure(&play) && ActiveActors(play) == 1);
    const auto& bg = play.colCtx.dyna.bgActors[front.bg];
    const auto* header = bg.colHeader;
    assert(bg.actor && bg.actor->update && !bg.actor->draw && bg.actor->category == ACTORCAT_BG);
    assert(bg.actor->scale.x == 1 && bg.actor->scale.y == 1 && bg.actor->scale.z == 1);
    assert(header && header->numVertices == 4 && header->numPolygons == 2);
    assert(bg.dynaLookup.floor.head == SS_NULL && bg.dynaLookup.ceiling.head == SS_NULL);
    assert(bg.dynaLookup.wall.head != SS_NULL);
    // Both lateral ends overlap the room's actual walls; every ordinary player
    // height is blocked. The ceiling/floor already close the vertical ends.
    for (float x : { -155.f, -80.f, 0.f, 80.f, 155.f }) {
        for (float y : { 5.f, 40.f, 70.f, 110.f, 140.f }) {
            const auto hit = Line(play, { x, y, 145 }, { x, y, 125 });
            assert(hit.hit && hit.bg == front.bg && std::abs(hit.point.z - 130) < .01f);
        }
    }
    for (float side : { -200.f, 200.f }) {
        const auto hit = Line(play, { 0, 60, 135 }, { side, 60, 135 });
        assert(hit.hit && hit.bg == BGCHECK_SCENE && std::abs(hit.point.x) < 170);
    }
    const auto roof = Line(play, { 0, 60, 135 }, { 0, 330, 135 });
    assert(roof.hit && roof.bg == BGCHECK_SCENE && roof.point.y < 300);
    for (int i = 0; i < header->numPolygons; ++i) {
        auto* poly = &header->polyList[i];
        assert(poly->normal.y == 0 && poly->normal.z > 0);
        assert(SurfaceType_GetWallFlags(&play.colCtx, poly, front.bg) == WALL_FLAG_0);
        assert(SurfaceType_CheckWallFlag0(&play.colCtx, poly, front.bg));
        assert(!SurfaceType_CheckWallFlag1(&play.colCtx, poly, front.bg));
        assert(!SurfaceType_CheckWallFlag2(&play.colCtx, poly, front.bg));
        assert(SurfaceType_GetBgCamIndex(&play.colCtx, poly, front.bg) == 0);
        assert(BgCheck_GetBgCamSettingImpl(&play.colCtx, 0, front.bg) == CAM_SET_NONE);
        const auto ordinary = Line(play, { 0, 60, 135 }, { 200, 60, 135 });
        assert(ordinary.hit && ordinary.bg == BGCHECK_SCENE);
        for (int source = 0; source < 3; ++source) {
            gMogmaMittsClimbActive = source == 0;
            climbCheat = source == 1;
            climbableSlateBg = source == 2 ? front.bg : -1;
            assert(!(SurfaceType_GetWallFlags(&play.colCtx, poly, front.bg) & WALL_FLAG_3) &&
                   "climbing powers must not turn the invisible counter barrier into a ladder");
            if (source == 2) climbableSlateBg = BGCHECK_SCENE;
            assert(SurfaceType_GetWallFlags(&play.colCtx, ordinary.poly, ordinary.bg) & WALL_FLAG_3);
        }
        gMogmaMittsClimbActive = 0;
        climbCheat = false;
        climbableSlateBg = -1;
    }
    // Probe the real raised counter ledge the player could previously mount.
    // Its floor stays at Y41, but the native forward probe at ledge + 5 now
    // meets our tall wall and its native WALL_FLAG_0 ledge rejection surface.
    Vec3f ledgePoint{ 0, 100, 120 };
    CollisionPoly* ledgeFloor = nullptr;
    s32 ledgeBg = BGCHECK_SCENE;
    const float ledgeY = BgCheck_EntityRaycastFloor5(&play.colCtx, &ledgeFloor, &ledgeBg, nullptr, &ledgePoint);
    assert(ledgeFloor && ledgeBg == BGCHECK_SCENE && std::abs(ledgeY - 41) < .01f);
    const auto ledgeWall = Line(play, { 0, ledgeY + 5, 150 }, { 0, ledgeY + 5, 120 });
    assert(ledgeWall.hit && ledgeWall.bg == front.bg &&
           SurfaceType_CheckWallFlag0(&play.colCtx, ledgeWall.poly, ledgeWall.bg));
    const auto cameraAfter = Line(play, { 0, 70, 180 }, { 0, 70, 90 }, true);
    assert(cameraAfter.hit == cameraBefore.hit && cameraAfter.bg == cameraBefore.bg);
    assert(!Line(play, { -11, 10, 381 }, { 0, 10, 150 }).hit && "customer approach stays open");
    assert(!Line(play, { -40, 10, 200 }, { 40, 10, 200 }).hit && "customer aisle stays open");
    assert(!Line(play, { -11, 10, 381 }, { -11, 10, 401 }).hit && "exit trigger remains reachable");
    for (size_t i = 0; i < floorPoints.size(); ++i) assert(Floor(play, floorPoints[i]) == floorBefore[i]);
    // The actual native wall resolver stops human/Goron-size spheres at the
    // customer side. The real talk-offer path then accepts through that wall;
    // it does not impose a line-of-sight rejection.
    static Player player;
    Actor keeper{};
    keeper.world.pos = { 0, 1, 86 };
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    for (float radius : { 10.f, 20.f, 40.f }) {
        Vec3f desired{ 0, 30, 131 };
        float x = desired.x, z = desired.z;
        CollisionPoly* wall = nullptr;
        s32 wallBg = BGCHECK_SCENE;
        assert(BgCheck_SphVsDynaWall(&play.colCtx, COLPOLY_IGNORE_ENTITY, &x, &z, &desired, radius,
                                   &wall, &wallBg, &player.actor));
        assert(wallBg == front.bg && std::abs(x) < .01f && std::abs(z - (130 + radius)) < .01f);
        player.actor.world.pos = { x, floorBefore[0], z };
        player.talkActor = nullptr;
        player.talkActorDistance = 1e6f;
        keeper.xzDistToPlayer = Math_Vec3f_DistXZ(&keeper.world.pos, &player.actor.world.pos);
        keeper.playerHeightRel = player.actor.world.pos.y - keeper.world.pos.y;
        assert(keeper.xzDistToPlayer < 100 && Actor_OfferTalk(&keeper, &play, 100));
        assert(player.talkActor == &keeper);
    }
    assert(talkQueues == 3);
    assert(std::memcmp(originalVertices.data(), room.vertices.data(), room.vertices.size() * sizeof(Vec3s)) == 0);
    assert(std::memcmp(originalPolygons.data(), room.polygons.data(), room.polygons.size() * sizeof(CollisionPoly)) == 0);
    // Leaving room zero releases only the owned native entry; return registers
    // one new entry, and reset after actual arena teardown touches no dead data.
    play.roomCtx.curRoom.num = 1;
    assert(!barrier.Ensure(&play) && barrier.OwnedBgId(&play) == -1);
    DynaPoly_UpdateContext(&play, &play.colCtx.dyna);
    assert(ActiveActors(play) == 0);
    play.roomCtx.curRoom.num = 0;
    assert(barrier.Ensure(&play));
    ClearArena(play);
    barrier.Reset();
    InitRoom(play, room);
    assert(barrier.Ensure(&play) && ActiveActors(play) == 1);
    play.sceneId = SCENE_00KEIKOKU;
    assert(!barrier.Ensure(&play));
    DynaPoly_UpdateContext(&play, &play.colCtx.dyna);
    assert(ActiveActors(play) == 0);
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    for (auto& flags : play.colCtx.dyna.bgActorFlags) flags = BGACTOR_IN_USE;
    assert(!barrier.Ensure(&play) && "full actor slots must fail safely");
    for (auto& flags : play.colCtx.dyna.bgActorFlags) flags = 0;
    auto& dyna = play.colCtx.dyna;
    const auto polyMax = dyna.polyListMax, vertexMax = dyna.vtxListMax, nodeMax = dyna.polyNodes.maxNodes;
    dyna.polyListMax = 1;
    assert(!barrier.Ensure(&play));
    dyna.polyListMax = polyMax;
    dyna.vtxListMax = 3;
    assert(!barrier.Ensure(&play));
    dyna.vtxListMax = vertexMax;
    dyna.polyNodes.maxNodes = 1;
    assert(!barrier.Ensure(&play));
    dyna.polyNodes.maxNodes = nodeMax;
    auto* nodes = dyna.polyNodes.tbl;
    dyna.polyNodes.tbl = nullptr;
    assert(!barrier.Ensure(&play));
    dyna.polyNodes.tbl = nodes;
    // Account for another actor before its polygons have been indexed. A free
    // actor slot alone is insufficient when that registration fills the pools.
    DynaPolyActor other{};
    DynaPolyActor_Init(&other, 0);
    other.actor.scale = { 1, 1, 1 };
    other.actor.update = [](Actor*, PlayState*) {};
    CollisionHeader otherMesh{};
    otherMesh.numPolygons = polyMax;
    otherMesh.numVertices = vertexMax;
    other.bgId = DynaPoly_SetBgActor(&play, &dyna, &other.actor, &otherMesh);
    assert(other.bgId >= 0 && other.bgId < BG_ACTOR_MAX);
    assert(!barrier.Ensure(&play) && ActiveActors(play) == 1);
    DynaPoly_DeleteBgActor(&play, &dyna, other.bgId);
    DynaPoly_UpdateContext(&play, &dyna);
    assert(barrier.Ensure(&play));
    DynaPoly_UpdateContext(&play, &dyna);
    assert(Line(play, { 0, 70, 180 }, { 0, 70, 90 }).bg == barrier.OwnedBgId(&play));
    ClearArena(play);
    barrier.Reset();
    assert(!barrier.Ensure(&play));
    std::puts("PASS MM counter barrier: native room/Dyna collision and wall displacement; no floor or climb surface; camera pass-through; approach, aisle, exit and talk at all tested radii; room/scene guards, teardown/reentry and capacity failure under ASan/UBSan");
#endif
}
