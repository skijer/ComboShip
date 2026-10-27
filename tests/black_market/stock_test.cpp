#include "rando/BlackMarket.h"
#include <cassert>
#include <iostream>
#include <set>

using namespace ComboRando::BlackMarket;

int main() {
    {
        using namespace ComboRando::BlackMarket;
        Session session;
        const auto seed = Generate(77);
        assert(!session.Bind(nlohmann::json(), 0));
        assert(session.Bind(seed, 0));
        CommitPurchase(session.state, 3);
        const auto saved = session.Save(0);
        assert(session.Save(1).is_null());
        assert(session.Bind(seed, 0));
        assert(session.state.purchased == 8);
        session.Load(nlohmann::json(), 0);
        assert(session.state.purchased == 0);
        session.Load(saved, 0);
        assert(session.state.purchased == 8);
        session.Load(saved, 1);
        assert(session.state.purchased == 0);
        assert(session.Bind(Generate(78), 0));
        session.Load(saved, 0);
        assert(session.state.purchased == 0);
        assert(!session.Bind(nlohmann::json::object(), 0));
        assert(!session.Active(0));
    }
    Options options;
    const auto first = Generate(123456u, options);
    assert(Validate(first));
    assert(first == Generate(123456u, options));
    assert(first != Generate(123457u, options));
    std::set<std::string> seen;
    for (uint32_t seed = 0; seed < 4096; ++seed) {
        const auto market = Generate(seed, options);
        assert(Validate(market));
        assert(market.at("stock").size() == 8);
        std::set<std::string> unique;
        int wildcards = 0;
        int tiers[3]{};
        for (const auto& offer : market.at("stock")) {
            const auto* item = Find(offer.at("key").get<std::string>());
            assert(item);
            assert(unique.insert(item->key).second);
            seen.insert(item->key);
            wildcards += item->wildcard;
            ++tiers[item->tier - 1];
            assert(offer.at("price") == item->price);
            assert(offer.at("clears") == item->tier - 1);
            assert(item->price >= (item->tier == 1 ? 30 : item->tier == 2 ? 100 : 160));
            assert(item->price <= (item->tier == 1 ? 90 : item->tier == 2 ? 150 : 200));
        }
        assert(wildcards == 2);
        assert(tiers[0] == 2 && tiers[1] == 4 && tiers[2] == 2);
    }
    for (const auto& item : Catalog())
        assert(seen.contains(item.key));
    assert(Catalog().size() >= 75);
    // A solo OoT file must never roll native MM masks/refills that need MM
    // assets, while retaining the fixed 6 NEI / 2 wildcard and 2/4/2 quotas.
    auto ootOptions = OptionsFromJson({ { "games", "oot" } });
    auto mmOptions = OptionsFromJson({ { "games", "mm" } });
    for (uint32_t seed = 0; seed < 512; ++seed) {
        const auto oot = Generate(seed, ootOptions);
        const auto mm = Generate(seed, mmOptions);
        assert(Validate(oot) && Validate(mm));
        assert(oot.value("games", "both") == "oot");
        assert(mm.value("games", "both") == "mm");
        for (const auto& offer : oot["stock"]) {
            const auto key = offer["key"].get<std::string>();
            assert(!key.starts_with("wildcard.") || key == "wildcard.mario_mask" ||
                   key == "wildcard.poke_ball" || key == "wildcard.magic_upgrade" ||
                   key == "wildcard.double_defense");
        }
    }
    assert(!first.contains("games")); // existing combined seeds stay byte-identical
    auto foreignOffer = Generate(7, mmOptions);
    foreignOffer["games"] = "oot";
    foreignOffer["games"] = "invalid";
    assert(!Validate(foreignOffer));
    assert(Find("wildcard.fierce_deity"));
    assert(Find("wildcard.poke_ball"));
    assert(Find("wildcard.mario_mask"));
    assert(Find("nei.cane.byrna"));
    assert(Find("nei.season.winter"));
    assert(Find("nei.rune.sensor"));
    assert(Find("nei.cane.ultrahand"));

    options.dungeonUnlocks = false;
    const auto ungated = Generate(123456u, options);
    for (size_t i = 0; i < 8; ++i) {
        assert(ungated["stock"][i]["key"] == first["stock"][i]["key"]);
        assert(ungated["stock"][i]["clears"] == 0);
    }
    options.pricing = Pricing::FollowSoH;
    options.soh.mode = PriceMode::Fixed;
    options.soh.fixedPrice = 375;
    auto fixed = Generate(123456u, options);
    assert(Validate(fixed));
    for (size_t i = 0; i < 8; ++i) {
        assert(fixed["stock"][i]["key"] == first["stock"][i]["key"]);
        assert(fixed["stock"][i]["price"] == 375);
    }
    options.soh.mode = PriceMode::Wallet;
    options.soh.walletWeights = { 0, 1, 0, 0, 0 };
    for (uint32_t seed = 0; seed < 100; ++seed) {
        const auto walletMarket = Generate(seed, options);
        for (const auto& offer : walletMarket.at("stock"))
            assert(offer.at("price") >= 1 && offer.at("price") <= 99);
    }
    options.soh.walletWeights = { 0, 0, 0, 0, 1 };
    options.soh.tycoon = false;
    const auto noTycoon = Generate(9, options);
    for (const auto& offer : noTycoon.at("stock"))
        assert(offer.at("price") <= 500);

    assert(!Validate(nlohmann::json::object()));
    auto invalid = first;
    invalid["version"] = 999;
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][1] = invalid["stock"][0];
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][0]["key"] = "unknown.item";
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][0]["price"] = -1;
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][0]["identity"] = "RI_WALLET_GIANT";
    assert(!Validate(invalid));

    State state;
    assert(StateFromJson(StateToJson(state)).purchased == 0);
    int tier3slot = -1;
    for (int i = 0; i < 8; ++i)
        if (first["stock"][i]["tier"] == 3)
            tier3slot = i;
    assert(tier3slot >= 0);
    assert(CanPurchase(first, state, tier3slot, 999, 0, false, true) == PurchaseResult::Locked);
    assert(CanPurchase(first, state, tier3slot, 0, 2, false, true) == PurchaseResult::NeedRupees);
    assert(CanPurchase(first, state, tier3slot, 999, 2, true, true) == PurchaseResult::Owned);
    assert(CanPurchase(first, state, tier3slot, 999, 2, false, false) == PurchaseResult::Unavailable);
    assert(CanPurchase(first, state, tier3slot, 999, 2, false, true) == PurchaseResult::Ready);
    CommitPurchase(state, tier3slot);
    assert(CanPurchase(first, state, tier3slot, 999, 2, false, true) == PurchaseResult::SoldOut);
    const auto reloaded = StateFromJson(StateToJson(state));
    assert(reloaded.purchased == state.purchased);
    assert(CanPurchase(first, reloaded, tier3slot, 999, 2, false, true) == PurchaseResult::SoldOut);
    assert(CanPurchase(first, state, -1, 999, 2, false, true) == PurchaseResult::Unavailable);
    assert(CanPurchase(first, state, 8, 999, 2, false, true) == PurchaseResult::Unavailable);
    assert(StateFromJson(nlohmann::json{ { "purchased", -1 } }).purchased == 0);
    assert(ClearCount(0xFFF) == 12);
    assert(ClearCount(0xFFFF) == 12);
    std::cout << "PASS: 4096 deterministic full-catalog inventories; quotas, independent pricing, validation and purchases\n";
}
