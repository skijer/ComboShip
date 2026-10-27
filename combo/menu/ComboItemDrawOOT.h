#include "ComboIkanaShieldResources.h"
#include "ItemGrantAuditBridge.h"
/* combo/menu/ComboItemDrawOOT.h — ComboShip: OOT-side bodies of the cross-game item-draw exports
 * (combo/menu/ComboItemDrawABI.h). The exact mirror of combo/menu/ComboItemDrawMM.h, in the opposite
 * direction: MM (2ship.dll) asks soh.dll which display lists render a foreign OOT item, then submits
 * them through "__OTR__@oot:"-routed paths resolved against OOT's ResourceManager (CrossRMRegistry).
 *
 * Combo-OWNED source compiled INTO soh.dll (the menu-extraction pattern) so the vendored item_list.cpp
 * keeps only a single include. Vanilla items defer to sDrawItemTable (soh/src/code/z_draw.c) via
 * GetItem_GetDrawTableEntry; the ~110 rando items that carry a bespoke Randomizer_Draw* func
 * (Item::SetCustomDrawFunc) are invisible to that gid-keyed table and are described by hand in
 * OOT_DescribeCustomDraw below.
 *
 * TU-GLUE HEADER: include ONCE from soh/soh/Enhancements/randomizer/item_list.cpp, inside its
 * #ifdef COMBO_BUILD, AFTER the Rando StaticData / item / ItemTableTypes headers are in scope. Not
 * standalone.
 */
#ifndef COMBO_ITEM_DRAW_OOT_H
#define COMBO_ITEM_DRAW_OOT_H

#include "ComboItemDrawABI.h"
#include "ComboCollectibleShimmerHost.h"

#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#include "ComboLiveCosmetics.h"
#include "ComboExport.h"
#include "ComboMaskShimmer.h"
#include "ComboSongDrawOOT.h"
#include "ComboBottleContents.h"
#include "ComboItemIconOwnership.h"
#include "objects/object_gi_melody/object_gi_melody.h"
#include "ComboItemEffectColors.h"
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "soh/Enhancements/randomizer/dungeon.h" // concrete normal/MQ keyring indices
#include "libultraship/bridge.h"                 // CVarGetInteger / CVarGetColor24 (cosmetic key/nut colors)
#include "libultraship/color.h"                  // Color_RGB8
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h" // COLORSCHEME_*
#include "variables.h"
#include "soh/OTRGlobals.h"             // rando-context null guards (MM calls us while OOT is dormant)
#include "soh/ResourceManagerHelpers.h" // owner-specific model queries used by Ikana recipes
#include <string>
#include <atomic>
#include <spdlog/spdlog.h>
#include "objects/object_gi_fire/object_gi_fire.h"           // gGiBlueFireFlameDL (boss-soul flame)
#include "objects/object_gi_key/object_gi_key.h"             // gGiSmallKeyDL
#include "objects/object_gi_bosskey/object_gi_bosskey.h"     // gGiBossKeyDL / gGiBossKeyGemDL
#include "objects/object_gi_map/object_gi_map.h"             // gGiDungeonMapDL
#include "objects/object_gi_compass/object_gi_compass.h"     // gGiCompassDL / gGiCompassGlassDL
#include "objects/object_gi_hearts/object_gi_hearts.h"       // gGiHeartBorderDL / gGiHeartContainerDL
#include "objects/object_gi_scale/object_gi_scale.h"         // gGiScaleDL / gGiScaleWaterDL
#include "objects/object_gi_bomb_2/object_gi_bomb_2.h"       // gGiBombchuDL
#include "objects/object_mamenoki/object_mamenoki.h"         // gMagicBeanSeedlingDL
#include "objects/object_toki_objects/object_toki_objects.h" // Master Sword model
#include "objects/object_gi_rupy/object_gi_rupy.h"           // gGiRupeeInnerDL / gGiRupeeOuterDL
#include "objects/object_gi_melody/object_gi_melody.h"       // gGiSongNoteDL
#include "objects/gameplay_keep/gameplay_keep.h"             // gRupeeDL / gRupeeSilverTex
#include <atomic>
#include <spdlog/spdlog.h>
// Boss-soul skeletons/animations/textures for the animated cross-game class (issue #86).
#include "objects/object_goma/object_goma.h"
#include "objects/object_kingdodongo/object_kingdodongo.h"
#include "objects/object_gnd/object_gnd.h"
#include "objects/object_fd/object_fd.h"
#include "objects/object_sst/object_sst.h"
#include "objects/object_tw/object_tw.h"
#include "objects/object_ganon2/object_ganon2.h"
#include "objects/object_bv/object_bv.h"               // Barinade's procedural skeletal model
#include "objects/object_mo/object_mo.h"               // non-skeletal Morpha core model
#include "overlays/actors/ovl_Boss_Goma/z_boss_goma.h" // BOSSGOMA_LIMB_EYE / BOSSGOMA_LIMB_IRIS

// Cosmetic tables owned by soh/.../draw.cpp; reused so the recipes can't drift from the real funcs.
extern "C" SaveContext gSaveContext; // Triforce shard count (live save state)
extern const char* SmallBodyCvarValue[10];
extern const char* SmallEmblemCvarValue[10];
extern Color_RGB8 SmallEmblemDefaultValue[10];
extern Color_RGB8 MapOrCompassColor[10];

// Portable slice of one sDrawItemTable row (defined COMBO_BUILD-guarded in soh/src/code/z_draw.c).
// outDrawKind = CwDrawKind (0 = simple OPA/XLU submission; else a non-portable func the consumer
// replicates). outColors is 16 bytes: primXlu[4], envXlu[4], primOpa[4], envOpa[4] (JEWEL/MUSIC_NOTE).
extern "C" s32 GetItem_GetDrawTableEntry(s32 drawId, void** outDlists, s32 maxDlists, s32* outXluStart, f32* outScale,
                                         s32* outDrawKind, uint8_t* outColors);

// Which setup DL the row's func emits (NULL = plain 25). The consumer must submit the same one.
extern "C" void GetItem_GetDrawSetupDLs(s32 drawId, void** outOpa, void** outXlu);
extern "C" void* GetItem_GetSetupDL(s32 index); // by index, for the bespoke Randomizer_Draw* funcs
extern "C" s32 GetItem_GetShimmerColor(s16 drawId, uint8_t color[4]);

// --- CW_DRAW_KIND_COLOR_LAYERS helpers: attach a prim/env color to one display list slot.
static void CwLayerPrim(CwItemDrawInfo* out, int32_t i, Color_RGB8 c) {
    out->layerPrimColor[i][0] = c.r;
    out->layerPrimColor[i][1] = c.g;
    out->layerPrimColor[i][2] = c.b;
    out->layerPrimColor[i][3] = 255;
    out->layerPrimMask |= 1 << i;
}
static void CwLayerEnv(CwItemDrawInfo* out, int32_t i, Color_RGB8 c) {
    out->layerEnvColor[i][0] = c.r;
    out->layerEnvColor[i][1] = c.g;
    out->layerEnvColor[i][2] = c.b;
    out->layerEnvColor[i][3] = 255;
    out->layerEnvMask |= 1 << i;
}

// Plain scaled OPA/XLU submission (the SIMPLE path) for funcs with no special GPU state.
static int32_t CwSimple(CwItemDrawInfo* out, const char* dl, bool xlu, float scale) {
    out->drawKind = CW_DRAW_KIND_SIMPLE;
    out->dlistCount = 1;
    out->xluStartIndex = xlu ? 0 : -1;
    out->scale = scale;
    out->dlists[0] = dl;
    return 1;
}

// Static custom GI bridge recipes. Paths stay in their asset owner's namespace, so its
// selected Alt equipment is resolved by that resource manager at deferred submission time.
extern "C" int32_t OOT_NeiAltAssetsEnabled(void);

static int32_t CwCustomGi(CwItemDrawInfo* out, const char* opa, float scale, const char* xlu = nullptr) {
    if (!OOT_NeiResourceExists(opa) || (xlu && !OOT_NeiResourceExists(xlu)))
        return 0; // Never queue a half-present split model or an unresolved archive path.
    CwSimple(out, opa, false, scale);
    out->drawKind = CW_DRAW_KIND_CUSTOM_GI;
    if (xlu) {
        out->dlists[1] = xlu;
        out->dlistCount = 2;
        out->xluStartIndex = 1;
    }
    return 1;
}

static int32_t CwNativeEquipment(CwItemDrawInfo* out, CwOotNativeEquipment equipment) {
    out->drawKind = CW_DRAW_KIND_OOT_NATIVE_EQUIPMENT;
    out->opCount = 1;
    out->ops[0] = { CW_OP_NATIVE_EQUIPMENT, static_cast<float>(equipment), 0, 0, {} };
    return 1;
}

static int32_t CwFlameGi(CwItemDrawInfo* out, const char* path, float scale, Color_RGB8 flame) {
    if (!CwCustomGi(out, path, scale))
        return 0;
    out->primColorXlu[0] = flame.r;
    out->primColorXlu[1] = flame.g;
    out->primColorXlu[2] = flame.b;
    out->primColorXlu[3] = 255;
    return 1;
}

static int32_t CwNativeMmSword(CwItemDrawInfo* out, const char* body, const char* detail, bool xlu) {
    CwSimple(out, body, false, 0.0f); // MM's GI-authored meshes need no scale or extra rotation.
    out->dlists[1] = detail;
    out->dlistCount = 2;
    out->xluStartIndex = xlu ? 1 : -1;
    return 1;
}

