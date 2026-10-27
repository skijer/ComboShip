#pragma once
#ifdef COMBO_BUILD
struct PlayState;
struct Player;
#ifdef __cplusplus
extern "C" {
#endif
int ComboLostWoodsFountain_TryExit(struct PlayState* play, struct Player* player, int entrance);
void ComboLostWoodsFountain_NormalizeEntry(void);
#ifdef __cplusplus
}
#endif
#endif
