// A forced element owns the built-in aura even with no saved sword selection.
#ifdef SWORD_FINISH_TEST_MM
#include "../../mm/tests/din_fire_fixture.h"
#else
#define main LegacyFireRendererMain
#include "../../soh/tests/din_fire_sword_test.c"
#undef main
#endif

int main(void) {
  for (int element = 1; element <= 3; ++element) {
#ifdef SWORD_FINISH_TEST_MM
    Setup();
    swordOption = alt = assets = 0;
#else
    setup();
    enabled = alt = assets = 0;
    player.heldItemId = ITEM_SWORD_MASTER;
#endif
    gfx.polyOpa.p = opa;
    gfx.polyXlu.p = xlu;
    elementalSelection = -1;
    elementalActive = elementalAvailable = 0;
    elementalOverride = element;
    resourceQueries = 0;
    ++play.gameplayFrames;
    DinFireSword_Update(&play, &player);
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_Draw(&play, &player);
    DinFireSword_DrawAfterPlayer(&play, &player);
    REQUIRE((gfx.polyOpa.p - opa) + (gfx.polyXlu.p - xlu) > 0);
    REQUIRE(resourceQueries == 0);
    elementalOverride = 0;
#ifdef SWORD_FINISH_TEST_MM
    ResetBuffers();
#else
    gfx.polyOpa.p = opa;
    gfx.polyXlu.p = xlu;
#endif
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_Draw(&play, &player);
    DinFireSword_DrawAfterPlayer(&play, &player);
    REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  }
  puts("PASS override renders built-in sword aura with no saved selection, "
       "arrows, assets or Alt");
  return 0;
}
