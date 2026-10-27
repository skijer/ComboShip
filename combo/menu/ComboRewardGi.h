#pragma once
#include <stdint.h>
#include <string.h>

// Stable profiles: Forest, Fire, Water, Spirit, Shadow, Light, Emerald, Ruby, Sapphire.
static inline uint32_t RewardGi_Color(int profile) {
    static const uint32_t colors[9] = { 0x54D85B, 0xFF593C, 0x459DFF, 0xFFA64D, 0xA675EB,
                                        0xFFE58A, 0x46E879, 0xFF4564, 0x459DFF };
    return profile >= 1 && profile <= 9 ? colors[profile - 1] : 0;
}
static inline const char* RewardGi_Texture(int profile) {
    static const char* const paths[6] = {
        "__OTR__objects/nei_reward_gi/forest", "__OTR__objects/nei_reward_gi/fire",
        "__OTR__objects/nei_reward_gi/water",  "__OTR__objects/nei_reward_gi/spirit",
        "__OTR__objects/nei_reward_gi/shadow", "__OTR__objects/nei_reward_gi/light",
    };
    return profile >= 1 && profile <= 9 ? paths[(profile - 1) % 6] : NULL;
}
static inline int RewardGi_ShimmerColor(int profile, uint8_t color[4]) {
    if (!color || profile < 1 || profile > 9)
        return 0;
    const uint32_t rgb = RewardGi_Color(profile);
    color[0] = (uint8_t)(rgb >> 16);
    color[1] = (uint8_t)(rgb >> 8);
    color[2] = (uint8_t)rgb;
    color[3] = 255;
    return 1;
}
typedef struct {
    uint32_t s, t;
} RewardGiScroll;
static inline RewardGiScroll RewardGi_Scroll(uint32_t frame) {
    // Quarter-texel offsets on the shared 32x32 tile. Integer periods make the
    // wrap continuous for the periodic material, including long-running saves.
    const uint32_t phase = frame % 1024u;
    const RewardGiScroll scroll = { phase % 128u, (128u - phase % 128u) % 128u };
    return scroll;
}
static inline int RewardGi_ProfileForPaths(const char* first, const char* second) {
    static const char* const roots[9] = {
        "objects/object_gi_medal/gGiForestMedallionFaceDL", "objects/object_gi_medal/gGiFireMedallionFaceDL",
        "objects/object_gi_medal/gGiWaterMedallionFaceDL",  "objects/object_gi_medal/gGiSpiritMedallionFaceDL",
        "objects/object_gi_medal/gGiShadowMedallionFaceDL", "objects/object_gi_medal/gGiLightMedallionFaceDL",
        "objects/object_gi_jewel/gGiKokiriEmeraldGemDL",    "objects/object_gi_jewel/gGiGoronRubyGemDL",
        "objects/object_gi_jewel/gGiZoraSapphireGemDL",
    };
    const char* paths[2] = { first, second };
    for (int p = 0; p < 2; ++p) {
        const char* path = paths[p];
        if (!path)
            continue;
        if (!strncmp(path, "__OTR__", 7))
            path += 7;
        if (*path == '@') {
            path = strchr(path, ':');
            if (!path)
                continue;
            ++path;
        }
        for (int i = 0; i < 9; ++i)
            if (!strcmp(path, roots[i]))
                return i + 1;
    }
    return 0;
}
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
// The incoming native pose is preserved. Owner selects the geometry; the
// shared private material images live in the OoT asset tree for both hosts.
void NeiGi_DrawRewardMaterial(struct PlayState* play, int profile, const char* surface, const char* setting,
                              const char* owner);
#ifdef __cplusplus
}
#endif
