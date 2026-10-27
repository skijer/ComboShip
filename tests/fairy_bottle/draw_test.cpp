#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "glass_commands.h"
#include "combo/menu/ComboFairyBottle.h"
#include "combo/menu/ComboBottleContents.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboSongDraw.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using u8 = uint8_t;
using f32 = float;
struct Gfx { int stream = 0; };
// This fixture supplies only fairy rows; the independent bottle fixture runs
// the real selected-wrapper classifier and the complete potion branches.
Gfx* ResourceMgr_LoadGfxByName(const char*) { return nullptr; }
int ComboBottleGi_HasPotionRecipe(const char*,const char*,const char*,Gfx*(*)(const char*)) { return 0; }
struct GraphicsContext {};
struct MtxF {
    float x = 0, y = 0, z = 0, sx = 1, sy = 1, sz = 1;
    bool billboard = false;
    float xx = 0, yy = 0, zz = 0, ww = 0;
};
using Mtx = MtxF;
struct PlayState {
    struct { GraphicsContext* gfxCtx; uint32_t frames = 0; } state;
    uint32_t gameplayFrames = 0;
    MtxF billboardMtxF;
};
#include "combo/menu/ComboCollectibleShimmerDraw.h"
extern "C" void OOT_SampleCollectibleShimmerColor(int32_t, uint8_t[4]) {
    assert(false && "fairy fixture unexpectedly sampled collectible colors");
}
extern "C" void NeiGi_DrawTintedShimmerOverlay(PlayState*, const uint8_t[4], const char*) {
    assert(false && "fairy fixture unexpectedly drew collectible shimmer");
}
extern "C" void NeiGi_DrawElementalArrow(PlayState*, int) {
    assert(false && "fairy fixture must not dispatch elemental arrows");
}
extern "C" void NeiGi_DrawElementalArrowForOwner(PlayState*,int,int,int) { assert(false); }
extern "C" int NeiGi_DrawElementalSpell(PlayState*,int,int,int) { assert(false); return 0; }
GraphicsContext gfx;
PlayState play{{&gfx}, 0, {}};
PlayState* gPlayState = &play;
char opaque[] = "opaque", glass[] = "glass", fairy[] = "fairy", matrixPath[] = "matrix";
char genericOpaque[] = "genericOpaque", genericGlass[] = "genericGlass";
char blueFire[] = "__OTR__objects/object_gi_fire/gGiBlueFireChamberstickDL";
const char* gGiFairyBottleTexAnim = "texAnim";
std::unordered_set<std::string> selectedMods, customModels;
int ResourceMgr_IsCustomAssetForGame(const char* game,const char* path) {
    assert(!strcmp(game,"oot"));
    return customModels.count(path) || customModels.count(std::string("__OTR__") + path);
}
int ResourceMgr_IsModAsset(const char* path) { return selectedMods.count(path) != 0; }
int ResourceMgr_IsModAssetForGame(const char* game, const char* path) {
    assert(!strcmp(game,"oot") && !strcmp(path,blueFire+7));
    return selectedMods.count(blueFire) != 0;
}

constexpr int MTXMODE_APPLY = 1, G_MTX_MODELVIEW = 2, G_MTX_LOAD = 4, G_TX_RENDERTILE = 0;
constexpr int GID_FAIRY = 0, GID_BOTTLE = 2, GID_SONG_GENERIC = 0x75, kMaxMatEntries = 8;
MtxF current, initial, loaded[2];
MtxF nativePlacement{0, 3, 0, .01f, .01f, .01f};
std::vector<MtxF> stack;
struct Draw {
    int stream;
    std::string path;
    MtxF pose;
    std::string owner;
};
std::vector<Draw> draws;
std::vector<std::string> events;
Gfx opa[64], xlu[64];
Gfx* opaPtr;
Gfx* xluPtr;
int foreignRestores = 0, hostFallbacks = 0, ownerScope = 1;
bool ownerPresent = true, matrixPresent = true, matrixPointerPresent = true, materialPresent = true;
bool fairyResources = false;
bool bundledShell = false;
bool readableGlass = true;
std::string gpuOwner;
std::vector<std::string> gpuOwners;
#ifdef COMBO_FAIRY_HOST_MM
const char* fairyHost = "mm";
#else
const char* fairyHost = "oot";
#endif
void PushGpuOwner(const char* owner) { gpuOwners.push_back(gpuOwner); gpuOwner=owner; }
void PopGpuOwner() { assert(!gpuOwners.empty()); gpuOwner=gpuOwners.back(); gpuOwners.pop_back(); }
int animationFrame = -1, skeletonDraws = 0;
int shimmerDraws = 0, moteDraws = 0;
uint8_t fairyPrim[4]{}, fairyEnv[4]{};

