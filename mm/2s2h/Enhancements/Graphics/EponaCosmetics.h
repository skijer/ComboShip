#ifndef MM_EPONA_COSMETICS_H
#define MM_EPONA_COSMETICS_H

#include "z64skin.h"

#ifdef __cplusplus
extern "C" {
#endif

// Temporarily substitute cached material lists for this native Epona draw.
int MMEponaCosmetics_BeginDraw(struct PlayState* play, Skin* skin);
void MMEponaCosmetics_EndDraw(struct PlayState* play, Skin* skin);

#ifdef __cplusplus
}
#endif

#endif
