#include "fixture_api.h"
#include "combo/menu/ComboFairyBottle.h"
#include "combo/menu/ComboItemDrawABI.h"
// Passive Black Market capture is outside the native key/bottle draw fixture.
void NeiGi_ReportBlackMarketModel(const char *owner, const char *opaque,
                                const char *translucent) {}
static const char gGiEmptyBottleCorkDL[] =
    "__OTR__objects/object_gi_bottle/gGiEmptyBottleCorkDL";
static const char gGiEmptyBottleGlassDL[] =
    "__OTR__objects/object_gi_bottle/gGiEmptyBottleGlassDL";
static int ResourceMgr_IsModAsset(const char *path) {
  if (!strcmp(path, gGiEmptyBottleCorkDL) || !strcmp(path, gGiEmptyBottleGlassDL))
    return (testAlt || testFairyEmptyMod) && !strcmp(path, gGiEmptyBottleGlassDL);
  const char *bundled[] = {
      "__OTR__objects/combo_bottle_gi/EmptyOpaque",
      "__OTR__objects/combo_bottle_gi/EmptyXlu",
      "__OTR__objects/combo_bottle_gi/BottleShell",
      "__OTR__objects/combo_bottle_gi/shell/mat_gGiBlueFireChamberstickDL_BottleGlassOutside"};
  for (int i = 0; i < 4; ++i)
    if (!strcmp(path, bundled[i]))
      return (testFairyBundledMod & (1 << i)) != 0;
  assert(!"Unexpected selected bottle resource");
  return 0;
}
static int ResourceMgr_FileExists(const char *path) {
  if (strstr(path, "objects/combo_dungeon_keys/")) {
    assert(strncmp(path, "__OTR__objects/combo_dungeon_keys/",
                   strlen("__OTR__objects/combo_dungeon_keys/")) == 0);
    return !testMissing || (testMissing == 2 && strstr(path, "MetalDL")) ||
           (testMissing == 3 && strstr(path, "EmblemDL"));
  }
  if (!strcmp(path, "__OTR__objects/combo_bottle_gi/EmptyOpaque") ||
      !strcmp(path, "__OTR__objects/combo_bottle_gi/EmptyXlu"))
    return 1;
  assert(!strcmp(path, "__OTR__objects/combo_bottle_gi/FairyXlu"));
  return !testFairyGlassMissing;
}
int testSpinRed = 17;
bool testAlt = true;
bool testFairyEmptyMod = false;
bool testFairyGlassMissing = false;
int testFairyBundledMod = 0, testKeyAltMask = 0;
int testColors = 1, testChanged = 0, testEmblemChanged = 0, testMissing = 0,
    testLoadFail = 0;