// Only standalone equipment DLs are eligible: never submit a player fist/body as a GI.
// The concrete award selects the sword family before this cosmetic choice is frozen on grant.
static int32_t CwAltSwordGi(RandomizerGet rg, CwItemDrawInfo* out) {
    if (!OOT_NeiAltAssetsEnabled())
        return 0;
    const char* selected = nullptr;
    const char* fire = nullptr;
    bool trueMaster = false;
    NeiGi::Kind shimmer = NeiGi::Kind::Neutral;
    switch (rg) {
        case RG_KOKIRI_SWORD:
        case RG_RAZOR_SWORD:
        case RG_GILDED_SWORD:
            shimmer = rg == RG_KOKIRI_SWORD  ? NeiGi::Kind::KokiriSword
                      : rg == RG_RAZOR_SWORD ? NeiGi::Kind::RazorSword
                                             : NeiGi::Kind::GildedSword;
            selected = "__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL";
            fire = "__OTR__objects/din_fire_sword/progressive/child/SwordDL";
            break;
        case RG_TRUE_MASTER_SWORD:
            trueMaster = true;
            shimmer = NeiGi::Kind::SwordAura;
            [[fallthrough]];
        case RG_MASTER_SWORD:
            if (!trueMaster)
                shimmer = NeiGi::Kind::MasterSword;
            selected = "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL";
            fire = "__OTR__objects/din_fire_sword/progressive/adult/SwordDL";
            break;
        case RG_BIGGORON_SWORD:
        case RG_GIANTS_KNIFE:
        case RG_GREAT_FAIRY_SWORD:
            shimmer = rg == RG_GREAT_FAIRY_SWORD ? NeiGi::Kind::GreatFairySword
                      : rg == RG_GIANTS_KNIFE    ? NeiGi::Kind::GiantsKnife
                                                 : NeiGi::Kind::BiggoronSword;
            selected = "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL";
            fire = "__OTR__objects/din_fire_sword/progressive/bgs/SwordDL";
            break;
        default:
            return 0;
    }
    if (CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0) && OOT_NeiResourceExists(fire))
        selected = fire;
    if (!CwCustomGi(out, selected, .04f))
        return 0;
    // This route returns before the authored descriptor. Carry the concrete
    // award's identity even when selected geometry owns the entire model.
    out->neiShimmer = static_cast<int32_t>(shimmer) + 1;
    out->itemShimmer = 1;
    out->stateDependent = 2;
    // Standalone equipment follows the native hand-local +X blade axis.
    // Tilt it into +Y; an X quarter-turn after this would lay it flat in XZ.
    out->opCount = 1;
    out->ops[0] = { CW_OP_ROTATE_Z, 16384.f, 0, 0, {} }; // +X blade to straight-up +Y.
    if (trueMaster) {
        out->primColorXlu[0] = 120;
        out->primColorXlu[1] = 180;
        out->primColorXlu[2] = 255;
        out->primColorXlu[3] = 255;
    }
    return 1;
}

extern "C" int32_t OOT_DescribeMmMaskDraw(int32_t itemId, CwItemDrawInfo* out);
extern "C" int32_t OOT_DescribeMmRemainsDraw(int32_t rg, CwItemDrawInfo* out);
extern "C" int32_t OOT_DescribeMmSpinAttackDraw(CwItemDrawInfo* out);

