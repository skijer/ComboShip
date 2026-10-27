// Missing the player's XLU segment binding must reproduce the crash address
// 0x0D000381, even when a later opaque NPC draw leaves a different matrix base.
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef HOST_OOT
#include "global.h"
#else
extern "C" {
#include "global.h"
}
#endif

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition);        \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

extern "C" {
#ifdef HOST_OOT
void IkanaTest_SetMatrixStack(MtxF *stack);
MtxF *Matrix_GetCurrent(void);
void *Graph_Alloc(GraphicsContext *gfx, size_t size) {
  gfx->polyOpa.d = reinterpret_cast<Gfx *>(
      reinterpret_cast<u8 *>(gfx->polyOpa.d) - ALIGN16(size));
  return gfx->polyOpa.d;
}
float NativeNpcPose_GetDrawScale(void **) { return 1.0f; }
const void *NativeNpcPose_GetSkinPlan(void **, s32) { return nullptr; }
s32 NativeNpcPose_SkinMatrixCount(const void *) { return 0; }
void NativeNpcPose_BlendSkinMatrix(const void *, s32, const MtxF *, MtxF *) {
  CHECK(false);
}
int32_t CVarGetInteger(const char *, int32_t value) { return value; }
void osSyncPrintfUnused(const char *, ...) {}
void lusprintf(const char *, int32_t, int32_t, const char *, ...) {}
Gfx *PakLoader_GetDLOverride(const char *) { return nullptr; }
void FrameInterpolation_RecordMatrixScale(f32, f32, f32, u8) {}
// Sanitized native animation tables retain this unrelated loader symbol. A
// call here would mean the fixture has left the draw path being tested.
s32 osRecvMesg(OSMesgQueue *, OSMesg *, s32) {
  CHECK(false);
  return -1;
}
#else
extern MtxF *sMatrixStack;
extern MtxF *sCurrentMatrix;
void *Lib_SegmentedToVirtual(void *ptr) { return ptr; }
#endif
f32 Math_SinS(s16 a) {
  return std::sin(a * 3.14159265358979323846f / 32768.0f);
}
f32 Math_CosS(s16 a) {
  return std::cos(a * 3.14159265358979323846f / 32768.0f);
}
int ResourceMgr_OTRSigCheck(char *) { return 0; }
Gfx *ResourceMgr_LoadGfxByName(const char *) { return nullptr; }
char *ResourceMgr_LoadIfDListByName(const char *) { return nullptr; }
void FrameInterpolation_RecordOpenChild(const void *, int) {}
void FrameInterpolation_RecordCloseChild(void) {}
void FrameInterpolation_RecordMatrixPush(void) {}
void FrameInterpolation_RecordMatrixPop(void) {}
void FrameInterpolation_RecordMatrixPut(MtxF *) {}
void FrameInterpolation_RecordMatrixTranslateRotateZYX(Vec3f *, Vec3s *) {}
void FrameInterpolation_RecordMatrixToMtx(Mtx *, char *, s32) {}
}

// Only renderer state is local; the resolver body is extracted unchanged.
class Interpreter {
public:
  uintptr_t mSegmentPointers[16] = {};
  void *SegAddr(uintptr_t);
};
#include "production_helpers.inc"

static Gfx sMesh[1], sShield[2];
static bool sRaiseShield;
static unsigned sShieldReads;
static Mtx *sExpectedBase;

