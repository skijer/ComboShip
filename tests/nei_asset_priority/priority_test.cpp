#include "../../combo/NeiAssetPriority.h"
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

int main() {
    const std::vector<std::string> stock{ "/game/soh.o2r", "/game/oot.o2r" };
    std::unordered_map<std::string, std::string> files{ { "objects/item", "/game/soh.o2r" } };
    auto owner = [&](const std::string& path) { auto it = files.find(path); return it == files.end() ? std::string{} : it->second; };
    auto check = [&](bool alt) { return NeiAssetPriority::UsesModAsset("__OTR__objects/item", alt, stock, owner); };
    assert(!check(false));
    assert(!NeiAssetPriority::UsesModAsset(nullptr, false, stock, owner));
    assert(!NeiAssetPriority::UsesModAsset("missing", false, stock, owner));
    files["objects/item"] = "/game/mods/soh.o2r"; // Same basename is still a mod.
    assert(check(false));
    files["objects/item"] = "/game/oot.o2r";
    files["alt/objects/item"] = "/game/mods/custom.zip";
    assert(!check(false));
    assert(check(true));
    files.erase("alt/objects/item");
    files["alt/objects/item.meta"] = "/game/mods/custom.zip";
    assert(check(true));
    assert(!check(false));
    files.erase("alt/objects/item.meta");
    files["objects/item.meta"] = "/game/mods/custom.zip";
    assert(check(false));
    // Canonical stock companion archives are builtin too, independent of host.
    const std::vector<std::string> mmStock{ "/game/2ship.o2r", "/game/mm.o2r", "/game/oot.o2r" };
    files.erase("objects/item.meta");
    assert(!NeiAssetPriority::UsesModAsset("objects/item", false, mmStock, owner));
    files["objects/item"] = "/game/mm.o2r";
    auto ootWithCompanion = stock;
    ootWithCompanion.push_back("/game/mm.o2r");
    assert(!NeiAssetPriority::UsesModAsset("objects/item", false, ootWithCompanion, owner));
    files["objects/item"] = "/game/mods/mm.o2r";
    assert(NeiAssetPriority::UsesModAsset("objects/item", false, ootWithCompanion, owner));
    files["objects/item"] = "/game/oot.o2r";
    // The caller supplies the resource owner's Alt flag; host state is irrelevant.
    files.erase("objects/item.meta");
    files["alt/objects/item"] = "/game/mods/custom.zip";
    const bool donorAlt = true, hostAlt = false;
    assert(check(donorAlt) && !check(hostAlt));
    // Never memoize a verdict: live archive/Alt changes must be observed.
    files["objects/item"] = "/game/mods/live.zip";
    assert(check(false));
    files["objects/item"] = "/game/./oot.o2r";
    assert(!check(false));
}
