#include "NeiGiPresentation.h"
#include "NeiResourceRouting.h"
#include "variables.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "ComboResolve.h"
#include "ComboSongDrawMM.h"
#include "ComboIkanaShieldResources.h"
#ifdef COMBO_BUILD
#include "ComboGiReceiptTrace.h"
#define COMBO_GI_RECEIPT_TRACE
#endif
#include "ComboSwordGiFit.h"
#include "ComboSwordGiLegacyFit.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <libultraship/bridge/consolevariablebridge.h>
extern "C" {
int ResourceMgr_IsModAssetForGame(const char* game, const char* path);
bool ResourceMgr_IsAltAssetsEnabled();
#include "functions.h"
#include "macros.h"
#include "mods/nei_oot_compat.h"
}
#include "../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiGiEnergyTexture.h"
#define COMBO_DIN_SWORD_GI_HOST_MM
#include "ComboDinSwordGi.h"
#undef COMBO_DIN_SWORD_GI_HOST_MM

using NeiGi::Kind;
namespace {
bool HasResource(const char* path) {
    return NeiResource_Available(path);
}
struct Binding {
    RandoItemId id;
    const char* slug;
};
const Binding kBindings[] = {
    { RI_OOT_ROCS_FEATHER, "rocs_feather" },
    { RI_OOT_NEI_ROCS_FEATHER, "rocs_feather" },
    { RI_OOT_NEI_WHIP, "whip" },
    { RI_OOT_NEI_FIRE_ROD, "fire_rod" },
    { RI_OOT_NEI_ICE_ROD, "ice_rod" },
    { RI_OOT_NEI_LIGHT_ROD, "light_rod" },
    { RI_OOT_NEI_DEKU_LEAF, "deku_leaf" },
    { RI_OOT_NEI_SWITCH_HOOK, "switch_hook" },
    { RI_OOT_NEI_MOGMA_MITTS, "mogma_mitts" },
    { RI_OOT_NEI_GUST_JAR, "gust_jar" },
    { RI_OOT_NEI_BALL_AND_CHAIN, "ball_and_chain" },
    { RI_OOT_NEI_TIME_GATE, "time_gate" },
    { RI_OOT_NEI_BEETLE, "beetle" },
    { RI_OOT_NEI_SHOVEL, "shovel" },
    { RI_OOT_NEI_HYLIAS_GRACE, "hylia_grace" },
    { RI_OOT_NEI_ZONAI_PERMAFROST, "zonai_permafrost" },
    { RI_OOT_NEI_DEMISE_DESTRUCTION, "demise_destruction" },
    { RI_OOT_NEI_ROCS_CAPE, "rocs_cape" },
    { RI_OOT_NEI_SPINNER, "spinner" },
    { RI_OOT_NEI_CANE_OF_SOMARIA, "cane_of_somaria" },
    { RI_OOT_NEI_CANE_SOMARIA_BLOCK, "cane_of_somaria" },
    { RI_OOT_NEI_CANE_SOMARIA_PLATFORM, "cane_of_somaria" },
    { RI_OOT_NEI_MINISH_CAP, "minish_cap" },
    { RI_OOT_NEI_LANTERN, "lantern" },
    { RI_OOT_NEI_MARIO_MASK, "mario_mask" },
    { RI_OOT_TRADE_COJIRO, "cojiro" },
    { RI_OOT_NEI_ELEMENTAL_WAND, "elemental_wand" },
    { RI_OOT_NEI_WAND_SAND_ROD, "sand_rod" },
    { RI_OOT_NEI_WAND_TORNADO_ROD, "tornado_rod" },
    { RI_OOT_NEI_WAND_WATER_ROD, "water_rod" },
    { RI_OOT_NEI_WAND_METEOR_ROD, "meteor_rod" },
    { RI_OOT_NEI_WAND_STORM_ROD, "storm_rod" },
    { RI_OOT_NEI_WAND_SHADOW_SCEPTER, "shadow_scepter" },
    { RI_OOT_EXT_DIVINE_SHIELD, "divine_shield" },
    { RI_OOT_EXT_SHEIKAH_SHIELD, "sheikah_shield" },
    { RI_SHIELD_MIRROR, "shield_of_ikana" },
    { RI_OOT_EXT_MAGIC_CAPE, "magic_cape" },
    { RI_OOT_EXT_SPIRIT_BREASTPLATE, "spirit_breastplate" },
    { RI_OOT_EXT_WATER_DRAGON_SCALE, "sages_tunic" },
    { RI_OOT_EXT_CHAMPIONS_TUNIC, "champions_tunic" },
    { RI_OOT_EXT_PEGASUS_ANKLET, "pegasus_anklet" },
    { RI_OOT_EXT_TRIDENT, "trident" },
    { RI_OOT_EXT_CLIMB_BOOTS, "climb_boots" },
    { RI_OOT_EXT_ROC_BOOTS, "roc_boots" },
    { RI_OOT_EXT_CANE_OF_BYRNA, "cane_of_byrna" },
    { RI_OOT_EXT_FOUR_SWORD, "four_sword" },
    { RI_PENDANT_OF_MEMORIES, "pendant_of_memories" },
    { RI_ROOM_KEY, "room_key" },
    { RI_OOT_NEI_SHEIKAH_SLATE, "sheikah_slate" },
    { RI_OOT_NEI_SLATE_RUNE_BOMB, "slate_bomb" },
    { RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE, "slate_master_cycle" },
    { RI_OOT_NEI_SLATE_RUNE_STASIS, "slate_stasis" },
    { RI_OOT_NEI_SLATE_RUNE_CRYONIS, "slate_cryonis" },
    { RI_OOT_NEI_DESIRE_SENSOR, "slate_sensor" },
    { RI_OOT_NEI_PHANTOM_HOURGLASS, "phantom_hourglass" },
    { RI_OOT_NEI_SHADOW_CRYSTAL, "shadow_crystal" },
    { RI_OOT_NEI_ROD_OF_SEASONS, "rod_of_seasons" },
    { RI_SWORD_KOKIRI, "mm_kokiri_sword" },
    { RI_SWORD_RAZOR, "razor_sword" },
    { RI_SWORD_GILDED, "gilded_sword" },
    { RI_OOT_MASTER_SWORD, "master_sword" },
    { RI_OOT_TRUE_MASTER_SWORD, "true_master_sword" },
    { RI_OOT_BIGGORON_SWORD, "biggoron_sword" },
    { RI_GREAT_FAIRY_SWORD, "great_fairy_sword" },
    { RI_OOT_IRON_KNUCKLE_AXE, "iron_knuckle_axe" },
};
// Only roots that differ from the OoT callback are listed here. Shared
// legacy roots are checked against both hosts by the owner's GI descriptor.
bool HasMmLegacyGiMod(RandoItemId item) {
    const char* opaque = nullptr;
    const char* second = nullptr;
    switch (item) {
        case RI_ROOM_KEY:
            opaque = "objects/object_gi_reserve_b_00/gGiRoomKeyDL";
            second = "objects/object_gi_reserve_b_00/gGiRoomKeyEmptyDL";
            break;
        case RI_OOT_NEI_FIRE_ROD:
            opaque = "objects/object_nei_fire_rod/Cylinder_001_opaque_dl";
            break;
        case RI_OOT_NEI_ICE_ROD:
            opaque = "objects/object_nei_ice_rod/ice_rod_opaque_dl";
            second = "objects/object_nei_ice_rod/ice_rod_transparent_dl";
            break;
        case RI_OOT_NEI_LIGHT_ROD:
            opaque = "objects/object_nei_light_rod/Cylinder_002_opaque_dl";
            second = "objects/object_nei_light_rod/Cylinder_002_transparent_dl";
            break;
        case RI_OOT_NEI_BALL_AND_CHAIN:
            opaque = "objects/object_nei_ball_and_chain/g_ball_and_chain_dl";
            break;
        case RI_OOT_NEI_BEETLE:
            opaque = "objects/object_nei_beetle/g_beetle_dl";
            break;
        case RI_OOT_NEI_SHOVEL:
            opaque = "objects/object_nei_shovel/gShovelGiveDL_opaque_dl";
            break;
        case RI_OOT_NEI_HYLIAS_GRACE:
            opaque = "objects/object_nei_magic_spell/gHyliaGraceGiveDL";
            break;
        case RI_OOT_NEI_ZONAI_PERMAFROST:
            opaque = "objects/object_nei_magic_spell/gZonaiPermafrostGiveDL";
            break;
        case RI_OOT_NEI_DEMISE_DESTRUCTION:
            opaque = "objects/object_nei_magic_spell/gDemiseDestructionGiveDL";
            break;
        case RI_SWORD_KOKIRI:
            opaque = "objects/object_gi_sword_1/gGiKokiriSwordGuardDL";
            second = "objects/object_gi_sword_1/gGiKokiriSwordBladeHiltDL";
            break;
        case RI_SWORD_RAZOR:
            opaque = "objects/object_gi_sword_2/gGiRazorSwordDL";
            second = "objects/object_gi_sword_2/gGiRazorSwordEmptyDL";
            break;
        case RI_SWORD_GILDED:
            opaque = "objects/object_gi_sword_3/gGiGildedSwordDL";
            second = "objects/object_gi_sword_3/gGiGildedSwordEmptyDL";
            break;
        case RI_GREAT_FAIRY_SWORD:
            opaque = "objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL";
            second = "objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL";
            break;
        case RI_SHIELD_MIRROR:
            return ResourceMgr_IsIkanaShieldGiAvailable() != 0;
        case RI_OOT_EXT_MAGIC_CAPE:
            opaque = "objects/object_gi_clothes/gGiTunicCollarDL";
            second = "objects/object_gi_clothes/gGiTunicDL";
            break;
        default:
            break;
    }
    return (opaque && ResourceMgr_IsModAssetForGame("mm", opaque)) ||
           (second && ResourceMgr_IsModAssetForGame("mm", second));
}

bool GetSelectedOwnerGi(RandoItemId item, CwItemDrawInfo* out) {
    if (item != RI_OOT_NEI_LANTERN && !ResourceMgr_IsAltAssetsEnabled())
        return false;
    const char* name = nullptr;
    switch (item) {
        case RI_OOT_NEI_LANTERN:
            // A native MM override owns its texture/material dependencies too.
            if (ResourceMgr_IsModAssetForGame("mm", "objects/object_poh/gPoeLanternDL")) {
                *out = CwItemDrawInfo{};
                out->drawKind = CW_DRAW_KIND_CUSTOM_GI;
                out->dlists[0] = "__OTR__@mm:objects/object_poh/gPoeLanternDL";
                out->dlistCount = 1;
                out->xluStartIndex = -1;
                out->scale = .025f;
                return true;
            }
            name = "Lantern";
            break;
        case RI_SWORD_KOKIRI:
            name = "Kokiri Sword";
            break;
        case RI_SWORD_RAZOR:
            name = "Razor Sword";
            break;
        case RI_SWORD_GILDED:
            name = "Gilded Sword";
            break;
        case RI_OOT_MASTER_SWORD:
            name = "Master Sword";
            break;
        case RI_OOT_TRUE_MASTER_SWORD:
            name = "True Master Sword";
            break;
        case RI_OOT_BIGGORON_SWORD:
            name = "Biggoron's Sword";
            break;
        case RI_GREAT_FAIRY_SWORD:
            name = "Great Fairy's Sword";
            break;
        default:
            return false;
    }
    static Fn_GetItemDrawInfo describe = nullptr;
    if (!describe)
        describe = reinterpret_cast<Fn_GetItemDrawInfo>(Combo_ResolveSym("soh", "OOT_GetItemDrawInfo"));
    CwItemDrawInfo selected{};
    if (!describe || describe(name, &selected) != 1 || selected.drawKind != CW_DRAW_KIND_CUSTOM_GI ||
        selected.dlistCount != 1)
        return false;
    if (item == RI_OOT_NEI_LANTERN) {
        if (selected.opCount != 0)
            return false;
    } else if (selected.opCount != 1 || selected.ops[0].op != CW_OP_ROTATE_Z || selected.neiShimmer <= 0 ||
               !NeiGi::IsSword(static_cast<Kind>(selected.neiShimmer - 1)))
        return false;
    *out = selected;
    return true;
}
} // namespace