#ifdef HOST_OOT
static void PostLimb(PlayState *play, s32 limb, Gfx **, Vec3s *, void *) {
  if (sRaiseShield && limb == PLAYER_LIMB_R_HAND)
#else
static void PostLimb(PlayState *play, s32 limb, Gfx **, Gfx **, Vec3s *,
                     Actor *) {
  if (sRaiseShield && limb == PLAYER_LIMB_RIGHT_HAND)
#endif
    DrawCachedShieldDL(play, sShield);
}

static void Replay(Interpreter &renderer, const Gfx *start, const Gfx *end,
                   unsigned depth = 0) {
  CHECK(depth < 8);
  for (const Gfx *command = start; command < end; ++command) {
    const uintptr_t w0 = command->words.w0, w1 = command->words.w1;
    const unsigned op = w0 >> 24;
    if (op == G_ENDDL)
      return;
    if (op == G_MOVEWORD && ((w0 >> 16) & 0xFF) == G_MW_SEGMENT) {
      const unsigned segment = (w0 & 0xFFFF) / 4;
      CHECK(segment < 16);
      renderer.mSegmentPointers[segment] = w1;
    } else if (op == G_DL) {
      const Gfx *target = static_cast<const Gfx *>(renderer.SegAddr(w1));
      CHECK(target == sMesh || target == sShield);
      Replay(renderer, target, target + (target == sShield ? 2 : 1), depth + 1);
    } else if (op == G_MTX && w1 == 0x0D000381) {
      const void *resolved = renderer.SegAddr(w1);
      if (resolved != sExpectedBase + 14) {
        std::fprintf(
            stderr,
            "FAIL shield matrix: 0x0D000381 resolved to %p; expected %p\n",
            resolved, static_cast<void *>(sExpectedBase + 14));
        std::exit(1);
      }
      ++sShieldReads;
    }
  }
}

static void RunFrame(s16 actorId, s32 lod, unsigned frame, bool raised,
                     bool nullActor = false) {
  // Four initial null meshes make RIGHT_HAND (limb 19) use matrix slot 14.
  LodLimb limbs[21] = {};
  void *pointers[21];
  for (unsigned i = 0; i < 21; ++i) {
    limbs[i].child = i == 20 ? LIMB_DONE : i + 1;
    limbs[i].sibling = LIMB_DONE;
    limbs[i].jointPos.x = 1;
    limbs[i].dLists[0] = limbs[i].dLists[1] = i < 4 ? nullptr : sMesh;
    pointers[i] = &limbs[i];
  }
  // Different frame allocations catch a binding accidentally cached from an
  // earlier draw/re-entry, even if a platform reuses the same call stack.
  alignas(16) static std::array<Gfx, 4096> opaFrames[3], xluFrames[3];
  auto &opa = opaFrames[frame];
  auto &xlu = xluFrames[frame];
  opa.fill({});
  xlu.fill({});
  Gfx overlay[64] = {};
  GraphicsContext gfx = {};
  gfx.polyOpa.p = opa.data();
  gfx.polyOpa.d = opa.data() + opa.size();
  gfx.polyXlu.p = xlu.data();
  gfx.polyXlu.d = xlu.data() + xlu.size();
  gfx.overlay.p = overlay;
  PlayState play = {};
  play.state.gfxCtx = &gfx;
  Actor actor = {};
  actor.id = actorId;
  Vec3s joints[22] = {};
  joints[0].x = 100 * frame;
  MtxF stack[64] = {};
  guMtxIdentF(stack[0].mf);
#ifdef HOST_OOT
  IkanaTest_SetMatrixStack(stack);
#else
  sMatrixStack = sCurrentMatrix = stack;
#endif
  sRaiseShield = raised;
  sShieldReads = 0;
  if (actorId == ACTOR_PLAYER)
    DrawPlayer(&play, pointers, joints, 17, PostLimb,
               nullActor ? nullptr : &actor, lod);
  else
    SkelAnime_DrawFlexLod(&play, pointers, joints, 17, nullptr, PostLimb,
                          &actor, lod);
  // Read the actual OPA producer because shield matrix allocations may follow
  // it.
  sExpectedBase = nullptr;
  for (const Gfx *c = opa.data(); c < gfx.polyOpa.p; ++c)
    if ((c->words.w0 >> 24) == G_MOVEWORD &&
        (c->words.w0 & 0xFFFFFF) == ((G_MW_SEGMENT << 16) | 0x34))
      sExpectedBase = reinterpret_cast<Mtx *>(c->words.w1);
  CHECK(sExpectedBase != nullptr);
  CHECK(Matrix_GetCurrent() == stack);
  float hand[4][4];
  guMtxL2F(hand, sExpectedBase + 14);
  CHECK(std::fabs(hand[3][0] - (18.0f + 100.0f * frame)) < 0.001f);
  if (actorId == ACTOR_PLAYER) {
    Interpreter clean;
    Replay(clean, xlu.data(), gfx.polyXlu.p);
    CHECK(sShieldReads == (raised ? 1u : 0u));
    sShieldReads = 0;
  }
  // Simulate another actor being drawn later in OPA. The deferred shield must
  // rebind its OWN player matrices when XLU executes afterwards.
  Mtx otherActor[17] = {};
  gSPSegment(gfx.polyOpa.p++, 0x0D, reinterpret_cast<uintptr_t>(otherActor));
  Interpreter renderer;
  Replay(renderer, opa.data(), gfx.polyOpa.p);
  CHECK(renderer.mSegmentPointers[13] ==
        reinterpret_cast<uintptr_t>(otherActor));
  Replay(renderer, xlu.data(), gfx.polyXlu.p);
  CHECK(sShieldReads == (raised ? 1u : 0u));
  CHECK(renderer.mSegmentPointers[13] ==
        reinterpret_cast<uintptr_t>(actorId == ACTOR_PLAYER ? sExpectedBase
                                                            : otherActor));
}

static void NullSkeleton() {
  Gfx opa[256] = {}, xlu[16] = {}, overlay[16] = {};
  GraphicsContext gfx = {};
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = opa + 256;
  gfx.polyXlu.p = xlu;
  gfx.overlay.p = overlay;
  PlayState play = {};
  play.state.gfxCtx = &gfx;
  Vec3s joints[22] = {};
  DrawPlayer(&play, nullptr, joints, 17, PostLimb, nullptr, 0);
  CHECK(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
}

int main() {
  static_assert(sizeof(Mtx) == 64);
  gSPEndDisplayList(sMesh);
  sShield[0].words.w0 = 0xDA380002;
  sShield[0].words.w1 = 0x0D000381;
  gSPEndDisplayList(&sShield[1]);
  for (unsigned frame = 0; frame < 3; ++frame)
    for (s32 lod : {0, 1}) {
      RunFrame(ACTOR_PLAYER, lod, frame, true);
      RunFrame(ACTOR_PLAYER, lod, frame, true, true);
      RunFrame(ACTOR_PLAYER, lod, frame, false);
      RunFrame(ACTOR_EN_TEST, lod, frame, false);
    }
  NullSkeleton();
  std::puts("PASS native LOD traversal + shield drawer + renderer resolver: 24 "
            "frames, "
            "cold/stale segments, near/far LOD, re-entry, lowered shield, NPC "
            "isolation, null inputs");
}
