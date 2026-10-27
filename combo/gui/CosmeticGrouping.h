#pragma once

#include <string_view>

namespace CosmeticGrouping {

// The native editors sort by stable cosmetic IDs. Remove only the component
// suffix so each adjacent primary/secondary (or equivalent) family stays intact.
inline std::string_view EffectFamily(std::string_view id) {
    constexpr std::string_view suffixes[] = {
        "Primary", "Secondary", "Prim", "Sec", "Core", "Outer", "Charge", "Burst", "Glow", "Highlights",
    };
    for (const auto suffix : suffixes) {
        if (id.size() > suffix.size() && id.substr(id.size() - suffix.size()) == suffix) {
            return id.substr(0, id.size() - suffix.size());
        }
    }
    return id;
}

class EffectFamilyBoundary {
  public:
    // Feed visible entries only. The views refer to the editors' stable map keys.
    bool Before(std::string_view id) {
        const auto family = EffectFamily(id);
        const bool boundary = !mPrevious.empty() && mPrevious != family;
        mPrevious = family;
        return boundary;
    }

  private:
    std::string_view mPrevious;
};

} // namespace CosmeticGrouping
