#include "global.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "2s2h/BenGui/HudEditor.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "tests/test_require.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <limits.h>
#include <string.h>

#include "2s2h/Rando/RupeeCounterDigits.h"

SaveContext gSaveContext;
PlayState* gPlayState;
u32 gUpgradeMasks[8] = {[UPG_WALLET] = 3};
u8 gUpgradeShifts[8];
u16 gUpgradeCapacities[8][4] = {[UPG_WALLET] = {99, 200, 500, 5000}};
const char* sCounterTextures[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
HudEditorElementID hudEditorActiveElement;
HudEditorElement hudEditorElements[HUD_EDITOR_ELEMENT_MAX];
static Gfx commands[256];
static GraphicsContext gfx;
static PlayState play;
static char drawn[6];
static int drawnCount;

void FrameInterpolation_RecordOpenChild(const void* source, int line) {}
void FrameInterpolation_RecordCloseChild(void) {}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* ctx, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* ctx, const char* file, s32 line) {}
void HudEditor_SetActiveElement(HudEditorElementID id) { hudEditorActiveElement = id; }
bool HudEditor_ShouldOverrideDraw(void) { return false; }
void HudEditor_ModifyDrawValues(s16* x, s16* y, s16* w, s16* h, s16* dx, s16* dy) {}
int32_t CVarGetInteger(const char* name, int32_t defaultValue) { return defaultValue; }
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    REQUIRE(flag == VB_DRAW_RUPEE_COUNTER);
    return result;
}
Gfx* Gfx_DrawTexRectI8(Gfx* dl, TexturePtr texture, s16 tw, s16 th, s16 x, s16 y, s16 w, s16 h, u16 dx, u16 dy) {
    const char* digit = texture;
    REQUIRE(drawnCount < 5);
    REQUIRE(digit != NULL);
    REQUIRE(digit[0] >= '0' && digit[0] <= '9' && digit[1] == '\0');
    drawn[drawnCount++] = digit[0];
    drawn[drawnCount] = '\0';
    REQUIRE(x == 43 + (drawnCount - 1) * 8 && y == 207);
    return dl;
}

#include "wallet_draw_production.inc"

static void checkDraw(s16 balance, int wallet, int tycoon, const char* expected) {
    gSaveContext.save.saveInfo.playerData.rupees = balance;
    gSaveContext.save.saveInfo.inventory.upgrades = wallet;
    gSaveContext.rupeeAccumulator = 17;
    gfx.overlay.p = commands;
    drawnCount = 0;
    drawn[0] = '\0';
    if (tycoon) {
        DrawTycoonRupeeCounter();
    } else {
        TestNativeDraw(&play);
    }
    if (strcmp(drawn, expected) != 0) {
        fprintf(stderr, "balance=%d wallet=%d path=%s expected=%s drawn=%s\n", balance, wallet,
                tycoon ? "Tycoon" : "native", expected, drawn);
        abort();
    }
    REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == balance);
    REQUIRE(gSaveContext.save.saveInfo.inventory.upgrades == (u32)wallet);
    REQUIRE(gSaveContext.rupeeAccumulator == 17);
}

int main(int argc, char** argv) {
    play.state.gfxCtx = &gfx;
    gPlayState = &play;
    play.interfaceCtx.magicAlpha = 255;
    if (argc == 1) {
        checkDraw(1667, 2, 0, "1667");
        checkDraw(99, 0, 0, "99");
        checkDraw(0, 0, 0, "00");
        checkDraw(200, 1, 0, "200");
        checkDraw(500, 2, 0, "500");
        checkDraw(999, 2, 0, "999");
        checkDraw(1000, 2, 0, "1000");
        checkDraw(9999, 2, 0, "9999");
        checkDraw(10000, 2, 0, "10000");
        checkDraw(INT16_MAX, 2, 0, "32767");
        checkDraw(-1, 2, 0, "000");
        checkDraw(INT16_MIN, 2, 0, "000");
    }
    // ComboShip shares OoT's 999 capacity: the Tycoon HUD must not add a zero.
    gUpgradeCapacities[UPG_WALLET][3] = 999;
    checkDraw(999, 3, 1, "999");
    checkDraw(0, 3, 1, "000");
    checkDraw(7, 3, 1, "007");
    checkDraw(-1, 3, 1, "000");
    checkDraw(INT16_MIN, 3, 1, "000");
    // Restored balances still display every digit without changing saved money.
    checkDraw(1000, 3, 1, "1000");
    checkDraw(1667, 3, 1, "1667");
    checkDraw(10000, 3, 1, "10000");
    checkDraw(INT16_MAX, 3, 1, "32767");
    REQUIRE(gUpgradeCapacities[UPG_WALLET][3] == 999);

    // Standalone 2Ship's genuine Tycoon capacity retains its four-digit HUD.
    gUpgradeCapacities[UPG_WALLET][3] = 5000;
    checkDraw(0, 3, 1, "0000");
    checkDraw(999, 3, 1, "0999");
    checkDraw(5000, 3, 1, "5000");
    checkDraw(9999, 3, 1, "9999");
    checkDraw(10000, 3, 1, "10000");
    checkDraw(INT16_MAX, 3, 1, "32767");
    checkDraw(INT16_MIN, 3, 1, "0000");
    puts("PASS restored 1667/wallet2 and decimal boundary fixtures preserve "
         "balances");
    for (int balance = INT16_MIN; balance <= INT16_MAX; ++balance) {
        char expected[6];
        for (int wallet = 0; wallet < 4; ++wallet) {
            int width = wallet == 0 ? 2 : 3;
            snprintf(expected, sizeof(expected), "%0*d", width, balance < 0 ? 0 : balance);
            checkDraw((s16)balance, wallet, 0, expected);
        }
        gUpgradeCapacities[UPG_WALLET][3] = 999;
        snprintf(expected, sizeof(expected), "%03d", balance < 0 ? 0 : balance);
        checkDraw((s16)balance, 3, 1, expected);
        REQUIRE(gUpgradeCapacities[UPG_WALLET][3] == 999);
        gUpgradeCapacities[UPG_WALLET][3] = 5000;
        snprintf(expected, sizeof(expected), "%04d", balance < 0 ? 0 : balance);
        checkDraw((s16)balance, 3, 1, expected);
        REQUIRE(gUpgradeCapacities[UPG_WALLET][3] == 5000);
    }
    puts("PASS all 65536 signed16 balances in native wallet0/1/2/3 and 999/5000 Tycoon: "
         "decimal textures, no save or capacity changes");
    return 0;
}
