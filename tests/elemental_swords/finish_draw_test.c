#define SWORD_FINISH_ADAPTER_TEST
#include "finish_host_test.c"

static int finishDrawMatrices;
Gfx* SwordFinishIce_GetDisplayList(uint32_t frame) {
  (void)frame;
  return NULL; // Native/Alt fallback; scoped material graph has its own fixture.
}
#ifdef SWORD_FINISH_TEST_MM
Mtx *Matrix_ToMtx(Mtx *dest) {
  ++finishDrawMatrices;
  return dest;
}
void Matrix_RotateYF(f32 angle, MatrixMode mode) {}
void Matrix_RotateZF(f32 angle, MatrixMode mode) {}
Hilite *func_800BCC68(Vec3f *pos, PlayState *state) { return NULL; }
#else
Mtx *Matrix_ToMtx(Mtx *dest, char *file, s32 line) {
  ++finishDrawMatrices;
  return dest;
}
void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) {}
void Matrix_Scale(f32 x, f32 y, f32 z, u8 mode) {}
static int finishIcePitchCount, finishIcePitchVaries;
static f32 finishIcePitchFirst;
void Matrix_RotateX(f32 angle, u8 mode) {
  REQUIRE(mode == MTXMODE_APPLY);
  REQUIRE(isfinite(angle) && angle >= -3.14159265f && angle <= 3.14159265f);
  if (!finishIcePitchCount++)
    finishIcePitchFirst = angle;
  else if (angle != finishIcePitchFirst)
    finishIcePitchVaries = 1;
}
void Matrix_RotateY(f32 angle, u8 mode) {}
void Matrix_RotateZ(f32 angle, u8 mode) {}
Hilite *func_8002EB44(Vec3f *object, Vec3f *eye, Vec3f *light,
                      GraphicsContext *context) {
  return NULL;
}
#endif
void gSPSegment(void *value, int segment, uintptr_t target) {
  Gfx *packet = value;
  packet->words.w0 = 0;
  packet->words.w1 = target;
}
void Matrix_ReplaceRotation(MtxF *matrix) {}
Gfx *Gfx_TwoTexScrollEx(GraphicsContext *context, s32 tile1, u32 x1, u32 y1,
                        s32 width1, s32 height1, s32 tile2, u32 x2, u32 y2,
                        s32 width2, s32 height2, s32 xs1, s32 ys1, s32 xs2,
                        s32 ys2) {
  static Gfx scroll[12];
  return scroll;
}