// ComboShip: describe the bespoke Randomizer_Draw* funcs (Item::SetCustomDrawFunc), which the
// gid-keyed sDrawItemTable is blind to — without this those items draw a plausible-but-wrong vanilla
// model. Each branch is a 1:1 description of the func in soh/.../randomizer/draw.cpp. Returns 1 when
// rg is handled; 0 means "fall through to the gid table".
static int32_t OOT_DescribeCustomDraw(RandomizerGet rg, CwItemDrawInfo* out) {
    // Include all 24 imported masks, including the five whose legacy native GIDs
    // would otherwise select OoT's child-trade model instead of MM's mask.
    static_assert(RG_MM_MASK_FIERCE_DEITY - RG_MM_MASK_POSTMAN == 23);
    static_assert(ITEM_MM_MASK_FIERCE_DEITY - ITEM_MM_MASK_POSTMAN == 23);
    if (rg >= RG_MM_MASK_POSTMAN && rg <= RG_MM_MASK_FIERCE_DEITY) {
        return OOT_DescribeMmMaskDraw(ITEM_MM_MASK_POSTMAN + (int32_t)rg - (int32_t)RG_MM_MASK_POSTMAN, out);
    }

    if (rg >= RG_MM_REMAINS_ODOLWA && rg <= RG_MM_REMAINS_TWINMOLD) {
        return OOT_DescribeMmRemainsDraw((int32_t)rg, out);
    }

    if (rg == RG_MM_GREAT_SPIN_ATTACK) {
        return OOT_DescribeMmSpinAttackDraw(out);
    }

    // RPG stat models use custom OPA draws; their legacy spell GIDs are only
    // placeholders and would otherwise show the wrong item at MM checks.
    switch (rg) {
        case RG_DEFENSE_UPGRADE:
            return CwSimple(out, gStatDefenseDL, false, 1.0f);
        case RG_SPEED_UPGRADE:
            return CwSimple(out, gStatSpeedDL, false, 1.0f);
        case RG_POWER_UPGRADE:
            return CwSimple(out, gStatPowerDL, false, 1.0f);
        case RG_CRAWL_SPEED_UPGRADE:
            return CwSimple(out, gStatCrawlSpeedDL, false, 1.0f);
        case RG_CLIMB_SPEED_UPGRADE:
            return CwSimple(out, gStatClimbSpeedDL, false, 1.0f);
        case RG_PUSH_SPEED_UPGRADE:
            return CwSimple(out, gStatPushSpeedDL, false, 1.0f);
        default:
            break;
    }

    // Boss souls: bespoke func the gid-keyed table can't express (vestigial skull-token gid).
    // The non-skeletal Morpha core has its own consumer. Keep canonical owner paths so its
    // Alt replacement is selected by OoT's RM just like the native DrawMorpha routine.
    if (rg >= RG_GOHMA_SOUL && rg <= RG_GANON_SOUL) {
        static const uint8_t flameColors[9][3] = {
            { 0, 255, 0 },     // Gohma
            { 255, 0, 100 },   // King Dodongo
            { 50, 255, 255 },  // Barinade
            { 4, 195, 46 },    // Phantom Ganon
            { 237, 95, 95 },   // Volvagia
            { 85, 180, 223 },  // Morpha
            { 126, 16, 177 },  // Bongo Bongo
            { 222, 158, 47 },  // Twinrova
            { 150, 150, 150 }, // Ganon
        };
        int slot = (int)rg - (int)RG_GOHMA_SOUL;
        out->dlistCount = 2;
        out->drawKind = CW_DRAW_KIND_BOSS_SOUL;
        out->xluStartIndex = 0;              // both layers XLU
        out->dlists[0] = gGiBlueFireFlameDL; // flame (grayscale-tinted)
        out->dlists[1] = gBossSoulSkullDL;   // generic soul skull
        if (slot == 5 && !CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("SimplerBossSoulModels"), 0)) {
            out->drawKind = CW_DRAW_KIND_OOT_MORPHA_SOUL;
            out->dlistCount = 3;
            out->dlists[1] = gMorphaCoreMembraneDL;
            out->dlists[2] = gMorphaCoreNucleusDL;
        }
        uint8_t skullEnv = (slot == 8) ? 0 : 255; // Ganon skull env black, else white
        for (int c = 0; c < 3; c++) {
            out->primColorXlu[c] = flameColors[slot][c];
            out->envColorXlu[c] = skullEnv;
        }
        out->primColorXlu[3] = 255;
        out->envColorXlu[3] = 255;
        return 1;
    }

    // Dungeon maps: prim-colored map body (Randomizer_DrawMap).
    if (rg >= RG_DEKU_TREE_MAP && rg <= RG_ICE_CAVERN_MAP) {
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlistCount = 1;
        out->xluStartIndex = -1;
        out->dlists[0] = gGiDungeonMapDL;
        CwLayerPrim(out, 0, MapOrCompassColor[rg - RG_DEKU_TREE_MAP]);
        return 1;
    }

    // Compasses: prim+half-env body (OPA) + glass (XLU) (Randomizer_DrawCompass).
    if (rg >= RG_DEKU_TREE_COMPASS && rg <= RG_ICE_CAVERN_COMPASS) {
        Color_RGB8 c = MapOrCompassColor[rg - RG_DEKU_TREE_COMPASS];
        out->setupDlXlu = GetItem_GetSetupDL(SETUPDL_5); // the glass layer, as Randomizer_DrawCompass does
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlistCount = 2;
        out->xluStartIndex = 1;
        out->dlists[0] = gGiCompassDL;
        out->dlists[1] = gGiCompassGlassDL;
        CwLayerPrim(out, 0, c);
        CwLayerEnv(out, 0, Color_RGB8{ (uint8_t)(c.r / 2), (uint8_t)(c.g / 2), (uint8_t)(c.b / 2) });
        return 1;
    }

    bool customKeys = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("CustomKeyModels"), 1) != 0;

    // Boss keys: custom body (env keyColor, OPA) + dungeon gem icon (env gemColor, XLU).
    if (rg >= RG_FOREST_TEMPLE_BOSS_KEY && rg <= RG_GANONS_CASTLE_BOSS_KEY) {
        static const char* icons[6] = {
            gBossKeyIconForestTempleDL, gBossKeyIconFireTempleDL,   gBossKeyIconWaterTempleDL,
            gBossKeyIconSpiritTempleDL, gBossKeyIconShadowTempleDL, gBossKeyIconGanonsCastleDL,
        };
        static const char* cvars[6] = {
            "gCosmetics.Key.ForestBoss", "gCosmetics.Key.FireBoss",   "gCosmetics.Key.WaterBoss",
            "gCosmetics.Key.SpiritBoss", "gCosmetics.Key.ShadowBoss", "gCosmetics.Key.GanonsBoss",
        };
        int slot = rg - RG_FOREST_TEMPLE_BOSS_KEY;
        out->stateDependent = 2;
        out->dlistCount = 2;
        out->xluStartIndex = 1;
        if (!customKeys) {
            out->drawKind = CW_DRAW_KIND_GRAYSCALE_LAYERS;
            out->dlists[0] = gGiBossKeyDL;
            out->dlists[1] = gGiBossKeyGemDL;
            if (CVarGetInteger((std::string(cvars[slot]) + "Body.Changed").c_str(), 0))
                CwLayerPrim(out, 0,
                            CwLiveCosmeticColor((std::string(cvars[slot]) + "Body.Value").c_str(), { 255, 255, 0 }));
            if (CVarGetInteger((std::string(cvars[slot]) + "Gem.Changed").c_str(), 0))
                CwLayerPrim(out, 1,
                            CwLiveCosmeticColor((std::string(cvars[slot]) + "Gem.Value").c_str(), { 255, 0, 0 }));
            return 1;
        }
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlists[0] = gBossKeyCustomDL;
        out->dlists[1] = icons[slot];
        CwLayerEnv(out, 0, CwLiveCosmeticColor((std::string(cvars[slot]) + "Body.Value").c_str(), { 255, 255, 0 }));
        CwLayerEnv(out, 1, CwLiveCosmeticColor((std::string(cvars[slot]) + "Gem.Value").c_str(), { 255, 0, 0 }));
        return 1;
    }

    // Small keys: custom body (env keyColor, OPA) + dungeon emblem (env emblemColor, XLU).
    if (rg >= RG_FOREST_TEMPLE_SMALL_KEY && rg <= RG_TREASURE_GAME_SMALL_KEY) {
        static const char* icons[10] = {
            gSmallKeyIconForestTempleDL,         gSmallKeyIconFireTempleDL,     gSmallKeyIconWaterTempleDL,
            gSmallKeyIconSpiritTempleDL,         gSmallKeyIconShadowTempleDL,   gSmallKeyIconBottomoftheWellDL,
            gSmallKeyIconGerudoTrainingGroundDL, gSmallKeyIconGerudoFortressDL, gSmallKeyIconGanonsCastleDL,
            gSmallKeyIconTreasureChestGameDL,
        };
        int slot = rg - RG_FOREST_TEMPLE_SMALL_KEY;
        out->stateDependent = 2;
        if (!customKeys) {
            CwSimple(out, gGiSmallKeyDL, false, 0.0f);
            out->drawKind = CW_DRAW_KIND_GRAYSCALE_LAYERS;
            CwLayerPrim(out, 0, CwLiveCosmeticColor(SmallBodyCvarValue[slot], { 255, 255, 255 }));
            return 1;
        }
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlistCount = 2;
        out->xluStartIndex = 1;
        out->dlists[0] = gSmallKeyCustomDL;
        out->dlists[1] = icons[slot];
        CwLayerEnv(out, 0, CwLiveCosmeticColor(SmallBodyCvarValue[slot], { 255, 255, 255 }));
        CwLayerEnv(out, 1, CwLiveCosmeticColor(SmallEmblemCvarValue[slot], SmallEmblemDefaultValue[slot]));
        return 1;
    }

    // Key rings retain the accepted concrete normal/MQ cluster recipes. Native
    // 9.3 repeats individual keys with matrix changes that the layer ABI cannot replay.
    if (rg >= RG_FOREST_TEMPLE_KEY_RING && rg <= RG_TREASURE_GAME_KEY_RING) {
        static const char* icons[10] = {
            gKeyringIconForestTempleDL,         gKeyringIconFireTempleDL,     gKeyringIconWaterTempleDL,
            gKeyringIconSpiritTempleDL,         gKeyringIconShadowTempleDL,   gKeyringIconBottomoftheWellDL,
            gKeyringIconGerudoTrainingGroundDL, gKeyringIconGerudoFortressDL, gKeyringIconGanonsCastleDL,
            gKeyringIconTreasureChestGameDL,
        };
        static const char* keys[10] = {
            gKeyringKeysForestTempleDL,         gKeyringKeysFireTempleDL,     gKeyringKeysWaterTempleDL,
            gKeyringKeysSpiritTempleDL,         gKeyringKeysShadowTempleDL,   gKeyringKeysBottomoftheWellDL,
            gKeyringKeysGerudoTrainingGroundDL, gKeyringKeysGerudoFortressDL, gKeyringKeysGanonsCastleDL,
            gKeyringKeysTreasureChestGameDL,
        };
        static const char* keysMQ[10] = {
            gKeyringKeysForestTempleMQDL,         gKeyringKeysFireTempleMQDL,   gKeyringKeysWaterTempleMQDL,
            gKeyringKeysSpiritTempleMQDL,         gKeyringKeysShadowTempleMQDL, gKeyringKeysBottomoftheWellMQDL,
            gKeyringKeysGerudoTrainingGroundMQDL, gKeyringKeysGerudoFortressDL, gKeyringKeysGanonsCastleMQDL,
            gKeyringKeysTreasureChestGameDL,
        };
        // 0 = not tied to a dungeon (Gerudo Fortress / Treasure Chest Game), so never MQ.
        static const Rando::DungeonKey slotDungeon[10] = {
            Rando::FOREST_TEMPLE, Rando::FIRE_TEMPLE,        Rando::WATER_TEMPLE,           Rando::SPIRIT_TEMPLE,
            Rando::SHADOW_TEMPLE, Rando::BOTTOM_OF_THE_WELL, Rando::GERUDO_TRAINING_GROUND, (Rando::DungeonKey)0,
            Rando::GANONS_CASTLE, (Rando::DungeonKey)0,
        };
        int slot = rg - RG_FOREST_TEMPLE_KEY_RING;
        out->stateDependent = 2;
        if (!customKeys) { // the vanilla path stacks five keys via matrix chaining — not portable
            CwSimple(out, gGiSmallKeyDL, false, 0.0f);
            out->drawKind = CW_DRAW_KIND_GRAYSCALE_LAYERS;
            CwLayerPrim(out, 0, CwLiveCosmeticColor(SmallBodyCvarValue[slot], { 255, 255, 255 }));
            return 1;
        }
        bool mq = slotDungeon[slot] != 0 && Rando::Context::GetInstance()->GetDungeon(slotDungeon[slot])->IsMQ();
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlistCount = 3;
        out->xluStartIndex = -1;
        out->dlists[0] = mq ? keysMQ[slot] : keys[slot];
        out->dlists[1] = gKeyringRingDL;
        out->dlists[2] = icons[slot];
        CwLayerEnv(out, 0, CwLiveCosmeticColor(SmallBodyCvarValue[slot], { 255, 255, 255 }));
        CwLayerEnv(out, 1, CwLiveCosmeticColor(CVAR_COSMETIC("Key.KeyringRing.Value"), { 255, 255, 255 }));
        CwLayerEnv(out, 2, CwLiveCosmeticColor(SmallEmblemCvarValue[slot], SmallEmblemDefaultValue[slot]));
        return 1;
    }

    // Overworld keys: white prim+env house key (Randomizer_DrawOverworldKey).
    if (rg >= RG_GUARD_HOUSE_KEY && rg <= RG_FISHING_HOLE_KEY) {
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlistCount = 1;
        out->xluStartIndex = -1;
        out->dlists[0] = gHouseKeyDL;
        CwLayerPrim(out, 0, { 255, 255, 255 });
        CwLayerEnv(out, 0, { 255, 255, 255 });
        return 1;
    }

    // Jabber Nuts: per-race (or generic) nut tinted by env color (Randomizer_DrawJabberNut).
    if (rg >= RG_SPEAK_DEKU && rg <= RG_SPEAK_ZORA) {
        static const char* nuts[6] = {
            gGiDekuJabbernutDL,   gGiGerudoJabbernutDL, gGiGoronJabbernutDL,
            gGiHylianJabbernutDL, gGiKokiriJabbernutDL, gGiZoraJabbernutDL,
        };
        static const char* colorCvars[6] = {
            CVAR_COSMETIC("Equipment.DekuJabberNut.Value"),   CVAR_COSMETIC("Equipment.GerudoJabberNut.Value"),
            CVAR_COSMETIC("Equipment.GoronJabberNut.Value"),  CVAR_COSMETIC("Equipment.HylianJabberNut.Value"),
            CVAR_COSMETIC("Equipment.KokiriJabberNut.Value"), CVAR_COSMETIC("Equipment.ZoraJabberNut.Value"),
        };
        static const Color_RGB8 defaults[6] = {
            { 255, 160, 32 }, { 128, 64, 0 }, { 255, 32, 0 }, { 255, 255, 0 }, { 128, 216, 48 }, { 96, 240, 255 },
        };
        int slot = rg - RG_SPEAK_DEKU;
        bool generic = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("GenericJabberNutModel"), 0) != 0;
        out->setupDlOpa = GetItem_GetSetupDL(SETUPDL_26); // Randomizer_DrawJabberNut uses 26Opa
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlistCount = 1;
        out->xluStartIndex = -1;
        out->dlists[0] = generic ? gGiJabbernutDL : nuts[slot];
        CwLayerEnv(out, 0,
                   generic ? CVarGetColor24(CVAR_COSMETIC("Equipment.JabberNut.Value"), { 255, 0, 216 })
                           : CVarGetColor24(colorCvars[slot], defaults[slot]));
        return 1;
    }

    // Bean souls: scaled magic-bean seedling (Randomizer_DrawBeanSprout).
    if (rg >= RG_DEATH_MOUNTAIN_CRATER_BEAN_SOUL && rg <= RG_ZORAS_RIVER_BEAN_SOUL) {
        return CwSimple(out, gMagicBeanSeedlingDL, false, 0.3f);
    }

    // Ocarina buttons: grayscale-tinted button glyph on XLU (Randomizer_DrawOcarinaButton).
    if (rg >= RG_OCARINA_A_BUTTON && rg <= RG_OCARINA_C_RIGHT_BUTTON) {
        static const char* buttons[5] = {
            gOcarinaAButtonDL,     gOcarinaCUpButtonDL,    gOcarinaCDownButtonDL,
            gOcarinaCLeftButtonDL, gOcarinaCRightButtonDL,
        };
        Color_RGB8 color;
        if (rg == RG_OCARINA_A_BUTTON) {
            color = { 80, 150, 255 };
            if (CVarGetInteger(CVAR_COSMETIC("HUD.AButton.Changed"), 0)) {
                color = CVarGetColor24(CVAR_COSMETIC("HUD.AButton.Value"), color);
            } else if (CVarGetInteger(CVAR_COSMETIC("DefaultColorScheme"), COLORSCHEME_N64) == COLORSCHEME_GAMECUBE) {
                color = { 80, 255, 150 };
            }
        } else {
            static const char* cCvars[4] = {
                CVAR_COSMETIC("HUD.CUpButton"),
                CVAR_COSMETIC("HUD.CDownButton"),
                CVAR_COSMETIC("HUD.CLeftButton"),
                CVAR_COSMETIC("HUD.CRightButton"),
            };
            color = { 255, 255, 50 };
            if (CVarGetInteger(CVAR_COSMETIC("HUD.CButtons.Changed"), 0)) {
                color = CVarGetColor24(CVAR_COSMETIC("HUD.CButtons.Value"), color);
            }
            std::string base = cCvars[rg - RG_OCARINA_C_UP_BUTTON];
            if (CVarGetInteger((base + ".Changed").c_str(), 0)) {
                color = CVarGetColor24((base + ".Value").c_str(), color);
            }
        }
        out->drawKind = CW_DRAW_KIND_GRAYSCALE_XLU;
        out->dlistCount = 1;
        out->xluStartIndex = 0;
        out->dlists[0] = buttons[rg - RG_OCARINA_A_BUTTON];
        out->primColorXlu[0] = color.r;
        out->primColorXlu[1] = color.g;
        out->primColorXlu[2] = color.b;
        out->primColorXlu[3] = 255;
        return 1;
    }

    // Bombchu Bag: mask + body, each with its own env color, both OPA (Randomizer_DrawBombchuBag).
    if (rg == RG_PROGRESSIVE_BOMBCHU_BAG || rg == RG_BOMBCHU_INF || rg == RG_BOMBCHU_20) {
        // RG_BOMBCHU_20 only draws the bag when the bombchu-bag option is on (DrawBombchuBagInLogic).
        if (rg == RG_BOMBCHU_20 &&
            OTRGlobals::Instance->gRandoContext->GetOption(RSK_BOMBCHU_BAG).Is(RO_BOMBCHU_BAG_NONE)) {
            CwSimple(out, gGiBombchuDL, false, 0.0f);
            out->setupDlOpa = GetItem_GetSetupDL(SETUPDL_26); // Randomizer_DrawBombchuBag uses 26Opa
            return 1;
        }
        out->setupDlOpa = GetItem_GetSetupDL(SETUPDL_26);
        out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
        out->dlistCount = 2;
        out->xluStartIndex = -1;
        out->dlists[0] = gBombchuBagMaskDL;
        out->dlists[1] = gBombchuBagBodyDL;
        CwLayerEnv(out, 0, CVarGetColor24(CVAR_COSMETIC("Equipment.ChuFace.Value"), { 0, 100, 150 }));
        CwLayerEnv(out, 1, CVarGetColor24(CVAR_COSMETIC("Equipment.ChuBody.Value"), { 180, 130, 50 }));
        return 1;
    }

    // Silver rupees: inner/outer rupee (NewDrops) or the textured drop rupee (Randomizer_DrawSilverRupee).
    if (rg >= RG_SHADOW_SILVER_BLADES && rg <= RG_GANONS_CASTLE_MQ_SILVER_SHADOW) {
        Color_RGB8 c = CwLiveCosmeticColor(CVAR_COSMETIC("Consumable.SilverRupee.Value"), { 255, 255, 255 });
        out->stateDependent = 2; // Keep palette / NewDrops selection live after the receipt is latched.
        out->itemShimmer = ComboCollectibleShimmer_Color(CW_COLLECTIBLE_SILVER, out->itemShimmerColor);
        if (CVarGetInteger("gEnhancements.NewDrops", 0) != 0) {
            out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
            out->dlistCount = 2;
            out->xluStartIndex = 1;
            out->dlists[0] = gGiRupeeInnerDL;
            out->dlists[1] = gGiRupeeOuterDL;
            CwLayerPrim(out, 0, c);
            CwLayerEnv(out, 0, Color_RGB8{ (uint8_t)(c.r / 5), (uint8_t)(c.g / 5), (uint8_t)(c.b / 5) });
            CwLayerPrim(out, 1, { 255, 255, 255 });
            CwLayerEnv(out, 1, Color_RGB8{ (uint8_t)(c.r * 0.75f), (uint8_t)(c.g * 0.75f), (uint8_t)(c.b * 0.75f) });
            return 1;
        }
        out->drawKind = CW_DRAW_KIND_SILVER_RUPEE;
        out->dlistCount = 1;
        out->xluStartIndex = -1;
        out->scale = 0.05f;
        out->dlists[0] = gRupeeDL;
        out->segTexPath = gRupeeSilverTex;
        if (CVarGetInteger(CVAR_COSMETIC("Consumable.SilverRupee.Changed"), 0)) {
            out->primColorOpa[0] = c.r;
            out->primColorOpa[1] = c.g;
            out->primColorOpa[2] = c.b;
            out->primColorOpa[3] = 255;
        }
        return 1;
    }

    // Nut bag: the Deku Nuts seg8 scroll under 26 Opa (Randomizer_DrawNutBag).
    if (rg == RG_DEKU_NUT_BAG || rg == RG_NUT_UPGRADE_INF || rg == RG_DEKU_NUT_CAPACITY_30 ||
        rg == RG_DEKU_NUT_CAPACITY_40) {
        out->setupDlOpa = GetItem_GetSetupDL(SETUPDL_26);
        out->drawKind = CW_DRAW_KIND_DEKU_NUTS;
        out->dlistCount = 1;
        out->xluStartIndex = -1;
        out->dlists[0] = gGiNutBagDL;
        return 1;
    }

    // Stick bag: plain model under 26 Opa (Randomizer_DrawStickBag).
    if (rg == RG_DEKU_STICK_BAG || rg == RG_STICK_UPGRADE_INF || rg == RG_DEKU_STICK_CAPACITY_20 ||
        rg == RG_DEKU_STICK_CAPACITY_30) {
        CwSimple(out, gGiStickBagDL, false, 0.0f);
        out->setupDlOpa = GetItem_GetSetupDL(SETUPDL_26);
        return 1;
    }

    switch (rg) {
        case RG_SCARECROWS_SONG: // dark green grayscale note (Randomizer_DrawScarecrowsSong)
            out->drawKind = CW_DRAW_KIND_GRAYSCALE_XLU;
            out->dlistCount = 1;
            out->xluStartIndex = 0;
            out->dlists[0] = gGiSongNoteDL;
            out->primColorXlu[0] = 40;
            out->primColorXlu[1] = 160;
            out->primColorXlu[2] = 40;
            out->primColorXlu[3] = 255;
            return 1;
        case RG_BOMB_ARROWS:
            if (!CwCustomGi(out, "__OTR__objects/object_nei_bombarrows/gBombarrowsGiveDL", .5f))
                return 0;
            out->opCount = 1;
            out->ops[0] = { CW_OP_ROTATE_Z, 32768.f, 0, 0, {} };
            return 1;
        case RG_EXT_CANE_OF_BYRNA:
            return CwCustomGi(out, "__OTR__objects/object_somaria/g_byrna_cane_give_dl", .25f);
        case RG_LANTERN:
            out->stateDependent = 2;
            return CwCustomGi(out, "__OTR__objects/object_poh/gPoeLanternDL", .025f);
        case RG_POKEBALL:
            out->neiEffect = static_cast<int32_t>(NeiGi::Kind::Pokeball);
            out->itemShimmer = CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0) != 0;
            out->stateDependent = 2;
            return CwCustomGi(out, "__OTR__objects/object_nei_pokeball/ItmPokeBall_opaque_dl", .18f);
        case RG_MARIO_MASK:
            out->stateDependent = 2;
            if (!CwCustomGi(out, "__OTR__objects/object_nei_mario_mask/g_mario_mask_dl", .038f))
                return 0;
            out->opCount = 2;
            out->ops[0] = { CW_OP_ROTATE_X, -16384.0f, 0, 0, {} };
            out->ops[1] = { CW_OP_NO_CULL, 0, 0, 0, {} };
            out->itemShimmer = 1;
            ComboOotMaskShimmerColor(8, out->itemShimmerColor);
            return 1;
        case RG_NET:
            return CwCustomGi(out, "__OTR__objects/object_nei_net/g_net_dl", .55f,
                              "__OTR__objects/object_nei_net/g_net_xlu_dl");
        case RG_ULTRASHOT:
            return CwFlameGi(out, "__OTR__objects/object_gi_hookshot/gGiLongshotDL", .5f, { 255, 240, 130 });
        case RG_BOTTOMLESS_BOTTLE:
            if (!CwCustomGi(out, "__OTR__objects/object_gi_bottle/gGiBottleStopperDL", 0.0f,
                            "__OTR__objects/object_gi_bottle/gGiBottleDL"))
                return 0;
            out->primColorXlu[0] = 190;
            out->primColorXlu[1] = 60;
            out->primColorXlu[2] = 230;
            out->primColorXlu[3] = 255;
            return 1;
        case RG_EXT_FOUR_SWORD:
            if (!CwCustomGi(out, "__OTR__objects/object_nei_four_sword/gNeiFourSwordBladeDL", .04f,
                            "__OTR__objects/object_nei_four_sword/gNeiFourSwordHiltDL"))
                return 0;
            out->xluStartIndex = -1; // Both blade and hilt are opaque in the native callback.
            out->opCount = 1;
            out->ops[0] = { CW_OP_ROTATE_Z, 16384.f, 0, 0, {} };
            return 1;
        case RG_EXT_SHIELD_OF_IKANA: {
            if (!ResourceMgr_IsIkanaShieldGiAvailable())
                return 0;
            out->stateDependent = 2;
            CwSimple(out, COMBO_IKANA_GI_OPA_MM, false, 0.f);
            out->dlists[1] = COMBO_IKANA_GI_XLU_MM;
            out->dlistCount = 2;
            out->xluStartIndex = 1;
            float tilt = 0.f, roll = 0.f;
            ResourceMgr_GetIkanaShieldGiPoseForGame("mm", COMBO_IKANA_GI_XLU, &tilt, &roll);
            // MM's SIMPLE consumer accepts pose-only rotations without
            // adding the custom-GI spin or changing native pass selection.
            out->opCount = 0;
            if (roll != 0.f)
                out->ops[out->opCount++] = { CW_OP_ROTATE_Z, roll * (32768.f / 3.14159265358979323846f), 0, 0, {} };
            if (tilt != 0.f)
                out->ops[out->opCount++] = { CW_OP_ROTATE_X, tilt * (32768.f / 3.14159265358979323846f), 0, 0, {} };
            return 1;
        }
        case RG_CLAWSHOT:
            return CwSimple(out, "__OTR__@mm:objects/object_gi_hookshot/gGiHookshotDL", false, 0.0f);
        case RG_EXT_PENDANT_OF_MEMORIES:
            CwSimple(out, "__OTR__@mm:objects/object_gi_reserve_c_01/gGiPendantOfMemoriesDL", false, .9f);
            out->drawKind = CW_DRAW_KIND_CUSTOM_GI;
            return 1;
        case RG_MM_PENDANT_OF_MEMORIES:
            // The imported row wins the shared English name at catalog boot.
            // Match MM's native OPA shell then XLU pendant, in MM's namespace.
            CwSimple(out, "__OTR__@mm:objects/object_gi_reserve_c_01/gGiPendantOfMemoriesEmptyDL", false, 0.f);
            out->dlists[1] = "__OTR__@mm:objects/object_gi_reserve_c_01/gGiPendantOfMemoriesDL";
            out->dlistCount = 2;
            out->xluStartIndex = 1;
            out->itemShimmer = 1;
            out->neiShimmer = static_cast<int32_t>(NeiGi::Kind::PendantOfMemories) + 1;
            out->stateDependent = 2;
            return 1;
        case RG_EXT_MAGIC_CAPE:
            if (!CwCustomGi(out, "__OTR__objects/object_nei_magic_cape/gNeiMagicCapeDL", .55f,
                            "__OTR__objects/object_nei_magic_cape/gNeiMagicCapeWaveDL"))
                return 0;
            out->xluStartIndex = -1;
            out->opCount = 2;
            out->ops[0] = { CW_OP_FRAME_PAIR, 3.0f, 0, 0, {} };
            out->ops[1] = { CW_OP_TRANSLATE, 0, 22.0f, 0, {} };
            return 1;
        case RG_IRON_KNUCKLE_AXE:
            return CwNativeEquipment(out, CW_OOT_EQUIP_AXE);
        case RG_EXT_SPIRIT_BREASTPLATE:
            return CwNativeEquipment(out, CW_OOT_EQUIP_SPIRIT_TUNIC);
        case RG_EXT_CHAMPIONS_TUNIC:
            return CwNativeEquipment(out, CW_OOT_EQUIP_CHAMPIONS_TUNIC);
        case RG_EXT_WATER_DRAGON_SCALE:
            return CwNativeEquipment(out, CW_OOT_EQUIP_SAGES_TUNIC);
        case RG_EXT_PEGASUS_ANKLET:
            return CwNativeEquipment(out, CW_OOT_EQUIP_PEGASUS_BOOTS);
        case RG_EXT_TRIDENT:
            return CwNativeEquipment(out, CW_OOT_EQUIP_TRIDENT);
        case RG_EXT_CLIMB_BOOTS:
            return CwNativeEquipment(out, CW_OOT_EQUIP_CLIMB_BOOTS);
        case RG_EXT_ROC_BOOTS:
            return CwNativeEquipment(out, CW_OOT_EQUIP_ROC_BOOTS);
        case RG_WAND_SAND_ROD:
            return CwCustomGi(out, "__OTR__objects/object_nei_wand_sand_rod/gNeiSandRodDL", .12f,
                              "__OTR__objects/object_nei_wand_sand_rod/gNeiSandRodXluDL");
        case RG_WAND_TORNADO_ROD:
            return CwCustomGi(out, "__OTR__objects/object_nei_wand_tornado_rod/gNeiTornadoRodDL", .12f,
                              "__OTR__objects/object_nei_wand_tornado_rod/gNeiTornadoRodXluDL");
        case RG_WAND_WATER_ROD:
            return CwCustomGi(out, "__OTR__objects/object_nei_wand_water_rod/gNeiWaterRodDL", .12f,
                              "__OTR__objects/object_nei_wand_water_rod/gNeiWaterRodXluDL");
        case RG_WAND_METEOR_ROD:
            return CwCustomGi(out, "__OTR__objects/object_nei_wand_meteor_rod/gNeiMeteorRodDL", .12f,
                              "__OTR__objects/object_nei_wand_meteor_rod/gNeiMeteorRodXluDL");
        case RG_WAND_STORM_ROD:
            return CwCustomGi(out, "__OTR__objects/object_nei_wand_storm_rod/gNeiStormRodDL", .12f,
                              "__OTR__objects/object_nei_wand_storm_rod/gNeiStormRodXluDL");
        case RG_WAND_SHADOW_SCEPTER:
            return CwCustomGi(out, "__OTR__objects/object_nei_wand_shadow_scepter/gNeiShadowScepterDL", .12f,
                              "__OTR__objects/object_nei_wand_shadow_scepter/gNeiShadowScepterXluDL");
        case RG_SEASON_SPRING:
        case RG_SEASON_SUMMER:
        case RG_SEASON_AUTUMN:
        case RG_SEASON_WINTER:
            out->drawKind = CW_DRAW_KIND_SEASON_GI;
            out->neiEffect = 1 + rg - RG_SEASON_SPRING;
            out->dlistCount = 0;
            out->xluStartIndex = -1;
            return 1;
        case RG_SHEIKAH_SLATE:
            return CwCustomGi(out, "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL", .35f);
        case RG_SLATE_RUNE_BOMB:
            return CwFlameGi(out, "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL", .35f, { 95, 220, 235 });
        case RG_SLATE_RUNE_MASTER_CYCLE:
            return CwFlameGi(out, "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL", .35f,
                             { 100, 230, 190 });
        case RG_SLATE_RUNE_STASIS:
            return CwFlameGi(out, "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL", .35f, { 250, 200, 70 });
        case RG_SLATE_RUNE_CRYONIS:
            return CwFlameGi(out, "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL", .35f,
                             { 150, 215, 255 });
        case RG_DESIRE_SENSOR:
            return CwFlameGi(out, "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL", .35f,
                             { 200, 130, 255 });
        case RG_PHANTOM_HOURGLASS:
            return CwCustomGi(out, "__OTR__objects/object_nei_phantom_hourglass/gNeiPhantomHourglassDL", .35f);
        case RG_SHADOW_CRYSTAL:
            return CwCustomGi(out, "__OTR__objects/object_nei_shadow_crystal/gNeiShadowCrystalDL", .35f);
        case RG_ROD_OF_SEASONS:
            if (!CwCustomGi(out, "__OTR__objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL", .35f))
                return 0;
            out->drawKind = CW_DRAW_KIND_SEASON_GI;
            out->neiEffect = 5;
            return 1;
        case RG_QUARTZ_OF_MOTION:
            return CwCustomGi(out, "__OTR__objects/object_nei_quartz_of_motion/gNeiQuartzOfMotionDL", .25f);
        case RG_EXT_DIVINE_SHIELD:
            return CwCustomGi(out, "__OTR__objects/object_nei_divine_shield/g_divine_shield_dl", .9f);
        case RG_EXT_SHEIKAH_SHIELD:
            return CwCustomGi(out, "__OTR__objects/object_nei_kite_shield/g_kite_shield_dl", .9f);
        case RG_RAZOR_SWORD:
            return CwNativeMmSword(out, "__OTR__@mm:objects/object_gi_sword_2/gGiRazorSwordDL",
                                   "__OTR__@mm:objects/object_gi_sword_2/gGiRazorSwordEmptyDL", false);
        case RG_GILDED_SWORD:
            return CwNativeMmSword(out, "__OTR__@mm:objects/object_gi_sword_3/gGiGildedSwordDL",
                                   "__OTR__@mm:objects/object_gi_sword_3/gGiGildedSwordEmptyDL", false);
        case RG_GREAT_FAIRY_SWORD:
            return CwNativeMmSword(out, "__OTR__@mm:objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL",
                                   "__OTR__@mm:objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL", true);
        case RG_TRUE_MASTER_SWORD:
            out->primColorXlu[0] = 120;
            out->primColorXlu[1] = 180;
            out->primColorXlu[2] = 255;
            out->primColorXlu[3] = 255; // Same independent weapon flame as the native callback.
            [[fallthrough]];
        case RG_MASTER_SWORD: // seg8 scroll + scale/rotate (Randomizer_DrawMasterSword)
            out->drawKind = CW_DRAW_KIND_MASTER_SWORD;
            out->dlistCount = 1;
            out->xluStartIndex = -1;
            out->dlists[0] = object_toki_objects_DL_001BD0;
            return 1;
        case RG_DOUBLE_DEFENSE: // grayscale-white border + heart container (Randomizer_DrawDoubleDefense)
            out->drawKind = CW_DRAW_KIND_DOUBLE_DEFENSE;
            out->dlistCount = 2;
            out->xluStartIndex = 0;
            out->dlists[0] = gGiHeartBorderDL;
            out->dlists[1] = gGiHeartContainerDL;
            return 1;
        case RG_BRONZE_SCALE: // scale model recolored bronze (Randomizer_DrawBronzeScale)
            out->drawKind = CW_DRAW_KIND_BRONZE_SCALE;
            out->dlistCount = 2;
            out->xluStartIndex = 0;
            out->dlists[0] = gGiScaleDL;
            out->dlists[1] = gGiScaleWaterDL;
            return 1;
        case RG_SKELETON_KEY: // env-tinted skeleton key (Randomizer_DrawSkeletonKey)
            out->drawKind = CW_DRAW_KIND_COLOR_LAYERS;
            out->dlistCount = 1;
            out->xluStartIndex = -1;
            out->dlists[0] = gSkeletonKeyDL;
            CwLayerEnv(out, 0, CVarGetColor24(CVAR_COSMETIC("Key.Skeleton.Value"), { 255, 255, 170 }));
            return 1;
        case RG_TRIFORCE_PIECE: { // shard cycles with the collected count (GetItem_DrawTriforcePiece)
            uint8_t current = gSaveContext.ship.quest.data.randomizer.triforcePiecesCollected % 3;
            const char* shard = (current == 1)   ? gTriforcePiece1DL
                                : (current == 2) ? gTriforcePiece2DL
                                                 : gTriforcePiece0DL;
            return CwSimple(out, shard, false, 0.035f);
        }
        case RG_TRIFORCE: // completed triforce (Randomizer_DrawTriforcePieceGI's fullTriforce model)
            return CwSimple(out, gTriforcePieceCompletedDL, false, 0.035f);
        case RG_ROCS_FEATHER:
            return CwSimple(out, gGiRocsFeatherDL, true, 0.0f);
        case RG_POWER_BRACELET:
            return CwSimple(out, gGiGrabDL, false, 0.0f);
        case RG_CLIMB:
            return CwSimple(out, gGiClimbDL, false, 0.0f);
        case RG_CRAWL:
            return CwSimple(out, gGiCrawlDL, false, 0.0f);
        case RG_OPEN_CHEST:
            return CwSimple(out, gGiOpenChestsDL, false, 0.0f);
        case RG_FISHING_POLE:
            return CwSimple(out, gGiFishingPoleDL, false, 0.0f);
        default:
            return 0;
    }
}

