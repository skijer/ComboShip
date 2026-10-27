// Real native GBI, typed DisplayList snapshots and packed I8 data. This is a
// command/shader contract test; it does not claim a configured game render.
#include "render/SwordFinishIceMaterial.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

static Vtx vertices[3] = {
    { { { -20, -10, 0 }, 0, { 0, 0 }, { 30, 60, 90, 255 } } },
    { { { 20, -10, 0 }, 0, { 1024, 0 }, { 60, 90, 30, 255 } } },
    { { { 0, 20, 0 }, 0, { 512, 1024 }, { 90, 30, 60, 255 } } },
};

static auto Source(bool combined, bool alt = false) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = combined ? "objects/gameplay_keep/gEffIceFragment2DL"
                          : "objects/gameplay_keep/gEffIceFragment2MaterialDL";
    if (alt)
        init->Path = "alt/" + init->Path;
    init->IsCustom = alt;
    auto source = std::make_shared<Fast::DisplayList>(init);
    source->UCode = ucode_f3dex2;
    // The hash payload deliberately resembles a vertex opcode. It must never
    // be mistaken for the first actual geometry command.
    Gfx image = gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_16b, 1, 0);
    image.words.w0 = (image.words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_HASH << 24);
    Gfx hash{};
    hash.words.w0 = G_VTX << 24;
    hash.words.w1 = 0x12345678;
    source->Instructions = {
        gsDPPipeSync(), image, hash,
        gsDPSetCombineMode(G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM),
        gsSPTexture(0xFFFF, 0xFFFF, 0, 0, G_ON),
        gsSPSetGeometryMode(G_LIGHTING | G_TEXTURE_GEN),
    };
    if (combined) {
        const Gfx model[] = { gsSPVertex(vertices, 3, 0), gsSP1Triangle(0, 1, 2, 0) };
        source->Instructions.insert(source->Instructions.end(), std::begin(model), std::end(model));
        if (alt) {
            char* path = strdup("objects/gameplay_keep/accepted_ice_vertices");
            source->Strings.push_back(path);
            source->Instructions[6].words.w0 =
                (source->Instructions[6].words.w0 & 0x00FFFFFF) | (G_VTX_OTR_FILEPATH << 24);
            source->Instructions[6].words.w1 = reinterpret_cast<uintptr_t>(path);
        }
    }
    const Gfx end = gsSPEndDisplayList();
    source->Instructions.push_back(end);
    return source;
}

static bool Same(const Gfx& a, const Gfx& b) {
    return a.words.w0 == b.words.w0 && a.words.w1 == b.words.w1;
}

static void CheckClone(bool combined, bool alt) {
    SwordFinishIce::Cache cache;
    auto source = Source(combined, alt);
    const auto original = source->Instructions;
    std::array<unsigned char, sizeof(vertices)> vertexBytes;
    std::memcpy(vertexBytes.data(), vertices, sizeof(vertices));
    Gfx* clone = cache.Get(source, 1, combined);
    assert(clone && clone != source->Instructions.data());
    const size_t boundary = combined ? 6 : original.size() - 1;
    assert((clone[boundary].words.w0 >> 24) == G_DL);
    assert(clone[boundary].words.w1 == reinterpret_cast<uintptr_t>(SwordFinishIce::Material()));
    for (size_t i = 0; i < original.size(); ++i) {
        assert(Same(source->Instructions[i], original[i]));
        assert(Same(clone[i + (i >= boundary)], original[i]));
    }
    assert(std::memcmp(vertexBytes.data(), vertices, sizeof(vertices)) == 0);
    assert(cache.Get(source, 1, combined) == clone);
    assert(cache.Get(source, 2, combined) == clone);

    // An in-place source change rebuilds without mutating submitted pointers.
    source->Instructions[4].words.w1 ^= 0x010000;
    Gfx* changed = cache.Get(source, 2, combined);
    assert(changed && changed != clone);
    assert(Same(clone[4], original[4]));
    assert(Same(changed[4], source->Instructions[4]));

    // Eviction/re-entry retains the source's filepath/string ownership while
    // old submitted graphs can still reference the clone.
    std::weak_ptr<Fast::DisplayList> prior = source;
    source.reset();
    assert(!prior.expired());
    if (alt && combined)
        assert(std::strcmp(reinterpret_cast<const char*>(clone[7].words.w1),
                           "objects/gameplay_keep/accepted_ice_vertices") == 0);
    auto replacement = Source(combined);
    Gfx* next = cache.Get(replacement, 3, combined);
    assert(next && Same(clone[0], original[0]));
    cache.Get(replacement, 4, combined);
    assert(!prior.expired());
    cache.Get(replacement, 5, combined);
    assert(prior.expired());
    cache.Clear();
}

