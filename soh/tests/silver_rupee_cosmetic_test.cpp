#include "global.h"
#include "objects/object_gi_rupy/object_gi_rupy.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "soh/cvar_prefixes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static constexpr const char* kValue = "gCosmetics./* REGISTERED_OPTION */.Value";
static constexpr const char* kChanged = "gCosmetics./* REGISTERED_OPTION */.Changed";
static std::map<std::string, int> integers;
static std::map<std::string, Color_RGB8> colors;
static Gfx opa[64], xlu[64];
static GraphicsContext gfx;
static PlayState play;
static Mtx matrix;
static float scale;
static std::vector<float> matrixStack;
static int shimmerDraws, sampledProfile;
static float shimmerScale;
static Color_RGB8 shimmerColor;

extern "C" int32_t CVarGetInteger(const char* path, int32_t fallback) {
    const auto found = integers.find(path);
    return found == integers.end() ? fallback : found->second;
}
extern "C" Color_RGB8 CVarGetColor24(const char* path, Color_RGB8 fallback) {
    const auto found = colors.find(path);
    return found == colors.end() ? fallback : found->second;
}
extern "C" void Gfx_SetupDL_25Opa(GraphicsContext*) {
}
extern "C" void Gfx_SetupDL_25Xlu(GraphicsContext*) {
}
extern "C" Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) {
    return &matrix;
}
extern "C" void Matrix_Scale(float x, float y, float z, u8) {
    if (x != y || y != z)
        std::abort();
    scale *= x;
}
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) {
}
extern "C" void FrameInterpolation_RecordCloseChild() {
}
extern "C" void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {
}
extern "C" void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {
}
extern "C" void gSPDisplayList(Gfx* command, Gfx* dl) {
    __gSPDisplayList(command, dl);
}
extern "C" void gSPSegment(void* command, int segment, uintptr_t path) {
    __gSPSegment(command, segment, path);
}

// Execute the production shared drop policy; sampler and GPU submission remain
// fixture boundaries, just like CVars and the native matrix allocation above.
extern "C" void Matrix_Push() {
    matrixStack.push_back(scale);
}
extern "C" void Matrix_Pop() {
    if (matrixStack.empty())
        std::abort();
    scale = matrixStack.back();
    matrixStack.pop_back();
}
#include "combo/menu/ComboCollectibleShimmerHost.h"
extern "C" void OOT_SampleCollectibleShimmerColor(int32_t profile, uint8_t color[4]) {
    sampledProfile = profile;
    const Color_RGB8 chosen = CVarGetColor24(kValue, { 255, 255, 255 });
    color[0] = chosen.r;
    color[1] = chosen.g;
    color[2] = chosen.b;
    color[3] = 255;
}
extern "C" void NeiGi_DrawTintedShimmerOverlay(PlayState* current, const uint8_t color[4], const char* owner) {
    if (current != &play || owner != nullptr || color[3] != 255)
        std::abort();
    ++shimmerDraws;
    shimmerScale = scale;
    shimmerColor = { color[0], color[1], color[2] };
}

/* NATIVE_DRAW */

