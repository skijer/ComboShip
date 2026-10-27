#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#if __has_include("combo/menu/ComboRewardGi.h")
#include "combo/menu/ComboRewardGi.h"
int main() {
    const unsigned colors[] = {0x54D85B, 0xFF593C, 0x459DFF, 0xFFA64D, 0xA675EB, 0xFFE58A,
                               0x46E879, 0xFF4564, 0x459DFF};
    for (int profile = 1; profile <= 9; ++profile) {
        assert(RewardGi_Color(profile) == colors[profile - 1]);
        assert(RewardGi_Texture(profile) && std::strstr(RewardGi_Texture(profile), "nei_reward_gi/"));
        unsigned char rgba[4]{};
        assert(RewardGi_ShimmerColor(profile, rgba) && rgba[3] == 255);
        assert((unsigned(rgba[0]) << 16 | unsigned(rgba[1]) << 8 | rgba[2]) == colors[profile - 1]);
    }
    assert(!std::strcmp(RewardGi_Texture(1), RewardGi_Texture(7)));
    assert(!std::strcmp(RewardGi_Texture(2), RewardGi_Texture(8)));
    assert(!std::strcmp(RewardGi_Texture(3), RewardGi_Texture(9)));
    for (int invalid : {-1, 0, 10, 999}) {
        unsigned char color[] = {1, 2, 3, 4};
        assert(!RewardGi_Texture(invalid) && !RewardGi_ShimmerColor(invalid, color));
        assert(color[0] == 1 && color[3] == 4);
    }
    assert(!RewardGi_ShimmerColor(1, nullptr));
    for (unsigned frame : {0u, 1u, 127u, 128u, 1023u, 1024u, std::numeric_limits<unsigned>::max()}) {
        const auto a = RewardGi_Scroll(frame), b = RewardGi_Scroll(frame % 1024u);
        assert(a.s == b.s && a.t == b.t && a.s < 128 && a.t < 128);
    }
    assert(RewardGi_Scroll(1).s != RewardGi_Scroll(0).s);
    assert(RewardGi_Scroll(1).t != RewardGi_Scroll(0).t);
    assert(RewardGi_ProfileForPaths("__OTR__objects/object_gi_medal/gGiLightMedallionFaceDL", nullptr) == 6);
    assert(RewardGi_ProfileForPaths("__OTR__@oot:objects/object_gi_jewel/gGiKokiriEmeraldGemDL", nullptr) == 7);
    assert(RewardGi_ProfileForPaths("__OTR__objects/object_gi_clothes/gGiTunicDL", nullptr) == 0);
    assert(RewardGi_ProfileForPaths(nullptr, nullptr) == 0);
    std::cout << "PASS nine reward palettes, six shared materials, continuous wrapped scroll, exact resource identity\n";
}
#else
int main() { std::cerr << "FAIL shared reward material policy is not implemented\n"; return 1; }
#endif
