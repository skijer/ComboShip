#include "global.h"
#include "entrances/SoloForest.h"
#include "entrances/NativeForest.h"
#include "actors/SariaHexEffect.h"
#include "mm/2s2h/Enhancements/Graphics/ComboForestPortalEffect.h"
#include "mm/2s2h/Enhancements/Graphics/WoodsMysteryHexEffect.h"
#include "soh/soh/Enhancements/randomizer/NeiGiRender.h"
#if __has_include("actors/ForestGiParticles.h")
#include "actors/ForestGiParticles.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#ifdef MM_PASSAGE_TEST
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Graphics/WoodsMysteryMakeover.h"
#endif

PlayState* gPlayState;
SaveContext gSaveContext{};
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static bool visit = true, safe = true, nativePortal = false, titleAsset = true;
static bool authoredPortal = false;
static bool nativeMystery = true, soloAvailable = false, sForestTitle;
static char kForestTitle[] = "__OTR__custom/skiejer_forest/title";
static NeiGi::Mesh submitted;
static unsigned submissions, allocations, pushes, pops;
static Vec3f origin;
static float yaw;
static NeiGi::Basis basis;
static std::vector<std::string> events;
static Vtx legacyArena[1600];
bool ForestVisit(const PlayState* play) { return play && visit; }
bool ForestSafe(const PlayState* play) { return play && safe; }
bool NativePortal(const PlayState*) { return nativePortal; }
extern "C" u8 ResourceMgr_FileExists(const char* path) {
    if (!std::strcmp(path, NativeForest::kAuthoredPortalMarker))
        return authoredPortal;
    return titleAsset && !std::strcmp(path, "custom/skiejer_forest/title");
}
extern "C" u8 ResourceMgr_FileAltExists(const char*) { return 0; }
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return false; }
extern "C" int ComboSoloForestMm_PassageAvailable() { return soloAvailable; }
int ComboPlaceTitleCard_Show(PlayState*, void*, int width, int height) {
    assert(width == 144 && height == 24); events.emplace_back("title"); return 1;
}
namespace WoodsMysteryNativeBridge { bool Active(const PlayState* play) { return play && nativeMystery; } }
extern "C" NeiGi::Basis NeiGi_CameraBasis(PlayState*) { return basis; }
extern "C" void NeiGi_DrawWorldParticles(PlayState*, const NeiGi::Mesh& mesh) {
    submitted = mesh; ++submissions; events.emplace_back("mesh");
}
void* Allocate(size_t size) { assert(size <= sizeof(legacyArena)); ++allocations; return legacyArena; }
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(context, size) Allocate(size)
#define Graph_Alloc(context, size) Allocate(size)
#undef OPEN_DISPS
#define OPEN_DISPS(context) events.emplace_back("open")
#undef CLOSE_DISPS
#define CLOSE_DISPS(context) events.emplace_back("close")
#define Gfx_SetupDL25_Xlu(context) events.emplace_back("setup")
#define Gfx_SetupDL_25Xlu(context) events.emplace_back("setup")
#define Matrix_Push() (++pushes)
#define Matrix_Pop() (++pops)
#define Matrix_Translate(x,y,z,mode) (origin = {float(x),float(y),float(z)})
#define Matrix_Scale(...) ((void)0)
#define Matrix_RotateYS(angle,mode) (yaw = (angle) * (6.28318530718f / 65536.0f))
#define Matrix_RotateY(angle,mode) (yaw = (angle))
#undef MATRIX_FINALIZE_AND_LOAD
#define MATRIX_FINALIZE_AND_LOAD(...) ((void)0)
#undef gSPMatrix
#define gSPMatrix(...) ((void)0)
#undef gSPClearGeometryMode
#define gSPClearGeometryMode(...) ((void)0)
#undef gSPTexture
#define gSPTexture(...) ((void)0)
#undef gDPSetRenderMode
#define gDPSetRenderMode(...) ((void)0)
#undef gDPSetCombineMode
#define gDPSetCombineMode(...) ((void)0)
#undef gSPVertex
#define gSPVertex(...) ((void)0)
#undef gSP2Triangles
#define gSP2Triangles(...) ((void)0)
#undef gDPPipeSync
#define gDPPipeSync(...) ((void)0)
/* PRODUCTION */