extern "C" {
#define Graph_Alloc(gfx, bytes) GRAPH_ALLOC(gfx, bytes)
#define NeiGi_DrawTexturedMesh NeiGi_DrawTexturedMeshNative
#define NEI_GI_ROTATE_Y Matrix_RotateYF
#define NEI_GI_ROTATE_Z Matrix_RotateZF
#define NEI_GI_NATIVE_MM 1
#include "../../../soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc"
#undef NEI_GI_NATIVE_MM
#undef NeiGi_DrawTexturedMesh
#undef NEI_GI_ROTATE_Y
#undef NEI_GI_ROTATE_Z
#undef Graph_Alloc
}

// Texture commands resolve through the interpreter's active resource manager,
// unlike G_DL_OTR_FILEPATH which parses @oot:. Keep the raw texture path and
// bracket exactly the native mesh submission in its asset owner's namespace.
extern "C" bool NeiGi_DrawTexturedMesh(PlayState* play, const NeiGi::Mesh& mesh,
                                       const NeiGi::TextureMaterial& material) {
    if (!play || !play->state.gfxCtx || !NeiGi_ValidMesh(mesh, &material) || !HasResource(material.path))
        return false;
    return NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, &material, false, "oot");
}

void DrawOotSlateRuneFlame(u8 r, u8 g, u8 b);

