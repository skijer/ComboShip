#define ResourceMgr_FileExists AuraLegacyFileExists
#define ResourceMgr_LoadGfxByName AuraLegacyLoadGfx
#include "din_fire_fixture.h"
#undef ResourceMgr_FileExists
#undef ResourceMgr_LoadGfxByName
#include "../../tests/elemental_swords/aura_gbi_checks.h"
#include "../../tests/elemental_swords/aura_held_fixture.h"
extern void DinFireSword_SetHandModel(Player*, void*, int) __attribute__((weak));
extern void DinFireSword_CopyHandModel(Player*, void*, void*) __attribute__((weak));
extern void DinFireSword_DrawHand(PlayState*, Player*, void*) __attribute__((weak));

static void AuraSetup(int element) {
    Setup();
    elementalSelection = elementalActive = element;
    swordOption = alt = assets = 0;
    resourceQueries = 0;
    DinFireSword_Reset();
}
static void AuraTick(void) {
    ++play.gameplayFrames;
    DinFireSword_Update(&play, &player);
}
static size_t AuraSubmit(void) {
    ResetBuffers();
    Player before = player;
    SaveContext save = gSaveContext;
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_Draw(&play, &player);
    DinFireSword_DrawAfterPlayer(&play, &player);
    REQUIRE(!memcmp(&before, &player, sizeof(player)) && !memcmp(&save, &gSaveContext, sizeof(save)));
    REQUIRE(matrixDepth == 0 && matrixCount < ARRAY_COUNT(matrices));
    return (gfx.polyOpa.p - opa) + (gfx.polyXlu.p - xlu);
}
static AuraGbi AuraDraw(void) {
    REQUIRE(AuraSubmit() > 0 && resourceQueries == 0);
    AuraGbi result = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
    REQUIRE(result.textures == 2 && result.triangles > 20 && result.vertices > 20);
    REQUIRE(result.matrices == (elementalActive == 1 ? 2 : 1));
    REQUIRE(result.min[0] > 0 && result.min[1] > -2000 && result.max[1] < 2000);
    return result;
}
static void Profiles(void) {
    for (int element = 1; element <= 3; ++element) {
        for (int age = 0; age < 2; ++age) {
            const u16 items[] = { ITEM_SWORD_KOKIRI, ITEM_SWORD_RAZOR, ITEM_SWORD_GILDED, ITEM_SWORD_MASTER,
                                  ITEM_SWORD_MASTER, ITEM_SWORD_BGS,   ITEM_SWORD_KNIFE,  ITEM_SWORD_GREAT_FAIRY };
            const s32 actions[] = { PLAYER_IA_SWORD_KOKIRI,     PLAYER_IA_SWORD_RAZOR,     PLAYER_IA_SWORD_GILDED,
                                    PLAYER_IA_SWORD_MASTER,     PLAYER_IA_SWORD_MASTER,    PLAYER_IA_SWORD_TWO_HANDED,
                                    PLAYER_IA_SWORD_TWO_HANDED, PLAYER_IA_SWORD_TWO_HANDED };
            for (int profile = 0; profile < 8; ++profile) {
                AuraSetup(element);
                adult = age;
                player.heldItemId = items[profile];
                player.itemAction = player.heldItemAction = actions[profile];
                player.leftHandType =
                    profile < 5 ? PLAYER_MODELTYPE_LH_ONE_HAND_SWORD : PLAYER_MODELTYPE_LH_TWO_HAND_SWORD;
                auraTrueMaster = profile == 4;
                AuraTick();
                AuraGbi result = AuraDraw();
                int tip = profile == 0 || (profile == 1 && !adult) ? 3000
                          : profile < 5                            ? 4000
                          : profile == 6                           ? 2500
                                                                   : 5500;
                REQUIRE(result.max[0] >= tip - 10);
                REQUIRE(result.max[0] <= tip + (element == 1 ? tip / 10 : 80));
                if (element != 1)
                    REQUIRE(gfx.polyOpa.p == opa);
            }
            for (int level = 1; level <= 2; ++level) {
                AuraSetup(element);
                adult = age;
                kokiriUpgrade = level;
                AuraTick();
                AuraDraw();
            }
        }
    }
}
static void LifetimeAndColors(void) {
    uintptr_t texture[3];
    for (int element = 1; element <= 3; ++element) {
        AuraSetup(element);
        AuraTick();
        AuraGbi first = AuraDraw();
        texture[element - 1] = first.texture[1];
        Gfx* end = gfx.polyXlu.p;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(end == gfx.polyXlu.p);
        AuraTick();
        AuraGbi next = AuraDraw();
        REQUIRE(first.scroll != next.scroll);
        play.pauseCtx.state = 1;
        AuraTick();
        AuraGbi paused = AuraDraw();
        REQUIRE(next.scroll == paused.scroll);
        play.pauseCtx.state = 0;
        ResetBuffers();
        DinFireSword_BeginPlayerDraw(&play, &player);
        DinFireSword_Draw(&play, &player);
        ++play.gameplayFrames;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
        ++play.sceneId;
        REQUIRE(AuraSubmit() == 0);
        AuraTick();
        AuraDraw();
        elementalActive = 0;
        REQUIRE(AuraSubmit() == 0);
    }
    REQUIRE(texture[0] != texture[1] && texture[1] != texture[2] && texture[0] != texture[2]);
    AuraSetup(1);
    customColors = 1;
    AuraTick();
    AuraDraw();
    REQUIRE(AuraHasColor(opa, gfx.polyOpa.p, G_SETPRIMCOLOR, 0x29F1C7));
    REQUIRE(AuraHasColor(xlu, gfx.polyXlu.p, G_SETENVCOLOR, 0x0C21B1));
    AuraSetup(2);
    AuraTick();
    AuraDraw();
    REQUIRE(AuraHasColor(xlu, gfx.polyXlu.p, G_SETENVCOLOR, 0x36A8FF));
    AuraSetup(3);
    AuraTick();
    AuraDraw();
    REQUIRE(AuraHasColor(xlu, gfx.polyXlu.p, G_SETENVCOLOR, 0xFFD26A));
}
static void OptionalPackDoesNotSelectGeometry(void) {
    for (int age = 0; age < 2; ++age) {
        AuraSetup(1);
        adult = age;
        AuraTick();
        AuraGbi native = AuraDraw();
        swordOption = alt = assets = 1;
        REQUIRE(DinFireSword_HandDL(&play, &player, hand) == NULL);
        REQUIRE(AuraSubmit() > 0);
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
            AuraTick();
            switch (gate) {
                case 0:
                    player.actor.scale.y = -.01f;
                    break;
                case 1:
                    player.transformation = PLAYER_FORM_DEKU;
                    break;
                case 2:
                    customForm = CUSTOM_FORM_KEATON;
                    break;
                case 3:
                    hideSword = 1;
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
                    player.stateFlags1 |= PLAYER_STATE1_8000000;
                    break;
                case 10:
                    player.stateFlags2 |= PLAYER_STATE2_20000000;
                    break;
                case 11:
                    play.transitionTrigger = 1;
                    break;
                case 12:
                    player.leftHandType = PLAYER_MODELTYPE_LH_CLOSED;
                    break;
            }
            REQUIRE(AuraSubmit() == 0);
        }
        AuraSetup(element);
        AuraTick();
        ResetBuffers();
        DinFireSword_BeginPlayerDraw(&play, &player);
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
        REQUIRE(AuraHasColor(opa, gfx.polyOpa.p, G_SETPRIMCOLOR, 0x5D2FC9));
        REQUIRE(AuraHasColor(opa, gfx.polyOpa.p, G_SETENVCOLOR, 0x11CA40));
        currentMatrix.xw = 999.f;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(matrixCount == 1 && !memcmp(&captured[0], &wrist, sizeof(wrist)));
        REQUIRE(currentMatrix.xw == 999.f && matrixDepth == 0);
        REQUIRE(AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p).textures == 2);
        ResetBuffers();
        DinFireSword_BeginPlayerDraw(&play, &player);
        DinFireSword_Draw(&play, &player);
        elementalActive = elementalSelection = element % 3 + 1;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
    }
    AuraSetup(0);
    swordOption = alt = assets = 1;
    AuraTick();
    REQUIRE(AuraSubmit() == 0);
}
static void ActualResolvedHand(void) {
    REQUIRE(DinFireSword_SetHandModel && DinFireSword_CopyHandModel && DinFireSword_DrawHand);
    const char* native[] = {
        "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingKokiriSwordDL",
        "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingRazorSwordDL",
        "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingGildedSwordDL",
        "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingGreatFairysSwordDL",
    };
    const int tips[] = { 1727, 1902, 3348, 5498 };
    for (int element = 1; element <= 3; ++element) {
        for (int model = 0; model < 4; ++model) {
            AuraSetup(element);
            AuraTick();
            ResetBuffers();
            DinFireSword_BeginPlayerDraw(&play, &player);
            // Missing OoT donor graph in real MM solo retains its native hand.
            DinFireSword_DrawHand(&play, &player, (void*)native[model]);
            DinFireSword_DrawAfterPlayer(&play, &player);
            AuraGbi result = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p);
            REQUIRE(result.textures == 2 && result.max[0] >= tips[model]);
            REQUIRE(result.max[0] < tips[model] + (element == 1 ? 400 : 80));
            REQUIRE(resourceQueries == 0);
        }
    }
    for (int graph = 0; graph <= 1; ++graph) {
        AuraSetup(2);
        kokiriUpgrade = 1;
        auraHeldGraph = graph;
        auraNativePieces = 1;
        AuraTick();
        ResetBuffers();
        DinFireSword_BeginPlayerDraw(&play, &player);
        Gfx* selected = hand;
        REQUIRE(WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12, 34, 56));
        Gfx copied[8];
        memcpy(copied, selected, sizeof(copied));
        DinFireSword_CopyHandModel(&player, selected, copied);
        DinFireSword_DrawHand(&play, &player, copied);
        DinFireSword_DrawAfterPlayer(&play, &player);
        int tip = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p).max[0];
        REQUIRE(graph ? tip >= 3000 && tip < 3080 : tip >= 1902 && tip < 1982);
    }
    for (int bgs = 0; bgs < 2; ++bgs) {
        AuraSetup(2);
        assets = 1;
        auraHeldGraph = 0;
        player.heldItemId = bgs ? ITEM_SWORD_BGS : ITEM_SWORD_MASTER;
        player.itemAction = player.heldItemAction = bgs ? PLAYER_IA_SWORD_TWO_HANDED : PLAYER_IA_SWORD_MASTER;
        player.leftHandType = bgs ? PLAYER_MODELTYPE_LH_TWO_HAND_SWORD : PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
        AuraTick();
        ResetBuffers();
        DinFireSword_BeginPlayerDraw(&play, &player);
        Gfx* selected = hand;
        AuraNativeImportedHands(&player, &selected);
        DinFireSword_DrawHand(&play, &player, selected);
        DinFireSword_DrawAfterPlayer(&play, &player);
        int tip = AuraInspect(opa, gfx.polyOpa.p, xlu, gfx.polyXlu.p).max[0];
        REQUIRE(bgs ? tip >= 5492 && tip < 5572 : tip >= 3387 && tip < 3467);
    }
}
int main(void) {
    Profiles();
    LifetimeAndColors();
    OptionalPackDoesNotSelectGeometry();
    VisibilityAndCapturedHand();
    ActualResolvedHand();
    puts("PASS MM selected sword auras: built-in GBI/texture/mesh, native/progressive profiles, captured pose and "
         "paused lifetime");
    return 0;
}
