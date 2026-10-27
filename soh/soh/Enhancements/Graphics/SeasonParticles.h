#pragma once

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
// Owns only private cosmetic state. Draw integrates at most one update per
// gameplay frame and may be called again for interpolation or paused controls.
void SeasonParticles_Draw(struct PlayState* play);
void SeasonParticles_Reset(void);
#ifdef __cplusplus
}
#endif
