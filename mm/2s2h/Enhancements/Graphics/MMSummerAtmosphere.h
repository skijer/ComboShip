#pragma once

#define MM_SUMMER_CVAR(name) "gEnhancements.Graphics.SummerAtmosphere." name

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
void MMSummerAtmosphere_Update(struct PlayState* play);
void MMSummerAtmosphere_Draw(struct PlayState* play);
void MMSummerAtmosphere_Reset(void);
#ifdef __cplusplus
}
#endif
