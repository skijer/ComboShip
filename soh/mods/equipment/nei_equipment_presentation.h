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
#ifdef NEI_EQUIPMENT_MM
extern int NeiResource_IsMod(const char* path);
#endif
#ifdef __cplusplus
}
#endif

enum {
    NEI_EQUIPMENT_DIVINE_SHIELD,
    NEI_EQUIPMENT_SHEIKAH_SHIELD,
    NEI_EQUIPMENT_IKANA_SHIELD,
    NEI_EQUIPMENT_BYRNA,
    NEI_EQUIPMENT_TRIDENT,
    NEI_EQUIPMENT_FOUR_BLADE,
    NEI_EQUIPMENT_FOUR_HILT,
    NEI_EQUIPMENT_AXE,
    NEI_EQUIPMENT_COUNT
};
static inline const char* NeiEquipment_ModelPath(int model) {
    static const char* paths[] = { "__OTR__objects/nei_held_redesign/divine_shield/gi_dl",
                                   "__OTR__objects/nei_held_redesign/sheikah_shield/gi_dl",
                                   "__OTR__objects/nei_held_redesign/shield_of_ikana/gi_dl",
                                   "__OTR__objects/nei_held_redesign/cane_of_byrna/gi_dl",
                                   "__OTR__objects/nei_held_redesign/trident/gi_dl",
                                   "__OTR__objects/nei_held_redesign/four_sword_blade/gi_dl",
                                   "__OTR__objects/nei_held_redesign/four_sword_hilt/gi_dl",
                                   "__OTR__objects/nei_held_redesign/iron_knuckle_axe/gi_dl" };
    return model >= 0 && model < NEI_EQUIPMENT_COUNT ? paths[model] : NULL;
}
static inline bool NeiEquipment_Available(const char* path) {
#ifdef NEI_EQUIPMENT_MM
    return NeiResource_Available(path);
#else
    return path &&
           (ResourceMgr_FileExists(path) || (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path)));
#endif
}
#include "nei_equipment_resources.inc"
static inline bool NeiEquipment_ModelAvailable(int model) {
    if (!NeiEquipment_ModelPath(model))
        return false;
    // Only 6–14 direct dependencies per selected model; never queue a partial graph.
    const char* const* dependency = sNeiEquipmentResources[model];
    for (; *dependency != NULL; ++dependency) {
        if (!NeiEquipment_Available(*dependency))
            return false;
    }
    return true;
}
static inline bool NeiEquipment_IkanaLocalMod(void) {
    return ResourceMgr_IsModAsset("__OTR__objects/object_link_child/gLinkHumanMirrorShieldDL") ||
           ResourceMgr_IsModAsset("__OTR__objects/object_link_child/gLinkHumanRightHandHoldingMirrorShieldDL");
}
static inline bool NeiEquipment_IsLegacyMod(int model, const char* path) {
    if (!path)
        return false;
    if (ResourceMgr_IsModAsset(path))
        return true;
    if (model == NEI_EQUIPMENT_IKANA_SHIELD) {
        // Either native hand/back half selects the whole native shield representation.
        const char* paths[] = { "__OTR__objects/object_link_child/gLinkHumanMirrorShieldDL",
                                "__OTR__objects/object_link_child/gLinkHumanRightHandHoldingMirrorShieldDL" };
        for (int i = 0; i < 2; ++i) {
            if (ResourceMgr_IsModAsset(paths[i]))
                return true;
#ifndef NEI_EQUIPMENT_MM
            if (ResourceMgr_IsModAssetForGame("mm", paths[i]))
                return true;
#endif
        }
        return false;
    }
#ifdef NEI_EQUIPMENT_MM
    return NeiResource_IsMod(path);
#else
    return false;
#endif
}
// Preserve the actual mod-owned resource, including base-path mods and live Alt
// changes. Deferred canonical paths avoid stale native getter caches.
// Ikana keeps both native halves in MM, or both in the selected OoT import owner.
static inline Gfx* NeiEquipment_LegacyDL(int model, const char* path) {
    static Gfx wrappers[NEI_EQUIPMENT_COUNT][2][2];
    if (!NeiEquipment_ModelPath(model) || !NeiEquipment_IsLegacyMod(model, path))
        return NULL;
    const int hand = strstr(path, "RightHandHoldingMirrorShield") != NULL;
#ifdef NEI_EQUIPMENT_MM
    if (model != NEI_EQUIPMENT_IKANA_SHIELD && !ResourceMgr_IsModAsset(path) && NeiResource_IsMod(path))
        path = NeiResource_Route(path);
#else
    if (model == NEI_EQUIPMENT_IKANA_SHIELD && !NeiEquipment_IkanaLocalMod()) {
        path = hand ? "__OTR__@mm:objects/object_link_child/gLinkHumanRightHandHoldingMirrorShieldDL"
                    : "__OTR__@mm:objects/object_link_child/gLinkHumanMirrorShieldDL";
    }
#endif
    if (!path)
        return NULL;
    gDma1p(&wrappers[model][hand][0], G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH);
    gSPEndDisplayList(&wrappers[model][hand][1]);
    return wrappers[model][hand];
}
// These process-lifetime wrappers queue the OTR resource in the caller's CURRENT
// native equipment frame/pass. No matrix, collision, actor, or ownership changes.
// A legacy mod model remains the caller's preferred resource.
static inline Gfx* NeiEquipment_ModelDL(int model, const char* legacyPath) {
    static Gfx wrappers[NEI_EQUIPMENT_COUNT][2];
    const char* path = NeiEquipment_ModelPath(model);
    if (!path || !NeiEquipment_ModelAvailable(model) || NeiEquipment_IsLegacyMod(model, legacyPath))
        return NULL;
#ifdef NEI_EQUIPMENT_MM
    path = NeiResource_Route(path);
    if (!path)
        return NULL;
#endif
    gDma1p(&wrappers[model][0], G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH);
    gSPEndDisplayList(&wrappers[model][1]);
    return wrappers[model];
}
static inline bool NeiEquipment_HasFourSword(void) {
    return NeiEquipment_ModelAvailable(NEI_EQUIPMENT_FOUR_BLADE) &&
           NeiEquipment_ModelAvailable(NEI_EQUIPMENT_FOUR_HILT);
}
