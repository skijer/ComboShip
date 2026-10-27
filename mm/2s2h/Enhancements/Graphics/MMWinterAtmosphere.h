#pragma once

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
void MMWinterAtmosphere_Update(struct PlayState* play);
void MMWinterAtmosphere_Draw(struct PlayState* play);
void MMWinterAtmosphere_Reset(void);
#ifdef __cplusplus
}
#endif
