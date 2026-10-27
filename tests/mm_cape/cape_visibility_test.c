/* Complete production cape draw/update with each host's native Player and GBI.
 * Resource, graphics allocation and matrix services are the fixture boundaries;
 * the cape's visibility guards, lifecycle and cloth code are unchanged.
 */
#include "global.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef CAPE_TEST_MM
#include "2s2h/BenPort.h"
#define NATIVE_HIDE_PLAYER PLAYER_STATE2_20000000
#else
#include "soh/ResourceManagerHelpers.h"
#define NATIVE_HIDE_PLAYER PLAYER_STATE2_DISABLE_DRAW
#endif

static u8 sVisible = 1;
static Vtx vertices[144];
static Mtx matrix;
static char pixels[32];

s32 CVarGetInteger(const char *name, s32 fallback) {
  (void)name;
  return fallback;
}
f32 CVarGetFloat(const char *name, f32 fallback) {
  (void)name;
  return fallback;
}
u8 ExtEquip_CapeVisible(void) { return sVisible; }
u8 ResourceMgr_FileExists(const char *path) {
  (void)path;
  return 1;
}
Vtx *ResourceMgr_LoadVtxByName(char *path) {
  (void)path;
  return vertices;
}
void FrameInterpolation_RecordOpenChild(const void *key, int line) {
  (void)key;
  (void)line;
}
void FrameInterpolation_RecordCloseChild(void) {}
void Matrix_Push(void) {}
void Matrix_Pop(void) {}
void Matrix_MultVec3f(Vec3f *src, Vec3f *dst) { *dst = *src; }
f32 Math_SinS(s16 angle) { return sinf(angle * (M_PI / 32768.0f)); }
f32 Math_CosS(s16 angle) { return cosf(angle * (M_PI / 32768.0f)); }
void Math_ApproachZeroF(f32 *value, f32 scale, f32 step) {
  const f32 change = fminf(fabsf(*value) * scale, step);
  *value += *value < 0.0f ? change : -change;
}

#ifdef CAPE_TEST_MM
void *OotAssets_LoadTexOrDList(const char *path) {
  (void)path;
  return pixels;
}
int ResourceMgr_OTRSigCheck(char *data) {
  return strncmp(data, "__OTR__", 7) == 0;
}
void Gfx_SetupDL25_Opa(GraphicsContext *gfxCtx) { (void)gfxCtx; }
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
  (void)x;
  (void)y;
  (void)z;
  (void)mode;
}
void Matrix_RotateXF(f32 angle, MatrixMode mode) {
  (void)angle;
  (void)mode;
}
void Matrix_RotateYF(f32 angle, MatrixMode mode) {
  (void)angle;
  (void)mode;
}
void Matrix_RotateZF(f32 angle, MatrixMode mode) {
  (void)angle;
  (void)mode;
}
Mtx *Matrix_Finalize(GraphicsContext *gfxCtx) {
  (void)gfxCtx;
  return &matrix;
}
f32 Math_Atan2F_XY(f32 x, f32 y) { return atan2f(y, x); }
#else
Gfx gCullBackDList[1];
void gSPDisplayList(Gfx *packet, Gfx *displayList) {
  __gSPDisplayList(packet, displayList);
}
void gSPSegment(void *packet, int segment, uintptr_t target) {
  __gSPSegment(packet, segment, target);
}
void gSPSegmentLoadRes(void *packet, int segment, uintptr_t target) {
  (void)target;
  __gSPSegment(packet, segment, (uintptr_t)vertices);
}
void Gfx_RegisterBlendedTexture(const char *texture, const u8 *mask,
                                const u8 *replacement) {
  (void)texture;
  (void)mask;
  (void)replacement;
}
void gSPInvalidateTexCache(Gfx *packet, uintptr_t texture) {
  packet->words.w0 = 0;
  packet->words.w1 = texture;
}
void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) {
  (void)x;
  (void)y;
  (void)z;
  (void)mode;
}
void Matrix_RotateX(f32 angle, u8 mode) {
  (void)angle;
  (void)mode;
}
void Matrix_RotateY(f32 angle, u8 mode) {
  (void)angle;
  (void)mode;
}
void Matrix_RotateZ(f32 angle, u8 mode) {
  (void)angle;
  (void)mode;
}
Mtx *Matrix_NewMtx(GraphicsContext *gfxCtx, char *file, s32 line) {
  (void)gfxCtx;
  (void)file;
  (void)line;
  return &matrix;
}
f32 Math_Atan2F(f32 x, f32 y) { return atan2f(y, x); }
#endif

