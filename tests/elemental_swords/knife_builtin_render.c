#define ResourceMgr_FileExists AuraLegacyFileExists
#define ResourceMgr_LoadGfxByName AuraLegacyLoadGfx
#define main KnifeLegacyMain
#include "../../soh/tests/din_fire_sword_test.c"
#undef main
#undef ResourceMgr_FileExists
#undef ResourceMgr_LoadGfxByName
#include "aura_gbi_checks.h"
#include "aura_held_fixture.h"

int main(void) {
  for (int rig = 0; rig < 2; ++rig) {
    for (int element = 1; element <= 3; ++element) {
      setup();
      gSaveContext.linkAge = rig;
      enabled = alt = assets = 0;
      player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
      player.leftHandType = PLAYER_MODELTYPE_LH_BGS;
      gSaveContext.bgsFlag = 0;
      elementalSelection = elementalActive = element;
      auraHeldGraph = 1;
      auraNativePieces = 0;
      // Repeated break/repair and item-ID changes must not retain an old fit.
      for (int state = 0; state < 8; ++state) {
        const int broken = state % 2 == 0;
        player.heldItemId = state < 4 ? ITEM_SWORD_BGS : ITEM_SWORD_KNIFE;
        gSaveContext.swordHealth = broken ? 0 : 8;
        ++play.gameplayFrames;
        DinFireSword_Update(&play, &player);
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        currentMatrix.xw = 17.f;
        currentMatrix.yw = 23.f;
        currentMatrix.zw = -31.f;
        const MtxF handPose = currentMatrix;
        DinFireSword_BeginPlayerDraw(&play, &player);
        Gfx *selected = hand;
        REQUIRE(WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12, 34,
                                               56));
        const Gfx *wrapper = (const Gfx *)selected[0].words.w1;
        const char *path = (const char *)wrapper[0].words.w1;
        REQUIRE(strstr(path, broken ? "/broken_giants_knife/"
                                    : "/giants_knife/") != NULL);
        Gfx frameSword[8];
        memcpy(frameSword, selected, sizeof(frameSword));
        DinFireSword_CopyHandModel(&player, selected, frameSword);
        DinFireSword_DrawHand(&play, &player, frameSword);
        currentMatrix.xw += 10000.f;
        DinFireSword_DrawAfterPlayer(&play, &player);
        // A missing appended profile must fail before a GBI walk.
        for (Gfx *p = opa; p < gfx.polyOpa.p; ++p)
          if ((p->words.w0 >> 24) == G_DL)
            REQUIRE(p->words.w1 != 0);
        for (Gfx *p = xlu; p < gfx.polyXlu.p; ++p)
          if ((p->words.w0 >> 24) == G_DL)
            REQUIRE(p->words.w1 != 0);
        AuraGbi geometry = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
        REQUIRE(geometry.max[0] >= (broken ? 2490 : 5490));
        REQUIRE(geometry.max[0] < (broken ? (element == 1 ? 2700 : 2540)
                                          : (element == 1 ? 5900 : 5580)));
        REQUIRE(geometry.textures == 2 && geometry.triangles > 40);
        REQUIRE(matrices > 0 &&
                !memcmp(&submittedMatrices[0], &handPose, sizeof(handPose)));
      }
    }
  }
  puts("PASS authored intact/broken Knife: both item IDs, break/repair, both "
       "ages, "
       "three elemental casings, final DL identity and captured pose");
  return 0;
}