void MM_DrawNeiGi(const CwItemDrawInfo& info, bool shop, int mmPickup) {
#ifdef COMBO_GI_RECEIPT_TRACE
    ComboGiReceiptTrace::Dispatch(info.resolvedName, info.drawKind, info.dlists, info.dlistCount, info.scale, -1,
                                  mmPickup);
#endif
    PlayState* play = gPlayState;
    if (play && info.drawKind == CW_DRAW_KIND_SEASON_GI) {
        if (info.neiEffect >= 1 && info.neiEffect <= 4 && info.dlistCount == 0 && info.opCount == 0)
            NeiGi_DrawSeasonOverlay(play, info.neiEffect, nullptr);
        return;
    }
    if (!play || info.dlistCount < 1 || !info.dlists[0])
        return;
    if (info.drawKind == CW_DRAW_KIND_CUSTOM_GI) {
        const float tilt = info.opCount == 1 && info.ops[0].op == CW_OP_ROTATE_Z
                               ? info.ops[0].a * (3.14159265358979323846f / 32768.f)
                               : 0.f;
        if (!NeiGi_ValidScale(info.scale) || !NeiGi_Finite(tilt) || info.neiShimmer < 1 ||
            info.neiShimmer > int(Kind::Last) + 1)
            return;
        const bool flame = tilt != 0.f && info.primColorXlu[3];
        // The native scroll/flame allocates before the shared renderer. Check
        // both passes together, and keep the flame in their selected GI fit.
        if (flame && !NeiGi_ArenaHasRoom(play, 12 * sizeof(Gfx), 4, 20, 40))
            return;
        Matrix_Push();
        if (flame) {
            Matrix_Push();
            if (NeiGi::IsSword(static_cast<Kind>(info.neiShimmer - 1))) {
                ComboSwordGi_ApplyPresentationSize(shop, mmPickup);
                ComboSwordGi_ApplyEffectFit(static_cast<Kind>(info.neiShimmer - 1), shop, mmPickup);
            }
            DrawOotSlateRuneFlame(info.primColorXlu[0], info.primColorXlu[1], info.primColorXlu[2]);
            Matrix_Pop();
        }
        NeiGi_DrawExternalPresentation(
            play, info.dlists[0], info.xluStartIndex == 1 && info.dlistCount > 1 ? info.dlists[1] : nullptr, info.scale,
            info.neiShimmer - 1, info.itemShimmer, "oot", shop, tilt, true, mmPickup);
        Matrix_Pop();
        return;
    }
    // The native flame runs before the bounded presentation renderer. Check
    // its scroll/matrix and the later model together before emitting either.
    if (info.neiSomariaUpgrade && !NeiGi_ArenaHasRoom(play, 12 * sizeof(Gfx), 4, 20, 40))
        return;
    Matrix_Push();
    if (info.neiSomariaUpgrade)
        DrawOotSlateRuneFlame(255, 60, 60);
    const char* owner = std::strncmp(info.dlists[0], "__OTR__@oot-gi-base:", 20) == 0 ? "oot-gi-base" : "oot";
    NeiGi_DrawPresentation(play, info.dlists[0],
                           info.xluStartIndex == 1 && info.dlistCount > 1 ? info.dlists[1] : nullptr, info.scale,
                           info.neiEffect, info.neiEffectCenter,
                           info.itemShimmer || CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0), owner, shop,
                           mmPickup, info.neiShimmer - 1);
    Matrix_Pop();
}

