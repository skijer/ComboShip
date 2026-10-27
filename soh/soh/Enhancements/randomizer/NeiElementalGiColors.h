#pragma once
#include <cstdint>
#include <initializer_list>

namespace NeiElementalGi {
struct Palette {
    uint32_t hot, edge, shimmer;
};
inline Palette DefaultPalette(int profile, bool spell = false) {
    if (spell) {
        if (profile == 1)
            return { 0xFFF1A8, 0xD52C05, 0xFA8B20 };
        if (profile == 2)
            return { 0xE9FFD1, 0x27984B, 0x64FF78 };
        return { 0xE4FFFF, 0x197FCB, 0x55B4FF };
    }
    if (profile == 1)
        return { 0xFFF1A8, 0xD52C05, 0xFA8B20 };
    if (profile == 2)
        return { 0xE8FFFF, 0x006FCB, 0x357CFF };
    return { 0xFFFFDA, 0xECAF32, 0xFDFF7B };
}
inline uint32_t Mix(uint32_t a, uint32_t b, float amount) {
    uint32_t out = 0;
    for (int shift : { 16, 8, 0 })
        out |= uint32_t(((a >> shift) & 255) * (1 - amount) + ((b >> shift) & 255) * amount + .5f) << shift;
    return out;
}
// A reset picker preserves the authored palette and the accepted shimmer hex.
template <class Read> inline Palette ReadPalette(int profile, bool spell, bool mmOwner, Read read) {
    auto color = DefaultPalette(profile, spell);
    if (profile < 1 || profile > 3)
        return color;
    static const char* oot[3][2] = {
        { "gCosmetics.Arrows.FirePrimary", "gCosmetics.Arrows.FireSecondary" },
        { "gCosmetics.Arrows.IcePrimary", "gCosmetics.Arrows.IceSecondary" },
        { "gCosmetics.Arrows.LightPrimary", "gCosmetics.Arrows.LightSecondary" },
    };
    static const char* mm[3][2] = {
        { "gCosmetic.Effects.FireArrowPrim", "gCosmetic.Effects.FireArrowSec" },
        { "gCosmetic.Effects.IceArrowPrim", "gCosmetic.Effects.IceArrowSec" },
        { "gCosmetic.Effects.LightArrowPrim", "gCosmetic.Effects.LightArrowSec" },
    };
    static const char* ootSpell[3][2] = {
        { "gCosmetics.Magic.DinsPrimary", "gCosmetics.Magic.DinsSecondary" },
        { "gCosmetics.Magic.FaroresPrimary", "gCosmetics.Magic.FaroresSecondary" },
        { "gCosmetics.Magic.NayrusPrimary", "gCosmetics.Magic.NayrusSecondary" },
    };
    static const char* mmSpell[3][2] = {
        { "gCosmetic.Magic.DinsPrimary", "gCosmetic.Magic.DinsSecondary" },
        { "gCosmetic.Magic.FaroresPrimary", "gCosmetic.Magic.FaroresSecondary" },
        { "gCosmetic.Magic.NayrusPrimary", "gCosmetic.Magic.NayrusSecondary" },
    };
    const auto& row = spell ? (mmOwner ? mmSpell : ootSpell)[profile - 1] : (mmOwner ? mm : oot)[profile - 1];
    const bool primary = read(row[0], color.hot), secondary = read(row[1], color.edge);
    if (primary || secondary)
        color.shimmer = secondary ? color.edge : color.hot;
    return color;
}
} // namespace NeiElementalGi
