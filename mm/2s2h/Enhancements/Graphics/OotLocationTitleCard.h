#pragma once
#include "global.h"

// Called only from the native scene-message path. Boss/custom TitleCardContext
// presenters do not enter this path. The option is latched for one message.
void OotLocationTitle_Reset(void);
void OotLocationTitle_Begin(PlayState* play);
int OotLocationTitle_Update(PlayState* play);
int OotLocationTitle_Draw(PlayState* play, Gfx** gfxP);
