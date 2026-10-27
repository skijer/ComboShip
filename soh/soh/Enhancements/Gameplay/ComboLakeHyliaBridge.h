#pragma once
#ifdef COMBO_BUILD
#include "global.h"
#ifdef __cplusplus
extern "C" {
#endif
int ComboLakeHyliaBridge_Get(PlayState* play, Vec3f* mouth, s16* yaw);
#ifdef __cplusplus
}
#endif
#endif
