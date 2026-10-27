#include "global.h"
#include "ComboGameMode.h"
#include "entrances/LostWoodsEntrance.h"
#include "2s2h/Enhancements/Gameplay/ComboWoodfallPassage.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

SaveContext gSaveContext{};
PlayState* gPlayState;
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static CollisionPoly floorPoly{}, wallPoly{};
static bool hasFloor = true, hasWall = true, steepFloor, exitFloor;
static bool dynamicFloor, submerged;
static float heading;
static int queries;
struct TestPoly {
    Vec3f vertices[3];
    Vec3s normal;
    unsigned ignoreEntity, exit;
};
// Native entrance wall and apron, translated relative to spawn zero. The
// optional --mm-archive check substitutes the complete native collision mesh.
static const TestPoly geometry[] = {
    /* NATIVE_GEOMETRY */
    {{{-165, 80, 40}, {-109, 80, -83}, {-210, 80, -177}}, {0, 32767, 0}, 0, 0},
    {{{-109, 80, -83}, {-210, 80, -177}, {-210, 476, -177}}, {-22324, 0, 23986}, 0, 0},
    {{{-109, 80, -83}, {-210, 476, -177}, {-109, 476, -83}}, {-22324, 0, 23986}, 0, 0},
    /* END_NATIVE_GEOMETRY */
};
static Vec3f Difference(const Vec3f& a, const Vec3f& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static float Dot(const Vec3f& a, const Vec3f& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vec3f Cross(const Vec3f& a, const Vec3f& b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
static bool Inside(const TestPoly& poly, const Vec3f& point) {
    const auto u = Difference(poly.vertices[1], poly.vertices[0]);
    const auto v = Difference(poly.vertices[2], poly.vertices[0]);
    const auto w = Difference(point, poly.vertices[0]);
    const float uu=Dot(u,u), uv=Dot(u,v), vv=Dot(v,v), wu=Dot(w,u), wv=Dot(w,v);
    const float denominator=uu*vv-uv*uv;
    if (std::fabs(denominator) < 0.01f) return false;
    const float s=(vv*wu-uv*wv)/denominator, t=(uu*wv-uv*wu)/denominator;
    return s >= -0.00001f && t >= -0.00001f && s+t <= 1.00001f;
}
static Vec3f Local(const Vec3f& p) {
    const float x = p.x - 1000, z = p.z - 2000;
    return {x * std::cos(heading) - z * std::sin(heading), p.y,
            x * std::sin(heading) + z * std::cos(heading)};
}
static Vec3f World(float x, float y, float z) {
    return {1000 + x * std::cos(heading) + z * std::sin(heading), y,
            2000 - x * std::sin(heading) + z * std::cos(heading)};
}
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState*, CollisionContext*, CollisionPoly** poly, s32* bg,
                                             Actor*, Vec3f* position) {
    ++queries; *bg = dynamicFloor ? 0 : BGCHECK_SCENE;
    const auto local = Local(*position);
    *poly = nullptr;
    float best = BGCHECK_Y_MIN;
    if (!hasFloor) return best;
    for (unsigned i=0; i<sizeof(geometry)/sizeof(*geometry); ++i) {
        const auto& candidate=geometry[i];
        if (candidate.ignoreEntity || candidate.normal.y <= 0) continue;
        const auto n=Cross(Difference(candidate.vertices[1],candidate.vertices[0]),
                           Difference(candidate.vertices[2],candidate.vertices[0]));
        if (std::fabs(n.y) < 0.001f) continue;
        Vec3f point=local;
        point.y=candidate.vertices[0].y-(n.x*(point.x-candidate.vertices[0].x)+
                                       n.z*(point.z-candidate.vertices[0].z))/n.y;
        if (point.y <= local.y && point.y > best && Inside(candidate,point)) {
            best=point.y; floorPoly.normal=candidate.normal; floorPoly.type=i; *poly=&floorPoly;
        }
    }
    if (steepFloor) floorPoly.normal.y=5000;
    return best;
}
extern "C" u32 SurfaceType_GetSceneExitIndex(CollisionContext*, CollisionPoly* poly, s32) {
    return exitFloor ? 1 : geometry[poly->type].exit;
}
extern "C" s32 WaterBox_GetSurface1_2(PlayState*, CollisionContext*, f32, f32, f32* height, WaterBox**) {
    *height = submerged ? 90 : 69; return true; // native basin is eleven units below the apron
}
extern "C" s32 BgCheck_EntityLineTest1(CollisionContext*, Vec3f* from, Vec3f* to, Vec3f* result,
                                        CollisionPoly** poly, s32, s32, s32, s32, s32* bg) {
    ++queries; if (!hasWall) return false;
    const auto a = Local(*from), b = Local(*to);
    const auto direction=Difference(b,a);
    float nearest=2;
    *poly=nullptr;
    for (const auto& candidate : geometry) {
        if (candidate.ignoreEntity || std::abs(candidate.normal.y) > 4096) continue;
        const auto n=Cross(Difference(candidate.vertices[1],candidate.vertices[0]),
                           Difference(candidate.vertices[2],candidate.vertices[0]));
        const float denominator=Dot(n,direction);
        if (std::fabs(denominator)<0.001f) continue;
        const float t=Dot(n,Difference(candidate.vertices[0],a))/denominator;
        Vec3f hit{a.x+t*direction.x,a.y+t*direction.y,a.z+t*direction.z};
        if (t>=0 && t<=1 && t<nearest && Inside(candidate,hit)) {
            nearest=t; *result=World(hit.x,hit.y,hit.z);
            wallPoly.normal={static_cast<s16>(std::lround(candidate.normal.x*std::cos(heading)+candidate.normal.z*std::sin(heading))),
                             candidate.normal.y,
                             static_cast<s16>(std::lround(-candidate.normal.x*std::sin(heading)+candidate.normal.z*std::cos(heading)))};
            *poly=&wallPoly;
        }
    }
    *bg=BGCHECK_SCENE; return *poly != nullptr;
}

/* ENDPOINT_PRODUCTION */

int main() {
    PlayState play{}; Player player{}; CollisionHeader collision{};
    gPlayState = &play; play.sceneId = SCENE_21MITURINMAE; play.roomCtx.curRoom.num = 0;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.colCtx.colHeader = &collision;
    gSaveContext.gameMode = GAMEMODE_NORMAL; gSaveContext.fileNum = 0;
    ActorEntry entries[2]{}; entries[1].pos = {1000, 80, 2000}; entries[1].rot.y = 0;
    EntranceEntry entrance{}; entrance.spawn = 1; entrance.room = 0; play.setupEntranceList = &entrance;
    const auto original = entries[1]; const auto save = gSaveContext;
    ComboWoodfallPassage_ReadSpawns(&play, entries, 2);
    Vec3f anchor{}; s16 yaw = 0;
    assert(ComboWoodfallPassage_Get(&play, &anchor, &yaw));
    assert(std::fabs(anchor.x-839.1374f)<0.1f && anchor.y == 80 && std::fabs(anchor.z-1871.4640f)<0.1f);
    assert(std::abs(yaw - -7818) <= 1);
    assert(!std::memcmp(&original, &entries[1], sizeof(original)) &&
           !std::memcmp(&save, &gSaveContext, sizeof(save)));
    Vec3f inside{anchor.x + std::sin(yaw*6.28318530718f/65536)*26, anchor.y,
                 anchor.z + std::cos(yaw*6.28318530718f/65536)*26};
    assert(ComboWoodfallPassage_AtReturn(&play, &inside));
    for (int distance=-180; distance<=180; ++distance) {
        const Vec3f ordinary{1000,80,2000+static_cast<float>(distance)};
        assert(!ComboWoodfallPassage_AtReturn(&play,&ordinary));
    }
    inside.y += 200; assert(!ComboWoodfallPassage_AtReturn(&play, &inside));
    const int cached = queries; ComboWoodfallPassage_Get(&play, &anchor, &yaw); assert(queries == cached);
    play.roomCtx.curRoom.num = 1; assert(!ComboWoodfallPassage_Get(&play, &anchor, &yaw));
    play.roomCtx.curRoom.num = 0;
    for (const int degrees : {90, 180, 270}) {
        heading = degrees * (3.14159265359f / 180.0f);
        entries[1].rot.y = static_cast<s16>((degrees << 7) | 31); // native yaw plus cutscene ID
        ComboWoodfallPassage_ReadSpawns(&play, entries, 2);
        assert(ComboWoodfallPassage_Get(&play, &anchor, &yaw));
        const auto local = Local(anchor);
        assert(std::fabs(local.x+160.8626f)<0.1f && std::fabs(local.z+128.5360f)<0.1f);
        assert(std::abs(static_cast<s16>(yaw - (DEG_TO_BINANG(degrees) - 7818))) <= 1);
    }
    heading = 0; entries[1].rot.y = 0;
    for (int failure = 0; failure < 6; ++failure) {
        ComboWoodfallPassage_ReadSpawns(&play, entries, 2);
        hasWall = failure != 0; hasFloor = failure != 1; steepFloor = failure == 2; exitFloor = failure == 3;
        dynamicFloor = failure == 4; submerged = failure == 5;
        assert(!ComboWoodfallPassage_Get(&play, &anchor, &yaw));
    }
    hasWall = hasFloor = true; steepFloor = exitFloor = dynamicFloor = submerged = false;
    ComboWoodfallPassage_ReadSpawns(&play, entries, 1); assert(!ComboWoodfallPassage_Get(&play, &anchor, &yaw));
    ComboWoodfallPassage_ReadSpawns(&play, entries, 2); ComboWoodfallPassage_Reset();
    assert(!ComboWoodfallPassage_Get(&play, &anchor, &yaw));
    for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
        gComboGameMode = mode; ComboWoodfallPassage_ReadSpawns(&play, entries, 2);
        assert(!ComboWoodfallPassage_Get(&play, &anchor, &yaw));
    }
    puts("PASS Woodfall wall passage: native entrance anchor, dry approach, no bounce, no resource/save mutation, guards");
}
