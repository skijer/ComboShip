#define ResourceMgr_FileExists AuraLegacyFileExists
#define ResourceMgr_LoadGfxByName AuraLegacyLoadGfx
#ifdef BIGGORON_TEST_MM
#include "../../mm/tests/din_fire_fixture.h"
#else
#define main BiggoronLegacyMain
#include "../../soh/tests/din_fire_sword_test.c"
#undef main
#endif
#undef ResourceMgr_FileExists
#undef ResourceMgr_LoadGfxByName
#include "aura_gbi_checks.h"
#include "aura_held_fixture.h"

int main(void) {
    for (int rig = 0; rig < 2; ++rig) {
        for (int element = 1; element <= 3; ++element) {
#ifdef BIGGORON_TEST_MM
            Setup();
            adult = rig;
            swordOption = alt = assets = 0;
            player.heldItemId = ITEM_SWORD_BGS;
            player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_TWO_HANDED;
            player.leftHandType = PLAYER_MODELTYPE_LH_TWO_HAND_SWORD;
#else
            setup();
            gSaveContext.linkAge = rig;
            enabled = alt = assets = 0;
            player.heldItemId = ITEM_SWORD_BGS;
            player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
            player.leftHandType = PLAYER_MODELTYPE_LH_BGS;
            gSaveContext.bgsFlag = 1;
            gSaveContext.swordHealth = 8;
#endif
            elementalSelection = elementalActive = element;
            auraHeldGraph = 1;
            auraNativePieces = 0;
            ++play.gameplayFrames;
            DinFireSword_Update(&play, &player);
            gfx.polyOpa.p = opa;
            gfx.polyXlu.p = xlu;
            currentMatrix.xx = .009f;
            currentMatrix.xy = .002f;
            currentMatrix.yx = -.002f;
            currentMatrix.yy = .009f;
            currentMatrix.zz = .01f;
            currentMatrix.ww = 1.f;
            currentMatrix.xw = 17.f;
            currentMatrix.yw = 23.f;
            currentMatrix.zw = -31.f;
            const MtxF handPose = currentMatrix;
            DinFireSword_BeginPlayerDraw(&play, &player);
            Gfx* selected = hand;
            REQUIRE(WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12, 34, 56));
            const Gfx* wrapper = (const Gfx*)selected[0].words.w1;
            const char* path = (const char*)wrapper[0].words.w1;
            REQUIRE(strstr(path, "nei_held_swords/biggoron_sword/") != NULL);
#if defined(COMBO_BUILD) || defined(BIGGORON_TEST_MM)
            REQUIRE(strstr(path, "__OTR__@oot-gi-base:") == path);
#endif
            Gfx frameSword[8];
            memcpy(frameSword, selected, sizeof(frameSword));
            DinFireSword_CopyHandModel(&player, selected, frameSword);
            // The real native post-limb callback receives this final pointer.
            DinFireSword_DrawHand(&play, &player, frameSword);
            currentMatrix.xw += 10000.f;
            currentMatrix.yw -= 20000.f;
            DinFireSword_DrawAfterPlayer(&play, &player);
            AuraGbi geometry = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
            REQUIRE(geometry.max[0] >= 5500 && geometry.max[0] < (element == 1 ? 5900 : 5580));
            REQUIRE(geometry.textures == 2 && geometry.triangles > 40);
#ifdef BIGGORON_TEST_MM
            REQUIRE(matrixCount > 0 && !memcmp(&captured[0], &handPose, sizeof(handPose)));
#else
            REQUIRE(matrices > 0 && !memcmp(&submittedMatrices[0], &handPose, sizeof(handPose)));
#endif
        }
    }
    puts("PASS Alt-off authored Biggoron selector, base resource route, final "
         "compound identity, full GBI and captured hand pose");
    return 0;
}