static void Require(bool value, const char* why) {
    if (!value) {
        std::fprintf(stderr, "FAIL native_palette: %s\n", why);
        std::exit(1);
    }
}
static uint32_t Rgba(Color_RGB8 c) {
    return uint32_t(c.r) << 24 | uint32_t(c.g) << 16 | uint32_t(c.b) << 8 | 255;
}
static void Check(bool drops, bool changed, Color_RGB8 color, bool shimmer) {
    integers["gEnhancements.CollectibleShimmer"] = shimmer;
    integers["gEnhancements.NewDrops"] = drops;
    integers[kChanged] = changed;
    colors[kValue] = color;
    // Migrated stale values must not override the registered editor's choice.
    colors["gCosmetics.Consumable_SilverRupee.Value"] = { 1, 2, 3 };
    integers["gCosmetics.Consumable_SilverRupee.Changed"] = 0;
    std::memset(&gfx, 0, sizeof(gfx));
    std::memset(&play, 0, sizeof(play));
    std::memset(opa, 0, sizeof(opa));
    std::memset(xlu, 0, sizeof(xlu));
    gfx.polyOpa.p = opa;
    gfx.polyXlu.p = xlu;
    play.state.gfxCtx = &gfx;
    scale = 1;
    shimmerDraws = 0;
    sampledProfile = -1;
    Require(matrixStack.empty(), "previous draw leaked a matrix scope");
    GetItemEntry entry{};
    Randomizer_DrawSilverRupee(&play, &entry);
    Require(shimmerDraws == shimmer && sampledProfile == (shimmer ? CW_COLLECTIBLE_SILVER : -1),
            "shared drop policy changed silver profile or disabled shimmer submission");
    Require(matrixStack.empty() && scale == (drops ? 1.0f : .05f), "shimmer did not restore the native model matrix");
    if (shimmer) {
        Require(shimmerScale == 1.0f, "shimmer conversion did not normalize the native silver drop scale");
        Require(shimmerColor.r == color.r && shimmerColor.g == color.g && shimmerColor.b == color.b,
                "sampled silver palette did not reach the shimmer overlay");
    }
    int inner = 0, outer = 0, drop = 0, gray = 0, grayColors = 0, segment = 0;
    uint32_t prim = 0, env = 0;
    auto stream = [&](Gfx* first, Gfx* last, bool translucent) {
        for (auto* command = first; command < last; ++command) {
            const auto op = (command->words.w0 >> 24) & 255;
            if (op == G_SETPRIMCOLOR)
                prim = command->words.w1;
            if (op == G_SETENVCOLOR)
                env = command->words.w1;
            if (op == G_SETINTENSITY) {
                ++grayColors;
                Require(command->words.w1 == Rgba(color), "registered picker color does not reach native grayscale");
            }
            if (op == G_SETGRAYSCALE)
                gray = command->words.w1;
            if (op == G_MOVEWORD && ((command->words.w0 >> 16) & 255) == G_MW_SEGMENT) {
                Require((command->words.w0 & 0xffff) == 8 * 4 && command->words.w1 == uintptr_t(gRupeeSilverTex),
                        "native silver texture segment changed");
                ++segment;
            }
            if (op != G_DL)
                continue;
            const auto* path = reinterpret_cast<const char*>(command->words.w1);
            if (!std::strcmp(path, gGiRupeeInnerDL)) {
                Require(!translucent && prim == Rgba(color),
                        "registered picker color does not reach native inner model");
                Require(env == Rgba({ uint8_t(color.r / 5), uint8_t(color.g / 5), uint8_t(color.b / 5) }),
                        "native inner shade changed");
                ++inner;
            } else if (!std::strcmp(path, gGiRupeeOuterDL)) {
                Require(translucent && prim == 0xffffffff, "native outer model stream/white prim changed");
                Require(env == Rgba({ uint8_t(color.r * .75f), uint8_t(color.g * .75f), uint8_t(color.b * .75f) }),
                        "native outer shade changed");
                ++outer;
            } else if (!std::strcmp(path, gRupeeDL)) {
                Require(!translucent && gray == changed,
                        "registered Changed flag does not control native drop grayscale");
                ++drop;
            } else
                Require(false, "unexpected silver-rupee model");
        }
    };
    stream(opa, gfx.polyOpa.p, false);
    stream(xlu, gfx.polyXlu.p, true);
    Require(gray == 0, "native grayscale was not restored");
    Require(inner == drops && outer == drops && drop == !drops, "native NewDrops selection changed");
    Require(segment == !drops && grayColors == (!drops && changed), "native texture/tint scope changed");
    Require(drops || scale == .05f, "native drop scale changed");
}
int main() {
    for (bool drops : { false, true })
        for (bool changed : { false, true })
            for (Color_RGB8 color : { Color_RGB8{ 29, 147, 231 }, Color_RGB8{ 213, 72, 45 } })
                for (bool shimmer : { false, true })
                    Check(drops, changed, color, shimmer);
    std::puts("PASS native palette: registered colors/Changed reach both live models; segment8, shades and grayscale "
              "restoration retained; shimmer off/on keeps silver palette, GI scale and matrix scope");
}
