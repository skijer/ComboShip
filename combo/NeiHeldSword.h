#pragma once
#include <stdbool.h>
#include <string.h>
#include "z64.h"
#ifdef NEI_EQUIPMENT_MM
#include "2s2h/Rando/NeiResourceRouting.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern u8 ResourceMgr_FileExists(const char* path);
extern u8 ResourceMgr_FileAltExists(const char* path);
extern bool ResourceMgr_IsAltAssetsEnabled(void);
extern int ResourceMgr_IsModAsset(const char* path);
extern int ResourceMgr_IsModAssetForGame(const char* game, const char* path);
extern s32 CVarGetInteger(const char* name, s32 defaultValue);
#ifdef COMBO_BUILD
extern int32_t OOT_NeiEnsureGiBaseOwner(void);
extern int32_t OOT_NeiResourceExists(const char* path);
#endif
#ifdef __cplusplus
}
#endif

enum {
    NEI_HELD_SWORD_KOKIRI,
    NEI_HELD_SWORD_MM_KOKIRI,
    NEI_HELD_SWORD_RAZOR,
    NEI_HELD_SWORD_GILDED,
    NEI_HELD_SWORD_MASTER,
    NEI_HELD_SWORD_TRUE_MASTER,
    NEI_HELD_SWORD_BIGGORON,
    NEI_HELD_SWORD_GREAT_FAIRY,
    NEI_HELD_SWORD_FOUR,
    NEI_HELD_SWORD_GIANTS_KNIFE,
    NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE,
    NEI_HELD_SWORD_COUNT,
};
enum {
    NEI_HELD_SWORD_OOT_ADULT,
    NEI_HELD_SWORD_OOT_CHILD,
    NEI_HELD_SWORD_MM_HUMAN,
    NEI_HELD_SWORD_FRAME_COUNT,
};
#include "NeiHeldSwordResources.inc"

static inline bool NeiHeldSword_UseVanilla(void) {
    return CVarGetInteger("gEnhancements.SkijerNEI.UseVanillaHeldSwords", 0) != 0;
}

static inline bool NeiHeldSword_Available(const char* path) {
#ifdef NEI_EQUIPMENT_MM
    // Both the wrapper and every authored mesh/material dependency belong to
    // OoT. A local MM name collision must never complete the donor graph.
    return path && NeiResource_Available(path);
#else
#ifdef COMBO_BUILD
    if (path && strncmp(path, "__OTR__@oot-gi-base:", 20) == 0)
        return OOT_NeiResourceExists(path) != 0;
#endif
    return path &&
           (ResourceMgr_FileExists(path) || (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path)));
#endif
}

static inline Gfx* NeiHeldSword_ModelDL(int sword, int frame) {
    static Gfx wrappers[2][NEI_HELD_SWORD_COUNT][NEI_HELD_SWORD_FRAME_COUNT][2];
    if (sword < 0 || sword >= NEI_HELD_SWORD_COUNT || frame < 0 || frame >= NEI_HELD_SWORD_FRAME_COUNT)
        return NULL;
    // Read on every draw so the cosmetic option switches live. The callers
    // retain their native/legacy fallback and selected custom equipment rules.
    if (NeiHeldSword_UseVanilla())
        return NULL;
    bool base = false;
#if defined(COMBO_BUILD) || defined(NEI_EQUIPMENT_MM)
    base = !ResourceMgr_IsAltAssetsEnabled();
    // The vanilla held graph uses the same shipped-only resource view as GI.
    // A base-path GI mod must not replace this promised authored mesh.
#ifdef NEI_EQUIPMENT_MM
    if (base && !NeiResource_EnsureGiBaseOwner())
#else
    if (base && !OOT_NeiEnsureGiBaseOwner())
#endif
        return NULL;
#endif
    const char* path = base ? sNeiHeldSwordBasePaths[sword][frame][0] : sNeiHeldSwordPaths[sword][frame][0];
    const char* matrix = base ? sNeiHeldSwordBasePaths[sword][frame][1] : sNeiHeldSwordPaths[sword][frame][1];
    if (!NeiHeldSword_Available(path) || !NeiHeldSword_Available(matrix))
        return NULL;
    for (const char* const* dependency = base ? sNeiHeldSwordBaseResources[sword] : sNeiHeldSwordResources[sword];
         *dependency; ++dependency)
        if (!NeiHeldSword_Available(*dependency))
            return NULL;
#ifdef NEI_EQUIPMENT_MM
    path = NeiResource_Route(path);
    if (!path)
        return NULL;
#endif
    gDma1p(&wrappers[base][sword][frame][0], G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH);
    gSPEndDisplayList(&wrappers[base][sword][frame][1]);
    return wrappers[base][sword][frame];
}

static inline bool NeiHeldSword_SelectedResource(const char* owner, const char* path) {
    return ResourceMgr_IsModAsset(path) || ResourceMgr_IsModAssetForGame(owner, path);
}