// Items whose model is chosen from live save state, so the consumer must re-resolve them every frame
// instead of caching the first model it saw (Item::GetGIEntry progressive tiers + Triforce shards).
static bool OOT_IsStateDependentDraw(RandomizerGet rg) {
    switch (rg) {
        case RG_PROGRESSIVE_ROCS:
        case RG_CANE_OF_SOMARIA:
        case RG_PROGRESSIVE_KOKIRI_SWORD:
        case RG_PROGRESSIVE_MASTER_SWORD:
        case RG_PROGRESSIVE_BGS:
        case RG_PROGRESSIVE_HAMMER:
        case RG_STONE_OF_AGONY:
        case RG_PROGRESSIVE_HOOKSHOT:
        case RG_PROGRESSIVE_STRENGTH:
        case RG_PROGRESSIVE_BOMB_BAG:
        case RG_PROGRESSIVE_BOW:
        case RG_PROGRESSIVE_SLINGSHOT:
        case RG_PROGRESSIVE_WALLET:
        case RG_PROGRESSIVE_SCALE:
        case RG_PROGRESSIVE_NUT_UPGRADE:
        case RG_PROGRESSIVE_STICK_UPGRADE:
        case RG_PROGRESSIVE_MAGIC_METER:
        case RG_PROGRESSIVE_OCARINA:
        case RG_PROGRESSIVE_GORONSWORD:
        case RG_PROGRESSIVE_BOMBCHU_BAG:
        case RG_TRIFORCE_PIECE:
            return true;
        default:
            return false;
    }
}