bool MM_DescribeNeiGi(RandoItemId item, CwItemDrawInfo* out) {
    if (!out)
        return false;
    *out = CwItemDrawInfo{};
    if (item >= RI_OOT_NEI_SEASON_SPRING && item <= RI_OOT_NEI_SEASON_WINTER) {
        *out = CwItemDrawInfo{};
        out->drawKind = CW_DRAW_KIND_SEASON_GI;
        out->neiEffect = 1 + item - RI_OOT_NEI_SEASON_SPRING;
        out->xluStartIndex = -1;
        return true;
    }
    const Binding* binding = nullptr;
    for (const auto& candidate : kBindings)
        if (candidate.id == item) {
            binding = &candidate;
            break;
        }
    if (!binding) {
        Kind kind;
        if (item == RI_OOT_NEI_POKE_BALL)
            kind = Kind::Pokeball;
        else if (item == RI_OOT_NEI_CANE_PACCI_FLIP || item == RI_OOT_NEI_CANE_PACCI_STONE ||
                 item == RI_OOT_NEI_CANE_PACCI_ULTRAHAND)
            kind = Kind::Pacci;
        else
            return false;
        out->neiShimmer = static_cast<int>(kind) + 1;
        out->itemShimmer = CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0) != 0;
        out->stateDependent = 2;
        return false;
    }
    static Fn_GetNeiGiDrawInfo describe = nullptr;
    if (!describe)
        describe = (Fn_GetNeiGiDrawInfo)Combo_ResolveSym("soh", "OOT_GetNeiGiDrawInfo");
    static Fn_GetNeiGiDrawInfoForAssets describeForAssets = nullptr;
    if (!describeForAssets)
        describeForAssets = (Fn_GetNeiGiDrawInfoForAssets)Combo_ResolveSym("soh", "OOT_GetNeiGiDrawInfoForAssets");
    if (!describe && !describeForAssets)
        return false;
    CwItemDrawInfo info{};
    const bool altAssets = ResourceMgr_IsAltAssetsEnabled();
    bool authored =
        (describeForAssets ? describeForAssets(binding->slug, altAssets, &info) : describe(binding->slug, &info)) == 1;
    out->neiShimmer = info.neiShimmer;
    const bool mandatory = info.neiShimmer > 0 && (NeiGi::IsSword(static_cast<Kind>(info.neiShimmer - 1)) ||
                                                   NeiGi::IsEquipment(static_cast<Kind>(info.neiShimmer - 1)) ||
                                                   item == RI_OOT_IRON_KNUCKLE_AXE || item == RI_OOT_NEI_MARIO_MASK);
    out->itemShimmer = mandatory || CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0);
    out->stateDependent = 2;
    const bool vanillaSword =
        !altAssets && info.neiShimmer > 0 && NeiGi::IsSword(static_cast<Kind>(info.neiShimmer - 1));
    const bool legacyMod = !vanillaSword && HasMmLegacyGiMod(item);
    if (!authored && !legacyMod && GetSelectedOwnerGi(item, &info)) {
        // Keep the selected standalone geometry and MM's concrete award
        // identity together when the owner declines the authored GI.
        info.neiShimmer = out->neiShimmer;
        authored = true;
    }
    if (!authored || legacyMod)
        return false;
    info.itemShimmer = out->itemShimmer;
    for (int i = 0; i < info.dlistCount; ++i) {
        info.dlists[i] = NeiResource_Route(info.dlists[i]);
        if (!info.dlists[i])
            return false;
    }
    info.neiSomariaUpgrade = item == RI_OOT_NEI_CANE_SOMARIA_BLOCK || item == RI_OOT_NEI_CANE_SOMARIA_PLATFORM;
    info.stateDependent = 2; // Model-pack selection remains live for concrete aliases.
    *out = info;
    return true;
}

