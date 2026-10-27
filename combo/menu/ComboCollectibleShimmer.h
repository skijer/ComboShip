#ifndef COMBO_COLLECTIBLE_SHIMMER_H
#define COMBO_COLLECTIBLE_SHIMMER_H

#include <stdint.h>

// itemShimmer is already an int32 in the cross-game ABI. Retain its palette
// mode through caches: legacy overlays keep their authored neutral sentinel;
// collectible overlays tint every nonwhite vertex to the editor's exact RGB.
enum { CW_ITEM_SHIMMER_NONE = 0, CW_ITEM_SHIMMER_LEGACY = 1, CW_ITEM_SHIMMER_TINTED = 2 };
typedef enum {
    CW_COLLECTIBLE_GREEN,
    CW_COLLECTIBLE_BLUE,
    CW_COLLECTIBLE_RED,
    CW_COLLECTIBLE_PURPLE,
    CW_COLLECTIBLE_LARGE,
    CW_COLLECTIBLE_SILVER,
    CW_COLLECTIBLE_HEART,
    CW_COLLECTIBLE_COUNT
} ComboCollectibleShimmerProfile;

typedef struct {
    const char* id;
    const char* changed;
    const char* value;
    uint8_t rgb[3];
} ComboCollectibleShimmerPalette;

static inline const ComboCollectibleShimmerPalette* ComboCollectibleShimmer_Palette(int mm, int profile) {
    static const ComboCollectibleShimmerPalette oot[CW_COLLECTIBLE_COUNT] = {
        { "Consumable.GreenRupee",
          "gCosmetics.Consumable.GreenRupee.Changed",
          "gCosmetics.Consumable.GreenRupee.Value",
          { 50, 255, 50 } },
        { "Consumable.BlueRupee",
          "gCosmetics.Consumable.BlueRupee.Changed",
          "gCosmetics.Consumable.BlueRupee.Value",
          { 50, 50, 255 } },
        { "Consumable.RedRupee",
          "gCosmetics.Consumable.RedRupee.Changed",
          "gCosmetics.Consumable.RedRupee.Value",
          { 255, 50, 50 } },
        { "Consumable.PurpleRupee",
          "gCosmetics.Consumable.PurpleRupee.Changed",
          "gCosmetics.Consumable.PurpleRupee.Value",
          { 150, 50, 255 } },
        { "Consumable.GoldRupee",
          "gCosmetics.Consumable.GoldRupee.Changed",
          "gCosmetics.Consumable.GoldRupee.Value",
          { 255, 190, 55 } },
        { "Consumable.SilverRupee",
          "gCosmetics.Consumable.SilverRupee.Changed",
          "gCosmetics.Consumable.SilverRupee.Value",
          { 255, 255, 255 } },
        { "Consumable.Hearts",
          "gCosmetics.Consumable.Hearts.Changed",
          "gCosmetics.Consumable.Hearts.Value",
          { 255, 70, 50 } },
    };
    static const ComboCollectibleShimmerPalette majora[CW_COLLECTIBLE_COUNT] = {
        { "Collectibles.RupeeGreen",
          "gCosmetic.Collectibles.RupeeGreen.Changed",
          "gCosmetic.Collectibles.RupeeGreen.Color",
          { 82, 255, 107 } },
        { "Collectibles.RupeeBlue",
          "gCosmetic.Collectibles.RupeeBlue.Changed",
          "gCosmetic.Collectibles.RupeeBlue.Color",
          { 132, 181, 239 } },
        { "Collectibles.RupeeRed",
          "gCosmetic.Collectibles.RupeeRed.Changed",
          "gCosmetic.Collectibles.RupeeRed.Color",
          { 239, 107, 107 } },
        { "Collectibles.RupeePurple",
          "gCosmetic.Collectibles.RupeePurple.Changed",
          "gCosmetic.Collectibles.RupeePurple.Color",
          { 255, 99, 231 } },
        { "Collectibles.RupeeOrange",
          "gCosmetic.Collectibles.RupeeOrange.Changed",
          "gCosmetic.Collectibles.RupeeOrange.Color",
          { 247, 148, 33 } },
        { "Collectibles.RupeeSilver",
          "gCosmetic.Collectibles.RupeeSilver.Changed",
          "gCosmetic.Collectibles.RupeeSilver.Color",
          { 189, 189, 198 } },
        { "HUD.Hearts", "gCosmetic.HUD.Hearts.Changed", "gCosmetic.HUD.Hearts.Color", { 255, 70, 50 } },
    };
    return profile >= 0 && profile < CW_COLLECTIBLE_COUNT ? (mm ? &majora[profile] : &oot[profile]) : 0;
}

#ifdef __cplusplus
extern "C" {
#endif
void OOT_SampleCollectibleShimmerColor(int32_t profile, uint8_t color[4]);
void MM_SampleCollectibleShimmerColor(int32_t profile, uint8_t color[4]);
#ifdef __cplusplus
}
#endif
#endif
