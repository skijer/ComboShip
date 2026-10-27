#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
// Stable receipt profiles: 1 Fire, 2 Ice, 3 Light. No actor IDs cross the DLL ABI.
static inline int NeiArrowGi_ProfileForDrawId(int id, int fire, int ice, int light) {
    return id == fire ? 1 : id == ice ? 2 : id == light ? 3 : 0;
}
void NeiGi_DrawElementalArrow(struct PlayState* play, int profile);
// The descriptor's owner selects the editor rows, independently of the host.
void NeiGi_DrawElementalArrowForOwner(struct PlayState* play, int profile, int mmOwner, int shop);
// Native selected core and restrained tip emission in the caller's shelf pose.
int NeiGi_DrawElementalArrowShop(struct PlayState* play, int profile);
// Profiles 1 Din, 2 Farore, 3 Nayru. False leaves the caller's native fallback available.
int NeiGi_DrawElementalSpell(struct PlayState* play, int profile, int mmOwner, int shop);
// Diamond/orb fallback: modelview, segment-8 scroll and foreign segment restore.
int NeiGi_CanDrawElementalSpellFallback(struct PlayState* play);
#ifdef __cplusplus
}
#endif
