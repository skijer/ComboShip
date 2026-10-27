#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef COMBO_BUILD
// Explicit custom/cross-game arrivals request the final destination's native
// card once. This is transient state, never a save or entrance-shuffle flag.
// OoT keeps it pending through the title-card hook until Player_Init consumes it.
void Combo_SOH_RequestCrossGameTitleCard(void);
int Combo_SOH_HasCrossGameTitleCard(void);
int Combo_SOH_ConsumeCrossGameTitleCard(void);
// Synchronous custom-card hook eligibility; never touches a travel request or save.
int Combo_SOH_SwapCustomTitleCardPresentation(int active);
int Combo_SOH_IsPresentingCustomTitleCard(void);
void Combo_MM_RequestCrossGameTitleCard(void);
int Combo_MM_ConsumeCrossGameTitleCard(void);
#else
static inline void Combo_SOH_RequestCrossGameTitleCard(void) {
}
static inline int Combo_SOH_HasCrossGameTitleCard(void) {
    return 0;
}
static inline int Combo_SOH_ConsumeCrossGameTitleCard(void) {
    return 0;
}
static inline int Combo_SOH_SwapCustomTitleCardPresentation(int active) {
    (void)active;
    return 0;
}
static inline int Combo_SOH_IsPresentingCustomTitleCard(void) {
    return 0;
}
static inline void Combo_MM_RequestCrossGameTitleCard(void) {
}
static inline int Combo_MM_ConsumeCrossGameTitleCard(void) {
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif
