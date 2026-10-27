// Regression: dropping the adult sheath's original DL must not drop its flex
// matrix when the final equipment policy hides the back. Head/shoulder vertices
// still address the canonical torso at segment 0x0D + 0x440.
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include "2s2h/Rando/NeiArticulatedPresentation.h"
#include "2s2h/Rando/NeiLanternPresentation.h"
extern "C" {
#include "global.h"
#include "din_fire_sword.h"
#include "din_fire_shield.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/weapon_upgrades.h"
}

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); std::exit(1); \
} } while (0)
#define ALB(sym) "__OTR__objects/object_link_boy/" sym
#define ADULT_LINK_LIMB_COUNT 21
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)

struct ArchiveLimb { Vec3s position; u8 child, sibling; const char* near; const char* far; };
struct MatrixConsumer { const char* path; unsigned offset; };
#include "archive_fixture.inc"

static LodLimb sLimbs[21];
static void* sLimbPointers[21];
static FlexSkeletonHeader sArchiveSkeleton;
static std::map<std::string, Gfx> sMeshes;
static std::map<const Mtx*, s32> sMatrixWriters;
static unsigned sMatrixConversions;
static s32 sActiveLimb;
static bool sAlt, sMod, sHideBack;
static Gfx* sFinalSheath;
static Gfx* sFinalLeftHand;
static Gfx* sFinalRightHand;
static unsigned sMissingMeshLoads;

static FlexSkeletonHeader* sSkel;
static u8 sReady, sSetupFailedPermanently, sIsMod, sIsForm, sIsChildRig;
static std::string sRigBase, sModEyePath[PLAYER_EYES_MAX], sModMouthPath[PLAYER_MOUTH_MAX];
static void* sModEye[PLAYER_EYES_MAX];
static void* sModMouth[PLAYER_MOUTH_MAX];
static const char* sOotEyeName[PLAYER_EYES_MAX] = {};
static const char* sOotMouthName[PLAYER_MOUTH_MAX] = {};
static const char* sOotChildEyeName[PLAYER_EYES_MAX] = {};
static const char* sOotChildMouthName[PLAYER_MOUTH_MAX] = {};

static std::string Clean(const char* path) {
    if (std::strncmp(path, "__OTR__", 7) == 0) path += 7;
    if (std::strncmp(path, "alt/", 4) == 0) path += 4;
    return path;
}

static Gfx* Mesh(const char* path) {
    const auto found = sMeshes.find(Clean(path));
    if (found == sMeshes.end()) {
        std::fprintf(stderr, "Unknown or missing fixture display list: %s\n", path);
        ++sMissingMeshLoads;
        return nullptr;
    }
    return &found->second;
}

