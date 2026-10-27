#ifndef COMBO_SARIA_H
#define COMBO_SARIA_H
#ifdef COMBO_BUILD
#include "global.h"
#ifdef __cplusplus
extern "C" {
#endif
int ComboSaria_IsParam(s16 params);
void ComboSaria_Init(Actor* actor, PlayState* play);
void ComboSaria_Destroy(Actor* actor, PlayState* play);
void ComboSaria_Update(Actor* actor, PlayState* play);
void ComboSaria_Draw(Actor* actor, PlayState* play);
#ifdef __cplusplus
}
#endif
#endif
#endif
