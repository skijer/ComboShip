#pragma once
// Keep resource signatures intact through gDPLoadTextureBlock: the interpreter
// then retains the Texture resource, flags and HD HByteScale/VPixelScale.
namespace RegionalBridgeMaterials {
enum class Material { LakeStone, SnowheadStone, SnowheadSnow, SnowheadIce, ValleySnow, LakeWater, LakeSand };
alignas(2) inline constexpr char kPath0[] = "__OTR__textures/comboship/regional/lake_hylia/water";
alignas(2) inline constexpr char kPath1[] = "__OTR__textures/comboship/regional/lake_hylia/stone";
alignas(2) inline constexpr char kPath2[] = "__OTR__textures/comboship/regional/snowhead/stone";
alignas(2) inline constexpr char kPath3[] = "__OTR__textures/comboship/regional/snowhead/ascent_snow";
alignas(2) inline constexpr char kPath4[] = "__OTR__textures/comboship/regional/snowhead/ice";
alignas(2) inline constexpr char kPath5[] = "__OTR__textures/comboship/regional/snowhead/valley_snow";
alignas(2) inline constexpr char kPath6[] = "__OTR__objects/object_spot06_objects/gLakeHyliaWaterTex";
// The old 00FEB8 bank tile contains grass bands. 010EB8 is the clean native rock.
alignas(2) inline constexpr char kPath7[] = "__OTR__scenes/shared/spot06_scene/spot06_sceneTex_010EB8";
alignas(2) inline constexpr char kPath8[] = "__OTR__objects/object_spot08_obj/gZorasFountainIceRampUpperSideTex";
alignas(2) inline constexpr char kPath9[] = "__OTR__objects/object_spot08_obj/gZorasFountainIceRampSurfaceTex";
alignas(2) inline constexpr char kPath10[] = "__OTR__objects/object_spot08_obj/gZorasFountainIceRampMiddleSlopeTex";
alignas(2) inline constexpr char kPath11[] = "__OTR__textures/comboship/regional/lake_hylia/sand";
alignas(2) inline constexpr char kPath12[] = "__OTR__scenes/shared/spot06_scene/spot06_sceneTex_00C2B8";
inline const char* OverridePath(Material material) {
    switch (material) {
        case Material::LakeWater:
            return kPath0;
        case Material::LakeStone:
            return kPath1;
        case Material::LakeSand:
            return kPath11;
        case Material::SnowheadStone:
            return kPath2;
        case Material::SnowheadSnow:
            return kPath3;
        case Material::SnowheadIce:
            return kPath4;
        case Material::ValleySnow:
            return kPath5;
    }
    return nullptr;
}
inline const char* FallbackPath(Material material) {
    switch (material) {
        case Material::LakeWater:
            return kPath6;
        case Material::LakeStone:
            return kPath7;
        case Material::LakeSand:
            return kPath12;
        case Material::SnowheadStone:
            return kPath8;
        case Material::SnowheadSnow:
        case Material::ValleySnow:
            return kPath9;
        case Material::SnowheadIce:
            return kPath10;
    }
    return nullptr;
}
template <class Exists> inline const char* Resolve(Material material, Exists exists) {
    const char* path = OverridePath(material);
    return path && exists(path + 7) ? path : FallbackPath(material);
}
} // namespace RegionalBridgeMaterials