static void CheckFallback() {
    SwordFinishIce::Cache cache;
    assert(!cache.Get(nullptr, 0, true));
    auto source = Source(true);
    source->UCode = ucode_f3d;
    assert(!cache.Get(source, 0, true));
    source = Source(false);
    assert(!cache.Get(source, 0, true)); // No combined geometry.
    source = Source(true);
    assert(!cache.Get(source, 0, false)); // Not an MM material-only list.
    source = Source(true);
    source->Instructions.resize(2); // Truncated extended hash command.
    assert(!cache.Get(source, 0, true));
    source = Source(true);
    source->Instructions.insert(source->Instructions.end() - 1, source->Instructions[3]);
    assert(!cache.Get(source, 0, true)); // Material overwrite inside mesh.
    for (unsigned opcode : { G_TEXTURE, G_LOADTLUT, G_SETOTHERMODE_H, G_SETOTHERMODE_L,
                              G_RDPSETOTHERMODE, G_SETPRIMCOLOR, G_SETENVCOLOR }) {
        source = Source(true, true);
        Gfx overwrite{};
        overwrite.words.w0 = opcode << 24;
        source->Instructions.insert(source->Instructions.end() - 1, overwrite);
        assert(!cache.Get(source, 0, true)); // No later material state may cancel the scope.
    }
    source = Source(true);
    source->Instructions[4] = Gfx{ gsSPDisplayList(0x06001000) };
    assert(!cache.Get(source, 0, true)); // Unknown nested material/geometry.
    for (unsigned opcode : { G_DL_INDEX, G_LOAD_UCODE, G_PUSH_SHADER, G_SETTIMG_FB, G_IMAGERECT,
                              G_SETPRIMCOLOR, G_SETENVCOLOR, 0x7f }) {
        source = Source(true, true);
        source->Instructions[4].words.w0 = opcode << 24;
        assert(!cache.Get(source, 0, true)); // Unknown control/shader/texture operations cannot be proven safe.
    }
    source = Source(true, true);
    source->Instructions[4] = Gfx{ gsSPSegment(8, 0x06000000) };
    assert(!cache.Get(source, 0, true)); // The native scroll segment must still belong to the caller.
    for (unsigned i = 0; i < 8; ++i)
        assert(cache.Get(Source(true), 1, true));
    assert(!cache.Get(Source(true), 1, true)); // Bounded reload fallback.
}

struct Sample { double color, alpha; };
static double Color(unsigned selector, double combined, double frost, double opacity,
                    double primitive, double environment, double shade) {
    switch (selector) {
        case 0: return combined;
        case 1: return frost;
        case 2: return opacity;
        case 3: return primitive;
        case 4: return shade;
        case 5: return environment;
        case 6: return 1;
        case 7: case 15: case 31: return 0;
        default: assert(false); return 0;
    }
}
static double Alpha(unsigned selector, double combined, double frost, double opacity,
                    double primitive, double environment) {
    return Color(selector, combined, frost, opacity, primitive, environment, 1);
}
static Sample Evaluate(const Gfx& combine, double frost, double opacity, double primitive,
                       double environment, double shade, double fade) {
    const auto a = combine.words.w0;
    const auto b = combine.words.w1;
    const unsigned ca[2] = { unsigned(a >> 20) & 15, unsigned(a >> 5) & 15 };
    const unsigned cb[2] = { unsigned(b >> 28) & 15, unsigned(b >> 24) & 15 };
    const unsigned cc[2] = { unsigned(a >> 15) & 31, unsigned(a) & 31 };
    const unsigned cd[2] = { unsigned(b >> 15) & 7, unsigned(b >> 6) & 7 };
    const unsigned aa[2] = { unsigned(a >> 12) & 7, unsigned(b >> 21) & 7 };
    const unsigned ab[2] = { unsigned(b >> 12) & 7, unsigned(b >> 3) & 7 };
    const unsigned ac[2] = { unsigned(a >> 9) & 7, unsigned(b >> 18) & 7 };
    const unsigned ad[2] = { unsigned(b >> 9) & 7, unsigned(b) & 7 };
    Sample result{};
    for (int cycle = 0; cycle < 2; ++cycle) {
        auto c = [&](unsigned s) { return Color(s, result.color, frost, opacity, primitive, environment, shade); };
        auto alpha = [&](unsigned s) { return Alpha(s, result.alpha, frost, opacity, 250.0 / 255, fade); };
        result = { (c(ca[cycle]) - c(cb[cycle])) * c(cc[cycle]) + c(cd[cycle]),
                   (alpha(aa[cycle]) - alpha(ab[cycle])) * alpha(ac[cycle]) + alpha(ad[cycle]) };
    }
    return result;
}

