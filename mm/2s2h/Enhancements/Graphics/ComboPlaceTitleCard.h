#pragma once
#ifdef COMBO_BUILD
#include "global.h"
#ifdef __cplusplus
extern "C" {
#endif
// 1 = presented, 0 = wait for safe gameplay, -1 = deliberately hidden.
int ComboPlaceTitleCard_Show(PlayState* play, TexturePtr texture, u8 width, u8 height);
#ifdef __cplusplus
}
#endif
#endif
