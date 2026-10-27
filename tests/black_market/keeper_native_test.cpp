#include "combo/BlackMarketKeeper.h"
#include "combo/BlackMarketGraphicsArena.h"
#include "global.h"
#include "mm/2s2h/Enhancements/Gameplay/BlackMarketResources.h"
#if __has_include("combo/BlackMarketKeeperAssets.h")
#include "combo/BlackMarketKeeperAssets.h"
BlackMarketKeeperAssets::NativeModel sNativeKeeper;
bool sNativeKeeperReady;
#endif
#ifndef OOT_KEEPER_TEST
#include "overlays/actors/ovl_En_Takaraya/z_en_takaraya.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#endif
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
PlayState *gPlayState;
static bool interior = true, pack = true, sTalking = true, sTitlePending = true,
            sCustomKeeper;
static bool expectCustom, nativeMissing, altEnabled, graphMissing, limbMissing,
    eyeMissing;
static int initializations, kills, nativeLoads, graphLoads, nativeBindings,
    draws, titles;
static unsigned limbIndex;
static std::set<std::string> files;
static std::set<std::string> altFiles;
constexpr int kLimbCount = 24, kHeadLimb = 5;
struct KeeperState {
  SkelAnime skel;
  Vec3s joints[24], morphs[24];
  int eye, timer;
} sKeeper;
#ifndef OOT_KEEPER_TEST
static std::array<Vec3s, TREASURE_CHEST_SHOP_GAL_LIMB_MAX> sKeeperJoints, sKeeperMorphs;
static PlayState* sNativeShopPlay;
static EnOssan* expectedShop;
static s16 sBrowseCamera = CAM_ID_NONE;
bool NativeShopOwnCamera(PlayState*) { return false; }
extern "C" s16 Play_GetActiveCamId(PlayState*) { return CAM_ID_MAIN; }
// Native cursor commands execute in native_mm_shop_actor_test.cpp. This fixture
// exercises the separate Gal skeleton/head/eye presentation with no owned view.
void EnOssan_DrawCursor(PlayState*, EnOssan*, f32, f32, f32, u8) { assert(false); }
void EnOssan_DrawStickDirectionPrompts(PlayState*, EnOssan*) { assert(false); }
#endif
static FlexSkeletonHeader nativeSkeleton;
static AnimationHeader nativeAnimation;
static std::array<StandardLimb, 23> nativeLimbs;
static std::array<void *, 23> nativeLimbTable;
static std::array<Gfx, 16> nativeGraph;
static std::array<unsigned char, 3> nativeEyes;
bool Interior() { return interior; }
bool Active() { return interior; }
bool PackAvailable() { return pack; }
extern "C" u8 ResourceMgr_FileExists(const char *path) {
  if (std::strncmp(path, "__OTR__", 7) == 0)
    path += 7;
  return files.count(path);
}
extern "C" u8 ResourceMgr_FileAltExists(const char *path) {
  return altFiles.count(path);
}
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return altEnabled; }
extern "C" void MmAssets_Init() { ++nativeLoads; }
extern "C" void *MmAssets_LoadSkeleton(const char *path) {
  assert(std::strstr(path, "objects/object_bg/gTreasureChestShopGalSkel"));
  ++nativeLoads;
  limbIndex = 0;
  return nativeMissing ? nullptr : &nativeSkeleton;
}
extern "C" void *MmAssets_LoadAnimation(const char *path) {
  assert(std::strstr(path, "objects/object_bg/object_bg_Anim_009890"));
  ++nativeLoads;
  return &nativeAnimation;
}
extern "C" void *MmAssets_LoadResourceStrict(const char *path) {
  ++nativeLoads;
  const std::string name(path);
  if (name.find("Limb") != std::string::npos) {
    const auto limb = limbIndex++;
    return limbMissing && limb == 4 ? nullptr : &nativeLimbs[limb];
  }
  if (eyeMissing)
    return nullptr;
  if (name.find("EyeOpenUpTex") != std::string::npos)
    return &nativeEyes[0];
  if (name.find("EyeHalfUpTex") != std::string::npos)
    return &nativeEyes[1];
  if (name.find("EyeClosedTex") != std::string::npos)
    return &nativeEyes[2];
  assert(false && "only native keeper limbs and eye images should be loaded");
  return nullptr;
}
extern "C" Gfx *MmAssets_LoadDisplayListGraphStrict(const char *path) {
  assert(std::strstr(path, "objects/object_bg/"));
  if (graphMissing)
    return nullptr;
  return &nativeGraph[graphLoads++ % 16];
}
extern "C" void MmAssets_EnsureStrictTextureBindings() { ++nativeBindings; }
void KeeperUpdate(Actor *, PlayState *) {}
void RecordInit(PlayState *, SkelAnime *skel, FlexSkeletonHeader *skeleton,
                AnimationHeader *animation, Vec3s *joints, Vec3s *morphs, int limbs) {
  assert(limbs == 24);
#ifndef OOT_KEEPER_TEST
  assert(expectedShop && skel == &expectedShop->skelAnime);
  assert(joints == sKeeperJoints.data() && morphs == sKeeperMorphs.data());
  assert(joints != expectedShop->jointTable && morphs != expectedShop->morphTable &&
         "Gal's 24 joints must never use the smaller native shop buffers");
  for (int i = 0; i < limbs; ++i) {
    joints[i] = {s16(i + 10), 20, 30};
    morphs[i] = {s16(i + 40), 50, 60};
  }
  skel->jointTable = joints;
  skel->morphTable = morphs;
#endif
  if (expectCustom) {
    assert(
        std::strcmp(
            reinterpret_cast<const char *>(skeleton),
            "__OTR__custom/skiejer_market/keeper/gTreasureChestShopGalSkel") ==
        0);
    assert(std::strcmp(reinterpret_cast<const char *>(animation),
                       BlackMarketKeeper::kAnimation) == 0);
  } else {
#ifdef OOT_KEEPER_TEST
    assert(animation == &nativeAnimation &&
           "native keeper must use an archive-scoped MM animation");
    assert(skeleton != &nativeSkeleton && skeleton->sh.limbCount == 23 &&
           skeleton->dListCount == 16);
    assert(graphLoads == 16 &&
           "every native limb graph must be imported from MM");
    assert(skeleton->sh.segment != nativeLimbTable.data() &&
           "do not mutate the shared native skeleton");
    for (unsigned i = 0; i < 23; ++i) {
      const auto *limb =
          static_cast<const StandardLimb *>(skeleton->sh.segment[i]);
      assert(limb != &nativeLimbs[i] && limb->child == nativeLimbs[i].child &&
             limb->sibling == 255);
      assert(limb->dList == (i < 16 ? &nativeGraph[i] : nullptr));
      assert(nativeLimbs[i].dList ==
             (i < 16 ? reinterpret_cast<Gfx *>(const_cast<char *>(
                           "__OTR__objects/object_bg/testDL"))
                     : nullptr));
    }
#else
    assert(std::strcmp(reinterpret_cast<const char *>(skeleton),
                       "__OTR__objects/object_bg/gTreasureChestShopGalSkel") ==
               0 &&
           "native-only market must not load a missing custom keeper");
    assert(std::strcmp(reinterpret_cast<const char *>(animation),
                       "__OTR__objects/object_bg/object_bg_Anim_009890") == 0);
#endif
  }
  ++initializations;
}
void Scale(Actor *actor, float value) {
  assert(value == BlackMarketKeeper::kScale);
  actor->scale = {value, value, value};
}
void RecordDraw() { ++draws; }
extern "C" void gSPSegment(void *command, int segment, uintptr_t target) {
  __gSPSegment(static_cast<Gfx *>(command), segment, target);
}
#define SkelAnime_InitFlex(...) RecordInit(__VA_ARGS__)
#define SkelAnime_DrawFlexOpa(...) RecordDraw()
#define Gfx_SetupDL25_Opa(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#undef OPEN_DISPS
#undef CLOSE_DISPS
#define OPEN_DISPS(ctx)                                                        \
  {                                                                            \
    GraphicsContext *__gfxCtx = (ctx)
#define CLOSE_DISPS(ctx) }
#define ActorShape_Init(...) ((void)0)
#define Actor_SetScale(actor, value) Scale(actor, value)
#define Actor_Kill(actor) (++kills)
#ifndef OOT_KEEPER_TEST
extern "C" int ComboPlaceTitleCard_Show(PlayState *, TexturePtr, u8, u8) {
  ++titles;
  return 1;
}
#else
// Title service boundaries used by the production presenter extracted below.
// Its native title behavior is covered by the location-title fixtures.
bool Safe(PlayState*) { return true; }
extern "C" s32 Player_InCsMode(PlayState*) { return false; }
int Combo_SOH_SwapCustomTitleCardPresentation(int value) {
  static int current;
  const int previous = current;
  current = value;
  return previous;
}
#define GameInteractor_Should(...) true
#endif
/* PRODUCTION */
void Initialize(Actor *actor, PlayState *play) {
#ifdef OOT_KEEPER_TEST
  KeeperInit(actor, play);
#else
  InitKeeper(actor);
#endif
}
void Scenario(bool custom, bool partial, bool useAlt = false) {
  sNativeKeeperReady = false;
  nativeMissing = graphMissing = limbMissing = eyeMissing = false;
  files.clear();
  altFiles.clear();
  altEnabled = useAlt;
  if (custom || partial)
    for (const auto *path : kBlackMarketResources)
      if (std::strncmp(path, "custom/skiejer_market/keeper/", 29) == 0)
        files.insert(path);
  if (partial)
    files.erase("custom/skiejer_market/keeper/ShopGalMMDTexture4");
  if (useAlt)
    altFiles.swap(files);
  expectCustom = custom && !partial;
  initializations = kills = nativeLoads = graphLoads = nativeBindings = draws =
      0;
  nativeSkeleton = {};
  nativeAnimation = {};
  nativeAnimation.common.frameCount = 30;
  nativeSkeleton.sh.limbCount = 23;
  nativeSkeleton.dListCount = 16;
  nativeSkeleton.sh.segment = nativeLimbTable.data();
  for (unsigned i = 0; i < 23; ++i) {
    nativeLimbs[i] = {};
    nativeLimbs[i].child = i == 22 ? 255 : i + 1;
    nativeLimbs[i].sibling = 255;
    nativeLimbs[i].dList = i < 16 ? reinterpret_cast<Gfx *>(const_cast<char *>(
                                        "__OTR__objects/object_bg/testDL"))
                                  : nullptr;
    nativeLimbTable[i] = &nativeLimbs[i];
  }
  PlayState play{};
  GraphicsContext gfx{};
  Gfx commands[8]{};
  play.state.gfxCtx = &gfx;
  gfx.polyOpa.p = commands;
  gPlayState = &play;
#ifdef OOT_KEEPER_TEST
  Actor keeper{};
#else
  EnOssan gal{};
  expectedShop = &gal;
  sNativeShopPlay = &play;
  for (auto& joint : gal.jointTable) joint = {501, 502, 503};
  for (auto& morph : gal.morphTable) morph = {701, 702, 703};
  Actor &keeper = gal.actor;
#endif
  Initialize(&keeper, &play);
  assert(initializations == 1 && !kills && keeper.update == KeeperUpdate &&
         keeper.draw == KeeperDraw);
  assert(keeper.destroy == nullptr &&
         (keeper.flags & ACTOR_FLAG_UPDATE_CULLING_DISABLED));
#ifdef OOT_KEEPER_TEST
  assert(sKeeper.timer == 90 && sKeeper.eye == 0);
#else
  assert(gal.blinkTimer == 90 && gal.eyeTexIndex == 0 && !sTalking);
  assert(gal.blinkFunc == NativeKeeperBlink);
  for (const auto& joint : gal.jointTable) assert(joint.x == 501 && joint.z == 503);
  for (const auto& morph : gal.morphTable) assert(morph.x == 701 && morph.z == 703);
  assert(sKeeperJoints.back().x == 33 && sKeeperMorphs.back().x == 63);
#endif
  Gfx *nativeHead = &nativeGraph[4];
  Gfx *head = nativeHead;
  for (int eye = 0; eye < 4; ++eye) {
#ifdef OOT_KEEPER_TEST
    sKeeper.eye = eye;
#else
    gal.eyeTexIndex = eye;
#endif
    head = nativeHead;
    KeeperLimb(&play, 5, &head, nullptr, nullptr, &keeper);
    if (expectCustom) {
      const char *names[] = {"ShopGalMMDBlinkHead0DL", "ShopGalMMDBlinkHead1DL",
                             "ShopGalMMDBlinkHead2DL",
                             "ShopGalMMDBlinkHead1DL"};
      assert(std::strstr(reinterpret_cast<char *>(head), names[eye]));
    } else
      assert(head == nativeHead && "native model must retain its native head");
    gfx.polyOpa.p = commands;
    KeeperDraw(&keeper, &play);
    if (expectCustom)
      assert(gfx.polyOpa.p == commands);
    else {
      assert(gfx.polyOpa.p == commands + 1 &&
             (commands[0].words.w0 & 0xFFFF) == 8 * 4);
#ifdef OOT_KEEPER_TEST
      const unsigned expected[] = {0, 1, 2, 1};
      assert(commands[0].words.w1 ==
             reinterpret_cast<uintptr_t>(&nativeEyes[expected[eye]]));
#else
      const char *expected[] = {"EyeOpenUpTex", "EyeHalfUpTex", "EyeClosedTex",
                                "EyeHalfUpTex"};
      assert(std::strstr(reinterpret_cast<char *>(commands[0].words.w1),
                         expected[eye]));
#endif
    }
  }
  assert(draws == 4);
  if (expectCustom)
    assert(nativeLoads == 0);
#ifdef OOT_KEEPER_TEST
  else
    assert(nativeBindings == 4);
#else
  gal.eyeTexIndex = 0; gal.blinkTimer = 90;
  for (int frame = 0; frame < 89; ++frame) NativeKeeperBlink(&gal);
  assert(gal.eyeTexIndex == 0 && gal.blinkTimer == 1);
  NativeKeeperBlink(&gal); assert(gal.eyeTexIndex == 1 && gal.blinkTimer == 2);
  for (int eye : {2, 3, 0}) {
    NativeKeeperBlink(&gal); NativeKeeperBlink(&gal);
    assert(gal.eyeTexIndex == eye);
  }
  assert(gal.blinkTimer == 80);
#endif
}
int main() {
  Scenario(true, false);
  Scenario(true, false, true);
  Scenario(false, false);
  Scenario(false, true);
#ifdef OOT_KEEPER_TEST
  const int cachedLoads = nativeLoads, cachedGraphs = graphLoads;
  const auto *cachedTable = sNativeKeeper.skeleton.sh.segment;
  for (int check = 0; check < 3; ++check)
    assert(LoadNativeKeeper());
  assert(nativeLoads == cachedLoads && graphLoads == cachedGraphs &&
         "successful preflight must not reimport limbs or DL graphs");
  assert(sNativeKeeper.skeleton.sh.segment == cachedTable &&
         "cached private hierarchy must retain its lifetime");
  sNativeKeeperReady = false;
  nativeMissing = true;
  initializations = kills = 0;
  PlayState play{};
  Actor keeper{};
  gPlayState = &play;
  Initialize(&keeper, &play);
  assert(kills == 1 && initializations == 0);
  assert(!sNativeKeeperReady);
  nativeMissing = false;
  for (auto *missing : {&limbMissing, &graphMissing, &eyeMissing}) {
    *missing = true;
    initializations = kills = graphLoads = 0;
    Initialize(&keeper, &play);
    assert(kills == 1 && initializations == 0 && !sNativeKeeperReady);
    *missing = false;
  }
  initializations = kills = graphLoads = 0;
  Initialize(&keeper, &play);
  assert(initializations == 1 && !kills && sNativeKeeperReady &&
         "a failed native import must be retried when dependencies become available");
#endif
  std::puts("PASS market keeper: complete custom retained; native and "
            "partial-custom fallback; native head/eye segment8; private "
            "hierarchy/strict graph import");
}
