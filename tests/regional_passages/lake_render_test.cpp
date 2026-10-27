// The runner inserts unmodified production draw, native GBI wrappers and arena
// functions. Only resource availability, matrix state and GPU consumption are
// doubled; no production command-writing macro is replaced.
#include "global.h"
#include "ComboGameMode.h"
#include "entrances/LakeHyliaBridge.h"
#include "entrances/LakeHyliaSurface.h"
#include "entrances/LostWoodsEntrance.h"
#include "render/RegionalBridgeMaterials.h"
#include "fast/lus_gbi.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <vector>

SaveContext gSaveContext{};
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static bool replacements;
static bool stoneReplacement, sandReplacement;
static float lakeWaterY;
static size_t openChildren;
static std::vector<Gfx> opa(8192), xlu(2048), overlay(64);
static int matrixDepth;
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) { ++openChildren; }
extern "C" void FrameInterpolation_RecordCloseChild() { assert(openChildren); --openChildren; }
extern "C" u8 ResourceMgr_FileExists(const char* path) {
    assert(std::strncmp(path, "textures/comboship/regional/", 27) == 0);
    if (std::strcmp(path, RegionalBridgeMaterials::OverridePath(RegionalBridgeMaterials::Material::LakeStone) + 7) == 0)
        return stoneReplacement;
    if (std::strcmp(path, RegionalBridgeMaterials::OverridePath(RegionalBridgeMaterials::Material::LakeSand) + 7) == 0)
        return sandReplacement;
    return replacements;
}
extern "C" int ResourceMgr_OTRSigCheck(char* path) {
    return path && std::memcmp(path, "__OTR__", 7) == 0;
}
extern "C" Gfx* PakLoader_GetDLOverride(const char*) { assert(false); return nullptr; }
extern "C" Gfx* ResourceMgr_LoadGfxByName(char*) { assert(false); return nullptr; }
extern "C" Vtx* ResourceMgr_LoadVtxByName(char*) { assert(false); return nullptr; }
extern "C" void Matrix_Push() { ++matrixDepth; }
extern "C" void Matrix_Pop() { assert(matrixDepth); --matrixDepth; }
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) {
    assert(x == 0 && y == 0 && z == 0 && mode == MTXMODE_NEW);
}
extern "C" Mtx* Matrix_NewMtx(GraphicsContext* gfx, char*, s32) {
    auto* matrix = static_cast<Mtx*>(Graph_Alloc(gfx, sizeof(Mtx)));
    std::memset(matrix, 0, sizeof(*matrix));
    return matrix;
}
extern "C" s32 WaterBox_GetSurface1(PlayState*, CollisionContext*, f32 x, f32 z, f32* y, WaterBox**) {
    assert(x == -1900 && z == 3757);
    *y = lakeWaterY;
    return true;
}
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#undef HREG
#define HREG(index) 0
/* NATIVE_GRAPH */
/* NATIVE_GBI */
/* NATIVE_ARENA */
namespace {
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int);
extern "C" void FrameInterpolation_RecordCloseChild(void);
namespace Lake = ComboLakeHylia;
struct CaveActor { DynaPolyActor dyna; };
static CaveActor cave;
static CaveActor* sCave = &cave;
/* PRODUCTION_DRAW */
}

