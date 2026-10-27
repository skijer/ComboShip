#include "global.h"
#include "ComboGameMode.h"
#include "actors/ForestGiParticles.h"
#include "2s2h/Enhancements/Graphics/WoodsMysteryMakeover.h"
#include <cassert>
#include <cstdio>
#include <cstring>

PlayState* gPlayState;
SaveContext gSaveContext{};
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static bool nativeReady, altEnabled, packAvailable;
static Vec3f anchor;
static float yaw;
static NeiGi::Mesh submitted;
static int draws, pushes, pops;
extern "C" int ComboSoloForestMm_PassageAvailable() { return false; }
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return altEnabled; }
extern "C" uint8_t ResourceMgr_FileExists(const char* path) {
    return packAvailable && !std::strcmp(path, WoodsMysteryMakeover::kBridgeMesh);
}
extern "C" uint8_t ResourceMgr_FileAltExists(const char* path) { return ResourceMgr_FileExists(path); }
namespace WoodsMysteryNativeBridge {
bool Active(const PlayState*) { return nativeReady; }
}
NeiGi::Basis NeiGi_CameraBasis(PlayState*) { return {}; }
void NeiGi_DrawWorldParticles(PlayState*, const NeiGi::Mesh& mesh) { submitted = mesh; ++draws; }
#define Matrix_Push() (++pushes)
#define Matrix_Pop() (++pops)
#define Matrix_Translate(x,y,z,mode) (anchor = {float(x),float(y),float(z)})
#define Matrix_RotateYS(angle,mode) (yaw = (angle) * (6.28318530718f / 65536.0f))

/* PRODUCTION */

int main(int argc, char** argv) {
    const bool behindCap = argc == 2 && !std::strcmp(argv[1], "--behind-cap");
    assert(argc == 1 || behindCap);
    PlayState play{};
    RoomShape shape{};
    RoomShapeCullableEntry entry{};
    play.sceneId = SCENE_26SARUNOMORI;
    play.roomCtx.curRoom.num = 1;
    play.roomCtx.curRoom.roomShape = &shape;
    shape.base.type = ROOM_SHAPE_TYPE_CULLABLE;
    shape.cullable.numEntries = 1;
    shape.cullable.entries = &entry;
    play.gameplayFrames = 75;
    gPlayState = &play;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    for (int variant = 0; variant < 3; ++variant) {
        nativeReady = variant < 2;
        altEnabled = variant != 0;
        packAvailable = variant == 2;
        entry.opa = reinterpret_cast<Gfx*>(const_cast<char*>(packAvailable ? WoodsMysteryMakeover::kBridgeMesh :
                                                            "__OTR__scenes/nonmq/Z2_26SARUNOMORI/native"));
        draws = pushes = pops = 0;
        DrawWoodsMysteryHexes();
        assert(draws == 1 && pushes == 1 && pops == 1);
        assert(submitted.count == ForestGiParticles::kMysteryMotes * ForestGiParticles::kTwinkleVertices);
        for (size_t i = 0; i < submitted.count; i += ForestGiParticles::kTwinkleVertices) {
            auto p = submitted.vertices[i].p;
            // Negative control: +front becomes -front, preserving the same
            // footprint and unsigned proximity while moving behind the cap.
            if (behindCap)
                p.z = -p.z;
            const float x = anchor.x + std::cos(yaw) * p.x + std::sin(yaw) * p.z;
            const float z = anchor.z - std::sin(yaw) * p.x + std::cos(yaw) * p.z;
            std::printf("%d %zu %.6f %.6f %.6f\n", variant, i / ForestGiParticles::kTwinkleVertices,
                        x, anchor.y + p.y, z);
        }
    }
}
