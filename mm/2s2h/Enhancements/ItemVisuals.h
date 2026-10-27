#ifndef ITEM_VISUALS_H
#define ITEM_VISUALS_H

#ifdef __cplusplus
extern "C" {
#endif

// Returns false when no custom draw or edited palette applies, or the selected GI is missing.
s32 GetItem_DrawDungeonItem(PlayState* play, s16 drawId, s32 owner);

// Asset paths and two color channels only; no game/save state mutation or resource load.
s32 GetItem_GetDungeonKeyModel(s16 drawId, s32 owner, const char** metalPath, const char** emblemPath,
                               Color_RGBA8* metalColor, Color_RGBA8* emblemColor);

// Palette-only helpers: work without replacement models and with Alt Assets disabled.
s32 GetItem_GetDungeonItemTint(s16 drawId, s32 owner, Color_RGBA8* color, u8* strength);
s32 GetItem_GetDungeonKeyEmblemTint(s32 owner, Color_RGBA8* color);
s32 GetItem_GetShimmerColor(s16 drawId, uint8_t color[4]);

#ifdef __cplusplus
}
#endif

#endif