void Matrix_Push() { stack.push_back(current); }
void Matrix_Pop() { assert(!stack.empty()); current = stack.back(); stack.pop_back(); }
void Matrix_Translate(float x, float y, float z, int) { current.x += x; current.y += y; current.z += z; }
void Matrix_Scale(float x, float y, float z, int) { current.sx *= x; current.sy *= y; current.sz *= z; }
void Matrix_ReplaceRotation(MtxF* pose) { current.billboard = pose == &play.billboardMtxF; }
void Matrix_Mult(MtxF* mtx, int) { current.x += mtx->x; current.y += mtx->y; current.z += mtx->z; current.sx *= mtx->sx; current.sy *= mtx->sy; current.sz *= mtx->sz; }
void Matrix_MtxToMtxF(Mtx* input, MtxF* output) { *output = *input; }
void LoadMatrix(Gfx* command, MtxF* pose) { loaded[command->stream] = *pose; }
void DrawList(Gfx* command, const void* path) {
    assert(ownerScope == 1 && "foreign load scope leaked into host drawing");
    draws.push_back({ command->stream, static_cast<const char*>(path), loaded[command->stream], gpuOwner });
    events.push_back("draw");
}
void Segment(Gfx* command, int segment, uintptr_t) { assert(command->stream == 1 && segment == 8); }
template <class... T> uintptr_t Gfx_TwoTexScrollEx(T...) { return 1; }
template <class T> T Lib_SegmentedToVirtual(T path) { return path; }
Mtx* ResourceMgr_LoadMtxByName(void* path) { assert(path == matrixPath); return &nativePlacement; }
void AnimatedMat_Draw(PlayState*, const char* path) { assert(path == gGiFairyBottleTexAnim); events.push_back("mat"); }
int ComboBottleContents_Draw(PlayState*, int) { assert(false); return 0; }
void GetItem_Draw(PlayState*, int id);
struct Color_RGB8 { uint8_t r,g,b; };
int CVarGetInteger(const char*,int) { return 0; }
Color_RGB8 CVarGetColor24(const char*,Color_RGB8 color) { return color; }
bool ComboOotMaskShimmerColor(int,uint8_t*) { return false; }
void ComboMaskShimmerColor(int,uint8_t*) { assert(false); }
bool DinFireShield_DrawItem(PlayState*,int) { return false; }
#define CVAR_COSMETIC(x) x
void ComboDrawMaskShimmer(PlayState*,const char*,const uint8_t color[4],const char*) {
    assert(color[0]==255 && color[1]==160 && color[2]==235 && color[3]==255);
    ++shimmerDraws;
}
void OOT_RestoreForeignSegs(PlayState*, int32_t* segs, int count) { assert(count == 1 && segs[0] == 8); ++foreignRestores; }
void MM_RestoreForeignSegs(int32_t* segs, int count) { OOT_RestoreForeignSegs(&play, segs, count); }
bool ComboForeignTexAnim_Run(PlayState*, const char* owner, const char* path, bool opa, int32_t* segs, int32_t* count) {
    assert(ownerScope == 1 && !strcmp(owner, "mm") && opa);
    if (!materialPresent || !path) return false;
    assert(!strcmp(path, "texAnim"));
    segs[0] = 8; *count = 1; events.push_back("mat"); return true;
}
void ComboForeignTexAnim_Restore(PlayState* play, int32_t* segs, int count, bool opa) {
    assert(opa); OOT_RestoreForeignSegs(play, segs, count); events.push_back("restore");
}
namespace Ship {
struct Resource {
    void* GetRawPointer() { return matrixPointerPresent ? &nativePlacement : nullptr; }
};
struct ResourceManager {
    std::shared_ptr<Resource> LoadResource(const char* path) {
        assert(ownerScope == 2 && !strcmp(path, "matrix"));
        return matrixPresent ? std::make_shared<Resource>() : nullptr;
    }
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const char* owner) {
        assert(!strcmp(owner, "mm"));
        return ownerPresent ? std::make_shared<ResourceManager>() : nullptr;
    }
};
struct ResourceManagerScope {
    ResourceManagerScope(std::shared_ptr<ResourceManager>) { assert(ownerScope == 1); ownerScope = 2; }
    ~ResourceManagerScope() { ownerScope = 1; }
};
}
struct ComboForeignDrawInfo {
    int count = 4;
    const char* dls[4] = {genericOpaque, genericGlass, fairy, "__OTR__@mm:matrix"};
    const char* matAnimPath = "texAnim";
    int drawKind = CW_DRAW_KIND_MM_FAIRY_CONTAINER, xluStart = 1, neiEffect = 0, neiShimmer = 0;
    int opCount = 0;
    CwDrawOp ops[CW_DRAW_MAX_OPS] = {};
    float scale = 0, neiEffectCenter[3]{};
    bool animOk = false;
    int32_t itemShimmer = CW_ITEM_SHIMMER_LEGACY;
    uint8_t itemShimmerColor[4] = {255,160,235,255}, primColorXlu[4]{};
    CwItemAnimDrawInfo anim{};
};
using ComboForeignDrawInfoOOT = ComboForeignDrawInfo;
using RandomizerCheck = int;
constexpr int RC_UNKNOWN_CHECK = -1;
struct GetItemEntry { int comboForeignCheck = 1, gid = GID_FAIRY; };
const ComboForeignDrawInfo* selectedForeign = nullptr;
RandomizerCheck OOT_GetQueuedDrawCheck() { return 1; }
const ComboForeignDrawInfo* ComboResolveForeignDrawInfo(int) { return selectedForeign; }
bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo*,const char*,PlayState*) { return false; }
NeiGi::Basis NeiGi_CameraBasis(PlayState*) { return {}; }
template<class... T> void NeiGi_DrawPresentation(T...) { assert(false); }
template<class... T> void NeiGi_DrawMesh(T...) { assert(false); }
template<class... T> void NeiGi_DrawSeasonOverlay(T...) { assert(false); }
template<class... T> void NeiGi_DrawSongOverlay(T...) { assert(false); }
float ComboSwordGi_SelectedTilt(float,bool) { assert(false && "fairy fixture unexpectedly tilted a sword"); return 0; }
template<class... T> void ComboSwordGi_ApplyModelsFit(T...) { assert(false); }
template<class... T> void ComboDrawSpinAttackGi(T...) { assert(false); }
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define POLY_OPA_DISP opaPtr
#define POLY_XLU_DISP xluPtr
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) events.push_back("opa")
#define Gfx_SetupDL_25Xlu(...) events.push_back("xlu")
#define Gfx_SetupDL25_Opa(...) events.push_back("opa")
#define Gfx_SetupDL25_Xlu(...) events.push_back("xlu")
#define OOT_FOREIGN_PIN_OPA() ((void)0)
#define OOT_FOREIGN_PIN_XLU() (gpuOwner="mm")
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() (gpuOwner="oot")
#define gSPComboRMPush(p,owner) ((void)(p),PushGpuOwner(owner))
#define gSPComboRMPop(p) ((void)(p),PopGpuOwner())
#define MATRIX_NEWMTX(...) (&current)
#define gSPMatrix(p,m,flags) LoadMatrix(p,m)
#define MATRIX_FINALIZE_AND_LOAD(p,ctx) LoadMatrix(p,&current)
#define COMBO_FOREIGN_MTX(p) LoadMatrix(p,&current)
#define gSPDisplayList(p,dl) DrawList(p,dl)
#define gSPSegment(p,seg,dl) Segment(p,seg,(uintptr_t)(dl))