void Reset() {
    submissions = allocations = pushes = pops = 0;
    submitted = {}; events.clear(); origin = {}; yaw = 0;
}
void CheckMesh(bool green) {
    assert(submissions == 1 && "world shimmer must submit the actual GI mesh path");
    assert(allocations == 0 && "native GI renderer owns guarded arena allocation");
    assert(pushes == 1 && pops == 1);
    assert(submitted.count > 0 && submitted.count <= submitted.vertices.size() && submitted.count % 3 == 0);
    for (size_t i = 0; i < submitted.count; ++i) {
        const auto& vertex = submitted.vertices[i];
        assert(std::isfinite(vertex.p.x) && std::isfinite(vertex.p.y) && std::isfinite(vertex.p.z));
        if (green) {
            const unsigned r = (vertex.rgb >> 16) & 255, g = (vertex.rgb >> 8) & 255, b = vertex.rgb & 255;
            assert(g >= r && g > b);
        } else {
            assert(vertex.rgb == 0x581880 || vertex.rgb == 0xFFFFFF);
        }
    }
}
bool EqualMesh(const NeiGi::Mesh& a, const NeiGi::Mesh& b) {
    if (a.count != b.count)
        return false;
    for (size_t i = 0; i < a.count; ++i) {
        const auto& x = a.vertices[i]; const auto& y = b.vertices[i];
        if (x.p.x != y.p.x || x.p.y != y.p.y || x.p.z != y.p.z || x.rgb != y.rgb || x.alpha != y.alpha)
            return false;
    }
    return true;
}
#ifdef OOT_SARIA_TEST
#define DrawShimmer(actor, play, seated) ComboForestActors_DrawSariaParticles(actor, play, seated)
#define PARTICLE_HEIGHT yDistToPlayer
#else
#define PARTICLE_HEIGHT playerHeightRel
#endif
int main() {
    PlayState play{}; GraphicsContext gfx{};
    play.state.gfxCtx = &gfx; gPlayState = &play;
    play.gameplayFrames = 75;
#if defined(MM_SARIA_TEST) || defined(OOT_SARIA_TEST)
    Actor actor{}; actor.world.pos = {500, 80, -400};
    #ifdef OOT_SARIA_TEST
    const s16 variants[] = {0x7F03, 0x7F13, 0x7F23, 0x7F33};
#else
    const s16 variants[] = {0x7F13, 0x7F23, 0x7F33, 0x7E13, 0x7E23, 0x7E33};
#endif
    for (s16 params : variants) {
        actor.params = params;
        Reset(); DrawShimmer(&actor, &play, ((params >> 4) & 15) == 3); CheckMesh(true);
        assert(origin.x == 500 && origin.y == 80 && origin.z == -400);
        auto first = submitted;
        Reset(); DrawShimmer(&actor, &play, ((params >> 4) & 15) == 3);
        assert(EqualMesh(first, submitted));
        ++play.gameplayFrames;
        Reset(); DrawShimmer(&actor, &play, ((params >> 4) & 15) == 3);
        assert(!EqualMesh(first, submitted));
        actor.world.pos = {-120, 12, 620};
        Reset(); DrawShimmer(&actor, &play, false);
        assert(origin.x == -120 && origin.y == 12 && origin.z == 620);
        actor.world.pos = {500, 80, -400};
    }
    for (int blocked = 0; blocked < 3; ++blocked) {
        play.pauseCtx.state = blocked == 0 ? 1 : 0;
        actor.xzDistToPlayer = blocked == 1 ? 701 : 0;
        actor.PARTICLE_HEIGHT = blocked == 2 ? 301 : 0;
        Reset(); DrawShimmer(&actor, &play, false); assert(!submissions && !pushes);
    }
    #ifdef OOT_SARIA_TEST
    actor.PARTICLE_HEIGHT = 0; gComboGameMode = COMBO_GAME_MM;
    Reset(); DrawShimmer(&actor, &play, false); assert(!submissions);
    puts("PASS production OoT Saria GI particles: four placed poses, authored anchor, animation, mode/pause/distance/height guards");
#else
    puts("PASS production MM Saria GI particles: six pose/reward variants, authored anchor, animation, pause/distance/height guards");
#endif
#elif defined(MM_PASSAGE_TEST)
    play.sceneId = SCENE_LOST_WOODS;
    ActorEntry spawn{}; play.linkActorEntry = &spawn;
    gSaveContext.gameMode = GAMEMODE_NORMAL; gSaveContext.fileNum = 0;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    Reset(); DrawPortalFinish(); CheckMesh(false);
    const Vec3f anchor = ComboOpeningForest_ReturnAnchor(&play);
    assert(origin.x == anchor.x && origin.y == anchor.y && origin.z == anchor.z);
    assert(EqualMesh(submitted, ForestGiParticles::Portal(play.gameplayFrames, false, basis)));
    RoomShape shape{};
    RoomShapeCullableEntry portalEntry{};
    portalEntry.opa = reinterpret_cast<Gfx*>(const_cast<char*>(NativeForest::kPortalRoot));
    shape.cullable.base.type = ROOM_SHAPE_TYPE_CULLABLE;
    shape.cullable.numEntries = 1;
    shape.cullable.entries = &portalEntry;
    play.roomCtx.curRoom.roomShape = &shape;
    assert(ComboOpeningForest_ReturnPackVersion(&play) == 9);
    Reset(); DrawPortalFinish(); CheckMesh(false);
    assert(EqualMesh(submitted, ForestGiParticles::Portal(play.gameplayFrames, false, basis)));
    authoredPortal = true;
    Reset(); DrawPortalFinish(); CheckMesh(false);
    assert(EqualMesh(submitted, ForestGiParticles::Portal(play.gameplayFrames, true, basis)));
    play.roomCtx.curRoom.roomShape = nullptr;
    Reset(); DrawWoodsMysteryHexes(); CheckMesh(true);
    assert(origin.x == 216.0f && origin.y == 0.0f && origin.z == -216.0f);
    assert(EqualMesh(submitted, ForestGiParticles::Mystery(play.gameplayFrames, basis)));
    play.pauseCtx.state = 1;
    Reset(); DrawPortalFinish(); DrawWoodsMysteryHexes(); assert(!submissions);
    play.pauseCtx.state = 0; play.csCtx.state = CS_STATE_RUN;
    Reset(); DrawPortalFinish(); DrawWoodsMysteryHexes(); assert(!submissions);
    play.csCtx.state = CS_STATE_IDLE; gComboGameMode = COMBO_GAME_MM;
    Reset(); DrawWoodsMysteryHexes(); assert(!submissions);
    soloAvailable = true;
    Reset(); DrawWoodsMysteryHexes(); CheckMesh(true);
    gComboGameMode = COMBO_GAME_BOTH; soloAvailable = false; nativeMystery = false;
    Reset(); DrawWoodsMysteryHexes(); assert(!submissions);
    puts("PASS both production bridges: exact NEI entrance-face meshes, legacy/authored sizes, pause/cutscene and solo/combined ownership guards");
#else
    Reset(); DrawForestPortal(); CheckMesh(false);
    assert(origin.x == COMBO_SOLO_FOREST_STUMP_X && origin.z == COMBO_SOLO_FOREST_STUMP_Z);
    assert(std::fabs(yaw - COMBO_SOLO_FOREST_STUMP_YAW * (6.28318530718f / 65536)) < .001f);
    nativePortal = true; Reset(); DrawForestPortal(); CheckMesh(false);
    for (size_t i = 0; i < submitted.count; ++i)
        assert(std::fabs(submitted.vertices[i].p.x) < 49 && submitted.vertices[i].p.y < 114);
    authoredPortal = true; Reset(); DrawForestPortal(); CheckMesh(false);
    assert(EqualMesh(submitted, ForestGiParticles::Portal(play.gameplayFrames, true, basis)));
    for (int blocked = 0; blocked < 4; ++blocked) {
        visit = blocked != 0; safe = blocked != 1;
        play.roomCtx.curRoom.num = blocked == 2 ? 1 : 0;
        gPlayState = blocked == 3 ? nullptr : &play;
        Reset(); DrawForestPortal(); assert(!submissions && !pushes);
    }
    gPlayState = &play; visit = safe = true; play.roomCtx.curRoom.num = 0; sForestTitle = true;
    Reset(); DrawForestPortal(); assert(!sForestTitle);
    puts("PASS production solo forest GI particles: both doorway sizes, unchanged anchor/yaw, safety guards and local title");
#endif
}