extern "C" {
SaveContext gSaveContext;
u16 gEquipMasks[] = { 0xF, 0xF0, 0xF00, 0xF000 };
u8 gEquipShifts[] = { 0, 4, 8, 12 };
extern MtxF* sMatrixStack;
extern MtxF* sCurrentMatrix;
s32 AdultLink_IsActive(void) { return 1; }
bool ResourceMgr_IsAltAssetsEnabled(void) { return sAlt; }
u8 ResourceMgr_FileExists(const char* path) {
    if (!sMod || std::strstr(path, "alt/") == nullptr) return 0;
    const std::string clean = Clean(path);
    return clean == "objects/object_link_boy/gLinkAdultSkel" || sMeshes.find(clean) != sMeshes.end();
}
Gfx* ResourceMgr_LoadGfxByName(const char* path) { return Mesh(path); }
char* ResourceMgr_LoadTexOrDListByName(const char*) { return nullptr; }
char* ResourceMgr_LoadIfDListByName(const char*) { return nullptr; }
int ResourceMgr_OTRSigCheck(char* data) { return data && std::memcmp(data, "__OTR__", 7) == 0; }
SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime*) {
    return sMod && Clean(path) == "objects/object_link_boy/gLinkAdultSkel" ? &sArchiveSkeleton.sh : nullptr;
}
void* MmAssets_LoadFromOotArchive(const char* path, size_t*) {
    return Clean(path) == "objects/object_link_boy/gLinkAdultSkel" ? &sArchiveSkeleton : nullptr;
}
void* OotAssets_LoadGfxDirect(const char* path) { return Mesh(path); }
s32 CVarGetInteger(const char* path, s32 value) {
    return std::strcmp(path, "gEnhancements.HideBackEquipment") == 0 ? sHideBack : value;
}
s32 CustomForms_ActiveForm(void) { return CUSTOM_FORM_NONE; }
const char* CustomForms_BasePath(void) { return nullptr; }
f32 CustomForms_RootScale(void) { return 1.0f; }
f32 CustomForms_RootDropBefore(void) { return 0.0f; }
f32 CustomForms_RootDropAfter(void) { return 0.0f; }
Gfx* CustomForms_HandDL(Player*, s32, u8* claimed) { *claimed = 0; return nullptr; }
u8 CustomForms_HidesSheath(Player*) { return 0; }
const char* ExtEquip_GetShieldDLOverride(void) { return nullptr; }
u8 ExtEquip_ShouldHideSwordDL(void) { return 0; }
u8 FourSword_IsEquipped(void) { return 0; }
u8 Trident_GoldenArmor(void) { return 0; }
s32 BossRemains_IsOdolwaWorn(void) { return 0; }
s32 BossRemains_IsGohtWorn(void) { return 0; }
u8 WeaponUpgrade_ApplyHeldSwordDL(Gfx**, void*, Player*, u8, u8, u8) { return 0; }
void* DinFireSword_HandDL(PlayState*, Player*, void*) { return nullptr; }
void* DinFireShield_HandDL(PlayState*, Player*, void*) { return nullptr; }
void DinFireSword_SetHandModel(Player*, void*, int) {}
void DinFireSword_CopyHandModel(Player*, void*, void*) {}
void Player_ApplyBackEquipmentVisibility(s32, Gfx**);
// Gameplay pose/IK is outside this slot-allocation test. Adult's full override
// still runs against this native equipment boundary and the real hide policy.
s32 Player_OverrideLimbDrawGameplayDefault(PlayState*, s32, Gfx**, Vec3f*, Vec3s*, Actor*) { return 0; }
void* Lib_SegmentedToVirtual(void* ptr) { return ptr; }
f32 Math_SinS(s16 angle) { return std::sin(angle * 3.14159265358979323846f / 32768.0f); }
f32 Math_CosS(s16 angle) { return std::cos(angle * 3.14159265358979323846f / 32768.0f); }
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild(void) {}
void FrameInterpolation_RecordMatrixPush(void) {}
void FrameInterpolation_RecordMatrixPop(void) {}
void FrameInterpolation_RecordMatrixPut(MtxF*) {}
void FrameInterpolation_RecordMatrixTranslateRotateZYX(Vec3f*, Vec3s*) {}
void FrameInterpolation_RecordMatrixToMtx(Mtx* dest, char*, s32) {
    sMatrixWriters[dest] = sActiveLimb;
    ++sMatrixConversions;
}
}
bool NeiArticulated_ApplySwitchHookHand(PlayState*, Player*, Gfx**, Gfx*) { return false; }
bool NeiLantern_UsesGrip(const Player*) { return false; }

#include "adult_production.inc"

static s32 DrawLimb(PlayState* play, s32 index, Gfx** dl, Vec3f* pos, Vec3s* rot, Actor* actor) {
    sActiveLimb = index;
    s32 result = AdultLink_OverrideLimb(play, index, dl, pos, rot, actor);
    if (index == PLAYER_LIMB_SHEATH) sFinalSheath = *dl;
    if (index == PLAYER_LIMB_LEFT_HAND) sFinalLeftHand = *dl;
    if (index == PLAYER_LIMB_RIGHT_HAND) sFinalRightHand = *dl;
    return result;
}

static void ResetModel(void) {
    sSkel = nullptr;
    sReady = sSetupFailedPermanently = sIsMod = sIsForm = sIsChildRig = 0;
    sRigBase.clear();
    sMeshes.clear();
    sMissingMeshLoads = 0;
    for (const char* path : kArchiveDisplayLists) {
        Gfx mesh = {};
        mesh.words.w0 = G_ENDDL << 24;
        sMeshes.emplace(path, mesh);
    }
    for (unsigned index = 0; index < 21; ++index) {
        const ArchiveLimb& limb = kArchiveLimbs[index];
        sLimbs[index] = { limb.position, limb.child, limb.sibling,
                         { *limb.near ? Mesh(limb.near) : nullptr, *limb.far ? Mesh(limb.far) : nullptr } };
        sLimbPointers[index] = &sLimbs[index];
    }
    sArchiveSkeleton = { { sLimbPointers, 21 }, kArchiveMatrixCount };
    // Setup dereferences canonical eye/mouth name tables even if textures are
    // absent. No texture pixels or texture lookup behavior is tested here.
    for (unsigned index = 0; index < PLAYER_EYES_MAX; ++index)
        sOotEyeName[index] = sOotChildEyeName[index] = "unused-eye";
    for (unsigned index = 0; index < PLAYER_MOUTH_MAX; ++index)
        sOotMouthName[index] = sOotChildMouthName[index] = "unused-mouth";
    CHECK(AdultLink_Setup());
    CHECK(sMissingMeshLoads == 0);
}

