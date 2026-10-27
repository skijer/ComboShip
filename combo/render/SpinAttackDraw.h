// Native TU glue. Include after host engine, cosmetics and GBI declarations.
#ifndef COMBO_SPIN_ATTACK_DRAW_H
#define COMBO_SPIN_ATTACK_DRAW_H
#include <libultraship/bridge/resourcebridge.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include "SpinAttackStyleResources.h"
#include "SpinAttackStyleDependencies.h"
#include "SpinAttackNativeMaterial.h"
#include "SpinAttackAccentsDraw.h"

typedef struct {
    uint32_t primary, environment;
} CwSpinPalette;

static inline CwSpinPalette CwSpinDraw_Palette(CwSpinVisualSelection visual, int great, int charge) {
    static const CwSpinPalette colors[] = {
        { 0xFFFFFF, 0x6464FF }, { 0xFFBE46, 0xB41905 }, { 0xBEF5FF, 0x2D6ED2 }, { 0xFFFAA9, 0xDCA01E },
        { 0xBED2FF, 0x6432DC }, { 0xC87DFF, 0x500F96 }, { 0xB9CDFF, 0x5537A0 }, { 0x6E1EAF, 0xC80A2D },
    };
    int index = visual.style == CW_SPIN_STYLE_NATIVE ? visual.nativeMaterialElement : visual.style;
    if (index < 0 || index >= CW_SPIN_STYLE_COUNT)
        index = 0;
    CwSpinPalette palette = colors[index];
#ifdef CW_SPIN_HOST_MM
    const char* id = charge ? (great ? "Effects.GreatSpinCharge" : "Effects.SpinSlashCharge")
                            : (great ? "Effects.GreatSpinBurst" : "Effects.SpinSlashBurst");
    Color_RGBA8 color = CosmeticEditor_GetChangedColor(
        (uint8_t)(palette.primary >> 16), (uint8_t)(palette.primary >> 8), (uint8_t)palette.primary, 255, id);
    palette.primary = (uint32_t)color.r << 16 | (uint32_t)color.g << 8 | color.b;
    color = CosmeticEditor_GetChangedColor((uint8_t)(palette.environment >> 16), (uint8_t)(palette.environment >> 8),
                                           (uint8_t)palette.environment, 255, id);
    palette.environment = (uint32_t)color.r << 16 | (uint32_t)color.g << 8 | color.b;
#else
    const char* changed =
        great ? CVAR_COSMETIC("SpinAttack.Level2Primary.Changed") : CVAR_COSMETIC("SpinAttack.Level1Primary.Changed");
    const char* value =
        great ? CVAR_COSMETIC("SpinAttack.Level2Primary.Value") : CVAR_COSMETIC("SpinAttack.Level1Primary.Value");
    if (CVarGetInteger(changed, 0)) {
        Color_RGB8 c = CVarGetColor24(value, (Color_RGB8){ 255, 255, 255 });
        palette.primary = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
    }
    changed = great ? CVAR_COSMETIC("SpinAttack.Level2Secondary.Changed")
                    : CVAR_COSMETIC("SpinAttack.Level1Secondary.Changed");
    value =
        great ? CVAR_COSMETIC("SpinAttack.Level2Secondary.Value") : CVAR_COSMETIC("SpinAttack.Level1Secondary.Value");
    if (CVarGetInteger(changed, 0)) {
        Color_RGB8 c = CVarGetColor24(value, (Color_RGB8){ 255, 255, 255 });
        palette.environment = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
    }
#endif
    return palette;
}

static inline int CwSpinDraw_Available(CwSpinVisualSelection visual) {
    CwSpinStyle style = visual.style;
    if (style == CW_SPIN_STYLE_NATIVE)
        style = (CwSpinStyle)visual.nativeMaterialElement;
    size_t count;
    const char* const* paths = CwSpinStyle_Dependencies(style, &count);
    if (count == 0)
        return 0;
    for (size_t i = 0; i < count; ++i)
        if (ResourceGetDataByName(paths[i]) == NULL)
            return 0;
    return 1;
}

static inline Gfx* CwSpinDraw_LoadDL(const char* path) {
    if (path == NULL)
        return NULL;
    size_t size = ResourceGetSizeByName(path);
    if (size == 0 || size % sizeof(Gfx) != 0 || size > 1024 * sizeof(Gfx))
        return NULL;
    Gfx* data = (Gfx*)ResourceGetDataByName(path);
    return data && ((uint32_t)data[size / sizeof(Gfx) - 1].words.w0 >> 24) == 0xDF ? data : NULL;
}

static inline Gfx* CwSpinDraw_Clone(PlayState* play, const char* native, const char* const textures[2],
                                    CwSpinPalette palette) {
    Gfx* source = CwSpinDraw_LoadDL(native);
    if (!source || !textures[0] || !textures[1])
        return NULL;
    size_t count = ResourceGetSizeByName(native) / sizeof(Gfx);
    size_t bytes = 2 * count * sizeof(Gfx);
    uintptr_t head = (uintptr_t)play->state.gfxCtx->polyOpa.p;
    uintptr_t tail = (uintptr_t)play->state.gfxCtx->polyOpa.d;
    // OPEN_DISPS writes an OPA marker even for an XLU-only draw. OoT removes
    // that marker without restoring its word, so keep it outside the clone.
    if (tail < head || tail - head < ((bytes + 15) & ~(size_t)15) + sizeof(Gfx))
        return NULL;
    Gfx* copy = (Gfx*)CwSpinDraw_Alloc(play->state.gfxCtx, bytes);
    if (copy == NULL)
        return NULL;
    return CwSpinMaterial_Clone(copy, 2 * count, source, count, ResourceGetCrcByName(textures[0]),
                                ResourceGetCrcByName(textures[1]), palette.primary, palette.environment)
               ? copy
               : NULL;
}

