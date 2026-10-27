// Read-only preview fixture: use production stock generation and shelf fitting.
#include "combo/rando/BlackMarket.h"
#include "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
#include <iostream>

int main() {
    using namespace ComboRando::BlackMarket;
    struct Example { const char* key; const char* slug; float scale; };
    constexpr Example examples[] = {
        { "nei.shovel", "shovel", .2f }, { "nei.lantern", "lantern", .025f },
        { "nei.fire_rod", "fire_rod", .2f }, { "nei.ice_rod", "ice_rod", .2f },
        { "nei.spinner", "spinner", .3f }, { "nei.switch_hook", "switch_hook", .01f },
        { "wildcard.great_fairy_sword", "great_fairy_sword", 1.f },
        { "wildcard.mario_mask", "mario_mask", 1.f }
    };
    Options options;
    for (const auto& example : examples) options.eligible.insert(example.key);
    auto market = Generate(0x534B4945, options);
    if (!Validate(market)) return 1;
    for (auto& offer : market["stock"]) {
        const Item* item = Find(offer["key"].get<std::string>());
        for (const auto& example : examples) {
            if (offer["key"] != example.key) continue;
            const auto path = std::string("__OTR__objects/nei_gi_redesign/") + example.slug + "/gi_dl";
            const auto* bounds = NeiGi::FindFrameBounds(path.c_str());
            if (!bounds) return 2;
            const auto fit = NeiGi::FrameFit(*bounds, example.scale, true);
            offer["slug"] = example.slug;
            offer["label"] = item->label;
            offer["wildcard"] = item->wildcard;
            offer["draw_scale"] = example.scale;
            offer["fit_scale"] = fit.scale;
            offer["fit_lift"] = fit.lift;
        }
    }
    market["preview_only"] = true;
    market["sample_note"] = "Production Generate with an eight-item eligible set; not a user seed or save.";
    std::cout << market.dump(2) << '\n';
}
