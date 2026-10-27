#include "global.h"
#include "entrances/SoloForest.h"
#include "entrances/NativeForest.h"
#include "actors/ForestGiParticles.h"
#include "soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include <cassert>
#include <cstdio>
#include <cstring>

PlayState* gPlayState;
static bool visit = true, safe = true, titleAssetAvailable = true, sForestTitle;
static char kForestTitle[] = "__OTR__custom/skiejer_forest/title";
static NeiGi::Mesh submitted;
static unsigned draws, pushes, pops, titles;
bool ForestVisit(const PlayState* play) { return play && visit; }
bool ForestSafe(const PlayState* play) { return play && safe; }
bool ResourceMgr_FileExists(const char* path) {
    return titleAssetAvailable && std::strcmp(path, "custom/skiejer_forest/title") == 0;
}
int ComboPlaceTitleCard_Show(PlayState*, void*, int width, int height) {
    assert(width == 144 && height == 24); ++titles; return 1;
}
extern "C" NeiGi::Basis NeiGi_CameraBasis(PlayState*) { return {}; }
extern "C" void NeiGi_DrawWorldParticles(PlayState*, const NeiGi::Mesh& mesh) { submitted = mesh; ++draws; }
#define Matrix_Push() (++pushes)
#define Matrix_Pop() (++pops)
#define Matrix_Translate(x,y,z,mode) assert((x)==COMBO_SOLO_FOREST_STUMP_X && (z)==COMBO_SOLO_FOREST_STUMP_Z)
#define Matrix_RotateYS(yaw,mode) assert((yaw)==COMBO_SOLO_FOREST_STUMP_YAW)
#define Matrix_RotateY(yaw,mode) assert(std::fabs((yaw)-COMBO_SOLO_FOREST_STUMP_YAW*(6.28318530718f/65536))<.001f)
/* PRODUCTION */
int main() {
    PlayState play{}; GraphicsContext gfx{}; gPlayState = &play;
    play.state.gfxCtx = &gfx; play.gameplayFrames = 75;
    DrawForestPortal();
    assert(draws == 1 && pushes == 1 && pops == 1);
    assert(submitted.count == ForestGiParticles::kPortalMotes * ForestGiParticles::kTwinkleVertices);
    for (int blocked = 0; blocked < 4; ++blocked) {
        draws = pushes = pops = 0;
        visit = blocked != 0; safe = blocked != 1;
        play.roomCtx.curRoom.num = blocked == 2 ? 1 : 0;
        gPlayState = blocked == 3 ? nullptr : &play;
        DrawForestPortal(); assert(!draws && !pushes && !pops);
    }
    visit = safe = true; play.roomCtx.curRoom.num = 0; gPlayState = &play;
    sForestTitle = true; DrawForestPortal(); assert(!sForestTitle);
#ifdef OOT_FOREST_TEST
    assert(play.actorCtx.titleCtx.width == 144 && play.actorCtx.titleCtx.height == 24);
    assert(play.actorCtx.titleCtx.durationTimer == 80 && play.actorCtx.titleCtx.delayTimer == 20);
    play.actorCtx.titleCtx = {};
#else
    assert(titles == 1);
#endif
    titleAssetAvailable = false;
    sForestTitle = true; titles = 0; DrawForestPortal(); assert(!sForestTitle && !titles);
#ifdef OOT_FOREST_TEST
    assert(!play.actorCtx.titleCtx.texture && !play.actorCtx.titleCtx.width && !play.actorCtx.titleCtx.durationTimer);
#endif
    alignas(Gfx) static const char nativeRoot[]="__OTR__custom/comboship/native_forest/portal_root";
#ifdef OOT_FOREST_TEST
    MeshHeader mesh{}; PolygonDlist2 entries[3]{};
    mesh.base.type=2; mesh.polygon2.num=3; mesh.polygon2.start=entries;
    play.roomCtx.curRoom.meshHeader=&mesh;
#else
    RoomShape mesh{}; RoomShapeCullableEntry entries[3]{};
    mesh.base.type=ROOM_SHAPE_TYPE_CULLABLE; mesh.cullable.numEntries=3; mesh.cullable.entries=entries;
    play.roomCtx.curRoom.roomShape=&mesh;
#endif
    entries[2].opa=(Gfx*)nativeRoot;
    draws = 0; DrawForestPortal(); assert(draws == 1);
    for (size_t i = 0; i < submitted.count; ++i) {
        const auto p = submitted.vertices[i].p;
        assert(std::abs(p.x) < 49 && p.y < 114);
    }
    puts("PASS native forest portal GI particles: actual native/packed size selection, unchanged anchor, guards and optional local title");
}
