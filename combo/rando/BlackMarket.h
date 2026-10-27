#pragma once

// Supplemental stock is deliberately separate from randomizer checks and fill RNG.
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <numeric>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace ComboRando::BlackMarket {

enum class Games { Both, Oot, Mm };

struct Item {
    const char* key;
    const char* identity;
    const char* label;
    uint8_t tier;
    uint16_t price;
    bool wildcard = false;
    bool renewable = false;
};

inline std::span<const Item> Catalog() {
    static constexpr Item items[] = {
        { "nei.shovel", "RI_OOT_NEI_SHOVEL", "Shovel", 1, 50 },
        { "nei.lantern", "RI_OOT_NEI_LANTERN", "Lantern", 1, 60 },
        { "nei.whip", "RI_OOT_NEI_WHIP", "Whip", 1, 80 },
        { "nei.beetle", "RI_OOT_NEI_BEETLE", "Beetle", 1, 80 },
        { "nei.season.spring", "RI_OOT_NEI_SEASON_SPRING", "Spring", 1, 60 },
        { "nei.season.summer", "RI_OOT_NEI_SEASON_SUMMER", "Summer", 1, 60 },
        { "nei.season.autumn", "RI_OOT_NEI_SEASON_AUTUMN", "Autumn", 1, 60 },
        { "nei.season.winter", "RI_OOT_NEI_SEASON_WINTER", "Winter", 2, 150 },
        { "nei.gust_jar", "RI_OOT_NEI_GUST_JAR", "Gust Jar", 2, 110 },
        { "nei.ball_chain", "RI_OOT_NEI_BALL_AND_CHAIN", "Ball and Chain", 2, 120 },
        { "nei.fire_rod", "RI_OOT_NEI_FIRE_ROD", "Fire Rod", 2, 120 },
        { "nei.ice_rod", "RI_OOT_NEI_ICE_ROD", "Ice Rod", 2, 120 },
        { "nei.light_rod", "RI_OOT_NEI_LIGHT_ROD", "Light Rod", 2, 150 },
        { "nei.bomb_arrows", "RI_OOT_NEI_BOMB_ARROWS", "Bomb Arrows", 2, 120 },
        { "nei.spinner", "RI_OOT_NEI_SPINNER", "Spinner", 2, 120 },
        { "nei.goddess_shield", "RI_OOT_EXT_DIVINE_SHIELD", "Goddess Shield", 2, 130 },
        { "nei.kite_shield", "RI_OOT_EXT_SHEIKAH_SHIELD", "Kite Shield", 2, 150 },
        { "nei.pegasus_boots", "RI_OOT_EXT_PEGASUS_ANKLET", "Pegasus Boots", 2, 140 },
        { "nei.climb_boots", "RI_OOT_EXT_CLIMB_BOOTS", "Climb Boots", 2, 100 },
        { "nei.sages_tunic", "RI_OOT_EXT_WATER_DRAGON_SCALE", "Sage's Tunic", 2, 120 },
        { "nei.magic_tunic", "RI_OOT_EXT_SPIRIT_BREASTPLATE", "Magic Tunic", 2, 150 },
        { "nei.champions_tunic", "RI_OOT_EXT_CHAMPIONS_TUNIC", "Champion's Tunic", 2, 150 },
        { "nei.magic_cape", "RI_OOT_EXT_MAGIC_CAPE", "Magic Cape", 3, 200 },
        { "nei.four_sword", "RI_OOT_EXT_FOUR_SWORD", "Four Sword", 3, 200 },
        { "nei.trident", "RI_OOT_EXT_TRIDENT", "Trident", 3, 200 },
        { "nei.cane.byrna", "RI_OOT_EXT_CANE_OF_BYRNA", "Cane of Byrna", 3, 180 },
        { "nei.mogma_mitts", "RI_OOT_NEI_MOGMA_MITTS", "Mogma Mitts", 2, 140 },
        { "nei.deku_leaf", "RI_OOT_NEI_DEKU_LEAF", "Deku Leaf", 2, 120 },
        { "nei.rocs_feather", "RI_OOT_NEI_ROCS_FEATHER", "Roc's Feather", 2, 120 },
        { "nei.rocs_cape", "RI_OOT_NEI_ROCS_CAPE", "Roc's Cape", 2, 150 },
        { "nei.rocs_boots", "RI_OOT_EXT_ROC_BOOTS", "Roc's Boots", 2, 150 },
        { "nei.switch_hook", "RI_OOT_NEI_SWITCH_HOOK", "Switch Hook", 2, 150 },
        { "nei.minish_cap", "RI_OOT_NEI_MINISH_CAP", "Minish Cap", 2, 130 },
        { "nei.dominion_rod", "RI_OOT_NEI_DOMINION_ROD", "Dominion Rod", 2, 140 },
        { "nei.hylias_grace", "RI_OOT_NEI_HYLIAS_GRACE", "Hylia's Grace", 3, 200 },
        { "nei.phantom_hourglass", "RI_OOT_NEI_PHANTOM_HOURGLASS", "Phantom Hourglass", 2, 150 },
        { "nei.time_gate", "RI_OOT_NEI_TIME_GATE", "Time Gate", 2, 150 },
        { "nei.zonai_timer", "RI_OOT_NEI_ZONAI_PERMAFROST", "Zonai Timer", 3, 200 },
        { "nei.shadow_crystal", "RI_OOT_NEI_SHADOW_CRYSTAL", "Shadow Crystal", 2, 150 },
        { "nei.demise", "RI_OOT_NEI_DEMISE_DESTRUCTION", "Demise Destruction", 2, 150 },
        { "nei.rune.remote_bomb", "RI_OOT_NEI_SLATE_RUNE_BOMB", "Rune: Remote Bomb", 2, 120 },
        { "nei.rune.master_cycle", "RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE", "Rune: Master Cycle", 2, 150 },
        { "nei.rune.stasis", "RI_OOT_NEI_SLATE_RUNE_STASIS", "Rune: Stasis", 2, 150 },
        { "nei.rune.cryonis", "RI_OOT_NEI_SLATE_RUNE_CRYONIS", "Rune: Cryonis", 2, 140 },
        { "nei.rune.sensor", "RI_OOT_NEI_DESIRE_SENSOR", "Rune: Sheikah Sensor", 2, 100 },
        { "nei.cane.statues", "RI_OOT_NEI_CANE_OF_SOMARIA", "Somaria: Statues", 2, 120 },
        { "nei.cane.blocks", "RI_OOT_NEI_CANE_SOMARIA_BLOCK", "Somaria: Blocks", 2, 130 },
        { "nei.cane.trirod", "RI_OOT_NEI_CANE_SOMARIA_PLATFORM", "Somaria: Trirod", 2, 150 },
        { "nei.cane.flip", "RI_OOT_NEI_CANE_PACCI_FLIP", "Pacci: Flip", 2, 100 },
        { "nei.cane.lift", "RI_OOT_NEI_CANE_PACCI_STONE", "Pacci: Lift", 2, 120 },
        { "nei.cane.ultrahand", "RI_OOT_NEI_CANE_PACCI_ULTRAHAND", "Pacci: Ultrahand", 2, 150 },
        { "nei.wand.sand", "RI_OOT_NEI_WAND_SAND_ROD", "Sand Rod", 2, 130 },
        { "nei.wand.tornado", "RI_OOT_NEI_WAND_TORNADO_ROD", "Tornado Rod", 2, 140 },
        { "nei.wand.water", "RI_OOT_NEI_WAND_WATER_ROD", "Water Rod", 2, 130 },
        { "nei.wand.meteor", "RI_OOT_NEI_WAND_METEOR_ROD", "Meteor Rod", 2, 150 },
        { "nei.wand.storm", "RI_OOT_NEI_WAND_STORM_ROD", "Storm Rod", 2, 150 },
        { "nei.wand.shadow", "RI_OOT_NEI_WAND_SHADOW_SCEPTER", "Shadow Scepter", 2, 120 },
        { "nei.bundle.slate", "RI_OOT_NEI_SHEIKAH_SLATE", "Slate: All Runes", 3, 200 },
        { "nei.bundle.seasons", "RI_OOT_NEI_ROD_OF_SEASONS", "Rod: All Seasons", 3, 180 },
        { "nei.bundle.wand", "RI_OOT_NEI_ELEMENTAL_WAND", "Wand: All Rods", 3, 200 },
        { "wildcard.deku_mask", "RI_MASK_DEKU", "Deku Mask", 2, 130, true },
        { "wildcard.goron_mask", "RI_MASK_GORON", "Goron Mask", 2, 150, true },
        { "wildcard.zora_mask", "RI_MASK_ZORA", "Zora Mask", 2, 150, true },
        { "wildcard.stone_mask", "RI_MASK_STONE", "Stone Mask", 2, 120, true },
        { "wildcard.great_fairy_mask", "RI_MASK_GREAT_FAIRY", "Great Fairy's Mask", 1, 60, true },
        { "wildcard.mask_truth", "RI_MASK_TRUTH", "Mask of Truth", 2, 110, true },
        { "wildcard.blast_mask", "RI_MASK_BLAST", "Blast Mask", 2, 110, true },
        { "wildcard.gibdo_mask", "RI_MASK_GIBDO", "Gibdo Mask", 2, 110, true },
        { "wildcard.garo_mask", "RI_MASK_GARO", "Garo's Mask", 2, 110, true },
        { "wildcard.captains_hat", "RI_MASK_CAPTAIN", "Captain's Hat", 2, 110, true },
        { "wildcard.bunny_hood", "RI_MASK_BUNNY", "Bunny Hood", 2, 120, true },
        { "wildcard.great_spin", "RI_GREAT_SPIN_ATTACK", "Great Spin Attack", 1, 80, true },
        { "wildcard.chateau", "RI_CHATEAU_ROMANI_REFILL", "Chateau Romani Refill", 3, 180, true, true },
        { "wildcard.magic_upgrade", "RI_DOUBLE_MAGIC", "Magic Meter Upgrade", 2, 150, true },
        { "wildcard.double_defense", "RI_DOUBLE_DEFENSE", "Double Defense", 3, 180, true },
        { "wildcard.great_fairy_sword", "RI_GREAT_FAIRY_SWORD", "Great Fairy's Sword", 3, 200, true },
        { "wildcard.fierce_deity", "RI_MASK_FIERCE_DEITY", "Fierce Deity Mask", 3, 200, true },
        { "wildcard.mario_mask", "RI_OOT_NEI_MARIO_MASK", "Mario Mask", 3, 160, true },
        { "wildcard.poke_ball", "RI_OOT_NEI_POKE_BALL", "Poke Ball", 3, 160, true },
    };
    return items;
}

inline const Item* Find(std::string_view key) {
    for (const auto& item : Catalog())
        if (key == item.key)
            return &item;
    return nullptr;
}

inline bool Eligible(const Item& item, Games games) {
    if (games != Games::Oot || !item.wildcard)
        return true; // Every NEI offer has native implementations in both games.
    const std::string_view key(item.key);
    // OoT-only never requires MM's archive for masks, sword models or bottles.
    return key == "wildcard.mario_mask" || key == "wildcard.poke_ball" || key == "wildcard.magic_upgrade" ||
           key == "wildcard.double_defense";
}

inline bool MatchesGames(const nlohmann::json& market, Games games) {
    const auto value = market.is_object() ? market.value("games", nlohmann::json("both")) : nlohmann::json();
    return value == (games == Games::Oot ? "oot" : games == Games::Mm ? "mm" : "both");
}

enum class Pricing { Tiered, FollowSoH };
enum class PriceMode { Vanilla, CheapBalanced, Balanced, Fixed, Range, Wallet };
struct SoHPrices {
    PriceMode mode = PriceMode::Wallet;
    uint16_t fixedPrice = 100;
    uint16_t rangeMin = 0, rangeMax = 200;
    std::array<uint16_t, 5> walletWeights{ 0, 1, 1, 1, 0 };
    bool tycoon = false;
    bool affordable = false;
};
struct Options {
    Games games = Games::Both;
    Pricing pricing = Pricing::Tiered;
    bool dungeonUnlocks = true;
    SoHPrices soh;
    // Empty means the complete catalog. Explicit filtering is seed-bound.
    std::set<std::string> eligible;
};

class Rng {
  public:
    explicit Rng(uint64_t seed) : state(seed) {
    }
    uint64_t Next() {
        uint64_t value = (state += UINT64_C(0x9E3779B97F4A7C15));
        value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
        return value ^ (value >> 31);
    }
    uint64_t Below(uint64_t limit) {
        if (!limit)
            throw std::invalid_argument("empty black-market random range");
        const uint64_t threshold = -limit % limit;
        uint64_t value;
        do {
            value = Next();
        } while (value < threshold);
        return value % limit;
    }
    uint16_t Range(uint16_t low, uint16_t high) {
        return static_cast<uint16_t>(low + Below(static_cast<uint64_t>(high) - low + 1));
    }

  private:
    uint64_t state;
};

inline uint16_t SoHPrice(Rng& rng, const SoHPrices& options, uint16_t vanillaFallback) {
    uint16_t price = vanillaFallback;
    switch (options.mode) {
        case PriceMode::Vanilla:
            break; // NEI has no vanilla retail price; use its explicit catalog price.
        case PriceMode::Fixed:
            price = std::min<uint16_t>(options.fixedPrice, 999);
            break;
        case PriceMode::Range:
            price = rng.Range(std::min<uint16_t>({ options.rangeMin, options.rangeMax, 999 }),
                              std::min<uint16_t>(std::max(options.rangeMin, options.rangeMax), 999));
            break;
        case PriceMode::Wallet: {
            auto weights = options.walletWeights;
            if (!options.tycoon)
                weights[4] = 0;
            const uint32_t total = std::accumulate(weights.begin(), weights.end(), uint32_t{});
            if (!total) {
                price = rng.Range(0, 500);
                break;
            }
            uint32_t roll = static_cast<uint32_t>(rng.Below(total));
            static constexpr uint16_t low[] = { 0, 1, 100, 201, 501 };
            static constexpr uint16_t high[] = { 0, 99, 200, 500, 998 };
            for (size_t i = 0; i < weights.size(); ++i) {
                if (roll < weights[i]) {
                    price = i == 0 ? 0 : rng.Range(low[i], high[i]);
                    break;
                }
                roll -= weights[i];
            }
            break;
        }
        case PriceMode::CheapBalanced: {
            static constexpr double cdf[] = { .097500187, .190002748, .277509301, .360018376, .437522571,
                                              .510021715, .577520272, .640029304, .697527584, .750024535,
                                              .797518749, .840011707, .877508776, .910010904, .937504342,
                                              .960004661, .977502132, .989998967, .997500116, 1.0 };
            const double roll = (rng.Next() >> 11) * (1.0 / 9007199254740992.0);
            for (size_t i = 0; i < std::size(cdf); ++i)
                if (roll < cdf[i]) {
                    price = static_cast<uint16_t>(i * 5);
                    break;
                }
            break;
        }
        case PriceMode::Balanced: {
            static constexpr double cdf[] = { .005326994, .014908518, .027114719, .041315285, .057136304, .074325887,
                                              .092667151, .112002061, .132198214, .153125390, .174696150, .196810540,
                                              .219388148, .242361379, .265657012, .289205134, .312970402, .336877590,
                                              .360881110, .384932772, .408976198, .432982176, .456902494, .480686053,
                                              .504313389, .527746488, .550938554, .573856910, .596465330, .618736235,
                                              .640646600, .662162782, .683240432, .703859801, .724001242, .743631336,
                                              .762722631, .781259986, .799198449, .816521905, .833208595, .849243398,
                                              .864579161, .879211177, .893112051, .906263928, .918639420, .930222611,
                                              .940985829, .950914731, .959992180, .968187000, .975495390, .981884488,
                                              .987344345, .991851853, .995389113, .997937921, .999481947, 1.0 };
            const double roll = (rng.Next() >> 11) * (1.0 / 9007199254740992.0);
            for (size_t i = 0; i < std::size(cdf); ++i)
                if (roll < cdf[i]) {
                    price = static_cast<uint16_t>(i * 5);
                    break;
                }
            break;
        }
    }
    if (options.affordable && options.mode != PriceMode::Fixed)
        return price > 500 ? 505 : price > 200 ? 205 : price > 99 ? 100 : price ? 5 : 0;
    return price;
}

inline nlohmann::json Generate(uint32_t seed, const Options& options = {}) {
    Rng stockRng(static_cast<uint64_t>(seed) ^ UINT64_C(0x534B49454A455200));
    Rng priceRng(static_cast<uint64_t>(seed) ^ UINT64_C(0x534B494550524943));
    std::vector<const Item*> candidates;
    for (const auto& item : Catalog())
        if (Eligible(item, options.games) && (options.eligible.empty() || options.eligible.contains(item.key)))
            candidates.push_back(&item);
    for (size_t i = candidates.size(); i > 1; --i)
        std::swap(candidates[i - 1], candidates[stockRng.Below(i)]);
    std::array<int, 3> remaining{ 2, 4, 2 };
    std::vector<const Item*> selected;
    // Resolve the two wildcard slots first. Every accepted wildcard has a chance;
    // the six NEI slots then fill the remaining tier quotas.
    for (bool wildcard : { true, false }) {
        const size_t needed = wildcard ? 2 : 8;
        for (const auto* item : candidates) {
            if (selected.size() == needed)
                break;
            if (item->wildcard != wildcard || remaining[item->tier - 1] == 0)
                continue;
            selected.push_back(item);
            --remaining[item->tier - 1];
        }
        if (selected.size() != needed)
            throw std::runtime_error("black-market catalog cannot satisfy eight-slot quotas");
    }
    // Shuffle display positions separately from eligibility/tier selection.
    for (size_t i = selected.size(); i > 1; --i)
        std::swap(selected[i - 1], selected[stockRng.Below(i)]);
    nlohmann::json stock = nlohmann::json::array();
    for (const auto* item : selected) {
        const uint16_t price =
            options.pricing == Pricing::Tiered ? item->price : SoHPrice(priceRng, options.soh, item->price);
        stock.push_back({ { "key", item->key },
                          { "identity", item->identity },
                          { "tier", item->tier },
                          { "price", price },
                          { "clears", options.dungeonUnlocks ? item->tier - 1 : 0 } });
    }
    nlohmann::json result = { { "version", 1 },
                              { "catalogVersion", 1 },
                              { "seed", seed },
                              { "pricing", options.pricing == Pricing::Tiered ? "tiered" : "soh" },
                              { "dungeonUnlocks", options.dungeonUnlocks },
                              { "stock", std::move(stock) } };
    if (options.games != Games::Both)
        result["games"] = options.games == Games::Oot ? "oot" : "mm";
    return result;
}

inline bool UnsignedInRange(const nlohmann::json& value, uint64_t maximum) {
    return value.is_number_unsigned() ? value.get<uint64_t>() <= maximum
                                      : value.is_number_integer() && value.get<int64_t>() >= 0 &&
                                            static_cast<uint64_t>(value.get<int64_t>()) <= maximum;
}

inline bool Validate(const nlohmann::json& market) {
    try {
        if (!market.is_object() || market.at("version") != 1 || market.at("catalogVersion") != 1 ||
            !UnsignedInRange(market.at("seed"), UINT32_MAX) || !market.at("dungeonUnlocks").is_boolean() ||
            !market.at("stock").is_array() || market.at("stock").size() != 8)
            return false;
        const auto pricing = market.at("pricing").get<std::string>();
        if (pricing != "tiered" && pricing != "soh")
            return false;
        const auto games = market.value("games", nlohmann::json("both"));
        if (games != "both" && games != "oot" && games != "mm")
            return false;
        std::set<std::string> keys;
        int wildcards = 0;
        std::array<int, 3> tiers{};
        for (const auto& offer : market.at("stock")) {
            const auto key = offer.at("key").get<std::string>();
            const Item* item = Find(key);
            if (!item || (games == "oot" && !Eligible(*item, Games::Oot)) || !keys.insert(key).second ||
                offer.at("identity") != item->identity || offer.at("tier") != item->tier ||
                !UnsignedInRange(offer.at("price"), 999) || (pricing == "tiered" && offer.at("price") != item->price) ||
                offer.at("clears") != (market.at("dungeonUnlocks").get<bool>() ? item->tier - 1 : 0))
                return false;
            wildcards += item->wildcard;
            ++tiers[item->tier - 1];
        }
        return wildcards == 2 && tiers == std::array<int, 3>{ 2, 4, 2 };
    } catch (const nlohmann::json::exception&) { return false; }
}

struct State {
    uint8_t purchased = 0;
};
inline nlohmann::json StateToJson(const State& state) {
    return { { "purchased", state.purchased } };
}
inline State StateFromJson(const nlohmann::json& json) {
    if (!json.is_object() || !json.contains("purchased") || !UnsignedInRange(json["purchased"], UINT8_MAX))
        return {};
    return { json["purchased"].get<uint8_t>() };
}

// Purchase bits belong to one slot AND its complete frozen offer list. A seed
// with the same number but different settings cannot inherit another market.
class Session {
  public:
    nlohmann::json market;
    State state;
    bool Bind(const nlohmann::json& value, int file) {
        if (file < 0 || file > 2 || !Validate(value)) {
            market = nullptr;
            state = {};
            slot = -1;
            return false;
        }
        if (slot != file || market != value)
            state = {};
        slot = file;
        market = value;
        return true;
    }
    bool Active(int file) const {
        return slot == file && slot >= 0 && !market.is_null();
    }
    nlohmann::json Save(int file) const {
        return Active(file) ? nlohmann::json{ { "market", market }, { "state", StateToJson(state) } }
                            : nlohmann::json();
    }
    void Load(const nlohmann::json& saved, int file) {
        state = {};
        if (Active(file) && saved.is_object() && saved.value("market", nlohmann::json()) == market)
            state = StateFromJson(saved.value("state", nlohmann::json()));
    }

  private:
    int slot = -1;
};

inline Options OptionsFromJson(const nlohmann::json& json) {
    Options options;
    if (!json.is_object())
        return options;
    const auto games = json.value("games", nlohmann::json("both"));
    options.games = games == "oot" ? Games::Oot : games == "mm" ? Games::Mm : Games::Both;
    options.pricing = json.value("pricing", 0) == 1 ? Pricing::FollowSoH : Pricing::Tiered;
    options.dungeonUnlocks = json.value("dungeonUnlocks", true);
    options.soh.mode = static_cast<PriceMode>(std::clamp(json.value("sohMode", 0), 0, 5));
    options.soh.fixedPrice = std::clamp(json.value("fixedPrice", 50), 0, 999);
    options.soh.rangeMin = std::clamp(json.value("rangeMin", 50), 0, 999);
    options.soh.rangeMax = std::clamp(json.value("rangeMax", 500), 0, 999);
    options.soh.tycoon = json.value("tycoon", false);
    options.soh.affordable = json.value("affordable", false);
    if (json.contains("walletWeights") && json["walletWeights"].is_array() && json["walletWeights"].size() == 5)
        for (size_t i = 0; i < 5; ++i)
            options.soh.walletWeights[i] = std::clamp(json["walletWeights"][i].get<int>(), 0, 100);
    return options;
}

enum class PurchaseResult { Ready, SoldOut, Locked, NeedRupees, Owned, Unavailable };
inline PurchaseResult CanPurchase(const nlohmann::json& market, const State& state, int slot, int rupees, int clears,
                                  bool owned, bool usable) {
    if (slot < 0 || slot >= 8 || !Validate(market))
        return PurchaseResult::Unavailable;
    const auto& offer = market["stock"][slot];
    const Item* item = Find(offer["key"].get<std::string>());
    if (!item->renewable && (state.purchased & (1u << slot)))
        return PurchaseResult::SoldOut;
    if (owned && !item->renewable)
        return PurchaseResult::Owned;
    if (clears < offer["clears"].get<int>())
        return PurchaseResult::Locked;
    if (!usable)
        return PurchaseResult::Unavailable;
    if (rupees < offer["price"].get<int>())
        return PurchaseResult::NeedRupees;
    return PurchaseResult::Ready;
}
inline void CommitPurchase(State& state, int slot) {
    if (slot >= 0 && slot < 8)
        state.purchased |= static_cast<uint8_t>(1u << slot);
}
inline int ClearCount(uint16_t bosses) {
    int count = 0;
    for (bosses &= 0xFFF; bosses; bosses &= static_cast<uint16_t>(bosses - 1))
        ++count;
    return count;
}

} // namespace ComboRando::BlackMarket