#include "cape_graph.inc"
#include "mods/equipment/behaviors/equip_magiccape.c"

static GraphicsContext graphics;
static PlayState play;
static Player player;
static Gfx opa[1024], xlu[1024], overlay[32];

static void ResetDisplayLists(void) {
  graphics.polyOpa.p = opa;
  graphics.polyOpa.d = opa + 1024;
  graphics.polyXlu.p = xlu;
  graphics.polyXlu.d = xlu + 1024;
  graphics.overlay.p = overlay;
  graphics.overlay.d = overlay + 32;
  play.state.gfxCtx = &graphics;
}

static void SetShoulders(void) {
#ifdef CAPE_TEST_MM
  player.bodyPartsPos[PLAYER_BODYPART_RIGHT_SHOULDER] =
      (Vec3f){500.0f, 80.0f, 506.0f};
  player.bodyPartsPos[PLAYER_BODYPART_LEFT_SHOULDER] =
      (Vec3f){500.0f, 80.0f, 494.0f};
#else
  sCapeRightShoulderPos = (Vec3f){500.0f, 80.0f, 506.0f};
  sCapeLeftShoulderPos = (Vec3f){500.0f, 80.0f, 494.0f};
  sCapeShouldersCaptured = 3;
#endif
}

static void ExpectDrawn(void) {
  ResetDisplayLists();
  MagicCape_Draw(&player, &play);
  assert(graphics.polyOpa.p > opa);
}

static void ExpectOmitted(void) {
  ResetDisplayLists();
  MagicCape_Draw(&player, &play);
  assert(graphics.polyOpa.p == opa && graphics.polyXlu.p == xlu);
}

int main(int argc, char **argv) {
  assert(argc == 2);
  MagicCape_Init();
  SetShoulders();
  ExpectDrawn();

  if (strcmp(argv[1], "draw") == 0) {
    // Direct draw callers must obey the exact bit native Player_Draw uses.
    player.stateFlags2 = NATIVE_HIDE_PLAYER;
    ExpectOmitted();
    player.stateFlags2 = 0;
    ExpectDrawn();
    // A different stateFlags2 bit must not suppress the cape.
    player.stateFlags2 = 1;
    ExpectDrawn();
    player.stateFlags1 = PLAYER_STATE1_ON_HORSE;
    ExpectOmitted();
    player.stateFlags1 = 0;
    ExpectDrawn();
  } else {
    assert(strcmp(argv[1], "resume") == 0);
    // Hidden body draws do not refresh shoulder anchors. Discard the old
    // cloth before reveal rather than integrating from its previous roots.
    sCapeNeedsRootSnap = 0;
    sCapeStrands[0].joints[0] = (Vec3f){-1000.0f, -1000.0f, -1000.0f};
    player.stateFlags2 = NATIVE_HIDE_PLAYER;
    MagicCape_Behavior(&player, &play);
    assert(!sCapeInitialized && sCapeNeedsRootSnap);
    ExpectOmitted();
    MagicCape_Behavior(&player, &play);
    assert(!sCapeInitialized);

    player.stateFlags2 = 0;
    player.actor.world.pos = (Vec3f){500.0f, 40.0f, 500.0f};
    MagicCape_Behavior(&player, &play);
    assert(sCapeInitialized && sCapeUpdateHasRun && sCapeNeedsRootSnap);
    SetShoulders();
    ExpectDrawn();
    assert(!sCapeUpdateHasRun && !sCapeNeedsRootSnap);
    assert(fabsf(sCapeStrands[0].joints[0].x - 500.0f) < 0.01f);
    assert(fabsf(sCapeStrands[0].joints[0].y - 80.0f) < 0.01f);

    // Existing user visibility/ownership cleanup remains authoritative.
    sVisible = 0;
    MagicCape_Cleanup();
    ExpectOmitted();
  }
  puts("PASS native cape hide/reveal contract");
  return 0;
}