static bool RunFrame(bool hidden, s32 sword, s32 hand, s32 shield, bool mirror, s32 lod) {
    alignas(16) std::array<Gfx, 4096> opa;
    std::memset(opa.data(), 0xCD, sizeof(opa));
    Gfx xlu[64] = {}, overlay[64] = {};
    GraphicsContext gfx = {};
    gfx.polyOpa.p = opa.data();
    gfx.polyOpa.d = opa.data() + opa.size();
    gfx.polyXlu.p = xlu;
    gfx.overlay.p = overlay;
    PlayState play = {};
    play.state.gfxCtx = &gfx;
    Player player = {};
    player.transformation = PLAYER_FORM_HUMAN;
    player.actor.scale.y = mirror ? -0.01f : 0.01f;
    player.leftHandType = hand;
    player.rightHandType = shield ? PLAYER_MODELTYPE_RH_SHIELD : PLAYER_MODELTYPE_RH_OPEN;
    player.currentShield = shield;
    player.sheathType = PLAYER_MODELTYPE_SHEATH_14;
    gSaveContext.save.saveInfo.equips.equipment = sword;
    Vec3s joints[22] = {};
    MtxF stack[32] = {};
    guMtxIdentF(stack[0].mf);
    sMatrixStack = sCurrentMatrix = stack;
    sMatrixWriters.clear();
    sMatrixConversions = 0;
    sHideBack = hidden;
    sFinalSheath = sFinalLeftHand = sFinalRightHand = nullptr;
    SkelAnime_DrawFlexLod(&play, sSkel->sh.segment, joints, sSkel->dListCount,
                         DrawLimb, nullptr, &player.actor, lod);
    // Read the producer's actual segment command, then resolve the archive's
    // addresses against the same matrix base the interpreter receives.
    Mtx* base = nullptr;
    unsigned segmentBindings = 0;
    for (const Gfx* command = opa.data(); command < gfx.polyOpa.p; ++command) {
        const uintptr_t w0 = command->words.w0;
        if ((w0 >> 24) == G_MOVEWORD && ((w0 >> 16) & 0xFF) == G_MW_SEGMENT &&
            (w0 & 0xFFFF) == 0x0D * 4) {
            base = reinterpret_cast<Mtx*>(command->words.w1);
            ++segmentBindings;
        }
    }
    CHECK(segmentBindings == 1 && base == reinterpret_cast<Mtx*>(gfx.polyOpa.d));
    bool passed = true;
    const auto torso = sMatrixWriters.find(base + 17);
    const s32 torsoWriter = torso == sMatrixWriters.end() ? 0 : torso->second;
    if (sMatrixConversions != 18 || torsoWriter != PLAYER_LIMB_TORSO) {
        std::fprintf(stderr,
                     "FAIL adult flex alt=%d mod=%d hideBack=%d mirror=%d lod=%d sword=%d hand=%d shield=%d: "
                     "wrote %u/18 matrices; canonical torso slot 17 writer=%d (expected limb %d)\n",
                     sAlt, sMod, hidden, mirror, lod, sword, hand, shield, sMatrixConversions,
                     torsoWriter, PLAYER_LIMB_TORSO);
        passed = false;
    }
    for (const MatrixConsumer& consumer : kConsumers) {
        CHECK(consumer.offset % sizeof(Mtx) == 0);
        CHECK(consumer.offset + sizeof(Mtx) <= kArchiveMatrixCount * sizeof(Mtx));
        if (sMatrixWriters.find(base + consumer.offset / sizeof(Mtx)) == sMatrixWriters.end()) {
            std::fprintf(stderr, "FAIL %s reads unwritten segment-0D offset 0x%X\n", consumer.path, consumer.offset);
            passed = false;
        }
    }
    CHECK((sFinalSheath == nullptr) == hidden);
    CHECK(sFinalLeftHand != nullptr && sFinalRightHand != nullptr);
    CHECK(sCurrentMatrix == stack); // Native traversal balances its matrix stack.
    return passed;
}

int main(void) {
    static_assert(sizeof(Mtx) == 64);
    bool passed = true;
    unsigned cases = 0;
    for (bool alt : { false, true }) for (bool mod : { false, true }) {
        if (mod && !alt) continue;
        sAlt = alt;
        sMod = mod;
        ResetModel();
        // Multiple equipment states retain the exact canonical 18-slot
        // layout. The final visibility setting may suppress geometry only.
        for (bool hidden : { false, true }) for (s32 lod : { 0, 1 })
            for (bool mirror : { false, true }) for (s32 sword : { 0, 1 })
                for (s32 hand : { PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_LH_ONE_HAND_SWORD,
                                  PLAYER_MODELTYPE_LH_TWO_HAND_SWORD })
                    for (s32 shield : { PLAYER_SHIELD_NONE, PLAYER_SHIELD_HEROS_SHIELD, PLAYER_SHIELD_MIRROR_SHIELD }) {
                        passed = RunFrame(hidden, sword, hand, shield, mirror, lod) && passed;
                        ++cases;
                    }
    }
    if (!passed) return 1;
    std::printf("PASS %u adult MM native flex frames: canonical slots, consumer bounds, equipment visibility, "
                "Alt fallback/mod, mirror, both LODs\n", cases);
}
