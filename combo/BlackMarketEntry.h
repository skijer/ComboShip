#pragma once

#include <array>
#include <cstring>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace BlackMarketEntry {
inline constexpr unsigned short kUnavailableText = 0x3F10;
inline constexpr char kManifest[] = "custom/skiejer_market/manifest.json";
inline constexpr char kManifestKey[] = "custom/skiejer_market/manifest";
inline constexpr std::array<const char*, 3> kMmDestination = {
    "scenes/nonmq/skiejer_market/skiejer_market",
    "scenes/nonmq/skiejer_market/skiejer_market_room_0",
    "scenes/nonmq/skiejer_market/collision",
};
inline constexpr std::array<const char*, 3> kOotDestination = {
    "scenes/shared/skiejer_market_oot/skiejer_market_oot",
    "scenes/shared/skiejer_market_oot/skiejer_market_oot_room_0",
    "scenes/nonmq/skiejer_market/collision",
};

template <typename Exists> bool LegacyResourceAvailable(const char* path, Exists exists) {
    // Raw JSON metadata loses its extension in OTRExtScanner. Keep the exact
    // lookup for providers that expose raw names, then check the logical key.
    return exists(path) || (std::strcmp(path, kManifest) == 0 && exists(kManifestKey));
}

template <typename Range, typename Exists> std::vector<std::string> Missing(const Range& paths, Exists exists) {
    std::vector<std::string> missing;
    for (const auto* path : paths)
        if (!exists(path))
            missing.emplace_back(path);
    return missing;
}

inline const char* ResultName(int result) {
    return result == 1 ? "entered" : result == 2 ? "blocked" : "unhandled";
}

inline nlohmann::json FailedChecks(const nlohmann::json& checks) {
    auto failed = nlohmann::json::array();
    for (const auto& [name, passed] : checks.items())
        if (!passed.get<bool>())
            failed.push_back(name);
    return failed;
}

template <typename Session> nlohmann::json MarketState(const Session& session, int file) {
    const auto& market = session.market;
    int boundSlot = -1;
    for (int slot = 0; slot < 3; ++slot)
        if (session.Active(slot))
            boundSlot = slot;
    const auto seed = market.is_object() ? market.value("seed", nlohmann::json()) : nlohmann::json();
    const auto count = market.is_object() && market.contains("stock") && market["stock"].is_array()
                           ? static_cast<int>(market["stock"].size())
                           : -1;
    return { { "active", session.Active(file) },
             { "bound_slot", boundSlot },
             { "seed", seed },
             { "stock_count", count },
             { "purchased", session.state.purchased } };
}
} // namespace BlackMarketEntry
