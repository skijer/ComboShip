#include "NeiGiPresentation.h"
#include "NeiGiEffectPolicy.h"
#include "NeiGiEnergyTexture.h"
#include "NeiGiRender.h"
#include "NeiGiShopFit.h"
#include "ComboIkanaShieldResources.h"
#include "ComboSongDrawOOT.h"
#include "objects/object_gi_melody/object_gi_melody.h"
#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_set>
#include "draw.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/frame_interpolation.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
}
#include "ComboSwordGiFit.h"
#include "ComboSwordGiLegacyFit.h"
#ifdef COMBO_BUILD
#include "ComboItemDrawABI.h"
#include "ComboResolve.h"
#endif

namespace {
using NeiGi::Kind;
struct Presentation {
    CustomDrawFunc draw;
    const char* opaque;
    const char* translucent;
    float scale;
    Kind effect;
    Vec3f effectCenter;
    NeiGi::ShopFit shop;
    int32_t identity = 0; // distinct awards sharing one legacy callback
    bool alwaysShimmer = false;
    int32_t nativeGid = -1;
    Kind shimmer = Kind::Neutral; // A separate palette leaves geometry/energy identity intact.
};
Kind PresentationShimmer(const Presentation& item) {
    return item.shimmer == Kind::Neutral ? item.effect : item.shimmer;
}

#define GI_PATH(slug) "__OTR__objects/nei_gi_redesign/" slug "/gi_dl"
#define GI_XLU(slug) "__OTR__objects/nei_gi_redesign/" slug "/gi_xlu_dl"
// These bindings are for presentation only. Actor and held-item resources retain their own paths.
const Presentation kPresentations[] = {
    { Randomizer_DrawRocsFeatherSkijer,
      GI_PATH("rocs_feather"),
      nullptr,
      .5f,
      Kind::Neutral,
      {},
      NeiGi::kFeatherShopFit },
    { Randomizer_DrawRocsFeather, GI_PATH("rocs_feather"), nullptr, .5f, Kind::Neutral, {}, NeiGi::kFeatherShopFit },
    { Randomizer_DrawWhip, GI_PATH("whip"), nullptr, .5f, Kind::Neutral, {} },
    { Randomizer_DrawFireRod,
      GI_PATH("fire_rod"),
      nullptr,
      .2f,
      Kind::Fire,
      { 9.883f, 30.415f, 0 },
      NeiGi::kRodShopFit },
    { Randomizer_DrawIceRod, GI_PATH("ice_rod"), nullptr, .2f, Kind::Ice, { 10.365f, 31.899f, 0 }, NeiGi::kRodShopFit },
    { Randomizer_DrawLightRod,
      GI_PATH("light_rod"),
      nullptr,
      .2f,
      Kind::Light,
      { 9.883f, 30.415f, 0 },
      NeiGi::kRodShopFit },
    { Randomizer_DrawDekuLeaf, GI_PATH("deku_leaf"), nullptr, .5f, Kind::Leaf, {} },
    { Randomizer_DrawSwitchHook, GI_PATH("switch_hook"), nullptr, .01f, Kind::Neutral, {}, NeiGi::kSwitchHookShopFit },
    { Randomizer_DrawMogmaMitts, GI_PATH("mogma_mitts"), nullptr, .5f, Kind::Neutral, {} },
    { Randomizer_DrawGustJar, GI_PATH("gust_jar"), nullptr, 5.f, Kind::Neutral, {} },
    { Randomizer_DrawBallAndChain,
      GI_PATH("ball_and_chain"),
      nullptr,
      .25f,
      Kind::Neutral,
      {},
      NeiGi::kBallAndChainShopFit },
    { Randomizer_DrawTimeGate, GI_PATH("time_gate"), nullptr, .5f, Kind::Neutral, {}, NeiGi::kTimeGateShopFit },
    { Randomizer_DrawBeetle, GI_PATH("beetle"), nullptr, .3f, Kind::Neutral, {} },
    { Randomizer_DrawShovel, GI_PATH("shovel"), nullptr, .2f, Kind::Neutral, {}, NeiGi::kShovelShopFit },
    { Randomizer_DrawHyliaGrace,
      GI_PATH("hylia_grace"),
      GI_XLU("hylia_grace"),
      1.f,
      Kind::Hylia,
      {},
      NeiGi::kSpellShopFit },
    { Randomizer_DrawZonaiPermafrost,
      GI_PATH("zonai_permafrost"),
      GI_XLU("zonai_permafrost"),
      1.f,
      Kind::Zonai,
      {},
      NeiGi::kSpellShopFit },
    { Randomizer_DrawDemiseDestruction,
      GI_PATH("demise_destruction"),
      GI_XLU("demise_destruction"),
      1.f,
      Kind::Demise,
      {},
      NeiGi::kSpellShopFit },
    { Randomizer_DrawRocsCape, GI_PATH("rocs_cape"), nullptr, .6f, Kind::Neutral, {}, NeiGi::kRocsCapeShopFit },
    { Randomizer_DrawSpinner, GI_PATH("spinner"), nullptr, .3f, Kind::Neutral, {} },
    { Randomizer_DrawBombArrows, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawCaneOfSomaria,
      GI_PATH("cane_of_somaria"),
      nullptr,
      .25f,
      Kind::Somaria,
      {},
      NeiGi::kSomariaShopFit },
    { Randomizer_DrawCaneSomariaUpgrade,
      GI_PATH("cane_of_somaria"),
      nullptr,
      .25f,
      Kind::Somaria,
      {},
      NeiGi::kSomariaShopFit },
    { Randomizer_DrawCanePacci, nullptr, nullptr, 1.f, Kind::Pacci, {} },
    { Randomizer_DrawCanePacciUpgrade, nullptr, nullptr, 1.f, Kind::Pacci, {} },
    { Randomizer_DrawCanePacciUltrahand, nullptr, nullptr, 1.f, Kind::Pacci, {} },
    { Randomizer_DrawPokeball, nullptr, nullptr, 1.f, Kind::Pokeball, {} },
    { Randomizer_DrawMarioMask, GI_PATH("mario_mask"), nullptr, 1.f, Kind::MarioMask, {}, {}, 0, true, -1 },
    { nullptr, GI_PATH("cojiro"), nullptr, 1.f, Kind::Neutral, {}, {}, 0, false, GID_COJIRO },
    { Randomizer_DrawMinishCap, GI_PATH("minish_cap"), nullptr, .5f, Kind::Neutral, {} },
    { Randomizer_DrawDominionRod, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawMagnesis, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawStasis, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawLantern, GI_PATH("lantern"), GI_XLU("lantern"), .025f, Kind::Neutral, {} },
    { Randomizer_DrawCryonis, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawElementalWand,
      GI_PATH("elemental_wand"),
      nullptr,
      1.f,
      Kind::Light,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("sand_rod"),
      nullptr,
      1.f,
      Kind::Sand,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_SAND_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("tornado_rod"),
      nullptr,
      1.f,
      Kind::Tornado,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_TORNADO_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("water_rod"),
      nullptr,
      1.f,
      Kind::Water,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_WATER_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("meteor_rod"),
      nullptr,
      1.f,
      Kind::Meteor,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_METEOR_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("storm_rod"),
      nullptr,
      1.f,
      Kind::Storm,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_STORM_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("shadow_scepter"),
      nullptr,
      1.f,
      Kind::Shadow,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_SHADOW_SCEPTER,
      false,
      -1 },
    { Randomizer_DrawExtDivineShield,
      GI_PATH("divine_shield"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::DivineShield },
    { Randomizer_DrawExtSheikahShield,
      GI_PATH("sheikah_shield"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::SheikahShield },
    { Randomizer_DrawExtShieldOfIkana,
      GI_PATH("shield_of_ikana"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::IkanaShield },
    { Randomizer_DrawExtMagicCape,
      GI_PATH("magic_cape"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::MagicCape },
    { Randomizer_DrawExtSpiritBreastplate,
      GI_PATH("spirit_breastplate"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::SpiritBreastplate },
    { Randomizer_DrawExtSagesTunic,
      GI_PATH("sages_tunic"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::SagesTunic },
    { Randomizer_DrawExtChampionsTunic,
      GI_PATH("champions_tunic"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::ChampionsTunic },
    { Randomizer_DrawExtPegasusAnklet,
      GI_PATH("pegasus_anklet"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::PegasusAnklet },
    { Randomizer_DrawExtTrident,
      GI_PATH("trident"),
      nullptr,
      1.f,
      Kind::Gold,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawExtClimbBoots,
      GI_PATH("climb_boots"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::ClimbBoots },
    { Randomizer_DrawExtRocBoots,
      GI_PATH("roc_boots"),
      nullptr,
      1.f,
      Kind::Gold,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawExtCaneOfByrna,
      GI_PATH("cane_of_byrna"),
      nullptr,
      1.f,
      Kind::CaneBlue,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawExtFourSword,
      GI_PATH("four_sword"),
      nullptr,
      1.f,
      Kind::FourSword,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawExtPendantOfMemories,
      GI_PATH("pendant_of_memories"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::PendantOfMemories },
    { Randomizer_DrawMmTradeQuest,
      GI_PATH("room_key"),
      nullptr,
      1.f,
      Kind::Neutral,
      {},
      { .65f, 8.f },
      RG_MM_ROOM_KEY },
    { Randomizer_DrawNeiSheikahSlate,
      GI_PATH("sheikah_slate"),
      nullptr,
      1.f,
      Kind::Slate,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneBomb,
      GI_PATH("slate_bomb"),
      nullptr,
      1.f,
      Kind::SlateBomb,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneMasterCycle,
      GI_PATH("slate_master_cycle"),
      nullptr,
      1.f,
      Kind::SlateCycle,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneStasis,
      GI_PATH("slate_stasis"),
      nullptr,
      1.f,
      Kind::SlateStasis,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneCryonis,
      GI_PATH("slate_cryonis"),
      nullptr,
      1.f,
      Kind::SlateCryonis,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneSensor,
      GI_PATH("slate_sensor"),
      nullptr,
      1.f,
      Kind::SlateSensor,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawNeiPhantomHourglass,
      GI_PATH("phantom_hourglass"),
      GI_XLU("phantom_hourglass"),
      1.f,
      Kind::Hourglass,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawNeiShadowCrystal,
      GI_PATH("shadow_crystal"),
      nullptr,
      1.f,
      Kind::DarkCrystal,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawNeiRodOfSeasons,
      GI_PATH("rod_of_seasons"),
      nullptr,
      1.f,
      Kind::SeasonCycle,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawProgressiveKokiriSword,
      GI_PATH("kokiri_sword"),
      nullptr,
      1.f,
      Kind::KokiriSword,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawProgressiveMasterSword,
      GI_PATH("master_sword"),
      nullptr,
      1.f,
      Kind::MasterSword,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kMasterSwordShopFit,
      0,
      true,
      -1 },
    { Randomizer_DrawMasterSword,
      GI_PATH("master_sword"),
      nullptr,
      1.f,
      Kind::MasterSword,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kMasterSwordShopFit,
      0,
      true,
      -1 },
    { Randomizer_DrawGiantsKnife,
      GI_PATH("giants_knife"),
      nullptr,
      1.f,
      Kind::GiantsKnife,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kBiggoronSwordShopFit,
      0,
      true,
      -1 },
    { Randomizer_DrawProgressiveBGS,
      GI_PATH("biggoron_sword"),
      nullptr,
      1.f,
      Kind::BiggoronSword,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kBiggoronSwordShopFit,
      0,
      true,
      -1 },
    { Randomizer_DrawRazorSword,
      GI_PATH("razor_sword"),
      nullptr,
      1.f,
      Kind::RazorSword,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawGildedSword,
      GI_PATH("gilded_sword"),
      nullptr,
      1.f,
      Kind::GildedSword,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kGildedSwordShopFit,
      0,
      true,
      -1 },
    { Randomizer_DrawTrueMasterSword,
      GI_PATH("true_master_sword"),
      nullptr,
      1.f,
      Kind::SwordAura,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kMasterSwordShopFit,
      0,
      true,
      -1 },
    { Randomizer_DrawGreatFairySword,
      GI_PATH("great_fairy_sword"),
      nullptr,
      1.f,
      Kind::GreatFairySword,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kGreatFairySwordShopFit,
      0,
      true,
      -1 },
    { Randomizer_DrawIronKnuckleAxe,
      GI_PATH("iron_knuckle_axe"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1,
      Kind::IronKnuckleAxe },
    { nullptr,
      GI_PATH("kokiri_sword"),
      nullptr,
      1.f,
      Kind::KokiriSword,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      GID_SWORD_KOKIRI },
    { nullptr,
      GI_PATH("biggoron_sword"),
      nullptr,
      1.f,
      Kind::BiggoronSword,
      { 0.0000f, 0.0000f, 0.0000f },
      NeiGi::kBiggoronSwordShopFit,
      0,
      true,
      GID_SWORD_BGS },
};
// MM owns a distinct Kokiri item even when OoT renders its exported recipe.
const Presentation kMmKokiriPresentation{
    nullptr, GI_PATH("mm_kokiri_sword"), nullptr, 1.f, Kind::MmKokiriSword, {}, { .85f, 14.f }, 0, true, -1
};
// The imported catalog alias keeps MM's existing two-pass trade model. It
// receives only the equipment identity overlay, never an authored replacement.
const Presentation kMmPendantPresentation{ Randomizer_DrawMmTradeQuest,
                                           nullptr,
                                           nullptr,
                                           1.f,
                                           Kind::Neutral,
                                           {},
                                           { .85f, 14.f },
                                           RG_MM_PENDANT_OF_MEMORIES,
                                           true,
                                           -1,
                                           Kind::PendantOfMemories };
#undef GI_PATH
#undef GI_XLU

const Presentation* FindPresentation(const GetItemEntry* entry) {
    if (!entry)
        return nullptr;
    if (entry->drawFunc == Randomizer_DrawMmTradeQuest && entry->drawItemId == RG_MM_PENDANT_OF_MEMORIES)
        return &kMmPendantPresentation;
    // Knife and Biggoron deliberately share the original native GID. Keep
    // their concrete award identity before selecting the generic GID fallback.
    if (entry->gid == GID_SWORD_BGS && ((entry->tableId == TABLE_RANDOMIZER && entry->drawItemId == RG_GIANTS_KNIFE) ||
                                        (!entry->drawFunc && entry->getItemId == GI_SWORD_KNIFE))) {
        for (const auto& candidate : kPresentations)
            if (candidate.draw == Randomizer_DrawGiantsKnife)
                return &candidate;
    }
    const Presentation* fallback = nullptr;
    for (const auto& candidate : kPresentations) {
        if (!entry->drawFunc && !candidate.draw && candidate.nativeGid == entry->gid)
            return &candidate;
        if (!entry->drawFunc || candidate.draw != entry->drawFunc)
            continue;
        if (candidate.identity && candidate.identity == entry->drawItemId)
            return &candidate;
        if (!candidate.identity)
            fallback = &candidate;
    }
    return fallback;
}

int SongForEntry(const GetItemEntry* entry) {
    if (!entry)
        return -1;
    if (entry->tableId == TABLE_RANDOMIZER) {
        if (entry->drawItemId == RG_MM_SONG_LULLABY_PROGRESSIVE)
            return CW_SONG_LULLABY_INTRO;
        const int song = ComboSongForOotItem(entry->drawItemId);
        if (song >= 0)
            return song;
    }
    // Native table calls retain unambiguous note identities. Imported aliases
    // above take priority over their deliberately shared native GIDs.
    switch (entry->gid) {
        case GID_SONG_MINUET:
            return CW_SONG_OOT_MINUET;
        case GID_SONG_BOLERO:
            return CW_SONG_OOT_BOLERO;
        case GID_SONG_SERENADE:
            return CW_SONG_OOT_SERENADE;
        case GID_SONG_REQUIEM:
            return CW_SONG_OOT_REQUIEM;
        case GID_SONG_NOCTURNE:
            return CW_SONG_OOT_NOCTURNE;
        case GID_SONG_PRELUDE:
            return CW_SONG_OOT_PRELUDE;
        case GID_SONG_ZELDA:
            return CW_SONG_OOT_ZELDA;
        case GID_SONG_EPONA:
            return CW_SONG_OOT_EPONA;
        case GID_SONG_SARIA:
            return CW_SONG_OOT_SARIA;
        case GID_SONG_SUN:
            return CW_SONG_SUN;
        case GID_SONG_TIME:
            return CW_SONG_TIME;
        case GID_SONG_STORM:
            return CW_SONG_STORMS;
        default:
            return -1;
    }
}

struct SeasonPresentation {
    CustomDrawFunc draw;
    const char* slug;
};
constexpr SeasonPresentation kSeasons[] = {
    { Randomizer_DrawSeasonSpring, "season_spring" },
    { Randomizer_DrawSeasonSummer, "season_summer" },
    { Randomizer_DrawSeasonAutumn, "season_autumn" },
    { Randomizer_DrawSeasonWinter, "season_winter" },
};

int SeasonForDraw(CustomDrawFunc draw) {
    for (size_t i = 0; i < std::size(kSeasons); ++i)
        if (kSeasons[i].draw == draw)
            return static_cast<int>(i) + 1;
    return 0;
}

float Spin(PlayState* play) {
    // Match DrawCustomItemDiamond's signed 16-bit rotation, including wrap.
    const uint32_t bits = (static_cast<uint32_t>(play->gameplayFrames) * 2u) & 0xFFFFu;
    const int32_t signedBits = bits >= 0x8000u ? static_cast<int32_t>(bits) - 0x10000 : bits;
    return signedBits * .01f;
}

bool HasResource(const char* path) {
    return path != nullptr &&
           (ResourceMgr_FileExists(path) || (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path)));
}

// Only resource-backed legacy GI routes participate. In particular, the
// wand's old GI is an inline stand-in: a held-only wand mod must retain the new
// GI while its held wrapper independently preserves that mod's model.
// The Lantern's old Poe actor mesh is also a stand-in. Generic Poe replacements
// must not suppress its authored cage/glass; dedicated GI overrides stay live.
bool HasLegacyGiMod(const Presentation& item, bool includeMmHost = false) {
    if (item.draw == Randomizer_DrawExtShieldOfIkana)
        return ResourceMgr_IsIkanaShieldGiAvailable() != 0;
    // This progressive callback draws a Kokiri placeholder. The real Master
    // callback and MM foreign recipe use Temple geometry instead.
    if (!includeMmHost && item.draw == Randomizer_DrawProgressiveMasterSword)
        return ResourceMgr_IsModAsset("objects/object_gi_sword_1/gGiKokiriSwordDL");
    if (!item.opaque)
        return false;
    struct Legacy {
        const char* slug;
        const char* owner;
        const char* opaque;
        const char* second = nullptr;
        const char* third = nullptr;
    };
    static constexpr Legacy models[] = {
        { "rocs_feather", "oot", "objects/object_nei_rocs_feather/rocs_feather_dl",
          "objects/object_rocs_feather/gGiRocsFeatherDL" },
        { "rocs_cape", "oot", "objects/object_nei_rocs_cape/rocs_cape_mesh_dl" },
        { "whip", "oot", "objects/object_nei_whip/whip_give_opaque_dl" },
        { "spinner", "oot", "objects/object_nei_spinner/n0b0_opaque_dl" },
        { "deku_leaf", "oot", "objects/object_nei_deku_leaf/g_dekuleaf_dl" },
        { "switch_hook", "oot", "objects/object_nei_switchhook/gSwitchHookGiveDL" },
        { "mogma_mitts", "oot", "objects/object_nei_mogma_mitts/gMogmaMittsGiveDL" },
        { "gust_jar", "oot", "objects/object_nei_gust_jar/jar_model_dl" },
        { "time_gate", "oot", "objects/object_nei_time_gate/g_timegate_dl" },
        { "minish_cap", "oot", "objects/object_nei_minish_cap/Cylinder_opaque_dl" },
        { "mario_mask", "oot", "objects/object_nei_mario_mask/g_mario_mask_dl" },
        { "cojiro", "oot", "objects/object_gi_niwatori/gGiChickenDL", "objects/object_gi_niwatori/gGiCojiroColorDL",
          "objects/object_gi_niwatori/gGiChickenEyesDL" },
        { "divine_shield", "oot", "objects/object_nei_divine_shield/g_divine_shield_dl" },
        { "sheikah_shield", "oot", "objects/object_nei_kite_shield/g_kite_shield_dl" },
        { "magic_cape", "oot", "objects/object_nei_magic_cape/gNeiMagicCapeDL",
          "objects/object_nei_magic_cape/gNeiMagicCapeWaveDL" },
        // Shared vanilla clothes packs replace the Goron/Zora tunic recipes,
        // not the three distinct authored equipment GIs. Their replacements
        // are selected at each item's own path by HasRedesignGiMod instead.
        { "pegasus_anklet", "oot", "objects/object_gi_hoverboots/gGiHoverBootsDL" },
        { "trident", "oot", "objects/object_gnd/gPhantomGanonSkelLimbsLimb_00C610DL_009298" },
        { "climb_boots", "oot", "objects/object_gi_boots_2/gGiIronBootsDL",
          "objects/object_gi_boots_2/gGiIronBootsRivetsDL" },
        { "roc_boots", "oot", "objects/object_gi_hoverboots/gGiHoverBootsDL" },
        { "cane_of_byrna", "oot", "objects/object_somaria/g_byrna_cane_give_dl" },
        { "cane_of_somaria", "oot", "objects/object_somaria/g_somaria_cane_give_dl" },
        { "four_sword", "oot", "objects/object_nei_four_sword/gNeiFourSwordBladeDL",
          "objects/object_nei_four_sword/gNeiFourSwordHiltDL" },
        { "pendant_of_memories", "mm", "objects/object_gi_reserve_c_01/gGiPendantOfMemoriesDL" },
        { "room_key", "mm", "objects/object_gi_reserve_b_00/gGiRoomKeyDL",
          "objects/object_gi_reserve_b_00/gGiRoomKeyEmptyDL" },
        { "sheikah_slate", "oot", "objects/object_nei_sheikah_slate/gNeiSheikahSlateDL" },
        { "slate_bomb", "oot", "objects/object_nei_sheikah_slate/gNeiSheikahSlateDL" },
        { "slate_master_cycle", "oot", "objects/object_nei_sheikah_slate/gNeiSheikahSlateDL" },
        { "slate_stasis", "oot", "objects/object_nei_sheikah_slate/gNeiSheikahSlateDL" },
        { "slate_cryonis", "oot", "objects/object_nei_sheikah_slate/gNeiSheikahSlateDL" },
        { "slate_sensor", "oot", "objects/object_nei_sheikah_slate/gNeiSheikahSlateDL" },
        { "rod_of_seasons", "oot", "objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL" },
        { "phantom_hourglass", "oot", "objects/object_nei_phantom_hourglass/gNeiPhantomHourglassDL" },
        { "shadow_crystal", "oot", "objects/object_nei_shadow_crystal/gNeiShadowCrystalDL" },
        { "kokiri_sword", "oot", "objects/object_gi_sword_1/gGiKokiriSwordDL" },
        { "mm_kokiri_sword", "mm", "objects/object_gi_sword_1/gGiKokiriSwordDL" },
        { "master_sword", "oot", "objects/object_toki_objects/object_toki_objects_DL_001BD0" },
        { "true_master_sword", "oot", "objects/object_toki_objects/object_toki_objects_DL_001BD0" },
        { "biggoron_sword", "oot", "objects/object_gi_longsword/gGiBiggoronSwordDL" },
        { "giants_knife", "oot", "objects/object_gi_longsword/gGiBiggoronSwordDL" },
        { "razor_sword", "mm", "objects/object_gi_sword_2/gGiRazorSwordDL",
          "objects/object_gi_sword_2/gGiRazorSwordEmptyDL" },
        { "gilded_sword", "mm", "objects/object_gi_sword_3/gGiGildedSwordDL",
          "objects/object_gi_sword_3/gGiGildedSwordEmptyDL" },
        { "great_fairy_sword", "mm", "objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL",
          "objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL" },
    };
    constexpr char prefix[] = "__OTR__objects/nei_gi_redesign/";
    if (std::strncmp(item.opaque, prefix, sizeof(prefix) - 1) != 0)
        return false;
    const char* slug = item.opaque + sizeof(prefix) - 1;
    for (const auto& model : models) {
        const size_t length = std::strlen(model.slug);
        if (std::strncmp(slug, model.slug, length) == 0 && std::strcmp(slug + length, "/gi_dl") == 0) {
            const auto selectedMod = [&](const char* path) {
                if (!path)
                    return false;
                // Imported MM files can be overridden in OoT's local archive
                // manager before TransformMasks_LoadMmDL reaches the donor.
                const bool sharedMmPath = includeMmHost && std::strcmp(model.slug, "magic_cape") != 0 &&
                                          std::strcmp(model.slug, "four_sword") != 0;
                return ResourceMgr_IsModAssetForGame("oot", path) ||
                       ((sharedMmPath || std::strcmp(model.owner, "oot") != 0) &&
                        ResourceMgr_IsModAssetForGame("mm", path));
            };
            const bool replaced = selectedMod(model.opaque) || selectedMod(model.second) || selectedMod(model.third);
            return replaced;
        }
    }
    return false;
}

const char* SelectedSwordPath(const Presentation& item, bool altAssets, bool (*available)(const char*)) {
    if (!item.opaque || !item.alwaysShimmer || !altAssets)
        return nullptr;
    const char* selected = nullptr;
    const char* fire = nullptr;
    if (std::strstr(item.opaque, "/kokiri_sword/") || std::strstr(item.opaque, "/mm_kokiri_sword/") ||
        std::strstr(item.opaque, "/razor_sword/") || std::strstr(item.opaque, "/gilded_sword/")) {
        selected = "__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL";
        fire = "__OTR__objects/din_fire_sword/progressive/child/SwordDL";
    } else if (std::strstr(item.opaque, "/master_sword/") || std::strstr(item.opaque, "/true_master_sword/")) {
        selected = "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL";
        fire = "__OTR__objects/din_fire_sword/progressive/adult/SwordDL";
    } else if (std::strstr(item.opaque, "/biggoron_sword/") || std::strstr(item.opaque, "/great_fairy_sword/") ||
               std::strstr(item.opaque, "/giants_knife/")) {
        selected = "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL";
        fire = "__OTR__objects/din_fire_sword/progressive/bgs/SwordDL";
    }
    if (!selected)
        return nullptr;
    // Match the exported recipe's protected Din priority before the selected
    // standalone weapon pack. Neither path uses the player fist/body DL.
    if (CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0) && available(fire))
        return fire;
    return available(selected) ? selected : nullptr;
}

bool HasRedesignGiMod(const Presentation& item) {
    return (item.opaque && ResourceMgr_IsModAssetForGame("oot", item.opaque)) ||
           (item.translucent && ResourceMgr_IsModAssetForGame("oot", item.translucent));
}

bool HasSelectedSword(const Presentation& item, bool altAssets, bool (*available)(const char*)) {
    return SelectedSwordPath(item, altAssets, available) != nullptr;
}

} // namespace

#include "ComboDinSwordGi.h"

static void NeiGi_DrawSelectedSword(PlayState* play, const char* path, bool shop = false, bool fit = true) {
    OPEN_DISPS(play->state.gfxCtx);
    if (fit) {
        ComboSwordGi_ApplyPresentationSize(shop);
        ComboSwordGi_ApplyFit("oot", path, .04f, ComboSwordGi_SelectedTilt(1.5707963267948966f, shop), shop);
    }
    Matrix_RotateY(Spin(play), MTXMODE_APPLY);
    // Standalone donor blades point along +X. Z alone turns that axis into
    // upright +Y; an extra X quarter turn would lay it flat in XZ.
    Matrix_RotateZ(ComboSwordGi_SelectedTilt(1.5707963267948966f, shop), MTXMODE_APPLY);
    Matrix_Scale(.04f, .04f, .04f, MTXMODE_APPLY);
    NeiGi_ReportBlackMarketModel("oot", path);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH);
    CLOSE_DISPS(play->state.gfxCtx);
    ComboDinSwordGi_DrawLayers(play, "oot", path);
}

// OPEN_DISPS declares interpolation callbacks with the enclosing C linkage.
extern "C" {
#include "NeiGiMeshRenderer.inc"

static void NeiGi_DrawSong(PlayState* play, int song) {
    uint8_t color[4];
    if (!ComboSongShimmerColor(song, color))
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 12);
    if (!arena)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    const char* colorDl = ComboSongOotColorDlist(song);
    if (colorDl) {
        gSPGrayscale(POLY_XLU_DISP++, false);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, colorDl, 0, G_DL_PUSH);
    } else {
        gDPSetGrayscaleColor(POLY_XLU_DISP++, color[0], color[1], color[2], color[3]);
        gSPGrayscale(POLY_XLU_DISP++, true);
    }
    gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, gGiSongNoteDL, 0, G_DL_PUSH);
    gSPGrayscale(POLY_XLU_DISP++, false);
    CLOSE_DISPS(play->state.gfxCtx);
    if (ComboSongHasOverlay(song))
        NeiGi_DrawSongOverlay(play, song, nullptr);
}

static void NeiGi_DrawEffects(PlayState* play, const Presentation& item, bool upgraded, bool legacy,
                              float particleScale = 1.f) {
    if (legacy && item.draw == Randomizer_DrawMarioMask)
        return; // The legacy callback already draws its mask overlay.
    if (upgraded && item.effect == Kind::SeasonCycle)
        NeiGi_DrawSeasonOverlay(play, 5, nullptr);
    const bool shimmer = CVarGetInteger(CVAR_NEI_GI_EFFECTS, 0) != 0;
    const bool energy = (upgraded || NeiGi::IsSword(item.effect)) &&
                        (NeiGi::IsRod(item.effect) || NeiGi::IsSpell(item.effect) || NeiGi::IsSpecial(item.effect));
    if (!shimmer && !energy && !item.alwaysShimmer)
        return;
    Matrix_Push();
    Matrix_RotateY(Spin(play), MTXMODE_APPLY);
    const auto camera = NeiGi_CameraBasis(play);
    if (energy) {
        Matrix_Push();
        Matrix_Translate(item.effectCenter.x, item.effectCenter.y, item.effectCenter.z, MTXMODE_APPLY);
        NeiGi_DrawMesh(play, NeiGi::SampleOrb(item.effect, camera), item.effect);
        NeiGi_DrawMesh(play, NeiGi::SampleEnergy(item.effect, play->gameplayFrames, camera));
        if (NeiGi::IsSpecial(item.effect))
            NeiGi_DrawMesh(play, NeiGi::SampleSpecial(item.effect, play->gameplayFrames, camera, particleScale));
        Matrix_Pop();
    }
    if (shimmer || item.alwaysShimmer)
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, camera, PresentationShimmer(item)));
    Matrix_Pop();
}
}

#ifdef COMBO_BUILD
static const char* NeiGi_BaseSwordPath(const char* path) {
    if (!path || std::strncmp(path, "__OTR__", 7) != 0 || !OOT_NeiEnsureGiBaseOwner())
        return nullptr;
    static std::unordered_set<std::string> paths;
    return paths.insert(std::string("__OTR__@oot-gi-base:") + (path + 7)).first->c_str();
}
#endif

static bool NeiGi_DrawImpl(PlayState* play, GetItemEntry* entry, bool shop) {
    if (play == nullptr || entry == nullptr)
        return false;
    if (const int song = SongForEntry(entry); song >= 0) {
        NeiGi_DrawSong(play, song);
        return true;
    }
    // Intrinsic season weather precedes archive/model selection and optional effects.
    if (const int season = SeasonForDraw(entry->drawFunc)) {
        NeiGi_DrawSeasonOverlay(play, season, nullptr);
        return true;
    }
    const Presentation* item = FindPresentation(entry);
    if (item == nullptr)
        return false;
#ifdef COMBO_BUILD
    if (NeiGi::IsSword(item->effect) && !ResourceMgr_IsAltAssetsEnabled()) {
        const char* opaque = NeiGi_BaseSwordPath(item->opaque);
        const char* translucent = item->translucent ? NeiGi_BaseSwordPath(item->translucent) : nullptr;
        if (opaque && OOT_NeiResourceExists(opaque) &&
            (!item->translucent || (translucent && OOT_NeiResourceExists(translucent)))) {
            const float center[] = { item->effectCenter.x, item->effectCenter.y, item->effectCenter.z };
            NeiGi_DrawPresentation(play, opaque, translucent, item->scale, static_cast<int>(item->effect), center,
                                   item->alwaysShimmer || CVarGetInteger(CVAR_NEI_GI_EFFECTS, 0), "oot-gi-base", shop);
        }
        return true; // Missing shipped data must not borrow a base-path mod.
    }
#endif
    const char* selectedSword = SelectedSwordPath(*item, ResourceMgr_IsAltAssetsEnabled(), HasResource);
    // Queue stable paths for the interpreter. Loading through the legacy GBI wrapper
    // here would evict/reload base resources on each draw when Alt Assets is enabled.
    // Archive presence checks preserve the original model if a required pass is absent.
    const bool upgraded = !selectedSword && !HasLegacyGiMod(*item) && HasResource(item->opaque) &&
                          (!item->translucent || HasResource(item->translucent));
    const bool authored = upgraded && !HasRedesignGiMod(*item);
    if (!upgraded && !entry->drawFunc && !item->alwaysShimmer)
        return false;
    // Procedural effects leave space for the later translucent model shell.
    // Intrinsic flames allocate a twelve-packet scroll and a matrix before
    // the model. Include their native setup/debug commands in the same scope.
    const bool flame = (authored && item->draw == Randomizer_DrawCaneSomariaUpgrade) ||
                       (selectedSword && item->effect == Kind::SwordAura);
    const bool crystalSheen = authored && item->translucent && NeiGi::IsSpell(item->effect);
    const size_t modelMatrices = selectedSword ? 1 : upgraded ? (item->translucent ? 2 : 1) : 0;
    NeiGi_ArenaScope arena(play, modelMatrices + (flame ? 1 : 0), flame ? 20 : 16, flame ? 32 : 16,
                           flame ? 12 * sizeof(Gfx) : 0);
    if (!arena)
        return true;
    Matrix_Push();
    MtxF callerMatrix;
    float particleScale = 1.f;
    if (!authored && NeiGi::IsSword(item->effect))
        ComboSwordGi_ApplyPresentationSize(shop);
    if (!authored && NeiGi::IsSword(item->effect))
        particleScale = ComboSwordGi_ParticleScale(item->effect, shop);
    Matrix_Get(&callerMatrix);
    if (selectedSword) {
        ComboSwordGi_ApplyFit("oot", selectedSword, .04f, ComboSwordGi_SelectedTilt(1.5707963267948966f, shop), shop);
    } else if (upgraded && !authored && NeiGi::IsSword(item->effect)) {
        const char* paths[] = { item->opaque, item->translucent };
        ComboSwordGi_ApplyModelsFit("oot", paths, item->translucent ? 2 : 1, item->scale, 0.f, shop);
    } else if (!upgraded && NeiGi::IsSword(item->effect)) {
        const bool fourSwordFallback = item->draw == Randomizer_DrawExtFourSword &&
                                       !HasResource("__OTR__objects/object_nei_four_sword/gNeiFourSwordBladeDL");
        const auto legacyKind =
            item->draw == Randomizer_DrawProgressiveMasterSword || fourSwordFallback ? Kind::KokiriSword : item->effect;
        const float legacyScale = fourSwordFallback                                     ? .55f
                                  : item->draw == Randomizer_DrawProgressiveMasterSword ? .6f
                                  : item->draw == Randomizer_DrawProgressiveKokiriSword ? .55f
                                  : item->draw == Randomizer_DrawProgressiveBGS         ? .5f
                                                                                        : 0.f;
        ComboSwordGi_ApplyLegacyFit("oot", legacyKind, shop, 0, legacyScale);
    } else if (authored) {
        if (const auto* bounds = NeiGi::FindFrameBounds(item->opaque)) {
            const auto fit = NeiGi::FrameFit(*bounds, item->scale, shop);
            particleScale = 1.f / fit.scale;
            Matrix_Translate(0, fit.lift, 0, MTXMODE_APPLY);
            Matrix_Scale(fit.scale, fit.scale, fit.scale, MTXMODE_APPLY);
        }
    }
    if (authored && item->draw == Randomizer_DrawExtSagesTunic)
        NeiGi_DrawSagesTunicMedallions(play);
    if (authored && item->draw == Randomizer_DrawCaneSomariaUpgrade) {
        // Retain the original red skill-upgrade flame with the authored cane.
        Randomizer_DrawCaneSomariaUpgradeFlame(play);
    }
    Matrix_Push();
    if (selectedSword) {
        if (item->effect == Kind::SwordAura) {
            Matrix_Push();
            Matrix_Put(&callerMatrix);
            ComboSwordGi_ApplyEffectFit(item->effect, shop);
            Randomizer_DrawTrueMasterSwordFlame(play);
            Matrix_Pop();
        }
        NeiGi_DrawSelectedSword(play, selectedSword, shop, false);
    } else if (upgraded) {
        OPEN_DISPS(play->state.gfxCtx);
        Matrix_Scale(item->scale, item->scale, item->scale, MTXMODE_APPLY);
        Matrix_RotateY(Spin(play), MTXMODE_APPLY);
        if (authored) {
            if (const auto* bounds = NeiGi::FindFrameBounds(item->opaque))
                NeiGi_ReportBlackMarketBounds(bounds->minimum, bounds->maximum);
        } else {
            NeiGi_ReportBlackMarketModel("oot", item->opaque, item->translucent);
        }
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        gSPGrayscale(POLY_OPA_DISP++, false);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, item->opaque, 0, G_DL_PUSH);
        CLOSE_DISPS(play->state.gfxCtx);
    } else if (entry->drawFunc == Randomizer_DrawExtFourSword) {
        Randomizer_DrawExtFourSwordPresentation(play, entry, shop);
    } else if (entry->drawFunc) {
        entry->drawFunc(play, entry);
    } else {
        GetItem_Draw(play, entry->gid);
    }
    Matrix_Pop();
    Matrix_Push();
    if (!authored && NeiGi::IsSword(item->effect)) {
        Matrix_Put(&callerMatrix);
        ComboSwordGi_ApplyEffectFit(item->effect, shop);
    }
    NeiGi_DrawEffects(play, *item, authored, !upgraded && !selectedSword, particleScale);
    Matrix_Pop();
    if (upgraded && item->translucent != nullptr) {
        // Composite the crystal skin over its contained energy, using the same pose.
        OPEN_DISPS(play->state.gfxCtx);
        Matrix_Push();
        Matrix_Scale(item->scale, item->scale, item->scale, MTXMODE_APPLY);
        Matrix_RotateY(Spin(play), MTXMODE_APPLY);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPGrayscale(POLY_XLU_DISP++, false);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, item->translucent, 0, G_DL_PUSH);
        Matrix_Pop();
        CLOSE_DISPS(play->state.gfxCtx);
    }
    if (crystalSheen) {
        // Optional polish must not displace the intact core/shell on a short
        // arena. Reserve its restore before admitting any highlight vertices.
        NeiGi_ArenaScope sheenArena(play, 1, 2, 20);
        if (sheenArena) {
            Matrix_Push();
            Matrix_RotateY(Spin(play), MTXMODE_APPLY);
            const auto sheen = NeiGi::SampleCrystalSheen(play->gameplayFrames, NeiGi_CameraBasis(play));
            if (NeiGi_DrawMeshMaterial(play, sheen, Kind::Neutral, nullptr))
                NeiGi_RestoreElemental(play);
            Matrix_Pop();
        }
    }
    Matrix_Pop();
    return true;
}

extern "C" bool NeiGi_Draw(PlayState* play, GetItemEntry* entry) {
    return NeiGi_DrawImpl(play, entry, false);
}

#ifdef COMBO_BUILD
extern "C" bool OOT_DrawComboForeignShop(PlayState* play, GetItemEntry* entry);
#endif

extern "C" bool NeiGi_DrawShop(PlayState* play, GetItemEntry* entry) {
    if (entry && !entry->drawFunc) {
        const int arrow = NeiArrowGi_ProfileForDrawId(entry->gid, GID_ARROW_FIRE, GID_ARROW_ICE, GID_ARROW_LIGHT);
        if (arrow && NeiGi_DrawElementalArrowShop(play, arrow))
            return true;
        const int profile = NeiArrowGi_ProfileForDrawId(entry->gid, GID_DINS_FIRE, GID_FARORES_WIND, GID_NAYRUS_LOVE);
        if (profile && NeiGi_DrawElementalSpell(play, profile, 0, 1))
            return true;
    }
    if (NeiGi_DrawImpl(play, entry, true))
        return true;
#ifdef COMBO_BUILD
    return OOT_DrawComboForeignShop(play, entry);
#else
    return false;
#endif
}

#ifdef COMBO_BUILD
#include "ComboItemDrawABI.h"
#include "ComboExport.h"

static bool NeiGi_FillSeasonInfo(int season, CwItemDrawInfo* out) {
    if (!out)
        return false;
    out->drawKind = CW_DRAW_KIND_SEASON_GI;
    out->neiEffect = season;
    out->dlistCount = 0;
    out->xluStartIndex = -1;
    return true;
}

static bool NeiGi_FillCrossGameInfo(const Presentation& item, CwItemDrawInfo* out, bool swordAltAssets = true) {
    if (!out)
        return false;
    // Identity belongs to the award, independently of the mesh selected by
    // either archive manager. A declined replacement still supplies shimmer
    // metadata, while leaving authored mesh-local energy unset.
    out->neiShimmer = static_cast<int32_t>(PresentationShimmer(item)) + 1;
    out->itemShimmer = item.alwaysShimmer || CVarGetInteger(CVAR_NEI_GI_EFFECTS, 0);
    out->stateDependent = 2;
    const bool vanillaSword = NeiGi::IsSword(item.effect) && !swordAltAssets;
    if (!item.opaque ||
        (!vanillaSword && (HasLegacyGiMod(item, true) ||
                           HasSelectedSword(item, OOT_NeiAltAssetsEnabled(),
                                            [](const char* path) { return OOT_NeiResourceExists(path) != 0; }))) ||
        !OOT_NeiResourceExists(item.opaque) || (item.translucent && !OOT_NeiResourceExists(item.translucent))) {
        return false;
    }
    out->dlists[0] = vanillaSword ? NeiGi_BaseSwordPath(item.opaque) : item.opaque;
    if (!out->dlists[0] || (vanillaSword && !OOT_NeiResourceExists(out->dlists[0])))
        return false;
    out->dlistCount = 1;
    out->xluStartIndex = -1;
    if (item.translucent) {
        out->dlists[1] = vanillaSword ? NeiGi_BaseSwordPath(item.translucent) : item.translucent;
        if (!out->dlists[1] || (vanillaSword && !OOT_NeiResourceExists(out->dlists[1])))
            return false;
        out->dlistCount = 2;
        out->xluStartIndex = 1;
    }
    out->scale = item.scale;
    // An override at either redesigned pass owns its geometry. Preserve its
    // deferred selection and identity overlay without authored bounds/FX.
    if (!vanillaSword && HasRedesignGiMod(item)) {
        out->drawKind = CW_DRAW_KIND_CUSTOM_GI;
        return true;
    }
    out->drawKind = CW_DRAW_KIND_NEI_GI;
    out->neiEffect = static_cast<int32_t>(item.effect);
    out->neiEffectCenter[0] = item.effectCenter.x;
    out->neiEffectCenter[1] = item.effectCenter.y;
    out->neiEffectCenter[2] = item.effectCenter.z;
    out->neiSomariaUpgrade = item.draw == Randomizer_DrawCaneSomariaUpgrade;
    return true;
}

extern "C" int32_t NeiGi_DescribeEntry(const GetItemEntry* entry, CwItemDrawInfo* out) {
    if (const int song = SongForEntry(entry); song >= 0) {
        if (!out)
            return 0;
        out->drawKind = CW_DRAW_KIND_SONG_GI;
        out->neiEffect = song;
        const char* colorDl = ComboSongOotColorDlist(song);
        out->dlistCount = 0;
        if (colorDl)
            out->dlists[out->dlistCount++] = colorDl;
        out->dlists[out->dlistCount++] = gGiSongNoteDL;
        out->xluStartIndex = 0;
        out->itemShimmer = ComboSongHasOverlay(song);
        ComboSongShimmerColor(song, out->itemShimmerColor);
        std::memcpy(out->primColorXlu, out->itemShimmerColor, 4);
        return 1;
    }
    if (entry)
        if (const int season = SeasonForDraw(entry->drawFunc))
            return NeiGi_FillSeasonInfo(season, out);
    const auto* item = FindPresentation(entry);
    return item ? NeiGi_FillCrossGameInfo(*item, out) : 0;
}

// Concrete presentation lookup for MM's native NEI items: never resolve through OoT's save.
extern "C" COMBO_EXPORT int32_t OOT_GetNeiGiDrawInfoForAssets(const char* slug, int32_t altAssets,
                                                              CwItemDrawInfo* out) {
    if (!slug || !out)
        return 0;
    if (std::strcmp(slug, "mm_kokiri_sword") == 0) {
        *out = CwItemDrawInfo{};
        return NeiGi_FillCrossGameInfo(kMmKokiriPresentation, out, altAssets != 0);
    }
    for (size_t i = 0; i < std::size(kSeasons); ++i) {
        if (std::strcmp(slug, kSeasons[i].slug) == 0) {
            *out = CwItemDrawInfo{};
            return NeiGi_FillSeasonInfo(static_cast<int>(i) + 1, out);
        }
    }
    constexpr char prefix[] = "__OTR__objects/nei_gi_redesign/";
    for (const auto& item : kPresentations) {
        if (!item.opaque)
            continue;
        const char* name = item.opaque + sizeof(prefix) - 1;
        const size_t length = std::strlen(slug);
        if (std::strncmp(name, slug, length) == 0 && std::strcmp(name + length, "/gi_dl") == 0) {
            *out = CwItemDrawInfo{};
            return NeiGi_FillCrossGameInfo(item, out, altAssets != 0);
        }
    }
    return 0;
}

extern "C" COMBO_EXPORT int32_t OOT_GetNeiGiDrawInfo(const char* slug, CwItemDrawInfo* out) {
    return OOT_GetNeiGiDrawInfoForAssets(slug, 1, out);
}
#endif
