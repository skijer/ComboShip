#pragma once
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace NeiAssetPriority {
inline std::string NormalizeArchive(const std::string& path) {
    if (path.empty())
        return {};
    return std::filesystem::absolute(path).lexically_normal().generic_string();
}

// Query the winning archive without loading a model or retaining resource pointers.
// Shipped XML models are also "custom" resources, so IsCustom is not mod provenance.
template <typename ArchiveForFile>
bool UsesModAsset(const char* path, bool altEnabled, const std::vector<std::string>& builtins,
                  ArchiveForFile archiveForFile) {
    if (!path || !*path)
        return false;
    std::string selected = path;
    if (selected.compare(0, 7, "__OTR__") == 0)
        selected.erase(0, 7);
    if (altEnabled && selected.compare(0, 4, "alt/") != 0 &&
        (!archiveForFile("alt/" + selected).empty() || !archiveForFile("alt/" + selected + ".meta").empty()))
        selected = "alt/" + selected;
    for (const auto& key : { selected, selected + ".meta" }) {
        const auto archive = archiveForFile(key);
        if (archive.empty())
            continue;
        const auto normalized = NormalizeArchive(archive);
        if (std::none_of(builtins.begin(), builtins.end(), [&](const std::string& builtin) {
                return !builtin.empty() && NormalizeArchive(builtin) == normalized;
            }))
            return true;
    }
    return false;
}
} // namespace NeiAssetPriority