int testCount = 0, testDepth = 0, testScales = 0, testSetup5 = 0;
Gfx testCommands[512];
Gfx *testOpa = testCommands, *testXlu = testCommands + 256;
const char *testPaths[32];
int testStreams[32];
bool testTint[32], testGray[2];
Color_RGBA8 testColorAtDraw[32], testEnv[2], testGrayColor[2];
static int stream(Gfx *p) { return p >= testCommands + 256; }
void TestReset(void) {
  testCount = testScales = testSetup5 = 0;
  assert(testDepth == 0);
  testGray[0] = testGray[1] = false;
  testEnv[0] = testEnv[1] = (Color_RGBA8){255, 255, 255, 255};
}
void TestSubmit(Gfx *p, const char *dl) {
  if (strcmp(dl, "Setup5") == 0) {
    testSetup5++;
    return;
  }
  assert(testCount < 32);
  int s = stream(p);
  testPaths[testCount] = dl;
  testStreams[testCount] = s;
  testTint[testCount] = testGray[s];
  testColorAtDraw[testCount] = testGray[s] ? testGrayColor[s] : testEnv[s];
  testCount++;
}
void TestEnv(Gfx *p, u8 r, u8 g, u8 b, u8 a) {
  testEnv[stream(p)] = (Color_RGBA8){r, g, b, a};
}
void TestGray(Gfx *p, bool on) { testGray[stream(p)] = on; }
void TestGrayColor(Gfx *p, u8 r, u8 g, u8 b, u8 a) {
  testGrayColor[stream(p)] = (Color_RGBA8){r, g, b, a};
}
bool ResourceMgr_IsAltAssetsEnabled(void) { return testAlt; }
u8 ResourceMgr_FileAltExists(const char *p) {
  assert(strncmp(p, "__OTR__objects/combo_dungeon_keys/",
                 strlen("__OTR__objects/combo_dungeon_keys/")) == 0);
  return (testKeyAltMask & (strstr(p, "EmblemDL") ? 2 : 1)) != 0;
}
Gfx *ResourceMgr_LoadGfxByName(const char *p) {
  return testLoadFail && strstr(p, "combo_dungeon_keys") ? NULL : (Gfx *)p;
}
int CVarGetInteger(const char *p, int fallback) {
  if (strcmp(p, "gEnhancements.DungeonItemColors") == 0)
    return testColors;
  assert(strncmp(p, "gCosmetic.Items.", 16) == 0 &&
         strstr(p, "Emblem.Changed"));
  return testEmblemChanged;
}
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a,
                                           const char *id) {
  if (strcmp(id, "Effects.GreatSpinBurst") == 0)
    return (Color_RGBA8){testSpinRed, 123, 241, 255};
  const char *emblemIds[] = {"Items.WoodfallEmblem", "Items.SnowheadEmblem",
                             "Items.GreatBayEmblem", "Items.StoneTowerEmblem"};
  for (int i = 0; i < 4; i++)
    if (strcmp(id, emblemIds[i]) == 0) {
      return testEmblemChanged ? (Color_RGBA8){210 + i, 120 + i, 60 + i, 255}
                               : (Color_RGBA8){r, g, b, a};
    }
  const char *ids[] = {"Items.Woodfall", "Items.Snowhead", "Items.GreatBay",
                       "Items.StoneTower"};
  int owner = -1;
  for (int i = 0; i < 4; i++)
    if (strcmp(id, ids[i]) == 0)
      owner = i;
  assert(owner >= 0);
  return testChanged ? (Color_RGBA8){10 + owner, 20 + owner, 30 + owner, 255}
                     : (Color_RGBA8){r, g, b, a};
}
void Matrix_Push(void) { testDepth++; }
void Matrix_Pop(void) {
  assert(testDepth > 0);
  testDepth--;
}
void Matrix_Scale(f32 x, f32 y, f32 z, s32 mode) {
  assert(x == .25f && y == .25f && z == .25f);
  testScales++;
}
void Matrix_Translate(f32 x, f32 y, f32 z, s32 m) {}
void Matrix_RotateZYX(s16 x, s16 y, s16 z, s32 m) {}
void Matrix_ReplaceRotation(void *p) {}
s32 GetItem_GetShimmerColor(s16 id, uint8_t *color) { return false; }
Gfx *Gfx_SetupDL(Gfx *p, int id) {
  assert(id == 5);
  testSetup5++;
  return p;
}
Gfx *GetItem_DrawDListWithCosmetics(Gfx *p, const char *dl, s16 id) {
  TestSubmit(p, dl);
  return p + 1;
}
void GetItem_DrawOpa0(PlayState *, s16);
void GetItem_DrawOpa0Xlu1(PlayState *, s16);
void GetItem_DrawCompass(PlayState *, s16);
typedef struct {
  void (*drawFunc)(PlayState *, s16);
  void *drawResources[8];
} Draw;
static Draw sDrawItemTable[] = {
    {GetItem_DrawOpa0, {"__OTR__objects/object_gi_key/gGiSmallKeyDL", NULL}},
    {GetItem_DrawOpa0Xlu1,
     {"__OTR__objects/object_gi_key_boss/gGiBossKeyDL",
      "__OTR__objects/object_gi_key_boss/gGiBossKeyGemDL"}},
    {GetItem_DrawOpa0, {"__OTR__objects/object_gi_map/gGiDungeonMapDL", NULL}},
    {GetItem_DrawCompass,
     {"__OTR__objects/object_gi_compass/gGiCompassDL",
      "__OTR__objects/object_gi_compass/gGiCompassGlassDL"}},
    {GetItem_DrawOpa0Xlu1, {(void *)gGiEmptyBottleCorkDL, (void *)gGiEmptyBottleGlassDL}},
};
static ComboFairyBottleShell GetItem_FairyBottleShell(s16);
s32 GetItem_GetDrawTableEntry(s32 id, void **out, s32 max, s32 *xs, f32 *scale,
                              s32 *scroll, s32 *kind) {
  if (id == GID_FAIRY_CONTAINER) {
    const ComboFairyBottleShell shell = GetItem_FairyBottleShell(id);
    assert(max >= 4);
    out[0] = (void *)shell.opaque;
    out[1] = (void *)shell.glass;
    out[2] = (void *)"__OTR__objects/object_gi_fairy/gGiFairyBottleContentsDL";
    out[3] = (void *)"__OTR__objects/object_gi_fairy/gGiFairyBottleBillboardRotMtx";
    *xs = 1;
    *scale = 0;
    *scroll = 0;
    *kind = CW_DRAW_KIND_MM_FAIRY_CONTAINER;
    return 4;
  }
  if (id < 0 || id > 3) {
    return 0;
  }
  int n = id == 1 || id == 3 ? 2 : 1;
  assert(max >= n);
  for (int i = 0; i < n; i++)
    out[i] = sDrawItemTable[id].drawResources[i];
  *xs = id == 1 || id == 3 ? 1 : -1;
  *scale = 0;
  *scroll = 0;
  *kind = 0;
  return n;
}
void GetItem_GetDrawSetupDLs(s32 id, void **opa, void **xlu) {
  *opa = NULL;
  *xlu = id == 3 ? (void *)"Setup5" : NULL;
}
/* PRODUCTION_NATIVE */

