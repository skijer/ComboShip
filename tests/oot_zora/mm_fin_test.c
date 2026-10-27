#include "libultraship/libultra/controller.h"
#undef BTN_A
#undef BTN_B
#undef BTN_Z
#undef BTN_START
#undef BTN_DUP
#undef BTN_DDOWN
#undef BTN_DLEFT
#undef BTN_DRIGHT
#undef BTN_L
#undef BTN_R
#undef BTN_CUP
#undef BTN_CDOWN
#undef BTN_CLEFT
#undef BTN_CRIGHT
#include "global.h"
#include "z64eff_blure.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define COSMETIC_ID(id) id
PlayState* gPlayState;
static PlayState play;
static Player player;
static EffectBlure fins[2];
static int changed = 1;
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    if (!changed) return (Color_RGBA8){r,g,b,a};
    if (!strcmp(id, "Trails.ZoraFinTrail")) return (Color_RGBA8){17,34,51,a};
    if (!strcmp(id, "Trails.ZoraKickTrail")) return (Color_RGBA8){61,72,83,a};
    if (!strcmp(id, "Trails.ZoraPunchTrail")) return (Color_RGBA8){91,82,73,a};
    return (Color_RGBA8){r,g,b,a};
}
void* Effect_GetByIndex(s32 index) { return &fins[index]; }
s16 EffectSs_LerpS16(s16 a, s16 b, f32 ratio) { return a + (b-a)*ratio; }
u8 EffectSs_LerpU8(u8 a, u8 b, f32 ratio) { return a + (b-a)*ratio; }
#include "mm_fin.inc"

static Color_RGBA8 readColor(int fin, float ratio) {
    Vec3s first, second;
    Color_RGBA8 start, end;
    EffectBlure_GetComputedValues(&fins[fin], 0, ratio, &first, &second, &start, &end);
    assert(end.a == (u8)(50*(1-ratio)));
    return start;
}
int main(void) {
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.transformation = PLAYER_FORM_ZORA;
    player.meleeWeaponEffectIndex[0] = 0; player.meleeWeaponEffectIndex[1] = 1;
    player.meleeWeaponEffectIndex[2] = -1;
    for (int i=0;i<2;++i) {
        const u8 outer[4] = {165,185,255,185}, inner[4] = {205,225,255,50};
        memcpy(fins[i].p1StartColor, outer, 4); memcpy(fins[i].p2StartColor, inner, 4);
        memset(fins[i].p1EndColor, 255, 3); memset(fins[i].p2EndColor, 255, 3);
    }
    player.stateFlags1 = PLAYER_STATE1_8000000;
    player.stateFlags3 = PLAYER_STATE3_8000;
    player.skelAnime.animation = (void*)gPlayerAnim_pz_fishswim;
    player.meleeWeaponAnimation = PLAYER_MWA_ZORA_PUNCH_KICK; // stale last attack must not recolor swimming
    for (int i=0;i<2;++i) {
        Color_RGBA8 color = readColor(i, 0);
        assert(color.r == 17 && color.g == 34 && color.b == 51 && color.a == 185);
    }
    player.stateFlags1 = player.stateFlags3 = 0;
    player.meleeWeaponState = 1;
    Color_RGBA8 color = readColor(0, 0);
    assert(color.r == 61 && color.g == 72 && color.b == 83);
    player.meleeWeaponAnimation = PLAYER_MWA_ZORA_PUNCH_LEFT;
    color = readColor(0, 0);
    assert(color.r == 91 && color.g == 82 && color.b == 73);
    player.meleeWeaponState = 0; player.stateFlags1 = PLAYER_STATE1_8000000;
    player.skelAnime.animation = NULL;
    color = readColor(0, 0);
    assert(color.r == 91 && "a fading underwater punch is not a swimming fin ribbon");
    player.skelAnime.animation = (void*)gPlayerAnim_pz_swimtowait;
    color = readColor(0, 0);
    assert(color.r == 17 && "fin ribbons retain their picker during the swim exit clip");
    player.meleeWeaponState = 0; player.stateFlags1 = PLAYER_STATE1_8000000;
    changed = 0; color = readColor(0, 0);
    assert(color.r == 165 && color.g == 185 && color.b == 255 && color.a == 185);
    color = readColor(0, .5f);
    assert(color.r == 210 && color.g == 220 && color.b == 255 && color.a == 92);
    puts("PASS MM independent fin/punch/kick colors, both fins, stale attack state, alpha and Reset");
}