using F3DGfx = Gfx;
using F3DVtx = Vtx;
/* NATIVE_COMBINER_DEFINITIONS */
struct Interpreter {
    uintptr_t mSegmentPointers[16]{};
    struct MaterialState {
        uint64_t combine_mode = 0;
        uint32_t other_mode_h = 0, other_mode_l = 0;
        RGBA prim_color{}, env_color{}, key_center{}, key_scale{};
        uint8_t prim_lod_fraction = 0, convert_k[6]{};
    } material;
    MaterialState* mRdp = &material;
    const char* texture = nullptr;
    bool waterMaterial = false;
    unsigned tileFormat = 0, tileSize = 0, tileWidth = 0, tileHeight = 0;
    Vtx firstWater{}, nextWater{}, lastWater{};
    bool hasWater = false;
    bool hasVerticalFall = false;
    size_t vertices = 0;
    float dryFloorArea = 0, sandFloorArea = 0;
    const GraphicsContext* gfx = nullptr;
    void* SegAddr(uintptr_t);
    void GfxDpSetCombineMode(uint32_t, uint32_t, uint32_t, uint32_t);
    void GfxDpSetOtherMode(uint32_t, uint32_t);
    void GfxSpSetOtherMode(uint32_t, uint32_t, uint64_t);
    void GfxDpSetPrimColor(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
    static void GenerateCC(ColorCombiner*, const ColorCombinerKey&);
    float mBufVbo[28]{};
    size_t mBufVboLen = 0;
    void PackInputs(ColorCombiner* comb, uint8_t fogFactor) {
        struct Vertex { RGBA color; float w; } vertex{{180, 180, 180, fogFactor}, 1};
        auto* v1 = &vertex;
        Vertex* v_arr[] = {&vertex};
        const int i = 0, numInputs = 7;
        const bool use_alpha = true, use_fog = true, use_blend_color = false;
        mBufVboLen = 0;
        /* NATIVE_PACKED_INPUTS */
    }
    void CheckOwnedMaterial() {
        ColorCombinerKey key{};
        const auto cycle = material.other_mode_h & (3u << G_MDSFT_CYCLETYPE);
        if (cycle == G_CYC_2CYCLE)
            key.options |= SHADER_OPT(_2CYC);
        if (waterMaterial)
            key.options |= SHADER_OPT(ALPHA) | SHADER_OPT(FOG);
        key.combine_mode = material.combine_mode;
        ColorCombiner comb{};
        GenerateCC(&comb, key);
        if (!waterMaterial) {
            const bool sand = std::strcmp(texture, RegionalBridgeMaterials::Resolve(
                RegionalBridgeMaterials::Material::LakeSand, ResourceMgr_FileExists)) == 0;
            assert(tileFormat == G_IM_FMT_RGBA && tileSize == G_IM_SIZ_16b);
            assert(tileWidth == unsigned(sand ? (sandReplacement ? 32 : 16) : 32));
            assert(tileHeight == unsigned(sand ? (sandReplacement ? 32 : 16) : (stoneReplacement ? 64 : 32)));
        }
        if (waterMaterial) {
            for (uint8_t fogFactor : {uint8_t(0), uint8_t(64), uint8_t(255)}) {
                PackInputs(&comb, fogFactor);
                bool mappedAlpha = false;
                for (int j = 0; j < 7; ++j)
                    if (comb.shader_input_mapping[1][j]) {
                        mappedAlpha = true;
                        assert(std::fabs(mBufVbo[j * 4 + 3] - material.prim_color.a / 255.f) < .00001f &&
                               "standard fog must preserve authored water opacity in the actual packed shader input");
                    }
                assert(mappedAlpha);
            }
            assert(material.prim_color.a < 255);
            assert((material.other_mode_l & FORCE_BL) && !(material.other_mode_l & Z_UPD) &&
                   "water must blend with its bed instead of using the opaque actor setup");
            assert(tileFormat == unsigned(replacements ? G_IM_FMT_I : G_IM_FMT_RGBA));
            assert(tileSize == unsigned(replacements ? G_IM_SIZ_8b : G_IM_SIZ_16b));
            assert(tileWidth == unsigned(replacements ? 64 : 32) && tileHeight == tileWidth &&
                   "native and replacement water must retain their own logical tile dimensions");
        }
        // A one-texture authored surface must be independent of tile1 left by
        // the scene. The production generator decides the shader's actual
        // texture dependencies, including the second-cycle TEXEL0/1 swap.
        if (!comb.usedTextures[0] || comb.usedTextures[1]) {
            std::fprintf(stderr, "FAIL cave material %s: texture0=%d texture1=%d; inherited scene tile1 must be unused\n",
                         texture ? texture : "<none>", comb.usedTextures[0], comb.usedTextures[1]);
            std::abort();
        }
    }
    void GfxSpVertex(uint32_t count, uint32_t offset, const F3DVtx* pointer) {
        assert(count == 4 && offset == 0);
        const auto address = reinterpret_cast<uintptr_t>(pointer);
        const auto& rock = RockVertices(stoneReplacement, sandReplacement);
        const auto rockBegin = reinterpret_cast<uintptr_t>(rock.data());
        const auto rockEnd = rockBegin + rock.size() * sizeof(Vtx);
        const auto tailBegin = reinterpret_cast<uintptr_t>(gfx->polyOpa.d);
        const auto tailEnd = reinterpret_cast<uintptr_t>(opa.data() + opa.size());
        assert((address >= rockBegin && address + count * sizeof(Vtx) <= rockEnd) ||
               (address >= tailBegin && address + count * sizeof(Vtx) <= tailEnd));
        // Measure actual queued, quantized vertices under their active texture.
        // A correct isolated partition is insufficient if the renderer submits
        // the old single-atlas floor or assigns sand to a narrow UV strip.
        bool floor = !waterMaterial;
        for (size_t i = 0; i < count; ++i)
            floor &= pointer[i].v.ob[1] == std::lround(Lake::kMouth.y);
        if (floor) {
            float twiceArea = 0;
            for (size_t i = 0; i < count; ++i) {
                const auto& a = pointer[i].v.ob;
                const auto& b = pointer[(i + 1) % count].v.ob;
                twiceArea += float(a[0]) * b[2] - float(b[0]) * a[2];
            }
            const float area = std::fabs(twiceArea) * .5f;
            dryFloorArea += area;
            if (std::strcmp(texture, RegionalBridgeMaterials::Resolve(
                    RegionalBridgeMaterials::Material::LakeSand, ResourceMgr_FileExists)) == 0)
                sandFloorArea += area;
        }
        if (waterMaterial && !hasWater) {
            firstWater = pointer[0];
            nextWater = pointer[3];
            hasWater = true;
        }
        if (waterMaterial)
            lastWater = pointer[2];
        if (waterMaterial && pointer[0].v.ob[0] == pointer[3].v.ob[0] &&
            pointer[3].v.ob[1] < pointer[0].v.ob[1]) {
            hasVerticalFall = true;
            assert(pointer[3].v.tc[1] > pointer[0].v.tc[1] &&
                   "falling crests need increasing UV distance down the vertical face");
            assert(material.prim_color.a >= 64 && "the falling sheet must retain visible opacity");
        }
        for (size_t i = 0; i < count; ++i)
            assert(pointer[i].v.cn[3] == 255);
        vertices += count;
    }
};
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << (width)) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << (width)) - 1))
/* NATIVE_INTERPRETER */