struct Actor {};
struct Vec3f { float x,y,z; };
struct Vec3s { s16 x,y,z; };
constexpr int SKELANIME_TYPE_FLEX = 1;
struct SkeletonHeader { int limbCount; int skeletonType = 0; };
struct FlexSkeletonHeader { SkeletonHeader sh; int dListCount; };
struct AnimationHeader { int lastFrame = 29; };
using AnimationHeaderCommon = AnimationHeader;
struct SkelAnime { void** skeleton; Vec3s* jointTable; };
#ifdef COMBO_FAIRY_HOST_MM
SkeletonHeader fairySkeleton{6};
#else
SkeletonHeader fairySkeleton{14};
#endif
AnimationHeader fairyAnimation;
extern "C" uint8_t ResourceMgr_FileExists(const char* path) {
    if (strstr(path, "combo_bottle_gi/FairyXlu"))
        return bundledShell && readableGlass;
    return strstr(path,"combo_bottle_gi") ? bundledShell : fairyResources;
}
extern "C" uint8_t ResourceMgr_FileAltExists(const char*) { return 0; }
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return false; }
extern "C" SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* registered) {
    assert(!registered && strstr(path,"gameplay_keep"));
    return &fairySkeleton;
}
extern "C" AnimationHeader* ResourceMgr_LoadAnimByName(const char* path) { assert(strstr(path,"gameplay_keep")); return &fairyAnimation; }
s16 Animation_GetLastFrame(AnimationHeader* animation) { return animation->lastFrame; }
void SkelAnime_Init(PlayState*,SkelAnime* skel,SkeletonHeader* skeleton,AnimationHeader*,Vec3s* joints,Vec3s*,s32 count) {
    assert(skeleton->limbCount+1 == count); skel->jointTable = joints;
}
void Animation_Change(SkelAnime*,AnimationHeader*,float speed,float start,float,int,float) {
    assert(speed == 0); animationFrame = (int)start;
}
template <typename Override>
Gfx* SkelAnime_Draw(PlayState* play,void**,Vec3s*,Override limb,void*,void*,Gfx* gfx) {
    assert(gpuOwner==fairyHost && "host fairy limbs were submitted under the foreign bottle's resource owner and disappear");
    ++skeletonDraws;
    Matrix_Push();
#ifdef COMBO_FAIRY_HOST_MM
    limb(play,6,nullptr,nullptr,nullptr,nullptr,&gfx);
#else
    limb(play,8,nullptr,nullptr,nullptr,nullptr,&gfx);
#endif
    assert(!current.billboard && "native fairy core DL owns the billboard rotation");
    Matrix_Pop();
    LoadMatrix(gfx,&current); DrawList(gfx,"fairy-vfx");
    return gfx+1;
}
template <typename Override>
Gfx* SkelAnime_DrawFlex(PlayState* play,void** skeleton,Vec3s* joints,int,Override limb,void*,void*,Gfx* gfx) {
    return SkelAnime_Draw(play,skeleton,joints,limb,nullptr,nullptr,gfx);
}
Gfx materialCommands[5];
void* Graph_Alloc(GraphicsContext*,size_t size) { assert(size == sizeof(materialCommands)); return materialCommands; }
constexpr int ANIMMODE_LOOP=0,G_RM_PASS=0,G_RM_ZB_CLD_SURF2=0;
#define GRAPH_ALLOC(ctx,size) Graph_Alloc(ctx,size)
#define gDPPipeSync(...) ((void)0)
#define gDPSetPrimColor(p,lod,min,r,g,b,a) (fairyPrim[0]=r,fairyPrim[1]=g,fairyPrim[2]=b,fairyPrim[3]=a)
#define gDPSetEnvColor(p,r,g,b,a) ((void)(p),fairyEnv[0]=r,fairyEnv[1]=g,fairyEnv[2]=b,fairyEnv[3]=a)
#define gDPSetRenderMode(...) ((void)0)
#define gSPEndDisplayList(...) ((void)0)
#define Gfx_SetupDL27_Xlu(...) events.push_back("fairy-setup")
#define Gfx_SetupDL_27Xlu(...) events.push_back("fairy-setup")
#include "combo/menu/ComboFairyBottleDraw.h"
#define xw x
#define yw y
#define zw z
#include "fairy_production.inc"
#undef xw
#undef yw
#undef zw

