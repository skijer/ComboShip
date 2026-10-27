#define main DinFireSword_LegacyRenderMain
#define ResourceMgr_FileExists AuraLegacyFileExists
#define ResourceMgr_LoadGfxByName AuraLegacyLoadGfx
#include "din_fire_sword_test.c"
#undef main
#undef ResourceMgr_FileExists
#undef ResourceMgr_LoadGfxByName
#include "../../tests/elemental_swords/aura_gbi_checks.h"
#include "../../combo/render/sword_element_aura_identity.h"
#include "../../tests/elemental_swords/aura_held_fixture.h"
#include "aura_accepted_alias.inc"
extern void DinFireSword_SetHandModel(Player*, void*, int) __attribute__((weak));
extern void DinFireSword_CopyHandModel(Player*, void*, void*) __attribute__((weak));
extern void DinFireSword_DrawHand(PlayState*, Player*, void*) __attribute__((weak));

static void AuraSetup(int element) {
    setup();
    player.heldItemId = ITEM_SWORD_MASTER;
    elementalSelection = elementalActive = element;
    enabled = alt = assets = 0;
    resourceQueries = 0;
}

static AuraGbi AuraDraw(void) {
    Player before = player;
    SaveContext save = gSaveContext;
    REQUIRE(draw() > 0);
    REQUIRE(!memcmp(&before, &player, sizeof(player)));
    REQUIRE(!memcmp(&save, &gSaveContext, sizeof(save)));
    REQUIRE(resourceQueries == 0);
    AuraGbi result = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
    REQUIRE(result.textures == 2 && result.triangles > 20 && result.vertices > 20);
    REQUIRE(result.matrices == (elementalActive == 1 ? 2 : 1));
    REQUIRE(result.min[0] > 0 && result.min[1] > -2000 && result.max[1] < 2000);
    return result;
}

static void Profiles(void) {
    for (int element = 1; element <= 3; ++element) {
        for (int age = 0; age < 2; ++age) {
            for (int profile = 0; profile < 8; ++profile) {
                AuraSetup(element);
                gSaveContext.linkAge = age;
                int tip = 4000;
                if (profile <= 2) {
                    player.heldItemId = ITEM_SWORD_KOKIRI;
                    player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
                    renderKokiriUpgrade = profile;
                    tip = profile == 0 ? 3000 : 4000;
                } else if (profile == 4) {
                    auraTrueMaster = 1;
                } else if (profile >= 5) {
                    player.heldItemId = profile == 6 ? ITEM_SWORD_KNIFE : ITEM_SWORD_BGS;
                    player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
                    player.leftHandType = PLAYER_MODELTYPE_LH_BGS;
                    gSaveContext.bgsFlag = profile == 5;
                    gSaveContext.swordHealth = profile == 6 ? 0 : 8;
                    renderFairyUpgrade = profile == 7;
                    tip = profile == 6 ? 2500 : 5500;
                }
                tick();
                AuraGbi result = AuraDraw();
                REQUIRE(result.max[0] >= tip - 10);
                REQUIRE(result.max[0] <= tip + (element == 1 ? tip / 10 : 80));
                if (element != 1)
                    REQUIRE(gfx.polyOpa.p == opa);
            }
        }
    }
}

static void LifetimeAndColors(void) {
    uintptr_t texture[3];
    for (int element = 1; element <= 3; ++element) {
        AuraSetup(element);
        tick();
        AuraGbi first = AuraDraw();
        texture[element - 1] = first.texture[1];
        Gfx* end = gfx.polyXlu.p;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(end == gfx.polyXlu.p);
        tick();
        AuraGbi next = AuraDraw();
        REQUIRE(first.scroll != next.scroll);
        play.pauseCtx.state = 1;
        tick();
        AuraGbi paused = AuraDraw();
        REQUIRE(next.scroll == paused.scroll);
        play.pauseCtx.state = 0;
        // A pose captured on a previous frame must never be submitted late.
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        DinFireSword_BeginPlayerDraw(&play, &player);
        DinFireSword_Draw(&play, &player);
        ++play.gameplayFrames;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
        tick();
        ++play.sceneNum;
        REQUIRE(draw() == 0);
        tick();
        AuraDraw();
        elementalActive = 0;
        REQUIRE(draw() == 0); // Checkbox/arrow gate disables the aura immediately.
    }
    REQUIRE(texture[0] != texture[1] && texture[1] != texture[2] && texture[0] != texture[2]);
    AuraSetup(1);
    customColors = 1;
    tick();
    AuraDraw();
    REQUIRE(AuraHasColor(opa, gfx.polyOpa.p, G_SETPRIMCOLOR, 0x29F1C7));
    REQUIRE(AuraHasColor(xlu, gfx.polyXlu.p, G_SETENVCOLOR, 0x0C21B1));
    AuraSetup(2);
    tick();
    AuraDraw();
    REQUIRE(AuraHasColor(xlu, gfx.polyXlu.p, G_SETENVCOLOR, 0x36A8FF));
    AuraSetup(3);
    tick();
    AuraDraw();
    REQUIRE(AuraHasColor(xlu, gfx.polyXlu.p, G_SETENVCOLOR, 0xFFD26A));
}