static int32_t OOT_DrawDependency(RandomizerGet rg, const CwItemDrawInfo& info) {
    if (rg == RG_MM_GREAT_SPIN_ATTACK || (rg >= RG_GOHMA_SOUL && rg <= RG_GANON_SOUL))
        return 2;
    if (OOT_IsStateDependentDraw(rg))
        return 1;
    if (info.stateDependent)
        return info.stateDependent;
    if (info.drawKind == CW_DRAW_KIND_NEI_GI)
        return 2;
    // Concrete swords can switch between the redesign and selected weapon pack.
    // Refresh both recipes; progressive requests still freeze at grant time above.
    switch (rg) {
        // Selected bottle recipes change their OPA/XLU split. Refresh the
        // descriptor after Alt changes while keeping acquisition state frozen.
        case RG_BOTTLE_WITH_RED_POTION:
        case RG_BOTTLE_WITH_GREEN_POTION:
        case RG_BOTTLE_WITH_BLUE_POTION:
        case RG_RED_POTION_REFILL:
        case RG_GREEN_POTION_REFILL:
        case RG_BLUE_POTION_REFILL:
        case RG_BUY_RED_POTION_30:
        case RG_BUY_RED_POTION_40:
        case RG_BUY_RED_POTION_50:
        case RG_BUY_GREEN_POTION:
        case RG_BUY_BLUE_POTION:
        case RG_KOKIRI_SWORD:
        case RG_RAZOR_SWORD:
        case RG_GILDED_SWORD:
        case RG_MASTER_SWORD:
        case RG_TRUE_MASTER_SWORD:
        case RG_BIGGORON_SWORD:
        case RG_GREAT_FAIRY_SWORD:
            return 2;
        default:
            return 0;
    }
}

