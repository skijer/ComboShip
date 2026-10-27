#ifndef COMBO_IKANA_SHIELD_RESOURCES_H
#define COMBO_IKANA_SHIELD_RESOURCES_H

#define COMBO_IKANA_GI_OPA "__OTR__objects/object_gi_shield_3/gGiMirrorShieldEmptyDL"
#define COMBO_IKANA_GI_XLU "__OTR__objects/object_gi_shield_3/gGiMirrorShieldDL"
#define COMBO_IKANA_GI_OPA_MM "__OTR__@mm:objects/object_gi_shield_3/gGiMirrorShieldEmptyDL"
#define COMBO_IKANA_GI_XLU_MM "__OTR__@mm:objects/object_gi_shield_3/gGiMirrorShieldDL"

#ifdef __cplusplus
extern "C" {
#endif
// Both native passes must be usable. Authored GI resources remain fallback only.
int ResourceMgr_IsIkanaShieldGiAvailable(void);
#ifdef __cplusplus
}
#endif

#endif