void DrawSong(RandoItemId item);

bool MM_TryDrawNeiGi(RandoItemId item, bool shop, int mmPickup) {
    if (!gPlayState)
        return false;
    const int arrow = NeiArrowGi_ProfileForDrawId(item, RI_ARROW_FIRE, RI_ARROW_ICE, RI_ARROW_LIGHT);
    if (shop && arrow)
        return NeiGi_DrawElementalArrowShop(gPlayState, arrow);
    const int spell = NeiArrowGi_ProfileForDrawId(item, RI_OOT_DINS_FIRE, RI_OOT_FARORES_WIND, RI_OOT_NAYRUS_LOVE);
    if (spell)
        return NeiGi_DrawElementalSpell(gPlayState, spell, 1, shop);
    // Resolve song identity before the shared draw table, whose aliases can
    // otherwise route imported/native songs through an unrelated note drawer.
    if (ComboSongForMmItem(item) >= 0) {
        DrawSong(item);
        return true;
    }
    CwItemDrawInfo info{};
    if (!MM_DescribeNeiGi(item, &info)) {
        // The declined descriptor still supplies the award's identity. A
        // missing shipped sword in vanilla mode must not reach legacy drawers
        // that can select a local or donor mod at a base resource path.
        return !ResourceMgr_IsAltAssetsEnabled() && info.neiShimmer > 0 &&
               NeiGi::IsSword(static_cast<Kind>(info.neiShimmer - 1));
    }
    MM_DrawNeiGi(info, shop, mmPickup);
    return true;
}