bool Near(float x, float y) { return std::abs(x - y) < .0001f; }
bool Same(const MtxF& a, const MtxF& b) {
    return Near(a.x,b.x) && Near(a.y,b.y) && Near(a.z,b.z) && Near(a.sx,b.sx) && Near(a.sy,b.sy) &&
           Near(a.sz,b.sz) && a.billboard == b.billboard;
}
extern "C" void ComboBottleShimmer_DrawMotes(PlayState* p,int profile) {
    assert(p==&play && profile==CW_SHIMMER_FAIRY && Same(current,initial));
    ++moteDraws;
}
void Reset(uint32_t frame) {
    gpuOwner=fairyHost; gpuOwners.clear();
    play.gameplayFrames = frame; play.state.frames = 999; // dormant-owner clock cannot drive the motion
    current = initial = {10,20,30,2,2,2};
    draws.clear(); events.clear(); stack.clear(); foreignRestores = hostFallbacks = skeletonDraws = shimmerDraws = moteDraws = 0;
    opaPtr = opa; xluPtr = xlu;
    for (auto& command : opa) command.stream = 0;
    for (auto& command : xlu) command.stream = 1;
}
void CheckDraw(float contentsY, bool foreign) {
    assert(draws.size() == 3 && draws[0].stream == 0 && draws[1].stream == 1 && draws[2].stream == 1);
    assert(draws[0].path == genericOpaque && draws[1].path == genericGlass);
    assert(draws[2].path == fairy && Same(draws[0].pose, initial) && Same(draws[1].pose, initial));
    auto expected = ComboFairyBottle_Sample(play.gameplayFrames);
    const auto& content = draws[2].pose;
    assert(Near(content.x, initial.x + expected.x) && Near(content.y, initial.y + contentsY + expected.y));
    assert(Near(content.z, initial.z + expected.z) && content.billboard);
    assert(Near(content.sx, 2 * (contentsY ? .01f : 1.f) * expected.scaleX) && Near(content.sy, 2 * (contentsY ? .01f : 1.f) * expected.scaleY));
    assert(Near(content.sz, 2 * (contentsY ? .01f : 1.f) * expected.scaleZ) && Same(current, initial) && stack.empty());
    assert(foreignRestores == (foreign ? 1 : 0) && hostFallbacks == 0);
}