static void OptionalPackDoesNotSelectGeometry(void) {
    for (int age = 0; age < 2; ++age) {
        AuraSetup(1);
        gSaveContext.linkAge = age;
        if (age) {
            player.heldItemId = ITEM_SWORD_KOKIRI;
            player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
        }
        tick();
        AuraGbi native = AuraDraw();
        enabled = alt = assets = 1;
        REQUIRE(DinFireSword_HandDL(&play, &player, hand, 12, 34, 56) == NULL);
        REQUIRE(draw() > 0);
        AuraGbi packed = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
        REQUIRE(!memcmp(native.min, packed.min, sizeof(native.min)));
        REQUIRE(!memcmp(native.max, packed.max, sizeof(native.max)));
        REQUIRE(packed.textures == 2 && packed.triangles == native.triangles);
        REQUIRE(resourceQueries == 0);
    }
}

static void VisibilityAndCapturedHand(void) {
    for (int element = 1; element <= 3; ++element) {
        for (int gate = 0; gate < 13; ++gate) {
            AuraSetup(element);
            tick();
            switch (gate) {
                case 0:
                    player.actor.scale.y = -.01f;
                    break;
                case 1:
                    transformed = 1;
                    break;
                case 2:
                    invisible = 1;
                    break;
                case 3:
                    otherOwner = 1;
                    break;
                case 4:
                    fourSwordOwner = 1;
                    break;
                case 5:
                    bossOwner = 1;
                    break;
                case 6:
                    bossOwner = 2;
                    break;
                case 7:
                    player.csAction = 1;
                    break;
                case 8:
                    player.stateFlags1 |= PLAYER_STATE1_DEAD;
                    break;
                case 9:
                    player.stateFlags1 |= PLAYER_STATE1_IN_WATER;
                    break;
                case 10:
                    player.stateFlags2 |= PLAYER_STATE2_DISABLE_DRAW;
                    break;
                case 11:
                    play.transitionTrigger = 1;
                    break;
                case 12:
                    player.leftHandType = PLAYER_MODELTYPE_LH_CLOSED;
                    break;
            }
            REQUIRE(draw() == 0);
        }
        AuraSetup(element);
        tick();
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        DinFireSword_BeginPlayerDraw(&play, &player);
        // A final hidden hand never calls Draw from the native post-limb hook.
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
        Player remote = player;
        DinFireSword_BeginPlayerDraw(&play, &remote);
        DinFireSword_Draw(&play, &remote);
        DinFireSword_DrawAfterPlayer(&play, &remote);
        REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
        DinFireSword_BeginPlayerDraw(&play, &player);
        currentMatrix.xw = 23.f;
        currentMatrix.yw = -17.f;
        MtxF wrist = currentMatrix;
        gDPSetPrimColor(gfx.polyOpa.p++, 0, 0, 93, 47, 201, 173);
        gDPSetEnvColor(gfx.polyOpa.p++, 17, 202, 64, 0);
        DinFireSword_Draw(&play, &player);
        REQUIRE(lastColor(opa, gfx.polyOpa.p, G_SETPRIMCOLOR) == 0x5D2FC9AD);
        REQUIRE(lastColor(opa, gfx.polyOpa.p, G_SETENVCOLOR) == 0x11CA4000);
        currentMatrix.xw = 999.f;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(matrices == 1 && !memcmp(&submittedMatrices[0], &wrist, sizeof(wrist)));
        REQUIRE(currentMatrix.xw == 999.f && depth == 0);
        REQUIRE(AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p).textures == 2);
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        DinFireSword_BeginPlayerDraw(&play, &player);
        DinFireSword_Draw(&play, &player);
        elementalActive = elementalSelection = element % 3 + 1;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
    }
    AuraSetup(0);
    enabled = alt = assets = 1;
    tick();
    REQUIRE(draw() == 0);
}
static void ActualResolvedHand(void) {
    REQUIRE(DinFireSword_SetHandModel && DinFireSword_CopyHandModel && DinFireSword_DrawHand);
    for (int element = 1; element <= 3; ++element) {
        AuraSetup(element);
        player.heldItemId = ITEM_SWORD_BGS;
        player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
        player.leftHandType = PLAYER_MODELTYPE_LH_BGS;
        renderFairyUpgrade = 1;
        tick();
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        DinFireSword_BeginPlayerDraw(&play, &player);
        // CustomEquipment retained an actual broken blade after the GF upgrade.
        DinFireSword_SetHandModel(&player, swordBlade, SWORD_AURA_NATIVE_OOT_BROKEN);
        DinFireSword_CopyHandModel(&player, swordBlade, handDL);
        DinFireSword_DrawHand(&play, &player, handDL);
        DinFireSword_DrawAfterPlayer(&play, &player);
        AuraGbi result = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
        REQUIRE(result.max[0] >= 1474 && result.max[0] < 1600);
        // A later PAK winner with a different list cannot inherit that fit.
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        DinFireSword_BeginPlayerDraw(&play, &player);
        DinFireSword_SetHandModel(&player, swordBlade, SWORD_AURA_NATIVE_OOT_BROKEN);
        DinFireSword_DrawHand(&play, &player, handDL);
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p).max[0] >= 5500);
    }
    Gfx* alias = sAcceptedBrokenAlias;
    REQUIRE(SwordAura_ResourceModel("__OTR__objects/object_custom_equip/gCustomBrokenLongswordDL", alias, 5) ==
            SWORD_AURA_DIN_BROKEN);
    for (int element = 1; element <= 3; ++element) {
        AuraSetup(element);
        player.heldItemId = ITEM_SWORD_BGS;
        player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
        player.leftHandType = PLAYER_MODELTYPE_LH_BGS;
        renderFairyUpgrade = 1;
        tick();
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        DinFireSword_BeginPlayerDraw(&play, &player);
        Gfx* selected = hand;
        // Execute CustomEquipment's real compose-and-identify function on the
        // accepted pack's broken alias, after the logical Great Fairy upgrade.
        BuildSwordHandItemDL(&play, &player, &selected, hand, alias,
                             "__OTR__objects/object_custom_equip/gCustomBrokenLongswordDL", false);
        DinFireSword_DrawHand(&play, &player, selected);
        DinFireSword_DrawAfterPlayer(&play, &player);
        AuraGbi result = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
        REQUIRE(result.max[0] >= 2500 && result.max[0] < 2700);
    }
    for (int graph = 0; graph <= 1; ++graph) {
        AuraSetup(2);
        renderKokiriUpgrade = 1;
        player.heldItemId = ITEM_SWORD_KOKIRI;
        player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
        auraHeldGraph = graph;
        auraNativePieces = 1;
        tick();
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        DinFireSword_BeginPlayerDraw(&play, &player);
        Gfx* selected = hand;
        REQUIRE(WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12, 34, 56));
        Gfx copied[8];
        memcpy(copied, selected, sizeof(copied));
        DinFireSword_CopyHandModel(&player, selected, copied);
        DinFireSword_DrawHand(&play, &player, copied);
        DinFireSword_DrawAfterPlayer(&play, &player);
        int tip = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p).max[0];
        REQUIRE(graph ? tip >= 4000 && tip < 4080 : tip >= 1902 && tip < 1982);
    }
}

int main(void) {
    Profiles();
    LifetimeAndColors();
    OptionalPackDoesNotSelectGeometry();
    VisibilityAndCapturedHand();
    ActualResolvedHand();
    puts("PASS SoH selected sword auras: built-in GBI/texture/mesh, profiles, hand capture, pause/scene lifetime and "
         "colors");
    return 0;
}