struct Counts { size_t vertices = 0, triangles = 0, textures = 0, pushes = 0, pops = 0; };
static Counts Walk(Interpreter& interpreter, Gfx* begin, Gfx* end, int depth = 0) {
    assert(depth < 2);
    Counts counts;
    for (auto* command = begin; command != end; ++command) {
        const auto opcode = command->words.w0 >> 24;
        switch (opcode) {
            case G_DL: {
                // The cave's only raw sublist is the engine's actual setup.
                auto* target = static_cast<Gfx*>(interpreter.SegAddr(command->words.w1));
                assert(target == sSetupDL[SETUPDL_25]);
                Walk(interpreter, target, target + 6, depth + 1);
                break;
            }
            case G_ENDDL:
                assert(depth == 1 && command + 1 == end);
                break;
            case G_VTX:
                gfx_vtx_handler_f3dex2(&interpreter, &command);
                counts.vertices += 4;
                break;
            case G_TRI2:
                interpreter.CheckOwnedMaterial();
                assert(command->words.w0 == (uintptr_t(G_TRI2) << 24 | 0x000204));
                assert(command->words.w1 == 0x000406);
                counts.triangles += 2;
                break;
            case G_SETTIMG: {
                const auto* path = static_cast<const char*>(interpreter.SegAddr(command->words.w1));
                assert(ResourceMgr_OTRSigCheck(const_cast<char*>(path)));
                assert(std::strcmp(path, RegionalBridgeMaterials::Resolve(
                    RegionalBridgeMaterials::Material::LakeStone, ResourceMgr_FileExists)) == 0 ||
                       std::strcmp(path, RegionalBridgeMaterials::Resolve(
                           RegionalBridgeMaterials::Material::LakeSand, ResourceMgr_FileExists)) == 0 ||
                       std::strcmp(path, RegionalBridgeMaterials::Resolve(
                           RegionalBridgeMaterials::Material::LakeWater, ResourceMgr_FileExists)) == 0);
                ++counts.textures;
                interpreter.texture = path;
                interpreter.waterMaterial = std::strcmp(path, RegionalBridgeMaterials::OverridePath(
                    RegionalBridgeMaterials::Material::LakeWater)) == 0 ||
                    std::strcmp(path, RegionalBridgeMaterials::FallbackPath(
                    RegionalBridgeMaterials::Material::LakeWater)) == 0;
                if (interpreter.waterMaterial && !replacements) {
                    assert(std::strcmp(path, "__OTR__objects/object_spot06_objects/gLakeHyliaWaterTex") == 0 &&
                           "the native fallback must be continuous crested water, not a circular ripple sprite");
                    assert(((command->words.w0 >> 21) & 7) == G_IM_FMT_RGBA);
                }
                break;
            }
            case G_SETCOMBINE:
                gfx_set_combine_handler_rdp(&interpreter, &command);
                break;
            case G_RDPSETOTHERMODE:
                gfx_rdp_set_other_mode_rdp(&interpreter, &command);
                break;
            case G_SETOTHERMODE_L:
                gfx_othermode_l_handler_f3dex2(&interpreter, &command);
                break;
            case G_SETTILE:
                if (((command->words.w1 >> 24) & 7) == G_TX_RENDERTILE) {
                    interpreter.tileFormat = (command->words.w0 >> 21) & 7;
                    interpreter.tileSize = (command->words.w0 >> 19) & 3;
                }
                break;
            case G_SETTILESIZE:
                interpreter.tileWidth = ((command->words.w1 >> 12) & 0xfff) / 4 + 1;
                interpreter.tileHeight = (command->words.w1 & 0xfff) / 4 + 1;
                break;
            case G_SETPRIMCOLOR:
                gfx_set_prim_color_handler_rdp(&interpreter, &command);
                break;
            case G_COMBO_RM_PUSH:
                assert(std::strcmp(reinterpret_cast<const char*>(command->words.w1), "oot") == 0);
                ++counts.pushes;
                break;
            case G_COMBO_RM_POP:
                ++counts.pops;
                break;
            case G_MTX:
                assert(command->words.w1 >= reinterpret_cast<uintptr_t>(interpreter.gfx->polyOpa.d));
                assert(command->words.w1 + sizeof(Mtx) <= reinterpret_cast<uintptr_t>(opa.data() + opa.size()));
                break;
            case G_NOOP: case G_GEOMETRYMODE: case G_TEXTURE:
            case G_LOADBLOCK: case G_RDPPIPESYNC: case G_RDPLOADSYNC:
                break;
            default:
                std::fprintf(stderr, "invalid cave opcode %#zx at command %td\n", size_t(opcode), command - begin);
                assert(false);
        }
    }
    return counts;
}