static inline void CwSpinDraw_SetColors(PlayState* play, CwSpinPalette palette, float alpha) {
    if (alpha < 0)
        alpha = 0;
    if (alpha > 1)
        alpha = 1;
    OPEN_DISPS(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, (uint8_t)(palette.primary >> 16), (uint8_t)(palette.primary >> 8),
                    (uint8_t)palette.primary, (uint8_t)(alpha * 255));
    gDPSetEnvColor(POLY_XLU_DISP++, (uint8_t)(palette.environment >> 16), (uint8_t)(palette.environment >> 8),
                   (uint8_t)palette.environment, 128);
    CLOSE_DISPS(play->state.gfxCtx);
}

static inline int CwSpinDraw_Burst(PlayState* play, CwSpinVisualSelection visual, int great, float alpha,
                                   const char* nativeDisk, const char* nativeCylinder) {
    if (!CwSpinDraw_Available(visual))
        return 0;
    CwSpinResourceRecipe recipe = CwSpinStyle_Resources(visual.style, visual.nativeMaterialElement, great);
    CwSpinPalette palette = CwSpinDraw_Palette(visual, great, 0);
    // Explicit styles need their authored shell, UV state and texture masks.
    // Only Native follows the currently selected host/custom shell.
    int nativeShape = visual.style == CW_SPIN_STYLE_NATIVE;
    // The imported Ice/Light great shells still embed the Fire donor's second
    // texture. Rebind their own sigil in a frame-owned copy of the authored
    // shell, retaining its geometry, UVs, scrolling and level-specific shape.
    int themedGreat = great && (visual.style == CW_SPIN_STYLE_ICE || visual.style == CW_SPIN_STYLE_LIGHT);
    Gfx* disk = nativeShape || themedGreat
                    ? CwSpinDraw_Clone(play, nativeShape ? nativeDisk : recipe.disk, recipe.burstTex, palette)
                    : CwSpinDraw_LoadDL(recipe.disk);
    Gfx* cylinder = nativeShape || themedGreat ? CwSpinDraw_Clone(play, nativeShape ? nativeCylinder : recipe.cylinder,
                                                                  recipe.burstTex, palette)
                                               : CwSpinDraw_LoadDL(recipe.cylinder);
    if (!disk || !cylinder)
        return 0;
    CwSpinDraw_SetColors(play, palette, alpha);
    OPEN_DISPS(play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, disk);
    gSPDisplayList(POLY_XLU_DISP++, cylinder);
    CLOSE_DISPS(play->state.gfxCtx);
    return 1;
}

static inline int CwSpinDraw_Charge(PlayState* play, CwSpinVisualSelection visual, int great, float alpha,
                                    const char* nativeCharge) {
    if (!CwSpinDraw_Available(visual))
        return 0;
    CwSpinResourceRecipe recipe = CwSpinStyle_Resources(visual.style, visual.nativeMaterialElement, great);
    CwSpinPalette palette = CwSpinDraw_Palette(visual, great, 1);
    int nativeShape = visual.style == CW_SPIN_STYLE_NATIVE || visual.style == CW_SPIN_STYLE_ICE ||
                      visual.style == CW_SPIN_STYLE_LIGHT;
    Gfx* charge = nativeShape ? CwSpinDraw_Clone(play, nativeCharge, recipe.chargeTex, palette)
                              : CwSpinDraw_LoadDL(recipe.charge);
    if (!charge)
        return 0;
    CwSpinDraw_SetColors(play, palette, alpha);
    OPEN_DISPS(play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, charge);
    CLOSE_DISPS(play->state.gfxCtx);
    if (CwSpinAccents_DrawCharge(play, visual.accentElement, great, play->gameplayFrames * 0.05f, alpha))
        CwSpinDraw_SetColors(play, palette, alpha);
    return 1;
}

static inline void CwSpinDraw_Accents(PlayState* play, CwSpinVisualSelection visual, int great, float phase,
                                      float alpha) {
    // The burst texture counter advances up to 2 per gameplay tick. Particle
    // motion uses the charge preview's 0.05-per-tick scale, avoiding large
    // matrix turns that collapse shards/sparkles during frame interpolation.
    float motionPhase = phase * 0.025f;
    size_t budget = great ? 24 : 12;
    int drawn = 0;
    if (visual.style == CW_SPIN_STYLE_DARK_ENERGY) {
        size_t sparks = great ? 8 : 4;
        drawn |= CwSpinAccents_DrawLimited(play, 1, great, motionPhase, alpha, sparks, 0xE61946);
        budget -= sparks;
    }
    drawn |= CwSpinAccents_DrawLimited(play, visual.accentElement, great, motionPhase, alpha, budget, 0);
    if (drawn)
        CwSpinDraw_SetColors(play, CwSpinDraw_Palette(visual, great, 0), alpha);
}
#endif