int main() {
    for (int mask = 0; mask < 32; ++mask) {
        selectedMods.clear();
        const char* paths[] = {opaque, glass, genericOpaque, genericGlass};
        for (int i = 0; i < 4; ++i) if (mask & (1 << i)) selectedMods.insert(paths[i]);
        if (mask & 16) selectedMods.insert(blueFire);
        for (auto shell : {oot::GetItem_FairyBottleShell(0), mm::GetItem_FairyBottleShell(0)}) {
            assert(shell.opaque == genericOpaque && shell.glass == genericGlass &&
                   "bottled fairy must use the empty-bottle casing, even with selected Blue Fire/fairy models");
        }
    }
    selectedMods.clear();
    customModels={blueFire};
    assert(oot::GetItem_FairyBottleShell(0).opaque == genericOpaque &&
           mm::GetItem_FairyBottleShell(0).opaque == genericOpaque &&
           "built-in Blue Fire model must not inject its casing or embedded flame into the fairy");
    customModels.clear();
    bundledShell=true;
    Reset(27);
    oot::GetItem_DrawFairy(&play, 0);
    CheckReadableGlass(draws[1].owner, draws[1].path);
    assert(strstr(oot::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/FairyXlu") &&
           strstr(mm::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/FairyXlu") &&
           "fresh build with no optional bottle pack must still use the bundled TP casing");
    selectedMods={glass};
    assert(strstr(oot::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/FairyXlu") &&
           strstr(mm::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/FairyXlu"));
    selectedMods={genericGlass};
    assert(oot::GetItem_FairyBottleShell(0).glass==genericGlass && mm::GetItem_FairyBottleShell(0).glass==genericGlass);
    assert(oot::GetItem_FairyBottleShell(0).opaque==oot::GetItem_EmptyBottleShell(GID_BOTTLE).opaque &&
           mm::GetItem_FairyBottleShell(0).glass==mm::GetItem_EmptyBottleShell(GID_BOTTLE).glass);
    const char* selectedShells[] = { genericOpaque, genericGlass, "__OTR__objects/combo_bottle_gi/EmptyOpaque",
                                     "__OTR__objects/combo_bottle_gi/EmptyXlu" };
    for (const char* selected : selectedShells) {
        selectedMods = { selected };
        const auto ootEmpty = oot::GetItem_EmptyBottleShell(GID_BOTTLE),
                   mmEmpty = mm::GetItem_EmptyBottleShell(GID_BOTTLE);
        assert(!strcmp(oot::GetItem_FairyBottleShell(0).opaque, ootEmpty.opaque) &&
               !strcmp(oot::GetItem_FairyBottleShell(0).glass, ootEmpty.glass) &&
               !strcmp(mm::GetItem_FairyBottleShell(0).opaque, mmEmpty.opaque) &&
               !strcmp(mm::GetItem_FairyBottleShell(0).glass, mmEmpty.glass) &&
               "selected casing replacements must keep both passes exact");
    }
    const char* selectedMaterials[] = {
        "__OTR__objects/combo_bottle_gi/shell/mat_gGiBlueFireChamberstickDL_BottleGlassOutside",
        "__OTR__objects/combo_bottle_gi/BottleShell"
    };
    for (const char* selected : selectedMaterials) {
        printf("SELECTED SHARED ASSET %s\n", selected);
        fflush(stdout);
        selectedMods = { selected };
        AssetOverrideMap overrides;
        for (const char* owner : { "oot", "mm" }) {
            const auto key = std::string(owner) + ":" + (selected + 7);
            auto commands = assetCommandLists.at(key);
            auto insertion = commands.begin();
            for (; insertion != commands.end(); ++insertion) {
                if (insertion->tag == "EndDisplayList" ||
                    (insertion->tag == "CallDisplayList" &&
                     insertion->attributes.at("Path").ends_with("gGiBlueFireChamberstickDL_tri_1")))
                    break;
            }
            // Represent an explicitly selected material with a distinct live
            // color; observe it after the production GPU path resolves the root.
            const AssetCommand color = {
                "SetPrimColor",
                { { "M", "0" }, { "L", "0" }, { "R", "24" }, { "G", "160" }, { "B", "210" }, { "A", "144" } }
            };
            insertion = commands.insert(insertion, color);
            commands.insert(insertion, { "PipeSync", {} });
            overrides[key] = commands;
        }
        for (int route = 0; route < 6; ++route) {
            const auto shell = route == 4 ? oot::GetItem_FairyBottleShell(0) : mm::GetItem_FairyBottleShell(0);
            ComboForeignDrawInfo modInfo;
            modInfo.dls[0] = shell.opaque;
            modInfo.dls[1] = shell.glass;
            Reset(27);
            if (route == 0)
                oot::GetItem_DrawFairy(&play, 0);
            else if (route == 1)
                mm::GetItem_DrawFairyBottle(&play, 1);
            else if (route == 2)
                mm::GetItem_DrawFairyContainer(&play, 0);
            else if (route == 3)
                OOT_DrawForeignFairyContainer(&play, &modInfo);
            else if (route == 4)
                MM_DrawForeignFairy(&modInfo);
            else
                OOT_DrawForeignFairyBottle(&play, &modInfo);
            const auto trace = TraceGlass(draws[1].owner, draws[1].path, &overrides);
            size_t triangles = 0;
            for (const auto& triangle : trace.triangles) {
                if (!triangle.path.ends_with("gGiBlueFireChamberstickDL_tri_1"))
                    continue;
                ++triangles;
                assert(triangle.material.at("SetPrimColor.A") == "144" &&
                       triangle.material.at("SetPrimColor.R") == "24" &&
                       triangle.material.at("SetPrimColor.G") == "160" &&
                       triangle.material.at("SetPrimColor.B") == "210" &&
                       "fairy wrapper bypassed or recolored the selected shared shell/material");
            }
            assert(triangles > 0 && draws[1].path == "__OTR__objects/combo_bottle_gi/EmptyXlu");
            assert(Same(current, initial) && stack.empty() && gpuOwners.empty());
        }
    }
    const char* selectedChildren[] = { "__OTR__objects/combo_bottle_gi/shell/BottleGlass",
                                       "__OTR__objects/combo_bottle_gi/shell/MetalPainted_1TestFromSkilar",
                                       "__OTR__objects/combo_bottle_gi/shell/gGiBlueFireChamberstickDL_vtx_0",
                                       "__OTR__objects/combo_bottle_gi/shell/gGiBlueFireChamberstickDL_vtx_1" };
    for (const char* selected : selectedChildren) {
        selectedMods = { selected };
        assert(strstr(oot::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/FairyXlu") &&
               strstr(mm::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/FairyXlu") &&
               "texture and vertex replacements must retain the shared child paths");
    }
    puts("PASS selected shared shell/material GPU commands and live mod color in all six routes; texture/vertex-only "
         "replacements retain fairy glass");
    selectedMods.clear();
    readableGlass = false;
    assert(strstr(oot::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/EmptyXlu") &&
           strstr(mm::GetItem_FairyBottleShell(0).glass, "combo_bottle_gi/EmptyXlu") &&
           "older archives without readable glass need the accepted bundled fallback");
    readableGlass = true;
    assert(strstr(oot::GetItem_EmptyBottleShell(GID_BOTTLE).glass, "combo_bottle_gi/EmptyXlu") &&
           strstr(mm::GetItem_EmptyBottleShell(GID_BOTTLE).glass, "combo_bottle_gi/EmptyXlu"));
    selectedMods.clear();bundledShell=false;
    for (uint32_t frame = 0; frame < 360; ++frame) {
        auto motion = ComboFairyBottle_Sample(frame), repeat = ComboFairyBottle_Sample(frame + 360);
        assert(std::abs(motion.x) <= .65001f && std::abs(motion.y) <= 1.15001f && std::abs(motion.z) <= .35001f);
        assert(motion.scaleX >= .73999f && motion.scaleX <= .82001f && Near(motion.scaleY,.78f) && Near(motion.scaleZ,.78f));
        assert(Near(motion.x,repeat.x) && Near(motion.y,repeat.y) && Near(motion.z,repeat.z));
    }
    assert(!Near(ComboFairyBottle_Sample(0).x, ComboFairyBottle_Sample(27).x));
    assert(!Near(ComboFairyBottle_Sample(0).scaleX, ComboFairyBottle_Sample(1).scaleX));
    puts("PASS empty casing for all 32 Blue Fire/fairy/empty mod combinations and full 360-frame motion envelope");
    for (uint32_t frame : {0u,1u,27u}) {
        auto motion = ComboFairyBottle_Sample(frame);
        printf("fairy frame=%u offset=(%.5f,%.5f,%.5f) scale=(%.5f,%.5f,%.5f)\n", frame,
               motion.x,motion.y,motion.z,motion.scaleX,motion.scaleY,motion.scaleZ);
    }

    void* resources[8]{};
    int xluStart = -1, scroll = 0, kind = -1; float scale = 0; u8 colors[16]{};
    for (int generic = 0; generic < 2; ++generic) {
        selectedMods.clear(); if (generic) selectedMods.insert(genericGlass);
        assert(oot::GetItem_GetDrawTableEntry(0,resources,8,&xluStart,&scale,&kind,colors) == 3);
        assert(kind == CW_DRAW_KIND_FAIRY && xluStart == 1 && resources[2] == fairy);
        assert(resources[0] == genericOpaque);
        for (int capacity = 0; capacity < 3; ++capacity)
            assert(oot::GetItem_GetDrawTableEntry(0,resources,capacity,&xluStart,&scale,&kind,colors) == 0);
        assert(mm::GetItem_GetDrawTableEntry(0,resources,8,&xluStart,&scale,&scroll,&kind) == 4);
        assert(kind == CW_DRAW_KIND_MM_FAIRY_CONTAINER && xluStart == 1 && resources[2] == fairy && resources[3] == matrixPath);
        assert(resources[0] == genericOpaque);
        for (int capacity = 0; capacity < 4; ++capacity)
            assert(mm::GetItem_GetDrawTableEntry(0,resources,capacity,&xluStart,&scale,&scroll,&kind) == 0);
        for (uint32_t frame : {0u,27u}) {
            Reset(frame); oot::GetItem_DrawFairy(&play,0); CheckDraw(0,false);
            Reset(frame); mm::GetItem_DrawFairyBottle(&play,1); CheckDraw(0,false);
            Reset(frame); mm::GetItem_DrawFairyContainer(&play,0); CheckDraw(nativePlacement.y,false);
        }
    }
    assert(CwMinDlistsForKind(CW_DRAW_KIND_MM_FAIRY_CONTAINER) == 4);
    puts("PASS native OoT/MM shell exports, complete MM contents+matrix recipe, stable shell and moving fairy command streams");

    bundledShell = true;
    selectedMods.clear();
    for (bool available : { true, false }) {
        readableGlass = available;
        const char* expected =
            available ? "__OTR__objects/combo_bottle_gi/FairyXlu" : "__OTR__objects/combo_bottle_gi/EmptyXlu";
        assert(oot::GetItem_GetDrawTableEntry(0, resources, 8, &xluStart, &scale, &kind, colors) == 3);
        assert(!strcmp((const char*)resources[1], expected));
        assert(mm::GetItem_GetDrawTableEntry(0, resources, 8, &xluStart, &scale, &scroll, &kind) == 4);
        assert(!strcmp((const char*)resources[1], expected));
        assert(mm::GetItem_GetDrawTableEntry(1, resources, 8, &xluStart, &scale, &scroll, &kind) == 3);
        assert(!strcmp((const char*)resources[1], expected));
        assert(oot::GetItem_GetDrawTableEntry(GID_BOTTLE, resources, 8, &xluStart, &scale, &kind, colors) == 2);
        assert(!strcmp((const char*)resources[1], "__OTR__objects/combo_bottle_gi/EmptyXlu"));
        assert(mm::GetItem_GetDrawTableEntry(GID_BOTTLE, resources, 8, &xluStart, &scale, &scroll, &kind) == 2);
        assert(!strcmp((const char*)resources[1], "__OTR__objects/combo_bottle_gi/EmptyXlu"));
    }
    readableGlass = true;
    bundledShell = false;
    puts("PASS fairy-only glass exports, exact selected shells and missing-new-asset fallback; ordinary empty bottles "
         "unchanged");

    selectedMods.clear(); ComboForeignDrawInfo info;
    for (uint32_t frame : {0u,27u}) {
        Reset(frame); OOT_DrawForeignFairyBottle(&play,&info); CheckDraw(0,true);
        Reset(frame); MM_DrawForeignFairy(&info); CheckDraw(0,true);
        Reset(frame); OOT_DrawForeignFairyContainer(&play,&info); CheckDraw(nativePlacement.y,true);
        assert(events.front() == "mat" && events.back() == "restore");
    }
    puts("PASS both foreign directions: active-host motion, OPA shell/XLU glass+fairy order, native placement and state restoration");
    fairyResources = true;
    selectedMods = {blueFire};
    for (uint32_t frame : {0u,1u,27u}) {
        Reset(frame); oot::GetItem_DrawFairy(&play,0);
        assert(draws.size() == 3 && draws[0].path == genericOpaque && draws[1].path == genericGlass && draws[2].path == "fairy-vfx");
        assert(skeletonDraws == 1 && animationFrame == (int)frame && Same(current,initial) && stack.empty());
        assert(fairyPrim[0] == 255 && fairyPrim[1] == 160 && fairyPrim[2] == 235);
        assert(fairyEnv[0] == 255 && fairyEnv[1] == 160 && fairyEnv[2] == 235);
        auto motion = ComboFairyBottle_Sample(frame);
        assert(Near(draws[2].pose.x,initial.x+motion.x) && Near(draws[2].pose.y,initial.y+motion.y));
        assert(Near(draws[2].pose.sx,2*motion.scaleX*.006f));
        Reset(frame); mm::GetItem_DrawFairyContainer(&play,0);
        assert(draws.size() == 3 && draws[0].path == genericOpaque && draws[1].path == genericGlass && draws[2].path == "fairy-vfx");
        info.dls[0]=genericOpaque; info.dls[1]=genericGlass;
        Reset(frame); OOT_DrawForeignFairyContainer(&play,&info);
        assert(draws.size() == 3 && draws[0].path == genericOpaque && draws[1].path == genericGlass && draws[2].path == "fairy-vfx" && foreignRestores == 1);
        Reset(frame); MM_DrawForeignFairy(&info);
        assert(draws.size() == 3 && draws[0].path == genericOpaque && draws[1].path == genericGlass && draws[2].path == "fairy-vfx" && foreignRestores == 1);
    }
    // The native contents billboard can include its own tiny sprite scale.
    // A real actor-derived fairy needs the bottle GI scale, not that scale twice.
    selectedMods.clear();
    Reset(27); mm::GetItem_DrawFairyContainer(&play,0);
    assert(draws.back().path == "fairy-vfx");
    assert(Near(draws.back().pose.sx,2*ComboFairyBottle_Sample(27).scaleX*.006f) &&
           "actor fairy inherited native billboard sprite scale and vanished");
    info.dls[0]=genericOpaque;info.dls[1]=genericGlass;
    Reset(27); OOT_DrawForeignFairyContainer(&play,&info);
    assert(Near(draws.back().pose.sx,2*ComboFairyBottle_Sample(27).scaleX*.006f));
    selectedMods={blueFire};
    fairyResources = false; Reset(27); oot::GetItem_DrawFairy(&play,0);
    assert(draws.size() == 3 && draws[0].path == genericOpaque && draws[1].path == genericGlass && draws[2].path == fairy && skeletonDraws == 0);
    selectedMods = {blueFire,glass};
    assert(oot::GetItem_FairyBottleShell(0).opaque == genericOpaque && mm::GetItem_FairyBottleShell(0).opaque == genericOpaque);
    puts("PASS empty shell despite selected Blue Fire, larger pink animated fairy, native contents fallback");
    selectedMods.clear(); bundledShell=true; fairyResources=true;
    for (int route=0;route<6;++route) {
        const auto shell = route == 4 ? oot::GetItem_FairyBottleShell(0) : mm::GetItem_FairyBottleShell(0);
        info.dls[0] = shell.opaque;
        info.dls[1] = shell.glass;
        MtxF first;
        for(uint32_t frame:{0u,27u}) {
            Reset(frame);
            if(route==0)oot::GetItem_DrawFairy(&play,0);
            else if(route==1)mm::GetItem_DrawFairyBottle(&play,1);
            else if(route==2)mm::GetItem_DrawFairyContainer(&play,0);
            else if(route==3)OOT_DrawForeignFairyContainer(&play,&info);
            else if(route==4)MM_DrawForeignFairy(&info);
            else OOT_DrawForeignFairyBottle(&play,&info);
            assert(draws[1].stream == 1 && draws[1].path == shell.glass);
            assert(draws[1].owner == (route == 3 || route == 5 ? "mm" : route == 4 ? "oot" : fairyHost));
            CheckReadableGlass(draws[1].owner, draws[1].path);
            const auto& pose=draws.back().pose;
            assert(draws.back().path=="fairy-vfx" && skeletonDraws==1);
            assert(moteDraws==1 && "accepted bundled fairy requires four local deterministic motes once at the incoming bottle pose");
            assert(Near(pose.sx,2*ComboFairyBottle_SampleForShell(frame,info.dls[0]).scaleX*.012f) && "bundled fairy must increase its local draw scale by 50 percent");
            assert(Same(draws[0].pose,initial) && Same(draws[1].pose,initial) &&
                   "enlarging the fairy must preserve the physical bottle scale");
            assert(pose.y>=initial.y-17.001f && pose.y<=initial.y-2.999f &&
                   "bundled TP casing needs the fairy centered inside its body, without the vanilla sprite anchor");
            assert(std::abs(pose.x-initial.x)<=4.001f && std::abs(pose.z-initial.z)<=2.001f);
            assert(gpuOwners.empty() && Same(current,initial) && stack.empty());
            if(frame==0)first=pose;
            else assert(std::hypot(pose.x-first.x,pose.y-first.y)>3.f && "bundled fairy bounce is too small to read");
        }
    }
    puts("PASS bundled fairy casing: readable alpha80 at actual submitted shell triangles, unchanged "
         "reflection/geometry/cleanup, centered bounce and correct GPU ownership in all native/foreign routes");
    bundledShell=false; fairyResources=false;
    selectedMods.clear(); info.dls[0]=opaque; info.dls[1]=glass;
    selectedForeign = &info; GetItemEntry entry;
    for (int missing = 0; missing < 6; ++missing) {
        Reset(27);
        ownerPresent = missing != 0; matrixPresent = missing != 1; matrixPointerPresent = missing != 2;
        materialPresent = missing != 3;
        info.dls[3] = missing == 4 ? "__OTR__@oot:matrix" : "__OTR__@mm:matrix";
        info.count = missing == 5 ? 3 : 4;
        OOT_DrawForeignFairyContainer(&play,&info);
        assert(hostFallbacks == 1 && draws.size()==3 && shimmerDraws==1 && stack.empty() && Same(current,initial) && ownerScope == 1);
        for (bool overlay : {true,false}) {
            info.itemShimmer=overlay;
            Reset(27); OOT_DrawComboForeign(&play,&entry);
            assert(hostFallbacks == 1 && draws.size()==3 && stack.empty() && Same(current,initial));
            assert(shimmerDraws==1 && "outer foreign dispatcher duplicated the native fallback's pink overlay");
        }
    }
    ownerPresent=matrixPresent=matrixPointerPresent=materialPresent=true;
    info.dls[3]="__OTR__@mm:matrix"; info.count=4; info.itemShimmer=true;
    Reset(27); OOT_DrawComboForeign(&play,&entry);
    assert(hostFallbacks==0 && shimmerDraws==1 && draws.size()==3 && stack.empty() && Same(current,initial));
    bundledShell=fairyResources=true;
    const auto bundledMmShell = mm::GetItem_FairyBottleShell(0);
    info.dls[0] = bundledMmShell.opaque;
    info.dls[1] = bundledMmShell.glass;
    for(bool overlay:{true,false}) {
        info.itemShimmer=overlay;Reset(27);OOT_DrawComboForeign(&play,&entry);
        assert(hostFallbacks==0 && skeletonDraws==1 && moteDraws==1 && shimmerDraws==int(overlay));
        assert(stack.empty() && Same(current,initial) && gpuOwners.empty());
    }
    puts("PASS real foreign dispatcher + native fallback: exactly one pink overlay for missing resources and complete recipes");
}