extern "C" int32_t NeiGi_DescribeEntry(const GetItemEntry* entry, CwItemDrawInfo* out);

extern "C" int32_t OOT_MagicJarUsesCustomAsset(const char* path);
extern "C" int32_t OOT_ApplyGiHeartCosmetics(const char* path, int32_t piece, uint8_t r, uint8_t g, uint8_t b,
                                             int32_t changed);

// Match native GetItem_DrawDListWithCosmetics for custom magic jars. The donor
// owns both the Alt classification and cosmetic value; the host only submits it.
static void OOT_DescribeMagicJar(s16 drawId, CwItemDrawInfo* out) {
    if (drawId != GID_MAGIC_SMALL && drawId != GID_MAGIC_LARGE)
        return;
    out->drawKind = CW_DRAW_KIND_MAGIC_JAR;
    out->stateDependent = 2; // live cosmetic edits and Alt toggles must re-resolve
    const bool changed = CVarGetInteger(CVAR_COSMETIC("Consumable.Magic.Changed"), 0);
    const Color_RGB8 color =
        changed ? CwLiveCosmeticColor(CVAR_COSMETIC("Consumable.Magic.Value"), { 0, 200, 0 }) : Color_RGB8{ 0, 200, 0 };
    out->itemShimmer = 1;
    out->itemShimmerColor[0] = color.r;
    out->itemShimmerColor[1] = color.g;
    out->itemShimmerColor[2] = color.b;
    out->itemShimmerColor[3] = 255;
    if (changed && OOT_MagicJarUsesCustomAsset(out->dlists[0])) {
        out->primColorOpa[0] = color.r;
        out->primColorOpa[1] = color.g;
        out->primColorOpa[2] = color.b;
        out->primColorOpa[3] = 255;
    }
}

// Keep the border pass independent. The native body uses the editor's own
// prim/env patch IDs; selected custom bodies use the accepted grayscale scope.
static void OOT_DescribeHeartCosmetics(s16 drawId, CwItemDrawInfo* out) {
    if (drawId != GID_HEART_PIECE && drawId != GID_HEART_CONTAINER)
        return;
    out->stateDependent = 2;
    if (out->dlistCount < 2)
        return;
    const Color_RGB8 color = CwLiveCosmeticColor(CVAR_COSMETIC("Consumable.Hearts.Value"), { 255, 70, 50 });
    const int32_t changed = CVarGetInteger(CVAR_COSMETIC("Consumable.Hearts.Changed"), 0);
    const int32_t custom =
        OOT_ApplyGiHeartCosmetics(out->dlists[1], drawId == GID_HEART_PIECE, color.r, color.g, color.b, changed);
    if (custom && changed) {
        out->drawKind = CW_DRAW_KIND_GRAYSCALE_LAYERS;
        CwLayerPrim(out, 1, color);
    }
}

static int32_t OOT_FillSongDrawInfo(RandomizerGet rg, CwItemDrawInfo* out) {
    const int song = ComboSongForOotItem(rg);
    if (song < 0)
        return 0;
    out->drawKind = CW_DRAW_KIND_SONG_GI;
    out->neiEffect = song;
    const char* colorDl = ComboSongOotColorDlist(song);
    out->dlistCount = 0;
    if (colorDl)
        out->dlists[out->dlistCount++] = colorDl;
    out->dlists[out->dlistCount++] = gGiSongNoteDL;
    out->xluStartIndex = 0;
    out->itemShimmer = ComboSongHasOverlay(song);
    ComboSongShimmerColor(song, out->primColorXlu);
    std::memcpy(out->itemShimmerColor, out->primColorXlu, 4);
    return 1;
}

static int32_t OOT_FillItemDrawInfo(RandomizerGet rg, CwItemDrawInfo* out) {
    RandomizerGet actual = RG_NONE;
    GetItemEntry gi = *Rando::StaticData::RetrieveItem(rg).GetGIEntry(&actual);
    // Self/base tiers also need a frozen name (Cane, Roc, Stone of Agony).
    // Otherwise a repeat receipt could ask the donor to describe its next tier.
    out->resolvedName = Rando::StaticData::RetrieveItem(actual != RG_NONE ? actual : rg).GetName().english.c_str();
    // Progressive items resolve to the tier actually owed; classify THAT item's draw func, not the
    // placeholder's (drawItemId carries the resolved RandomizerGet for rando-table entries).
    RandomizerGet effRg = actual != RG_NONE                  ? actual
                          : (gi.tableId == TABLE_RANDOMIZER) ? (RandomizerGet)gi.drawItemId
                                                             : rg;
    if (effRg == RG_MAGIC_MIRROR)
        return CwDescribeMagicMirror(out);
    switch (effRg) {
        case RG_KOKIRI_SWORD:
        case RG_MASTER_SWORD:
        case RG_BIGGORON_SWORD:
        case RG_RAZOR_SWORD:
        case RG_GILDED_SWORD:
        case RG_TRUE_MASTER_SWORD:
        case RG_GREAT_FAIRY_SWORD:
            out->itemShimmer = 1;
            ComboMaskShimmerColor(0, out->itemShimmerColor);
            break;
        default:
            break;
    }
    int statProfile = -1;
    switch (effRg) {
        case RG_DEFENSE_UPGRADE:
            statProfile = 0;
            break;
        case RG_SPEED_UPGRADE:
            statProfile = 1;
            break;
        case RG_POWER_UPGRADE:
            statProfile = 2;
            break;
        case RG_MAGIC_STAT_UPGRADE:
            statProfile = 3;
            break;
        case RG_CRAWL_SPEED_UPGRADE:
            statProfile = 4;
            break;
        case RG_CLIMB_SPEED_UPGRADE:
            statProfile = 5;
            break;
        case RG_PUSH_SPEED_UPGRADE:
            statProfile = 6;
            break;
        default:
            break;
    }
    if (ComboRpgShimmerColor(statProfile, out->itemShimmerColor))
        out->itemShimmer = 1;
    if (CwAltSwordGi(effRg, out))
        return 1;
    if (NeiGi_DescribeEntry(&gi, out)) {
        return 1;
    }
    const int32_t customResult = OOT_DescribeCustomDraw(effRg, out);
    if (customResult != 0) {
        return customResult; // Preserve a donor's transient CW_DRAW_NOT_READY.
    }
    // Concrete skills must retain their own tint/flame even when they use the legacy cane.
    if (gi.drawFunc == Randomizer_DrawCaneOfSomaria)
        out->neiLegacyCane = 1;
    else if (gi.drawFunc == Randomizer_DrawCanePacci)
        out->neiLegacyCane = 2;
    else if (gi.drawFunc == Randomizer_DrawCaneSomariaUpgrade)
        out->neiLegacyCane = 3;
    else if (gi.drawFunc == Randomizer_DrawCanePacciUpgrade)
        out->neiLegacyCane = 4;
    else if (gi.drawFunc == Randomizer_DrawCanePacciUltrahand)
        out->neiLegacyCane = 6;
    if (out->neiLegacyCane) {
        out->drawKind = CW_DRAW_KIND_NEI_CANE;
        out->stateDependent = 2;
        out->itemShimmer = CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0) != 0;
        const bool pacci = out->neiLegacyCane == 2 || out->neiLegacyCane == 4 || out->neiLegacyCane == 6;
        const uint8_t color[4] = { 255, static_cast<uint8_t>(pacci ? 215 : 60), static_cast<uint8_t>(pacci ? 70 : 60),
                                   255 };
        std::memcpy(out->itemShimmerColor, color, 4);
        return 1;
    }
    if (effRg == RG_BOTTLE_WITH_MAGIC_MUSHROOM || effRg == RG_MM_BOTTLE_GOLD_DUST) {
        const int content = effRg == RG_BOTTLE_WITH_MAGIC_MUSHROOM ? CW_BOTTLE_MUSHROOM : CW_BOTTLE_GOLD_DUST;
        out->drawKind = CW_DRAW_KIND_BOTTLE_CONTENTS;
        out->dlistCount = 1;
        out->xluStartIndex = 0;
        out->dlists[0] = content == CW_BOTTLE_MUSHROOM ? "__OTR__objects/combo_bottle_gi/MushroomBottle"
                                                       : "__OTR__objects/combo_bottle_gi/GoldDustBottle";
        out->itemShimmer = ComboBottleContents_Color(content, out->itemShimmerColor);
        return 1;
    }
    if (OOT_FillSongDrawInfo(effRg, out))
        return 1;
    if (gi.drawFunc != nullptr) {
        return 0; // A custom callback has no native gid row; gid 0 would masquerade as a bottle.
    }
    // ComboShip: a bespoke draw func with no row above falls back to the gid model; say so once.
    if (gi.drawFunc != nullptr && effRg >= 0 && effRg < RG_MAX) {
        static std::atomic<bool> sWarned[RG_MAX];
        if (!sWarned[effRg].exchange(true)) {
            SPDLOG_WARN("[combo] OOT item {} has a custom draw func but no cross-game draw row", (int)effRg);
        }
    }
    void* dls[CW_DRAW_MAX_DLISTS] = {};
    int32_t xluStart = -1;
    f32 scale = 0.0f;
    int32_t drawKind = CW_DRAW_KIND_SIMPLE;
    uint8_t colors[16] = {};
    // Potion recipe classification loads the selected wrappers. The dormant
    // donor's Alt state must win even when MM currently owns the Context.
    const auto drawOwner = Ship::CrossRMRegistry::Get("oot");
    if (!drawOwner)
        return CW_DRAW_NOT_READY;
    Ship::ResourceManagerScope drawOwnerScope(drawOwner);
    int32_t n = GetItem_GetDrawTableEntry((s32)gi.gid, dls, CW_DRAW_MAX_DLISTS, &xluStart, &scale, &drawKind, colors);
    if (n <= 0) {
        return 0; // unsupported/non-portable draw func
    }
    out->dlistCount = n;
    out->xluStartIndex = xluStart;
    out->scale = scale;
    out->drawKind = drawKind;
    if (drawKind == CW_DRAW_KIND_ELEMENTAL_ARROW)
        out->neiEffect = NeiArrowGi_ProfileForDrawId(effRg, RG_FIRE_ARROWS, RG_ICE_ARROWS, RG_LIGHT_ARROWS);
    if (drawKind == CW_DRAW_KIND_MAGIC_SPELL)
        out->neiEffect = NeiArrowGi_ProfileForDrawId(effRg, RG_DINS_FIRE, RG_FARORES_WIND, RG_NAYRUS_LOVE);
    if (drawKind == CW_DRAW_KIND_FAIRY || effRg == RG_EMPTY_BOTTLE)
        out->stateDependent = 2; // selected bottle shell follows live owner Alt/mod state
    for (int32_t i = 0; i < 4; i++) {
        out->primColorXlu[i] = colors[i];
        out->envColorXlu[i] = colors[4 + i];
        out->primColorOpa[i] = colors[8 + i];
        out->envColorOpa[i] = colors[12 + i];
    }
    for (int32_t i = 0; i < n; i++) {
        out->dlists[i] = (const char*)dls[i];
    }
    // Rows drawn under a setup other than 25 (masks/bombchu/medallions = 26 Opa, sold-out/compass
    // = 5 Xlu): carry it so the consumer submits the same GPU state, not its own 25.
    void* setupOpa = nullptr;
    void* setupXlu = nullptr;
    GetItem_GetDrawSetupDLs((s32)gi.gid, &setupOpa, &setupXlu);
    out->setupDlOpa = setupOpa;
    out->setupDlXlu = setupXlu;
    uint8_t nativeShimmerColor[4] = {};
    const int32_t nativeShimmer = GetItem_GetShimmerColor((s16)gi.gid, nativeShimmerColor);
    if (ComboCollectibleShimmer_ProfileForDrawId((s16)gi.gid) >= 0)
        out->stateDependent = 2; // Refresh opt-in and owner palette even when shimmer was disabled.
    if (nativeShimmer) {
        out->itemShimmer = nativeShimmer;
        std::memcpy(out->itemShimmerColor, nativeShimmerColor, sizeof(nativeShimmerColor));
    }
    // Apply after native eligibility so dormant-owner rainbow sampling drives
    // the foreign jar body and its shimmer from the same host-frame hue.
    OOT_DescribeMagicJar((s16)gi.gid, out);
    OOT_DescribeHeartCosmetics((s16)gi.gid, out);
    return 1;
}