#ifndef TEST_NO_MAIN
int main(void) {
  const char *p, *e;
  Color_RGBA8 m, c;
  PlayState play = {0};
  // Bundled MM models are available without a mod and with Alt Assets off.
  testAlt = false;
  for (int owner = 0; owner < 4; owner++)
    for (int kind = 0; kind < 2; kind++) {
      assert(GetItem_GetDungeonKeyModel(kind, owner, &p, &e, &m, &c));
      assert(strstr(p, "__OTR__objects/combo_dungeon_keys/") &&
             strstr(e, "__OTR__objects/combo_dungeon_keys/"));
    }
  testAlt = true;
  // Each canonical root can resolve from the bundle or an enabled Alt pack.
  for (int alt = 0; alt < 2; ++alt)
    for (int missing = 0; missing < 4; ++missing)
      for (int replacements = 0; replacements < 4; ++replacements) {
        testAlt = alt;
        testMissing = missing;
        testKeyAltMask = replacements;
        const bool metal = missing == 0 || missing == 2 ||
                           (alt && (replacements & 1));
        const bool emblem = missing == 0 || missing == 3 ||
                            (alt && (replacements & 2));
        for (int owner = 0; owner < 4; ++owner)
          for (int kind = 0; kind < 2; ++kind)
            assert((bool)GetItem_GetDungeonKeyModel(kind, owner, &p, &e, &m, &c) ==
                   (metal && emblem));
      }
  testAlt = true;
  testMissing = testKeyAltMask = 0;
  for (int owner = 0; owner < 4; owner++)
    for (int kind = 0; kind < 4; kind++) {
      testColors = 1;
      testChanged = 1;
      testEmblemChanged = 1;
      TestReset();
      assert(GetItem_DrawDungeonItem(&play, kind, owner));
      if (kind < 2) {
        assert(testCount == 2 && testDepth == 0 && testScales == 1);
        assert(strstr(testPaths[0], "MetalDL") &&
               strstr(testPaths[1], "EmblemDL"));
        assert(testColorAtDraw[0].r == 10 + owner &&
               testColorAtDraw[1].r == 210 + owner);
        assert(!testTint[0] && !testTint[1]);
      } else {
        assert(testCount == (kind == 3 ? 2 : 1) && testTint[0]);
        assert(testColorAtDraw[0].r == 10 + owner &&
               testColorAtDraw[0].a == (kind == 2 ? 96 : 176));
        if (kind == 3)
          assert(testStreams[1] == 1 && !testTint[1] && testSetup5 == 1);
        assert(!testGray[0]);
      }
    }
  for (int owner = 0; owner < 4; owner++)
    for (int kind = 0; kind < 2; kind++) {
      const int defaults[4] = {236, 129, 99, 201};
      testColors = 1;
      testChanged = 1;
      testEmblemChanged = 0;
      assert(GetItem_GetDungeonKeyModel(kind, owner, &p, &e, &m, &c) &&
             m.r == 10 + owner && c.r == defaults[owner]);
      testColors = 1;
      testChanged = 0;
      testEmblemChanged = 1;
      assert(GetItem_GetDungeonKeyModel(kind, owner, &p, &e, &m, &c) &&
             m.r == (kind ? 233 : 213) && c.r == 210 + owner);
      testColors = 0;
      testChanged = 1;
      testEmblemChanged = 1;
      assert(GetItem_GetDungeonKeyModel(kind, owner, &p, &e, &m, &c) &&
             m.r == (kind ? 233 : 213) && c.r == 210 + owner);
      testColors = 0;
      testChanged = 0;
      testEmblemChanged = 0;
      assert(GetItem_GetDungeonKeyModel(kind, owner, &p, &e, &m, &c) &&
             m.r == (kind ? 233 : 213) && c.r == defaults[owner]);
    }
  testColors = 0;
  testChanged = 1;
  testEmblemChanged = 1;
  assert(GetItem_GetDungeonKeyModel(0, 3, &p, &e, &m, &c) && m.r == 213 &&
         c.r == 213);
  assert(GetItem_GetDungeonKeyModel(1, 3, &p, &e, &m, &c) && m.r == 233 &&
         c.r == 213);
  assert(!GetItem_GetDungeonKeyModel(2, 3, &p, &e, &m, &c));
  assert(!GetItem_GetDungeonKeyModel(0, -1, &p, &e, &m, &c));
  assert(!GetItem_GetDungeonKeyModel(0, 4, &p, &e, &m, &c));
  assert(!GetItem_GetDungeonKeyModel(0, 3, NULL, &e, &m, &c));
  // Both bodies and native gems must work when no custom key is selectable.
  for (int config = 0; config < 3; config++)
    for (int owner = 0; owner < 4; owner++)
      for (int kind = 0; kind < 4; kind++)
        for (int colors = 0; colors < 2; colors++)
          for (int crest = 0; crest < 2; crest++) {
            testAlt = config != 0;
            testMissing = config == 2 ? 2 : 1;
            testColors = colors;
            testChanged = 1;
            testEmblemChanged = crest;
            TestReset();
            bool tinted = colors || (kind == GID_KEY_BOSS && crest);
            assert(GetItem_DrawDungeonItem(&play, kind, owner) == tinted);
            if (!tinted) {
              assert(testCount == 0);
              continue;
            }
            assert(testCount == (kind == 1 || kind == 3 ? 2 : 1) &&
                   testScales == 0);
            assert(!strstr(testPaths[0], "combo_dungeon_keys") &&
                   testTint[0] == (bool)colors);
            if (colors)
              assert(testColorAtDraw[0].r == 10 + owner &&
                     testColorAtDraw[0].a == (kind == 2   ? 96
                                              : kind == 3 ? 176
                                                          : 192));
            if (kind == 1 || kind == 3) {
              assert(testStreams[1] == 1 &&
                     testTint[1] == (kind == 1 && crest));
              if (kind == 1 && crest)
                assert(testColorAtDraw[1].r == 210 + owner &&
                       testColorAtDraw[1].a == 192);
              if (kind == 3)
                assert(testSetup5 == 1 && !testTint[1]);
            }
            assert(!testGray[0] && !testGray[1]);
          }
  testAlt = true;
  testMissing = 0;
  testColors = 1;
  testEmblemChanged = 0;
  testAlt = false;
  assert(GetItem_GetDungeonKeyModel(0, 0, &p, &e, &m, &c));
  testAlt = true;
  testMissing = 2;
  assert(!GetItem_GetDungeonKeyModel(0, 0, &p, &e, &m, &c));
  testMissing = 0;
  testColors = 1;
  testLoadFail = 1;
  TestReset();
  assert(GetItem_DrawDungeonItem(&play, 0, 0));
  assert(testCount == 1 && strstr(testPaths[0], "object_gi_key/"));
  testLoadFail = 0;
  puts("Native C: bundled keys work with Alt off/on; every vanilla GI family "
       "passes missing and partial-resource fallbacks; palette bodies, independent boss gems, "
       "clear compass glass and cleanup pass.");
  return 0;
}

#endif
