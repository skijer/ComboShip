#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int finChanged = 1, swordChanged = 1, boomerangChanged;
static const Color_RGBA8 finColor = {17, 34, 51, 255};
static const Color_RGBA8 swordColor = {91, 82, 73, 255};

int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (!strcmp(key, CVAR_COSMETIC("Trails.ZoraFinTrail.Changed"))) return finChanged;
    if (!strcmp(key, CVAR_COSMETIC("Trails.MasterSword.Changed"))) return swordChanged;
    if (!strcmp(key, CVAR_COSMETIC("Trails.Boomerang.Changed"))) return boomerangChanged;
    return fallback;
}
Color_RGBA8 CVarGetColor(const char* key, Color_RGBA8 fallback) {
    if (!strcmp(key, CVAR_COSMETIC("Trails.ZoraFinTrail.Value"))) return finColor;
    if (!strcmp(key, CVAR_COSMETIC("Trails.MasterSword.Value"))) return swordColor;
    if (!strcmp(key, CVAR_COSMETIC("Trails.Boomerang.Value"))) return finColor;
    return fallback;
}
#include "zora_blure.inc"

static EffectBlure trail(TrailType type, int color) {
    EffectBlure result = {0};
    EffectBlureInit2 init = {0};
    memset(init.p1StartColor, color, 3); init.p1StartColor[3] = 255;
    memset(init.p2StartColor, color, 3); init.p2StartColor[3] = 64;
    memset(init.p1EndColor, color, 3); memset(init.p2EndColor, color, 3);
    init.elemDuration = 4; init.trailType = type;
    EffectBlure_Init2(&result, &init);
    return result;
}

int main(void) {
    // The appended fin type is 9, leaving every existing effect ID unchanged.
    EffectBlure fin = trail((TrailType)9, 255), sword = trail(TRAIL_TYPE_MASTER_SWORD, 255);
    EffectBlure punch = trail(TRAIL_TYPE_SWORDS, 255);
    EffectBlure other = trail(TRAIL_TYPE_REST, 123);
    EffectBlure_Update(&sword);
    EffectBlure_Update(&fin);
    assert(fin.p1StartColor.r == finColor.r && fin.p1StartColor.g == finColor.g && fin.p1StartColor.b == finColor.b);
    assert(fin.p2StartColor.a == 64 && fin.p1EndColor.a == 0 && fin.p2EndColor.a == 0);
    EffectBlure_Update(&punch);
    EffectBlure_Update(&other);
    assert(punch.p1StartColor.r == 255 && punch.p1StartColor.g == 255 && punch.p1StartColor.b == 255);
    assert(other.p1StartColor.r == 123 && other.p1StartColor.g == 123 && other.p1StartColor.b == 123);
    finChanged = 0;
    EffectBlure_Update(&fin);
    assert(fin.p1StartColor.r == 255 && fin.p2StartColor.g == 255 && fin.p1EndColor.b == 255);
    assert(fin.p2StartColor.a == 64 && fin.p2EndColor.a == 0);
    swordChanged = 0;
    EffectBlure_Update(&sword);
    assert(sword.p1StartColor.r == 255 && sword.p1EndColor.b == 255);
    EffectBlure thrown = trail(TRAIL_TYPE_BOOMERANG, 255);
    EffectBlureInit2 cyan = {0};
    const u8 palette[4][4] = {{100,220,255,255},{50,180,255,64},{50,180,255,0},{50,180,255,0}};
    memcpy(cyan.p1StartColor, palette[0], 4); memcpy(cyan.p2StartColor, palette[1], 4);
    memcpy(cyan.p1EndColor, palette[2], 4); memcpy(cyan.p2EndColor, palette[3], 4);
    cyan.elemDuration = 4; cyan.trailType = TRAIL_TYPE_BOOMERANG;
    EffectBlure_Init2(&thrown, &cyan);
    EffectBlure_Update(&thrown);
    assert(!memcmp(&thrown.p1StartColor, palette[0], 4) && !memcmp(&thrown.p2StartColor, palette[1], 4));
    boomerangChanged = 1; EffectBlure_Update(&thrown);
    assert(thrown.p1StartColor.r == 17 && thrown.p2StartColor.a == 64);
    boomerangChanged = 0; EffectBlure_Update(&thrown);
    assert(!memcmp(&thrown.p1StartColor, palette[0], 4) && !memcmp(&thrown.p2EndColor, palette[3], 4));
    puts("PASS OoT fin color isolation, white Reset, native alpha and existing sword Reset");
}
