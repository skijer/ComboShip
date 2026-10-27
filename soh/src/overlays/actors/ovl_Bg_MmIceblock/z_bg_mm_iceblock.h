#ifndef Z_BG_MM_ICEBLOCK_H
#define Z_BG_MM_ICEBLOCK_H

#include "global.h"

typedef enum {
    MM_ICEBLOCK_GROWING,
    MM_ICEBLOCK_SOLID,
    MM_ICEBLOCK_MELTING,
} BgMmIceblockPhase;

typedef struct BgMmIceblock {
    DynaPolyActor dyna;
    ColliderCylinder collider;
    void* resources;
    Gfx* model;
    Vec3f fullScale;
    f32 progress;
    f32 meltSpeed;
    f32 waterHeight;
    f32 meshBottom;
    s16 growTimer;
    s16 lifetime;
    u8 colliderInitialized;
    BgMmIceblockPhase phase;
} BgMmIceblock;

void BgMmIceblock_Init(Actor* thisx, PlayState* play);
void BgMmIceblock_Destroy(Actor* thisx, PlayState* play);
void BgMmIceblock_Update(Actor* thisx, PlayState* play);
void BgMmIceblock_Draw(Actor* thisx, PlayState* play);

#endif
