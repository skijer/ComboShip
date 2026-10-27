#pragma once
#include <cstdint>

// Editor row keys only. They never identify a soundbank or enter an audio queue.
namespace ComboSceneMusic {
inline constexpr uint16_t kForest = 0xff00, kMarket = 0xff01;
inline bool IsEntry(uint16_t key) {
    return key == kForest || key == kMarket;
}
inline bool Matches(uint16_t request, uint16_t native) {
    return request == native || request == (native | 0x8000); // Native async start flag.
}
template <class Map, class GetValue>
uint16_t ResolveEntry(const Map& map, uint16_t key, int bgmTypes, GetValue getValue) {
    const auto& row = map.at(key);
    const auto fallback = row.sequenceId;
    const int selected = getValue(row.sfxKey, fallback);
    const auto found = map.find(selected);
    if (selected < 0 || selected > 0xffff || IsEntry(uint16_t(selected)) || found == map.end() ||
        !found->second.canBeUsedAsReplacement)
        return fallback;
    const auto& value = found->second;
    if constexpr (requires { value.type; }) {
        if (!(value.type & bgmTypes))
            return fallback;
    } else {
        if (!(value.category & bgmTypes))
            return fallback;
    }
    return uint16_t(selected);
}
} // namespace ComboSceneMusic
