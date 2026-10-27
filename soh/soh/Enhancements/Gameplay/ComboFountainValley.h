#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
// Ready only after native/Djipi resources and fitted collision are prepared.
int ComboFountainValley_Ready(struct PlayState* play);
#ifdef __cplusplus
}
#endif