MM_NeiGiFallbackShimmer::MM_NeiGiFallbackShimmer(RandoItemId item, bool shop, int mmPickup, Kind fitKind)
    : mKind(Kind::Neutral), mEnabled(false), mShop(shop), mMmPickup(mmPickup) {
    if (!gPlayState)
        return;
    // Its legacy drawer already has an unconditional mask shimmer.
    if (item == RI_OOT_NEI_MARIO_MASK)
        return;
    CwItemDrawInfo info{};
    MM_DescribeNeiGi(item, &info);
    mEnabled = info.itemShimmer && info.neiShimmer > 0 && info.neiShimmer <= static_cast<int>(Kind::Last) + 1;
    if (mEnabled)
        mKind = static_cast<Kind>(info.neiShimmer - 1);
    if (mEnabled) {
        Matrix_Push();
        if (NeiGi::IsSword(mKind)) {
            ComboSwordGi_ApplyPresentationSize(shop, mmPickup);
            ComboSwordGi_ApplyLegacyFit("mm", fitKind == Kind::Neutral ? mKind : fitKind, shop, mmPickup);
        }
        Matrix_Push();
    }
}

MM_NeiGiFallbackShimmer::~MM_NeiGiFallbackShimmer() {
    if (!mEnabled)
        return;
    Matrix_Pop();
    Matrix_Pop(); // Restore the caller before composing award-space effects.
    Matrix_Push();
    if (NeiGi::IsSword(mKind)) {
        ComboSwordGi_ApplyPresentationSize(mShop, mMmPickup);
        ComboSwordGi_ApplyEffectFit(mKind, mShop, mMmPickup);
        NeiGi_DrawMesh(gPlayState,
                       NeiGi::SampleSpecial(mKind, gPlayState->gameplayFrames, NeiGi_CameraBasis(gPlayState),
                                            ComboSwordGi_ParticleScale(mKind, mShop, mMmPickup)));
    }
    NeiGi_DrawMesh(gPlayState,
                   NeiGi::SampleShimmer(gPlayState->gameplayFrames, true, NeiGi_CameraBasis(gPlayState), mKind));
    Matrix_Pop();
}