int main() {
    GraphicsContext gfx{};
    PlayState play{};
    CollisionHeader collision{};
    play.state.gfxCtx = &gfx;
    play.sceneNum = SCENE_LAKE_HYLIA;
    play.colCtx.colHeader = &collision;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    s16 phaseZeroT = 0;
    for (bool stone : {false, true})
      for (bool sand : {false, true})
       for (bool alt : {false, true})
        for (float level : {Lake::kFullWaterY, Lake::kFullWaterY - 8, Lake::kChannelFloorY + 2,
                           Lake::kDrainedWaterY, Lake::kFullWaterY})
            for (u32 frame : {0u, 1u, 54u, 2047u, 2048u, 0xffffffffu}) {
                replacements = alt;
                stoneReplacement = stone;
                sandReplacement = sand;
                lakeWaterY = level;
                play.gameplayFrames = frame;
                auto initialize = [](TwoHeadGfxArena& arena, std::vector<Gfx>& commands) {
                    std::memset(commands.data(), 0xA9, commands.size() * sizeof(Gfx));
                    arena.p = commands.data();
                    arena.d = commands.data() + commands.size();
                };
                initialize(gfx.polyOpa, opa);
                initialize(gfx.polyXlu, xlu);
                initialize(gfx.overlay, overlay);
                CaveDraw(&cave.dyna.actor, &play);
                assert(gfx.polyOpa.p > opa.data() && gfx.polyOpa.p <= gfx.polyOpa.d);
                assert(gfx.polyXlu.p <= gfx.polyXlu.d && gfx.overlay.p <= gfx.overlay.d);
                assert(openChildren == 0 && matrixDepth == 0);
                Interpreter interpreter;
                interpreter.gfx = &gfx;
                const auto rock = Walk(interpreter, opa.data(), gfx.polyOpa.p);
                const auto water = Walk(interpreter, xlu.data(), gfx.polyXlu.p);
                const auto quads = SurfaceGeometry().stone.size() + SurfaceGeometry().sand.size();
                assert(rock.vertices == quads * 4);
                assert(rock.triangles == quads * 2);
                assert(rock.textures == 2 && rock.pushes == 1 && rock.pops == 1);
                assert(interpreter.dryFloorArea > 0 &&
                       interpreter.sandFloorArea / interpreter.dryFloorArea > .42f &&
                       interpreter.sandFloorArea / interpreter.dryFloorArea < .58f &&
                       "the active native/Alt draw must assign dedicated sand to roughly half the platform");
                const bool inlet = Lake::HasInlet(level);
                if (inlet) {
                    assert(interpreter.hasWater);
                    assert(interpreter.firstWater.v.ob[1] > interpreter.lastWater.v.ob[1] &&
                           interpreter.lastWater.v.ob[0] > -1825 &&
                           interpreter.lastWater.v.ob[0] <= -1823 &&
                           interpreter.lastWater.v.ob[1] == std::lround(level) &&
                           "the actual queued water vertices must descend beyond the lip to the native lake");
                    assert(interpreter.hasVerticalFall && "the queued draw must contain vertical falling water");
                    assert(interpreter.nextWater.v.ob[0] > interpreter.firstWater.v.ob[0] &&
                           interpreter.nextWater.v.tc[1] > interpreter.firstWater.v.tc[1]);
                    if (frame == 0)
                        phaseZeroT = interpreter.firstWater.v.tc[1];
                    if (frame == 1)
                        assert(interpreter.firstWater.v.tc[1] < phaseZeroT &&
                               "outward wave crests must move from the cave toward positive X and the lake");
                    if (frame == 54)
                        assert((interpreter.firstWater.v.tc[1] - phaseZeroT) % (interpreter.tileWidth * 32) == 0 &&
                               "scroll wrap must return through an exact whole texture period");
                } else {
                    assert(!interpreter.hasWater);
                }
                const size_t expectedQuads = !inlet ? 0 : level == Lake::kFullWaterY ? 30 :
                    level == Lake::kFullWaterY - 8 ? 32 : 34;
                assert(water.vertices == expectedQuads * 4);
                assert(water.triangles == expectedQuads * 2);
                assert(water.textures == size_t(inlet) && water.pushes == size_t(inlet) && water.pops == size_t(inlet));
            }
    puts("PASS actual CaveDraw/GBI/native combiner and vertex dispatch: dedicated sand covers roughly half the emitted platform in every material path; stone/water depend only on owned texture0; setup lists, command validity, static/frame vertex lifetime, native/replacement full/drained/refilled draws");
}