int main(void) {
  for (int element = 1; element <= 4; ++element) {
    World();
    DinFireSword_FinishContext(&play);
    enum { MASS_FINISH_COUNT = 256 };
    uint8_t heard[MASS_FINISH_COUNT] = {0};
    for (int slot = 0; slot < MASS_FINISH_COUNT; ++slot) {
      SwordFinishEffect *effect = &sFinish.effects[slot];
      effect->life = 24;
      effect->element = element <= 3 ? element : 1 + slot % 3;
      effect->shape.count = 1;
      effect->shape.points[0] = (SwordFinishPoint){slot * 5, 20, 30, 10};
      effect->shape.origin = effect->shape.points[0];
    }
    gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
    gfx.polyXlu.d = xlu + ARRAY_COUNT(xlu);
    finishDrawMatrices = 0;
#ifndef SWORD_FINISH_TEST_MM
    finishIcePitchCount = finishIcePitchVaries = 0;
#endif
    DinFireSword_FinishDrawAll(&play);
    REQUIRE(finishDrawMatrices == SWORD_FINISH_DRAW_POINT_LIMIT);
#ifndef SWORD_FINISH_TEST_MM
    if (element == SWORD_ELEMENT_ICE)
      REQUIRE(finishIcePitchCount == SWORD_FINISH_DRAW_POINT_LIMIT &&
              finishIcePitchVaries); // Native ice clumps must not share a flat pitch.
    else if (element <= SWORD_ELEMENT_LIGHT)
      REQUIRE(finishIcePitchCount == 0); // Fire/light remain billboard effects.
    else
      REQUIRE(finishIcePitchCount > 0 &&
              finishIcePitchCount < SWORD_FINISH_DRAW_POINT_LIMIT);
#endif
    REQUIRE(finishSoundCount == 32);
    for (int slot = 0; slot < MASS_FINISH_COUNT; ++slot)
      heard[slot] |= sFinish.effects[slot].sounded;
    REQUIRE(gfx.polyXlu.p < xlu + ARRAY_COUNT(xlu));
    unsigned cursor = sFinishDrawCursor;
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
    gfx.polyXlu.p = xlu;
    gfx.polyXlu.d = xlu + ARRAY_COUNT(xlu);
    DinFireSword_FinishDrawAll(&play);
    REQUIRE(sFinishDrawCursor != cursor);
    REQUIRE(finishSoundCount == 32); // The quota spans repeated draws.
    play.pauseCtx.state = 1;
    DinFireSword_FinishDrawAll(&play);
    play.pauseCtx.state = 0;
    DinFireSword_FinishDrawAll(&play);
    REQUIRE(finishSoundCount == 32); // Pausing does not refill the quota.
    for (int frame = 1; frame < MASS_FINISH_COUNT / 32; ++frame) {
      ++play.gameplayFrames;
      gfx.polyOpa.p = opa;
      gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
      gfx.polyXlu.p = xlu;
      gfx.polyXlu.d = xlu + ARRAY_COUNT(xlu);
      DinFireSword_FinishDrawAll(&play);
      REQUIRE(finishSoundCount == (frame + 1) * 32);
      for (int slot = 0; slot < MASS_FINISH_COUNT; ++slot)
        heard[slot] |= sFinish.effects[slot].sounded;
    }
    for (int slot = 0; slot < MASS_FINISH_COUNT; ++slot)
      REQUIRE(heard[slot]);
    // Native passes consumed the rest of both arenas: emit nothing.
    gfx.polyOpa.d = gfx.polyOpa.p;
    gfx.polyXlu.d = gfx.polyXlu.p;
    Gfx *before = gfx.polyXlu.p;
    finishDrawMatrices = 0;
#ifndef SWORD_FINISH_TEST_MM
    finishIcePitchCount = 0;
#endif
    DinFireSword_FinishDrawAll(&play);
    REQUIRE(gfx.polyXlu.p == before && finishDrawMatrices == 0);
#ifndef SWORD_FINISH_TEST_MM
    REQUIRE(finishIcePitchCount == 0);
#endif
#ifndef SWORD_FINISH_TEST_MM
    gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
    gfx.polyXlu.d = xlu + ARRAY_COUNT(xlu);
    graphFailure = 1;
    DinFireSword_FinishDrawAll(&play);
    REQUIRE(gfx.polyXlu.p == before && finishDrawMatrices == 0);
    REQUIRE(finishIcePitchCount == 0);
    graphFailure = 0;
#endif
    // Release all stable source positions before constructing another world.
    DinFireSword_FinishContext(NULL);
  }
  // An ordinary small group retains native per-frame held-light cadence.
  World();
  DinFireSword_FinishContext(&play);
  for (int slot = 0; slot < 3; ++slot) {
    SwordFinishEffect *effect = &sFinish.effects[slot];
    effect->life = 24;
    effect->element = SWORD_ELEMENT_LIGHT;
    effect->shape.count = 1;
    effect->shape.origin = effect->shape.points[0] =
        (SwordFinishPoint){slot * 5, 20, 30, 10};
  }
  for (int frame = 0; frame < 3; ++frame) {
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
    gfx.polyXlu.p = xlu;
    gfx.polyXlu.d = xlu + ARRAY_COUNT(xlu);
    DinFireSword_FinishDrawAll(&play);
    REQUIRE(finishSoundCount == (frame + 1) * 3 && finishStopCount == 0);
    DinFireSword_FinishDrawAll(&play);
    REQUIRE(finishSoundCount == (frame + 1) * 3 && finishStopCount == 0);
    ++play.gameplayFrames;
  }
  DinFireSword_FinishContext(NULL);
  puts("PASS real finish rendering: fire/ice/light GBI, 32-point/32-request "
       "budgets, arena exhaustion and 256-effect fairness");
}