// A selected hand/equipment representation owns the whole held sword. Native
// combined hands follow the physical rig as well as the logical equipment:
// adult one-hand tiers use Master hands, child tiers use Kokiri hands, and child
// BGS uses the ceremonial Master hand. Keep both LODs/cross-age sources owned.
static inline bool NeiHeldSword_EquipmentSelected(int sword, int frame) {
    const bool twoHand = sword == NEI_HELD_SWORD_BIGGORON || sword == NEI_HELD_SWORD_GREAT_FAIRY ||
                         sword == NEI_HELD_SWORD_GIANTS_KNIFE || sword == NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE;
    const char* custom = sword == NEI_HELD_SWORD_MASTER || sword == NEI_HELD_SWORD_TRUE_MASTER
                             ? "__OTR__objects/object_custom_equip/gCustomMasterSwordDL"
                         : twoHand ? "__OTR__objects/object_custom_equip/gCustomLongswordDL"
                                   : "__OTR__objects/object_custom_equip/gCustomKokiriSwordDL";
    if (NeiHeldSword_SelectedResource("oot", custom))
        return true;
    if (sword == NEI_HELD_SWORD_GIANTS_KNIFE &&
        NeiHeldSword_SelectedResource("oot", "__OTR__objects/object_custom_equip/gCustomBreakableLongswordDL"))
        return true;
    if (sword == NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE &&
        NeiHeldSword_SelectedResource("oot", "__OTR__objects/object_custom_equip/gCustomBrokenLongswordDL"))
        return true;
    const char* const oot[] = {
        "__OTR__objects/object_link_child/gLinkChildLeftFistAndKokiriSwordNearDL",
        "__OTR__objects/object_link_child/gLinkChildLeftFistAndKokiriSwordFarDL",
        "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingMasterSwordNearDL",
        "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingMasterSwordFarDL",
        "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingBgsNearDL",
        "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingBgsFarDL",
    };
    const int family = sword == NEI_HELD_SWORD_MASTER || sword == NEI_HELD_SWORD_TRUE_MASTER ? 2 : twoHand ? 4 : 0;
    for (int i = family; i < family + 2; ++i)
        if (NeiHeldSword_SelectedResource("oot", oot[i]))
            return true;
    if (frame == NEI_HELD_SWORD_OOT_ADULT || frame == NEI_HELD_SWORD_OOT_CHILD) {
#ifndef NEI_EQUIPMENT_MM
        if (!twoHand) {
            // EquipmentAlwaysVisible can interchange the two one-hand rig
            // sources, including Four Sword's child extended-item carrier.
            for (int i = 0; i < 4; ++i)
                if (NeiHeldSword_SelectedResource("oot", oot[i]))
                    return true;
        } else {
            // The native BGS array also contains these health-dependent hand
            // variants. A selected combined hand remains authoritative.
            if (NeiHeldSword_SelectedResource(
                    "oot", "__OTR__objects/object_link_boy/gLinkAdultHandHoldingBrokenGiantsKnifeDL") ||
                NeiHeldSword_SelectedResource(
                    "oot", "__OTR__objects/object_link_boy/gLinkAdultHandHoldingBrokenGiantsKnifeFarDL"))
                return true;
        }
#endif
        if (frame == NEI_HELD_SWORD_OOT_CHILD && twoHand) {
            if (NeiHeldSword_SelectedResource(
                    "oot", "__OTR__objects/object_link_child/gLinkChildLeftHandHoldingMasterSwordDL"))
                return true;
        } else {
            const int physical = frame == NEI_HELD_SWORD_OOT_CHILD ? 0 : twoHand ? 4 : 2;
            for (int i = physical; i < physical + 2; ++i)
                if (NeiHeldSword_SelectedResource("oot", oot[i]))
                    return true;
        }
    }
    // MM swords can also be selected/imported in OoT; explicit owner queries
    // keep those mods authoritative even when the local object names overlap.
    const char* mm =
        sword == NEI_HELD_SWORD_RAZOR    ? "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingRazorSwordDL"
        : sword == NEI_HELD_SWORD_GILDED ? "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingGildedSwordDL"
        : sword == NEI_HELD_SWORD_GREAT_FAIRY
            ? "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingGreatFairysSwordDL"
            : "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingKokiriSwordDL";
    return (frame == NEI_HELD_SWORD_MM_HUMAN || sword == NEI_HELD_SWORD_RAZOR || sword == NEI_HELD_SWORD_GILDED ||
            sword == NEI_HELD_SWORD_GREAT_FAIRY) &&
           NeiHeldSword_SelectedResource("mm", mm);
}

// Standalone progressive piece mods keep the previous native blade/hilt path.
static inline bool NeiHeldSword_UpgradePiecesSelected(int sword) {
    const char* blade = NULL;
    const char* hilt = NULL;
    if (sword == NEI_HELD_SWORD_RAZOR) {
        blade = "__OTR__objects/gameplay_keep/gRazorSwordBladeDL";
        hilt = "__OTR__objects/gameplay_keep/gRazorSwordHandleDL";
    } else if (sword == NEI_HELD_SWORD_GILDED) {
        blade = "__OTR__objects/object_link_child/gLinkHumanGildedSwordBladeDL";
        hilt = "__OTR__objects/object_link_child/gLinkHumanGildedSwordHandleDL";
    } else if (sword == NEI_HELD_SWORD_GREAT_FAIRY) {
        blade = "__OTR__objects/object_link_child/gLinkHumanGreatFairysSwordDL";
    }
    return (blade && NeiHeldSword_SelectedResource("mm", blade)) || (hilt && NeiHeldSword_SelectedResource("mm", hilt));
}
