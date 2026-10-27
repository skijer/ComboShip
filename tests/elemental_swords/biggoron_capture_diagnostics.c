#define CVarGetInteger BiggoronLegacyCVar
#define lusprintf BiggoronLegacyLog
#ifdef BIGGORON_TEST_MM
#include "../../mm/tests/din_fire_fixture.h"
#else
#define main BiggoronLegacyMain
#include "../../soh/tests/din_fire_sword_test.c"
#undef main
#endif
#undef CVarGetInteger
#undef lusprintf
#include <stdarg.h>
#include <stdio.h>

static int diagnosticsEnabled, diagnosticLogs;
static char diagnostic[512];
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return !strcmp(key, "gDeveloperTools.SwordAuraDiagnostics") ? diagnosticsEnabled
                                                                : BiggoronLegacyCVar(key, fallback);
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(diagnostic, sizeof(diagnostic), fmt, args);
    va_end(args);
    ++diagnosticLogs;
}

static void Submit(int model) {
    gfx.polyOpa.p = opa;
    gfx.polyXlu.p = xlu;
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_SetHandModel(&player, hand, model);
    DinFireSword_DrawHand(&play, &player, hand);
    DinFireSword_DrawAfterPlayer(&play, &player);
    REQUIRE(gfx.polyXlu.p > xlu);
}

int main(void) {
#ifdef BIGGORON_TEST_MM
    Setup();
    swordOption = alt = assets = 0;
    player.heldItemId = ITEM_SWORD_BGS;
    player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_TWO_HANDED;
    player.leftHandType = PLAYER_MODELTYPE_LH_TWO_HAND_SWORD;
#else
    setup();
    enabled = alt = assets = 0;
    player.heldItemId = ITEM_SWORD_BGS;
    player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
    player.leftHandType = PLAYER_MODELTYPE_LH_BGS;
    gSaveContext.bgsFlag = 1;
    gSaveContext.swordHealth = 8;
#endif
    elementalSelection = elementalActive = 2;
    ++play.gameplayFrames;
    DinFireSword_Update(&play, &player);
    Submit(SWORD_AURA_MODERN_DIN_BGS);
    REQUIRE(diagnosticLogs == 0);
    diagnosticsEnabled = 1;
    Submit(SWORD_AURA_MODERN_DIN_BGS);
    REQUIRE(diagnosticLogs == 1);
    REQUIRE(strstr(diagnostic, "logical=6") && strstr(diagnostic, "fit=22") && strstr(diagnostic, "matched=1"));
    Submit(SWORD_AURA_MODERN_DIN_BGS);
    REQUIRE(diagnosticLogs == 1);
    Submit(SWORD_AURA_MODERN_DIN_BROKEN);
    REQUIRE(diagnosticLogs == 2 && strstr(diagnostic, "fit=23"));
    puts("PASS opt-in sword aura diagnostics report actual fit once per "
         "model/rig/source kind");
    return 0;
}
