#pragma once
#include "ComboSwordGiFit.h"
#include "../../soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include <cstdint>
#include <cstring>

extern "C" int ResourceMgr_IsModAssetForGame(const char* game, const char* path);
extern "C" uint8_t MmAssets_IsAvailable(void);

// These are the original legacy callbacks' model spaces, not authored NEI
// replacements. Query the same winning local/donor root that the callback draws.
static inline void ComboSwordGi_ApplyLegacyFit(const char* host, NeiGi::Kind kind, bool shop = false, int mmPickup = 0,
                                               float drawScale = 0.f) {
    const char* importedPrimary = kind == NeiGi::Kind::RazorSword    ? "objects/object_gi_sword_2/gGiRazorSwordDL"
                                  : kind == NeiGi::Kind::GildedSword ? "objects/object_gi_sword_3/gGiGildedSwordDL"
                                  : kind == NeiGi::Kind::GreatFairySword
                                      ? "objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL"
                                      : nullptr;
    if (!std::strcmp(host, "oot") && importedPrimary && !ResourceMgr_IsModAssetForGame("oot", importedPrimary) &&
        !ResourceMgr_IsModAssetForGame("mm", importedPrimary) && !MmAssets_IsAvailable()) {
        // Match LoadMmDLOnce's retained tint fallback only on proven donor
        // absence. An unsupported selected graph must never select this path.
        drawScale = kind == NeiGi::Kind::GreatFairySword ? .5f : .55f;
        kind = kind == NeiGi::Kind::GreatFairySword ? NeiGi::Kind::BiggoronSword : NeiGi::Kind::KokiriSword;
    }
    const char* paths[2] = {};
    int count = 1;
    float scale = 1.f, tilt = 0.f;
    const char* fitOwner = host;
#define COMBO_LEGACY_SWORD_PATH(path, donorMm)                                                                     \
    ((!std::strcmp(host, "mm") || ((donorMm) && !ResourceMgr_IsModAssetForGame("oot", path))) ? "__OTR__@mm:" path \
                                                                                              : "__OTR__@oot:" path)
    switch (kind) {
        case NeiGi::Kind::KokiriSword:
            paths[0] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_1/gGiKokiriSwordDL", false);
            break;
        case NeiGi::Kind::MmKokiriSword:
            paths[0] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_1/gGiKokiriSwordGuardDL", true);
            paths[1] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_1/gGiKokiriSwordBladeHiltDL", true);
            count = 2;
            break;
        case NeiGi::Kind::RazorSword:
            paths[0] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_2/gGiRazorSwordDL", true);
            paths[1] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_2/gGiRazorSwordEmptyDL", true);
            count = 2;
            break;
        case NeiGi::Kind::GildedSword:
            paths[0] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_3/gGiGildedSwordDL", true);
            paths[1] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_3/gGiGildedSwordEmptyDL", true);
            count = 2;
            break;
        case NeiGi::Kind::GreatFairySword:
            paths[0] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL", true);
            paths[1] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL", true);
            count = 2;
            break;
        case NeiGi::Kind::MasterSword:
        case NeiGi::Kind::SwordAura:
            paths[0] = COMBO_LEGACY_SWORD_PATH("objects/object_toki_objects/object_toki_objects_DL_001BD0", false);
            scale = .05f;
            tilt = 2.1f;
            break;
        case NeiGi::Kind::BiggoronSword:
        case NeiGi::Kind::GiantsKnife:
            if (!std::strcmp(host, "mm") &&
                !ResourceMgr_IsModAssetForGame("mm", "objects/object_gi_longsword/gGiBiggoronSwordDL")) {
                paths[0] = "objects/object_gi_longsword/gGiBiggoronSwordDL";
                fitOwner = "oot-companion";
            } else {
                paths[0] = COMBO_LEGACY_SWORD_PATH("objects/object_gi_longsword/gGiBiggoronSwordDL", false);
            }
            break;
        case NeiGi::Kind::FourSword:
            if (!std::strcmp(host, "mm")) {
                paths[0] = "__OTR__@mm:objects/object_gi_sword_1/gGiKokiriSwordGuardDL";
                paths[1] = "__OTR__@mm:objects/object_gi_sword_1/gGiKokiriSwordBladeHiltDL";
            } else {
                paths[0] = "__OTR__@oot:objects/object_nei_four_sword/gNeiFourSwordBladeDL";
                paths[1] = "__OTR__@oot:objects/object_nei_four_sword/gNeiFourSwordHiltDL";
                scale = .04f;
                tilt = ComboSwordGi_SelectedTilt(1.5707963267948966f, shop);
            }
            count = 2;
            break;
        default:
            return;
    }
#undef COMBO_LEGACY_SWORD_PATH
    if (drawScale > 0.f)
        scale = drawScale;
    ComboSwordGi_ApplyModelsFit(fitOwner, paths, count, scale, tilt, shop, mmPickup);
}
