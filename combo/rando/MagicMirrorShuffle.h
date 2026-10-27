#pragma once
#include <string>
#include <unordered_set>
namespace ComboRando::MagicMirrorShuffle {
inline constexpr const char* kGenerationCvar = "gRando.Generation.ShuffleMagicMirror";
// A native search result only. This name never enters either world's check pool.
inline constexpr const char* kOotAccessProbe = "__combo_magic_mirror_lost_woods_access";
inline constexpr const char* kMmStumpCheck = "Lost Woods Magic Mirror Stump";
inline bool Requested(bool oot, bool mm) {
    return oot || mm;
}
inline void AddOotRoute(const std::unordered_set<std::string>& oot, std::unordered_set<std::string>& mm) {
    if (oot.contains(kOotAccessProbe))
        mm.insert(kMmStumpCheck);
}
} // namespace ComboRando::MagicMirrorShuffle
