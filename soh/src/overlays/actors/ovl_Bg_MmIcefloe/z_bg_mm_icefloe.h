#ifndef Z_BG_MM_ICEFLOE_H
#define Z_BG_MM_ICEFLOE_H

#include "global.h"

typedef enum {
    MM_ICEFLOE_GROWING,
    MM_ICEFLOE_FLOATING,
    MM_ICEFLOE_MELTING,
} BgMmIcefloePhase;

typedef struct BgMmIcefloe {
    DynaPolyActor dyna;
    void* resources;
    Gfx* model;
    f32 fullScale;
    f32 meshTop;
    f32 surfaceOffset;
    s32 timer;
    BgMmIcefloePhase phase;
} BgMmIcefloe;

void BgMmIcefloe_Init(Actor* thisx, PlayState* play);
void BgMmIcefloe_Destroy(Actor* thisx, PlayState* play);
void BgMmIcefloe_Update(Actor* thisx, PlayState* play);
void BgMmIcefloe_Draw(Actor* thisx, PlayState* play);

#endif