static void CheckMaterial() {
    const Gfx* material = SwordFinishIce::Material();
    const Gfx* combine = nullptr;
    unsigned images = 0, tiles = 0, dimensions = 0, scrollCalls = 0;
    for (unsigned i = 0; i < 64; ++i) {
        const auto& command = material[i];
        const unsigned op = command.words.w0 >> 24;
        if (op == G_ENDDL) break;
        if (op == G_SETCOMBINE) combine = &command;
        if (op == G_SETTIMG) {
            const uintptr_t want = reinterpret_cast<uintptr_t>(images ? sSwordFinishIceOpacityTex : sSwordFinishIceFrostTex);
            assert(command.words.w1 == want);
            assert(((command.words.w0 >> 21) & 7) == G_IM_FMT_I);
            ++images;
        }
        if (op == G_SETTILE && ((command.words.w1 >> 24) & 7) != G_TX_LOADTILE) {
            const unsigned tile = (command.words.w1 >> 24) & 7;
            const unsigned line = (command.words.w0 >> 9) & 511;
            const unsigned tmem = command.words.w0 & 511;
            assert(tile == tiles && tile < 2);
            assert(((command.words.w0 >> 19) & 3) == G_IM_SIZ_8b);
            assert(line == (tile ? 8 : 4) && tmem == (tile ? 64 : 0));
            assert(tmem * 8 + line * 8 * (tile ? 32 : 16) <= 4096);
            ++tiles;
        }
        if (op == G_SETTILESIZE) {
            const unsigned tile = (command.words.w1 >> 24) & 7;
            assert((((command.words.w1 >> 12) & 4095) / 4 + 1) == (tile ? 64 : 32));
            assert(((command.words.w1 & 4095) / 4 + 1) == (tile ? 32 : 16));
            ++dimensions;
        }
        if (op == G_DL) {
            assert(command.words.w1 == 0x08000000);
            ++scrollCalls;
        }
    }
    assert(images == 2 && tiles == 2 && dimensions == 2 && scrollCalls == 1 && combine);
    const auto [low, high] = std::minmax_element(std::begin(sSwordFinishIceFrostTex), std::end(sSwordFinishIceFrostTex));
    const double dark = *low / 255.0, bright = *high / 255.0;
    const auto clear = Evaluate(*combine, dark, 0.8, 95.0 / 255, 230.0 / 255, 1, 1);
    const auto frost = Evaluate(*combine, bright, 0.8, 95.0 / 255, 230.0 / 255, 1, 1);
    const auto shadow = Evaluate(*combine, bright, 0.8, 95.0 / 255, 230.0 / 255, .35, 1);
    const auto fading = Evaluate(*combine, bright, 0.8, 95.0 / 255, 230.0 / 255, 1, .2);
    const auto porous = Evaluate(*combine, bright, 0.6, 95.0 / 255, 230.0 / 255, 1, 1);
    assert(frost.color - clear.color > .25); // Texture controls visible frost contrast.
    assert(shadow.color < frost.color * .4); // Native facet lighting survives.
    assert(fading.alpha < frost.alpha * .21); // Life envelope still fades.
    assert(porous.alpha < frost.alpha * .8); // The second texture is active.
    assert(frost.alpha > .7 && frost.alpha < 1); // Translucent ice, without holes.
}

int main() {
    for (bool alt : { false, true }) {
        CheckClone(true, alt);
        CheckClone(false, alt);
    }
    CheckFallback();
    CheckMaterial();
    puts("PASS typed OOT/MM material clones: exact geometry/UV/normal preservation, hash payload parsing, "
         "Alt/unsupported fallback, bounded cache ownership, I8/TMEM tiles, frost/opacity/shade/fade shader semantics");
}
