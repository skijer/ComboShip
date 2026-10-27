#!/usr/bin/env python3
"""Compare the supplemental stream with the real SoH retail pricing bodies."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
native = (ROOT / "soh/soh/Enhancements/randomizer/3drando/shops.cpp").read_text()

def block(signature):
    begin = native.index(signature)
    opening = native.index("{", begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (native[end] == "{") - (native[end] == "}")
        end += 1
    return native[begin:end]

arrays = "\n".join(re.search(r"static constexpr std::array<double, \d+> " + name + r" = \{.*?\};", native, re.S)[0]
                   for name in ("ShopPriceProbability", "CheapPriceProbability"))
preamble = r'''
#include "rando/BlackMarket.h"
#include <cassert>
#include <iostream>
namespace Native {
using ComboRando::BlackMarket::Rng;
Rng* rng;
int Random(int low, int high) { return low + static_cast<int>(rng->Below(high - low)); }
double RandomDouble() { return (rng->Next() >> 11) * (1.0 / 9007199254740992.0); }
enum { RO_PRICE_VANILLA, RO_PRICE_CHEAP_BALANCED, RO_PRICE_BALANCED, RO_PRICE_FIXED, RO_PRICE_RANGE,
       RO_PRICE_SET_BY_WALLET, RSK_INCLUDE_TYCOON_WALLET = 20 };
struct PriceSettingsStruct {
    int main=0, fixedPrice=1, range1=2, range2=3, noWallet=4, childWallet=5, adultWallet=6,
        giantWallet=7, tycoonWallet=8, affordable=9;
};
struct Option {
    uint16_t value=0;
    uint16_t Get() const { return value; }
    bool Is(int other) const { return value == other; }
    explicit operator bool() const { return value != 0; }
};
namespace Rando {
struct Location { uint16_t GetVanillaPrice() const { return 120; } const char* GetName() { return "market"; } };
struct Context {
    Option options[21]{};
    static Context* GetInstance() { static Context instance; return &instance; }
    Option& GetOption(int id) { return options[id]; }
};
}
#define SPDLOG_ERROR(...) ((void)0)
uint16_t GetCheapBalancedPrice();
'''
main = r'''
}
int main() {
    using namespace ComboRando::BlackMarket;
    auto* context = Native::Rando::Context::GetInstance();
    Native::Rando::Location location;
    for (int mode=0; mode<6; ++mode) {
        for (int scenario=0; scenario<5; ++scenario) {
            SoHPrices options;
            options.mode = static_cast<PriceMode>(mode);
            options.fixedPrice=375;
            options.rangeMin=25;
            options.rangeMax=325;
            options.tycoon=scenario != 3;
            options.affordable=scenario == 4;
            options.walletWeights = scenario == 1 ? std::array<uint16_t,5>{0,0,0,0,0} :
                                    scenario == 2 ? std::array<uint16_t,5>{1,0,0,0,0} :
                                                    std::array<uint16_t,5>{2,4,3,2,1};
            context->options[0].value=mode;
            context->options[1].value=75;
            context->options[2].value=5;
            context->options[3].value=65;
            for (int i=0; i<5; ++i) context->options[4+i].value=options.walletWeights[i];
            context->options[9].value=options.affordable;
            context->options[20].value=options.tycoon;
            for (uint64_t seed=0; seed<10000; ++seed) {
                Rng actualRng(seed), nativeRng(seed);
                Native::rng=&nativeRng;
                for (int slot=0; slot<8; ++slot) {
                    const auto expected=Native::GetRandomPrice(&location, {});
                    const auto actual=SoHPrice(actualRng, options, 120);
                    if (actual != expected) {
                        std::cerr << "price mismatch mode=" << mode << " scenario=" << scenario
                                  << " seed=" << seed << " slot=" << slot << " expected=" << expected
                                  << " actual=" << actual << '\n';
                        return 1;
                    }
                }
            }
        }
    }
    std::cout << "PASS: 2.4 million prices match production SoH modes, wallet weights and affordability\n";
}
'''
source = preamble + arrays + "\n" + "\n".join(block(s) for s in (
    "uint16_t GetCheapBalancedPrice()", "uint16_t GetPriceFromSettings(", "uint16_t GetRandomPrice(")) + main
with tempfile.TemporaryDirectory(prefix="market-prices-") as directory:
    path = Path(directory) / "prices.cpp"
    path.write_text(source)
    binary = Path(directory) / "prices"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-O2",
                    "-I" + str(ROOT / "combo"), str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
