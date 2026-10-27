#include "global.h"
#include "mods/extended_equipment.h"
#include "mods/nei_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NeiSaveData sSave;
static Gfx sCommands[64];
static Gfx *sCommandCursor;
static Vtx sUpgradeColVtx[16];
static int sIcons[2];
static u8 sIconAvailable[2], sDrawAlpha[2], sWolfSelected;
static int sDrawCount[2], sChecks, sFailures;

NeiSaveData *Nei_Save(void) { return &sSave; }

// Ownership is controlled by the saved flags; no trade acquisition occurs
// during this probe.
u8 TradeAdult_IsOwnedIndex(s32 index) {
  (void)index;
  return 0;
}

u8 WolfLinkForm_IsSelected(void) { return sWolfSelected; }

// Resource loading is the boundary fixture; the production code decides whether
// and how to draw.
void *ExtEquip_GetCapeIcon(void) {
  return sIconAvailable[0] ? &sIcons[0] : NULL;
}

void *ExtEquip_GetPendantIcon(void) {
  return sIconAvailable[1] ? &sIcons[1] : NULL;
}

/* PRODUCTION_ACCESSORS */

#undef POLY_OPA_DISP
#define POLY_OPA_DISP sCommandCursor
// Vertex upload is outside the alpha boundary; MM implements it as a linked GPU
// helper.
#undef gSPVertex
#define gSPVertex(...) ((void)0)

static u8 PrimitiveAlpha(void) {
  u8 alpha = 0;
  for (Gfx *command = sCommands; command < sCommandCursor; ++command) {
    if ((command->words.w0 >> 24) == G_SETPRIMCOLOR) {
      alpha = command->words.w1 & 0xFF;
    }
  }
  return alpha;
}

static void RecordIcon(void *icon) {
  const int row = icon == &sIcons[0] ? 0 : 1;
  assert(icon == &sIcons[row]);
  ++sDrawCount[row];
  sDrawAlpha[row] = PrimitiveAlpha();
}

#ifdef CAPE_TEST_MM
void KaleidoScope_DrawTexQuadRGBA32(GraphicsContext *gfxCtx, void *icon,
                                    u16 width, u16 height, u16 point) {
#else
void KaleidoScope_DrawQuadTextureRGBA32(GraphicsContext *gfxCtx, void *icon,
                                        u16 width, u16 height, u16 point) {
#endif
  (void)gfxCtx;
  (void)width;
  (void)height;
  (void)point;
  RecordIcon(icon);
}

static void DrawPassives(PlayState *play) {
  PauseContext *pauseCtx = &play->pauseCtx;
  sCommandCursor = sCommands;
  memset(sDrawCount, 0, sizeof(sDrawCount));
  memset(sDrawAlpha, 0, sizeof(sDrawAlpha));
  // Both native hosts emit the actual gDPSetPrimColor commands into this
  // stream.
  gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
  /* PRODUCTION_DRAW */
  assert(sCommandCursor <= sCommands + ARRAY_COUNT(sCommands));
}

static void Check(int condition, const char *label) {
  ++sChecks;
  if (!condition) {
    fprintf(stderr,
            "FAIL: %s (owned=%u hidden=%u pendantOwned=%u effectOff=%u, "
            "capeAlpha=%u)\n",
            label, sSave.capeOwned, sSave.capeHidden, sSave.pendantOwned,
            sSave.pendantEffectOff, sDrawAlpha[0]);
    ++sFailures;
  }
}

int main(void) {
  static PlayState play;
#ifndef CAPE_TEST_MM
  static Vtx equipVtx[64];
  play.pauseCtx.equipVtx = equipVtx;
#endif
  const u8 pageAlphas[] = {0, 1, 63, 127, 128, 254, 255};
  for (u8 owned = 0; owned < 2; ++owned) {
    for (u8 hidden = 0; hidden < 2; ++hidden) {
      for (u8 pendantOwned = 0; pendantOwned < 2; ++pendantOwned) {
        for (u8 effectOff = 0; effectOff < 2; ++effectOff) {
          for (u8 available = 0; available < 2; ++available) {
            for (u8 wolf = 0; wolf < 2; ++wolf) {
              for (unsigned alphaIndex = 0;
                   alphaIndex < ARRAY_COUNT(pageAlphas); ++alphaIndex) {
                memset(&sSave, 0, sizeof(sSave));
                sSave.capeOwned = owned;
                sSave.capeHidden = hidden;
                sSave.pendantOwned = pendantOwned;
                sSave.pendantEffectOff = effectOff;
                const NeiSaveData before = sSave;
                const u8 pageAlpha = pageAlphas[alphaIndex];
                play.pauseCtx.alpha = pageAlpha;
                sIconAvailable[0] = sIconAvailable[1] = available;
                sWolfSelected = wolf;
                DrawPassives(&play);
                Check(sDrawCount[0] == (owned && available),
                      "cape drawing follows ownership and resource");
                if (sDrawCount[0]) {
                  Check(sDrawAlpha[0] == pageAlpha,
                        "owned cape retains page alpha for Show and Hide");
                }
                Check(sDrawCount[1] == (pendantOwned && available),
                      "pendant drawing follows ownership and resource");
                if (sDrawCount[1]) {
#ifdef CAPE_TEST_MM
                  const u8 pendantOn = !effectOff && !wolf;
#else
                  const u8 pendantOn = !effectOff;
#endif
                  Check(sDrawAlpha[1] ==
                            (pendantOn ? pageAlpha : (pageAlpha >> 1)),
                        "pendant retains its active and inactive alpha");
                }
                Check(MAGIC_REQ(12) == (owned ? 6 : 12),
                      "cape magic savings remain active by ownership for Show "
                      "and Hide");
                Check(play.pauseCtx.alpha == pageAlpha &&
                          memcmp(&before, &sSave, sizeof(sSave)) == 0,
                      "drawing preserves pause fade and all saved flags");
#ifndef CAPE_TEST_MM
                Check(PrimitiveAlpha() == pageAlpha,
                      "SoH restores page alpha after passive draw");
#endif
              }
            }
          }
        }
      }
    }
  }
#ifdef CAPE_TEST_MM
  printf("MM cape icon: %d checks, %d failures\n", sChecks, sFailures);
#else
  printf("SoH cape icon: %d checks, %d failures\n", sChecks, sFailures);
#endif
  return sFailures ? 1 : 0;
}