// Cross-game item draw info. MM resolves this via Combo_ResolveSym to learn which OOT display lists
// render a foreign item; itemName is the OOT English item name (the foreign map's grant key).
// Returns 0 for unknown/non-portable items, CW_DRAW_NOT_READY while OOT's rando state is down.
// The whole body is inside the try: an unwind across the C ABI into 2ship.dll is unrecoverable.
static bool OOT_BossSoulUsesSkeleton(RandomizerGet rg); // defined with the animated ABI below

extern "C" COMBO_EXPORT int32_t OOT_GetItemDrawInfo(const char* itemName, CwItemDrawInfo* out) {
    ItemGrantAudit::Scope itemGrantAuditScope("OOT preview-DrawInfo", -1, -1, true);
    try {
        if (itemName == nullptr || out == nullptr) {
            return 0;
        }
        auto& nameMap = Rando::StaticData::itemNameToEnum;
        auto it = nameMap.find(itemName);
        if (it == nameMap.end()) {
            return 0;
        }
        RandomizerGet rg = it->second;
        // Sold Out / Hint have no model of their own: Item::GetGIEntry falls back to RG_NONE, whose
        // gid 0 is GID_BOTTLE, so the gid path would draw a misleading bottle. Sentinel instead.
        if (rg == RG_NONE || rg == RG_COMBO_FOREIGN || rg == RG_SOLD_OUT || rg == RG_HINT) {
            return 0;
        }
        // Boss souls drawing their real skeleton have no static DL row — served by the animated ABI.
        if (OOT_BossSoulUsesSkeleton(rg)) {
            return 0;
        }
        // We run on MM's graph thread while OOT is dormant, and Item::GetGIEntry dereferences
        // gRandomizer/gRandoContext for progressive tiers. Transient: tell MM to retry, not to
        // negative-cache the sentinel for the rest of the save slot.
        if (OTRGlobals::Instance == nullptr || OTRGlobals::Instance->gRandomizer == nullptr ||
            OTRGlobals::Instance->gRandoContext == nullptr) {
            return CW_DRAW_NOT_READY;
        }
        *out = CwItemDrawInfo{};
        const int32_t result = OOT_FillItemDrawInfo(rg, out);
        if (result != 1) {
            return result;
        }
        // Keep appearance-only refreshes live after acquisition freezes progressive tiers.
        out->stateDependent = OOT_DrawDependency(rg, *out);
        return 1;
    } catch (...) { return 0; }
}

static int32_t OOT_FillItemIconInfo(RandomizerGet rg, CwItemIconInfo* out, bool resolveProgressive = true) {
    RandomizerGet actual = RG_NONE;
    auto gi = resolveProgressive ? Rando::StaticData::RetrieveItem(rg).GetGIEntry(&actual)
                                 : Rando::StaticData::RetrieveItem(rg).GetGIEntryUnresolved();
    auto item = Rando::StaticData::RetrieveItem(actual != RG_NONE ? actual : rg);
    if ((actual != RG_NONE ? actual : rg) == RG_EXT_SHIELD_OF_IKANA) {
        out->path = COMBO_IKANA_SHIELD_ICON;
        out->width = out->height = 32;
    } else if ((actual != RG_NONE ? actual : rg) == RG_DOUBLE_DEFENSE) {
        // Its grant row deliberately stores RG_DOUBLE_DEFENSE in itemId. That
        // number is also ITEM_FISH; it is not an inventory-icon index.
        out->path = static_cast<const char*>(gItemIcons[ITEM_HEART_CONTAINER]);
        out->width = out->height = 24;
    } else if (item.HasCustomIcon()) {
        out->path = item.GetCustomIcon();
        out->width = out->height = item.GetCustomIconSize() == ICON_SIZE_24 ? 24 : 32;
    } else if (gi && gi->itemId >= 0 && gi->itemId < ITEM_ROCS_FEATHER_SKIJER) {
        out->path = static_cast<const char*>(gItemIcons[gi->itemId]);
        out->width = out->height = gi->itemId >= ITEM_MEDALLION_FOREST && gi->itemId <= ITEM_HEART_PIECE_2 ? 24 : 32;
    }
    if (!out->path || std::strncmp(out->path, "__OTR__", 7) != 0)
        return 0;
    // These native icon families are IA8, unlike the square RGBA32 item/quest icons.
    if (std::strstr(out->path, "/gSongNoteTex")) {
        out->width = 16;
        out->height = 24;
        out->isIA8 = 1;
        out->hasColor = ComboSongShimmerColor(ComboSongForOotItem(actual != RG_NONE ? actual : rg), out->color);
    } else if (std::strstr(out->path, "/gOcarinaBtnIcon")) {
        out->width = out->height = 16;
        out->isIA8 = 1;
    } else if (std::strstr(out->path, "/gHeartPieceIcon")) {
        out->width = out->height = 48;
        out->isIA8 = 1;
    }
    return 1;
}

extern "C" COMBO_EXPORT int32_t OOT_GetItemIconInfo(const char* itemName, CwItemIconInfo* out) {
    ItemGrantAudit::Scope itemGrantAuditScope("OOT preview-IconInfo", -1, -1, true);
    try {
        if (!itemName || !out)
            return 0;
        *out = CwItemIconInfo{};
        const auto it = Rando::StaticData::itemNameToEnum.find(itemName);
        if (it == Rando::StaticData::itemNameToEnum.end() || it->second == RG_NONE || it->second == RG_COMBO_FOREIGN ||
            it->second == RG_SOLD_OUT || it->second == RG_HINT)
            return 0;
        if (!OTRGlobals::Instance || !OTRGlobals::Instance->gRandomizer || !OTRGlobals::Instance->gRandoContext)
            return CW_DRAW_NOT_READY;
        return OOT_FillItemIconInfo(it->second, out);
    } catch (...) { return 0; }
}

// Seed previews deliberately keep the pool identity and its catalog icon.
// Equipment currently owned by either game must never choose the preview tier.
extern "C" COMBO_EXPORT int32_t OOT_GetSeedItemIconInfo(const char* itemName, CwItemIconInfo* out) {
    try {
        if (!itemName || !out || !OTRGlobals::Instance || !OTRGlobals::Instance->gRandoContext)
            return 0;
        *out = CwItemIconInfo{};
        const auto it = Rando::StaticData::itemNameToEnum.find(itemName);
        if (it == Rando::StaticData::itemNameToEnum.end() || it->second == RG_NONE || it->second == RG_COMBO_FOREIGN ||
            it->second == RG_HINT || it->second == RG_SOLD_OUT)
            return 0;
        return OOT_FillItemIconInfo(it->second, out, false);
    } catch (...) { return 0; }
}

// Boss-soul flame colors, shared by the simplified recipe above and the skeletal one below.
static const uint8_t kBossSoulFlameColors[9][3] = {
    { 0, 255, 0 },     // Gohma
    { 255, 0, 100 },   // King Dodongo
    { 50, 255, 255 },  // Barinade
    { 4, 195, 46 },    // Phantom Ganon
    { 237, 95, 95 },   // Volvagia
    { 85, 180, 223 },  // Morpha
    { 126, 16, 177 },  // Bongo Bongo
    { 222, 158, 47 },  // Twinrova
    { 150, 150, 150 }, // Ganon
};

