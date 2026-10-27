#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int32_t s32;
typedef int16_t s16;
typedef uint8_t u8;
typedef float f32;
typedef char Gfx;
typedef struct {
  u8 r, g, b, a;
} Color_RGBA8;
typedef struct PlayState {
  struct {
    void *gfxCtx;
  } state;
  int billboardMtxF;
} PlayState;
enum {
  GID_KEY_SMALL,
  GID_KEY_BOSS,
  GID_DUNGEON_MAP,
  GID_COMPASS,
  GID_BOTTLE,
  GID_FAIRY_CONTAINER,
  GID_CHATEAU = 0x73, // Native MM z64item.h value; used by the shared draw producer.
  MTXMODE_APPLY = 1,
  SETUPDL_5 = 5
};
extern bool testAlt;
extern bool testFairyEmptyMod;
extern bool testFairyGlassMissing;
extern int testFairyBundledMod, testKeyAltMask;
extern int testSpinRed;
extern int testColors, testChanged, testEmblemChanged, testMissing,
    testLoadFail;
extern int testCount, testDepth, testScales, testSetup5;
extern Gfx testCommands[512];
extern Gfx *testOpa, *testXlu;
extern const char *testPaths[32];
extern int testStreams[32];
extern bool testTint[32];
extern Color_RGBA8 testColorAtDraw[32], testEnv[2], testGrayColor[2];
extern bool testGray[2];
void TestReset(void);
void TestSubmit(Gfx *, const char *);
void TestEnv(Gfx *, u8, u8, u8, u8);
void TestGray(Gfx *, bool);
void TestGrayColor(Gfx *, u8, u8, u8, u8);
bool ResourceMgr_IsAltAssetsEnabled(void);
u8 ResourceMgr_FileAltExists(const char *);
Gfx *ResourceMgr_LoadGfxByName(const char *);
int CVarGetInteger(const char *, int);
Color_RGBA8 CosmeticEditor_GetChangedColor(u8, u8, u8, u8, const char *);
void Matrix_Push(void);
void Matrix_Pop(void);
void Matrix_Scale(f32, f32, f32, s32);
void Matrix_Translate(f32, f32, f32, s32);
void Matrix_RotateZYX(s16, s16, s16, s32);
void Matrix_ReplaceRotation(void *);
Gfx *Gfx_SetupDL(Gfx *, int);
Gfx *GetItem_DrawDListWithCosmetics(Gfx *, const char *, s16);
s32 GetItem_GetDungeonKeyModel(s16, s32, const char **, const char **,
                               Color_RGBA8 *, Color_RGBA8 *);
s32 GetItem_GetDungeonItemTint(s16, s32, Color_RGBA8 *, u8 *);
s32 GetItem_GetDungeonKeyEmblemTint(s32, Color_RGBA8 *);
s32 GetItem_GetDrawTableEntry(s32, void **, s32, s32 *, f32 *, s32 *, s32 *);
void GetItem_GetDrawSetupDLs(s32, void **, void **);
s32 GetItem_GetShimmerColor(s16, uint8_t *);
#define ARRAY_COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define POLY_OPA_DISP testOpa
#define POLY_XLU_DISP testXlu
#define OPEN_DISPS(ctx)                                                        \
  do {                                                                         \
    testOpa = testCommands;                                                    \
    testXlu = testCommands + 256;
#define CLOSE_DISPS(ctx)                                                       \
  }                                                                            \
  while (0)
#define Gfx_SetupDL25_Opa(ctx) ((void)0)
#define Gfx_SetupDL25_Xlu(ctx) ((void)0)
#define Gfx_SetupDL_25Opa(ctx) ((void)0)
#define Gfx_SetupDL_25Xlu(ctx) ((void)0)
#define gSPGrayscale(pkt, on) TestGray((Gfx *)(pkt), (on))
#define gDPSetGrayscaleColor(pkt, r, g, b, a)                                  \
  TestGrayColor((Gfx *)(pkt), r, g, b, a)
#define gDPSetPrimColor(pkt, m, l, r, g, b, a) ((void)(pkt))
#define gDPSetEnvColor(pkt, r, g, b, a) TestEnv((Gfx *)(pkt), r, g, b, a)
#define MATRIX_FINALIZE_AND_LOAD(pkt, ctx) ((void)(pkt))
#define COMBO_FOREIGN_MTX(pkt) ((void)(pkt))
#define gSPDisplayList(pkt, dl) TestSubmit((Gfx *)(pkt), (const char *)(dl))
#define gDPPipeSync(pkt) ((void)(pkt))
#define OOT_FOREIGN_PIN_OPA() gSPGrayscale(POLY_OPA_DISP++, false)
#define OOT_FOREIGN_PIN_XLU() gSPGrayscale(POLY_XLU_DISP++, false)