// Randomizer_DrawBossSoul's blue-fire flame: grayscale-tinted, billboarded, on a segment-8 scroll.
static void OOT_AnimBossSoulFlame(CwItemAnimDrawInfo* out, int slot) {
    out->flameDlPath = gGiBlueFireFlameDL;
    out->flameGrayscale = 1;
    out->flameTranslate[1] = -70.0f;
    out->flameScale[0] = out->flameScale[1] = out->flameScale[2] = 5.0f;
    out->flameColor[0] = kBossSoulFlameColors[slot][0];
    out->flameColor[1] = kBossSoulFlameColors[slot][1];
    out->flameColor[2] = kBossSoulFlameColors[slot][2];
    out->flameColor[3] = 255;
    out->flameHasSeg = 1;
    out->flameSeg.kind = CW_ANIM_SEG_TEXSCROLL;
    out->flameSeg.segment = 8;
    out->flameSeg.onXlu = 1;
    out->flameSeg.width1 = out->flameSeg.width2 = 16;
    out->flameSeg.height1 = out->flameSeg.height2 = 32;
    out->flameSeg.xStep2 = 1;
    out->flameSeg.yStep2 = -8;
}

// Overflow writes to the last slot but still bumps the count, so the consumer's validation rejects
// the whole recipe (sentinel) instead of us writing out of range or silently dropping a bind.
static CwAnimSegBind* OOT_AnimSeg(CwItemAnimDrawInfo* out, int32_t kind, int32_t segment) {
    int32_t i = out->segCount++;
    CwAnimSegBind* s = &out->segs[i < CW_ANIM_MAX_SEGS ? i : CW_ANIM_MAX_SEGS - 1];
    s->kind = kind;
    s->segment = segment;
    s->onOpa = 1;
    return s;
}

static void OOT_AnimLimbEnv(CwItemAnimDrawInfo* out, int32_t from, int32_t to, uint8_t r, uint8_t g, uint8_t b,
                            uint8_t a) {
    int32_t i = out->limbColorCount++;
    CwAnimLimbColor* c = &out->limbColors[i < CW_ANIM_MAX_LIMB_COLORS ? i : CW_ANIM_MAX_LIMB_COLORS - 1];
    c->limbFrom = from;
    c->limbTo = to;
    c->env[0] = r;
    c->env[1] = g;
    c->env[2] = b;
    c->env[3] = a;
}

// ComboShip (issue #86): the boss souls' REAL boss skeletons, described for MM to render through
// combo/menu/ComboForeignAnim.h. 1:1 with DrawGohma/DrawKingDodongo/... in soh/.../randomizer/
// draw.cpp. Barinade carries its native procedural profile and frozen clip pose.
// Morpha has no skeleton and is served by its native core display lists through the static ABI.
static bool OOT_BossSoulUsesSkeleton(RandomizerGet rg) {
    if (rg < RG_GOHMA_SOUL || rg > RG_GANON_SOUL ||
        CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("SimplerBossSoulModels"), 0)) {
        return false;
    }
    int slot = (int)rg - (int)RG_GOHMA_SOUL;
    return slot != 5; // Morpha uses the static core route.
}

static int32_t OOT_FillBossSoulAnim(int slot, CwItemAnimDrawInfo* out) {
    out->opa = 1;
    out->stateDependent = 1; // SimplerBossSoulModels can be toggled mid-session
    out->hiddenLimb = -1;
    OOT_AnimBossSoulFlame(out, slot);
    switch (slot) {
        case 0: // Gohma
            out->skelPath = gGohmaSkel;
            out->animPath = gGohmaIdleCrouchedAnim;
            out->limbCount = 86;
            out->nonFlexSkeleton = 1;
            out->translatePre[1] = -20.0f;
            out->scale = 0.005f;
            OOT_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 8);
            OOT_AnimLimbEnv(out, 0, 255, 0, 255, 170, 255); // OverrideLimbDrawGohma's default
            OOT_AnimLimbEnv(out, BOSSGOMA_LIMB_EYE, BOSSGOMA_LIMB_EYE, 255, 255, 255, 63);
            OOT_AnimLimbEnv(out, BOSSGOMA_LIMB_IRIS, BOSSGOMA_LIMB_IRIS, 255, 255, 255, 255);
            return 1;
        case 1: // King Dodongo
            out->skelPath = object_kingdodongo_Skel_01B310;
            out->animPath = object_kingdodongo_Anim_00F0D8;
            out->limbCount = 49;
            out->nonFlexSkeleton = 1;
            out->translatePre[1] = -20.0f;
            out->scale = 0.003f;
            return 1;
        case 2: // Barinade
            out->skelPath = gBarinadeBodySkel;
            out->animPath = gBarinadeBodyAnim;
            out->limbCount = 64;
            out->nonFlexSkeleton = 1;
            out->translatePre[1] = -25.0f;
            out->scale = 0.03f;
            out->proceduralProfile = CW_ANIM_PROFILE_OOT_BARINADE;
            out->proceduralDlPaths[0] = gBarinadeDL_008D70;
            out->proceduralDlPaths[1] = gBarinadeDL_008BB8;
            out->freezeLastFrame = 1;
            return 1;
        case 3: // Phantom Ganon
            out->skelPath = gPhantomGanonSkel;
            out->animPath = gPhantomGanonNeutralAnim;
            out->limbCount = 26;
            out->nonFlexSkeleton = 1;
            out->translatePre[1] = 10.0f;
            out->scale = 0.007f;
            out->hasModelEnvColor = 1;
            out->modelEnvColor[0] = out->modelEnvColor[1] = out->modelEnvColor[2] = out->modelEnvColor[3] = 255;
            OOT_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 8);
            return 1;
        case 4: { // Volvagia
            out->skelPath = gVolvagiaHeadSkel;
            out->animPath = gVolvagiaHeadEmergeAnim;
            out->limbCount = 7;
            out->nonFlexSkeleton = 1;
            out->scale = 0.007f;
            OOT_AnimSeg(out, CW_ANIM_SEG_PATH, 9)->path = gVolvagiaEyeOpenTex;
            CwAnimSegBind* s = OOT_AnimSeg(out, CW_ANIM_SEG_TEXSCROLL, 8);
            s->yBase1 = 120;
            s->xStep1 = 4;
            s->width1 = s->height1 = s->width2 = s->height2 = 32;
            s->xStep2 = 3;
            s->yStep2 = -2;
            out->hasPrimColor = 1;
            out->primColor[0] = out->primColor[1] = out->primColor[2] = out->primColor[3] = 255;
            out->hasModelEnvColor = 1;
            out->modelEnvColor[0] = out->modelEnvColor[1] = out->modelEnvColor[2] = out->modelEnvColor[3] = 255;
            return 1;
        }
        case 6: // Bongo Bongo
            out->skelPath = gBongoLeftHandSkel;
            out->animPath = gBongoLeftHandIdleAnim;
            out->limbCount = 27;
            out->translatePre[1] = -25.0f;
            out->scale = 0.006f;
            OOT_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 8);
            out->hasPrimColor = 1;
            out->primLodFrac = 0x80;
            out->primColor[0] = out->primColor[1] = out->primColor[2] = out->primColor[3] = 255;
            return 1;
        case 7: { // Twinrova (Kotake)
            out->skelPath = gTwinrovaKotakeSkel;
            out->animPath = gTwinrovaKotakeKoumeFlyAnim;
            out->limbCount = 27;
            out->translatePre[1] = -10.0f;
            out->scale = 0.01f;
            OOT_AnimSeg(out, CW_ANIM_SEG_PATH, 10)->path = gTwinrovaKotakeKoumeEyeOpenTex;
            out->segs[out->segCount - 1].onXlu = 1;
            CwAnimSegBind* s8 = OOT_AnimSeg(out, CW_ANIM_SEG_TEXSCROLL, 8);
            s8->onOpa = 0;
            s8->onXlu = 1;
            s8->width1 = s8->height1 = s8->width2 = 32;
            s8->height2 = 64;
            s8->xStep2 = 1;
            s8->xMask2 = 0x7F;
            s8->yStep2 = -7;
            s8->yMask2 = 0xFF;
            CwAnimSegBind* s9 = OOT_AnimSeg(out, CW_ANIM_SEG_TEXSCROLL, 9);
            s9->onOpa = 0;
            s9->onXlu = 1;
            s9->singleLayer = 1;
            s9->width1 = 32;
            s9->height1 = 64;
            s9->yStep1 = 1;
            s9->yMask1 = 0xFF;
            // A custom skeleton keeps its authored head. The native ice-hair
            // post-pass remains active, with its owning Alt model/materials.
            CwAnimLimbDL* l = &out->limbDLs[out->limbDLCount++];
            l->limbIndex = 21;
            if (!OOT_MagicJarUsesCustomAsset(gTwinrovaKotakeSkel))
                l->dlPath = gTwinrovaKotakeHeadDL;
            l->postDlPath = gTwinrovaKotakeIceHairDL;
            l->postXlu = 1;
            return 1;
        }
        case 8: // Ganon
            out->skelPath = gGanonSkel;
            out->animPath = gGanonGuardIdleAnim;
            out->limbCount = 47;
            out->translatePre[1] = -33.0f;
            out->scale = 0.005f;
            OOT_AnimSeg(out, CW_ANIM_SEG_PATH, 8)->path = gGanonEyeOpenTex;
            OOT_AnimLimbEnv(out, 42, 255, 255, 255, 255, 255); // OverrideLimbDrawGanon brightens the tail
            return 1;
        default:
            return 0;
    }
}

// Animated variant: the skeletal cross-game class. Only OOT's boss souls qualify today — every other
// animated get-item draw (fairy, blue fire, poes, skull token) is a segment-8 texture scroll rather
// than a SkelAnime skeleton and rides the static ABI. Returns 0 otherwise; MM falls back to its
// sentinel. Mirror of MM_GetItemAnimDrawInfo.
// Whole body inside the try: an unwind across the C ABI into 2ship.dll is unrecoverable.
extern "C" COMBO_EXPORT int32_t OOT_GetItemAnimDrawInfo(const char* itemName, CwItemAnimDrawInfo* out) {
    ItemGrantAudit::Scope itemGrantAuditScope("OOT preview-AnimDrawInfo", -1, -1, true);
    try {
        if (itemName == nullptr || out == nullptr) {
            return 0;
        }
        auto& nameMap = Rando::StaticData::itemNameToEnum;
        auto it = nameMap.find(itemName);
        if (it == nameMap.end() || !OOT_BossSoulUsesSkeleton(it->second)) {
            return 0;
        }
        *out = CwItemAnimDrawInfo{};
        return OOT_FillBossSoulAnim((int)it->second - (int)RG_GOHMA_SOUL, out);
    } catch (...) { return 0; }
}

#endif // COMBO_ITEM_DRAW_OOT_H
