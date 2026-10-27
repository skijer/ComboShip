#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "fast/interpreter.h"

namespace Fast {
struct ShaderProgram {
    uint64_t id0, id1;
    CCFeatures features{};
};
class BackendDefaults : public GfxRenderingAPI {
  public:
#include "backend_defaults.inc"
};
class RecordingBackend : public BackendDefaults {
  public:
    std::map<std::pair<uint64_t, uint64_t>, ShaderProgram> programs;
    std::vector<uint32_t> output;
    std::vector<uint64_t> events;
    GfxClipParameters clip{true, false};
    size_t queries = 0, triangles = 0;
    bool recording = true;
    GfxClipParameters GetClipParameters() override { return clip; }
    ShaderProgram* LookupShader(uint64_t a, uint64_t b) override {
        auto i = programs.find({a, b});
        return i == programs.end() ? nullptr : &i->second;
    }
    ShaderProgram* CreateAndLoadNewShader(uint64_t a, uint64_t b) override {
        auto& p = programs[{a, b}];
        p.id0 = a; p.id1 = b;
        gfx_cc_get_features(a, b, &p.features);
        LoadShader(&p);
        return &p;
    }
    void LoadShader(ShaderProgram* p) override { event(1, p->id0); event(2, p->id1); }
    void UnloadShader(ShaderProgram*) override { event(3, 0); }
    void ShaderGetInfo(ShaderProgram* p, uint8_t* count, bool textures[2]) override {
        ++queries;
        *count = p->features.numInputs;
        textures[0] = p->features.usedTextures[0];
        textures[1] = p->features.usedTextures[1];
    }
    void SetSamplerParameters(int s, bool linear, uint32_t cms, uint32_t cmt) override {
        event(4, s | (linear << 8) | (cms << 16) | (cmt << 24));
    }
    void SetDepthTestAndMask(bool a, bool b) override { event(5, a | (b << 1)); }
    void SetZmodeDecal(bool a) override { event(6, a); }
    void SetViewport(int x, int y, int w, int h) override { event(7, x); event(7, y); event(7, w); event(7, h); }
    void SetScissor(int x, int y, int w, int h) override { event(8, x); event(8, y); event(8, w); event(8, h); }
    void SetUseAlpha(bool a) override { event(9, a); }
    void SetCurrentPrimDepth(float z) override { uint32_t bits; memcpy(&bits, &z, 4); event(10, bits); }
    void DrawTriangles(float* data, size_t length, size_t count) override {
        triangles += count;
        event(11, count); event(12, length);
        if (recording) {
            size_t at = output.size();
            output.resize(at + length);
            memcpy(output.data() + at, data, length * sizeof(float));
        }
    }
    void event(uint64_t kind, uint64_t value) { if (recording) { events.push_back(kind); events.push_back(value); } }
};

// Texture pixels/archives are outside this fixture; preserve observable imports
// and sampler-node updates used by triangle submission.
void Interpreter::ImportTexture(int i, int tile, bool replacement) {
    static_cast<RecordingBackend*>(mRapi)->event(13, i | (tile << 8) | (replacement << 16));
}
void Interpreter::ImportTextureMask(int i, int tile) {
    static_cast<RecordingBackend*>(mRapi)->event(14, i | (tile << 8));
}
} // namespace Fast

struct ForestDrawLog {
    std::string layer, path, archive, backend;
    int format, width, height, tile, tmem;
    bool rdpGray, vertexGray, shaderGray;
    uint32_t rgba;
    uint64_t shader0, shader1;
    unsigned sample;
};
namespace Fast {
struct ForestArchiveFixture {
    std::map<std::string, std::shared_ptr<Ship::Archive>> files;
    auto GetArchiveFromFile(const std::string& path) {
        return files[path];
    }
};
struct ForestManagerFixture {
    ForestArchiveFixture archives;
    auto GetArchiveManager() {
        return &archives;
    }
};
static ForestManagerFixture* ActiveResMgr() {
    static ForestManagerFixture manager;
    return &manager;
}
// Archive traversal and read-only Zora logging are outside the triangle/vertex
// fixtures. Production submission, sampler state and metadata classification run.
static void TraceZoraCosmeticDraw(const RawTexMetadata&, const RDP&, bool, uint64_t, uint64_t, uint32_t) {
}
} // namespace Fast
static std::vector<ForestDrawLog> forestDrawLogs;
static void recordForestDraw(const char*, const char* layer, const std::string& path, const std::string& archive,
                             int format, int width, int height, unsigned tile, unsigned tmem, bool rdpGray,
                             bool vertexGray, uint32_t rgba, bool shaderGray, uint64_t shader0, uint64_t shader1,
                             const char* backend, unsigned sample) {
    forestDrawLogs.push_back({ layer, path, archive, backend ? backend : "<none>", format, width, height, int(tile),
                               int(tmem), rdpGray, vertexGray, shaderGray, rgba, shader0, shader1, sample });
}
#define SPDLOG_INFO(...) recordForestDraw(__VA_ARGS__)
#include "triangle_production.inc"

using namespace Fast;

// Compile unchanged against the parent commit to demonstrate the missing reuse.
template<class T> void beginRun(T& gfx, bool enabled) {
    if constexpr (requires { gfx.mTriangleState; gfx.mTriangleStateReuseAllowed; }) {
        gfx.mTriangleState.valid = false;
        gfx.mTriangleStateReuseAllowed = enabled;
    }
}

struct Fixture {
    RecordingBackend backend;
    Interpreter gfx;
    TextureCacheNode textures[2]{};
    Fixture() {
        gfx.mRapi = &backend;
        *gfx.mRsp = {};
        *gfx.mRdp = {};
        gfx.mRdp->other_mode_l = Z_CMP | Z_UPD;
        gfx.mRsp->geometry_mode = G_ZBUFFER;
        gfx.mRdp->combine_mode = uint64_t(G_CCMUX_SHADE) << 13;
        gfx.mRdp->prim_color = {51, 102, 153, 204};
        gfx.mRdp->env_color = {200, 150, 100, 50};
        gfx.mRdp->fog_color = {10, 20, 30, 40};
        gfx.mRdp->blend_color = {40, 30, 20, 10};
        gfx.mRdp->grayscale_color = {100, 120, 130, 140};
        for (unsigned i = 0; i < MAX_VERTICES; ++i) {
            auto& v = gfx.mRsp->loaded_vertices[i];
            v = {float(i % 8) * .125f, float(i / 8) * .125f, .25f, 1.f + float(i % 3),
                 float(i * 17), float(i * 31), {uint8_t(i * 3), uint8_t(i * 5), uint8_t(i * 7), 255}, 0};
        }
        for (int i = 0; i < 2; ++i) {
            gfx.mRenderingState.mTextures[i] = &textures[i];
            gfx.mRdp->texture_tile[i].siz = G_IM_SIZ_16b;
            gfx.mRdp->texture_tile[i].line_size_bytes = 64;
            gfx.mRdp->texture_tile[i].tmem_index = i;
            gfx.mRdp->texture_tile[i].lrs = gfx.mRdp->texture_tile[i].lrt = 124;
            gfx.mRdp->loaded_texture[i].size_bytes = gfx.mRdp->loaded_texture[i].orig_size_bytes = 2048;
            gfx.mRdp->loaded_texture[i].line_size_bytes = 64;
            gfx.mRdp->loaded_texture[i].full_image_line_size_bytes = 64;
        }
    }
    void triangles(unsigned n) {
        for (unsigned t = 0; t < n; ++t) {
            unsigned i = t % 30;
            gfx.GfxSpTri1(i, i + 1, i + 2, false);
        }
    }
};

static void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

static void reuseTest() {
    Fixture f;
    beginRun(f.gfx, true);
    f.triangles(32);
    f.gfx.Flush();
    require(f.backend.triangles == 32, "reuse must preserve all 32 submitted triangles");
    require(f.backend.queries == 1, "unchanged 32-triangle run must query shader layout once");
    std::cout << "PASS unchanged triangle run prepares shader layout once\n";
}

static void configure(Fixture& f, unsigned variant);

template<class T> uint64_t packedMask(const T& gfx) {
    if constexpr (requires { gfx.mTriangleState.packedVertexMask; }) return gfx.mTriangleState.packedVertexMask;
    return 0;
}

static void packedVertexReuseTest() {
    Fixture f;
    configure(f, 3);
    beginRun(f.gfx, true);
    f.gfx.GfxSpTri1(0, 1, 2, false);
    require(packedMask(f.gfx) == 7, "an unchanged triangle run must retain packed attributes for its three indices");
    f.gfx.GfxSpTri1(1, 2, 0, false);
    f.gfx.Flush();
    const auto& output = f.backend.output;
    const size_t stride = output.size() / 6;
    require(memcmp(output.data() + stride, output.data() + stride * 3, stride * sizeof(uint32_t)) == 0,
            "shared vertex must emit the same attributes when it changes position in a triangle");
    // New vertex/material commands reset prepared state. Reusing a stale packed
    // vertex here would corrupt animation or scrolling even when the index matches.
    beginRun(f.gfx, true);
    f.gfx.mRsp->loaded_vertices[0].u += 64;
    f.gfx.mRsp->loaded_vertices[0].color.r = 255;
    f.gfx.GfxSpTri1(0, 1, 2, false);
    f.gfx.Flush();
    Fixture expected;
    configure(expected, 3);
    *expected.gfx.mRsp = *f.gfx.mRsp;
    beginRun(expected.gfx, false);
    expected.gfx.GfxSpTri1(0, 1, 2, false);
    expected.gfx.Flush();
    require(memcmp(f.backend.output.data() + 6 * stride, expected.backend.output.data(),
                   expected.backend.output.size() * sizeof(uint32_t)) == 0,
            "new triangle run must repack a changed vertex");
    std::cout << "PASS shared packed vertices preserve output and reset between runs\n";
}

static void packedFlushAndBoundaryTest() {
    Fixture baseline, optimized;
    for (auto* f : {&baseline, &optimized}) {
        configure(*f, 3);
        beginRun(f->gfx, f == &optimized);
        f->gfx.GfxSpTri1(63, 0, 63, false);
        if (f == &optimized) require(packedMask(f->gfx) == ((uint64_t{1} << 63) | 1),
                                     "index 63 and repeated indices must fit the validity mask");
        f->gfx.Flush();
        require(packedMask(f->gfx) == 0, "explicit flush must invalidate submission-buffer offsets");
        // Preserve prepared material state across this flush, then overwrite the
        // buffer in a new index order. Stale offsets would alias the new output.
        f->gfx.GfxSpTri1(0, 1, 63, false);
        f->gfx.GfxSpTri1(1, 63, 0, false);
        f->gfx.Flush();
        for (unsigned i = 64; i < 68; ++i) f->gfx.mRsp->loaded_vertices[i] = f->gfx.mRsp->loaded_vertices[i-64];
        f->gfx.GfxSpTri1(64, 65, 67, true);
        f->gfx.GfxSpTri1(65, 66, 67, true);
        require(packedMask(f->gfx) == 0, "temporary rectangle vertices must bypass packed reuse");
        f->gfx.Flush();
    }
    require(baseline.backend.output == optimized.backend.output && baseline.backend.events == optimized.backend.events,
            "flush, index-63 repeats and rectangle vertices must preserve exact output");
    std::cout << "PASS packed offsets reset on flush and preserve index/rectangle boundaries\n";
}


static uint64_t combine(unsigned a, unsigned b, unsigned c, unsigned d, unsigned aa, unsigned ab, unsigned ac, unsigned ad) {
    return (a & 15) | ((b & 15) << 4) | ((c & 31) << 8) | ((d & 7) << 13) |
           (uint64_t((aa & 7) | ((ab & 7) << 3) | ((ac & 7) << 6) | ((ad & 7) << 9)) << 16);
}

static void configure(Fixture& f, unsigned variant) {
    auto& r = *f.gfx.mRdp;
    const unsigned material = variant % 5;
    uint64_t shade = combine(15, 15, 31, G_CCMUX_SHADE, 7, 7, 7, G_ACMUX_SHADE);
    uint64_t texture = combine(G_CCMUX_TEXEL0, 15, G_CCMUX_SHADE, 7, G_ACMUX_TEXEL0, 7, G_ACMUX_SHADE, 7);
    uint64_t dual = combine(G_CCMUX_TEXEL0, G_CCMUX_TEXEL1, G_CCMUX_ENVIRONMENT, G_CCMUX_PRIMITIVE,
                            G_ACMUX_TEXEL0, G_ACMUX_TEXEL1, G_ACMUX_PRIMITIVE, G_ACMUX_ENVIRONMENT);
    r.combine_mode = material == 0 ? shade : (material == 1 ? texture : dual);
    r.other_mode_h = (variant & 1 ? G_TF_BILERP : G_TF_POINT);
    if (material >= 3) {
        r.other_mode_h |= G_CYC_2CYCLE;
        r.combine_mode |= combine(G_CCMUX_COMBINED, 15, G_CCMUX_SHADE, 7, G_ACMUX_COMBINED, 7, G_ACMUX_SHADE, 7) << 28;
    }
    r.other_mode_l = Z_CMP | Z_UPD;
    if (variant & 2) r.other_mode_l |= (G_BL_CLR_MEM << 20) | (G_BL_1MA << 16);
    if (variant & 4) r.other_mode_l |= uint32_t(G_BL_CLR_FOG) << 30;
    if (variant & 8) r.other_mode_l = (r.other_mode_l & ~(3u << 30)) | (uint32_t(G_BL_CLR_BL) << 30);
    if (variant & 16) r.other_mode_l |= G_ZS_PRIM | ZMODE_DEC;
    if (variant & 32) r.other_mode_l |= CVG_X_ALPHA;
    r.grayscale = variant & 64;
    r.prim_depth = (variant * 71) % 32768;
    r.prim_lod_fraction = variant % 256;
    r.viewport = {int16_t(variant % 7), int16_t(variant % 11), 640, 480};
    r.scissor = {0, 0, 640, 480};
    r.viewport_or_scissor_changed = true;
    f.gfx.mRsp->geometry_mode = G_ZBUFFER;
    switch ((variant / 128) % 4) {
        case 1: f.gfx.mRsp->geometry_mode |= F3DEX2_G_CULL_FRONT; break;
        case 2: f.gfx.mRsp->geometry_mode |= F3DEX2_G_CULL_BACK; break;
        case 3: f.gfx.mRsp->geometry_mode |= F3DEX2_G_CULL_BOTH; break;
    }
    f.gfx.mRsp->extra_geometry_mode = variant & 512 ? G_EX_INVERT_CULLING : 0;
    f.backend.clip = {bool(variant & 1024), bool(variant & 2048)};
    for (unsigned i = 0; i < MAX_VERTICES; ++i) {
        auto& v = f.gfx.mRsp->loaded_vertices[i];
        v.w = (variant & 4096) && i % 3 == 0 ? -1.f : 1.f + float(i % 3);
        v.clip_rej = (variant & 8192) && i < 10 ? 1 : 0;
    }
    for (unsigned i = 0; i < 2; ++i) {
        auto& tile = r.texture_tile[i];
        tile.uls = float(variant % 16) * 1.25f;
        tile.ult = float(variant % 32) * 1.5f;
        tile.lrs = tile.uls + (variant & 64 ? 60 : 124);
        tile.lrt = tile.ult + 124;
        tile.cms = (variant / 4) % 4;
        tile.cmt = (variant / 16) % 4;
        tile.masks = variant & 8 ? 4 : 0;
        tile.maskt = variant & 16 ? 5 : 0;
        tile.shifts = (variant + i) % 16;
        tile.shiftt = (variant / 16 + i) % 16;
        r.textures_changed[i] = true;
        r.loaded_texture[i].masked = variant & 32;
        r.loaded_texture[i].blended = variant & 64;
        r.loaded_texture[i].raw_tex_metadata.h_byte_scale = variant & 128 ? 2.f : 1.f;
    }
}

static void equalOutput(const Fixture& a, const Fixture& b) {
    require(a.backend.output == b.backend.output, "reuse must preserve bitwise vertex output");
    require(a.backend.events == b.backend.events, "reuse must preserve backend draw/state event order");
    require(a.backend.triangles == b.backend.triangles, "reuse must preserve submitted triangle count");
}

static uint64_t digest(const std::vector<uint32_t>& data) {
    uint64_t result = 1469598103934665603ull;
    for (auto bits : data) { result ^= bits; result *= 1099511628211ull; }
    return result;
}

static void equivalenceTest() {
    Fixture baseline, optimized;
    for (unsigned variant = 0; variant < 16384; ++variant) {
        configure(baseline, variant);
        configure(optimized, variant);
        beginRun(baseline.gfx, false);
        beginRun(optimized.gfx, true);
        unsigned count = variant % 127 == 0 ? 600 : 17;
        baseline.triangles(count);
        optimized.triangles(count);
        baseline.gfx.Flush(); optimized.gfx.Flush();
        equalOutput(baseline, optimized);
        // Comparison above is exact; retain a compact reproducible parent-build digest below.
        if (variant % 1024 == 1023) {
            std::cout << "output " << variant << " " << std::hex << digest(optimized.backend.output) << std::dec << '\n';
            baseline.backend.output.clear(); optimized.backend.output.clear();
            baseline.backend.events.clear(); optimized.backend.events.clear();
        }
    }
    std::cout << "PASS 16384 state/clip/cull/texture variants; bitwise vertices and backend events identical\n";
}

template<class Table> bool dispatch(const Table& table, int8_t opcode, F3DGfx** cmd, Interpreter* gfx) {
    if constexpr (requires { table.invoke(opcode, cmd, gfx); }) return table.invoke(opcode, cmd, gfx);
    else return table.at(opcode).second(cmd);
}
template<class T> void enable(T& gfx, bool enabled) {
    if constexpr (requires { gfx.mTriangleStateReuseEnabled; }) gfx.mTriangleStateReuseEnabled = enabled;
}
#ifdef GFX_EXPLICIT_DISPATCH
#define HANDLER_CONTEXT Interpreter* gfx,
#define HANDLER_INSTANCE
#else
#define HANDLER_CONTEXT
#define HANDLER_INSTANCE auto gfx = mInstance.lock();
#endif
static bool changeColor(HANDLER_CONTEXT F3DGfx**) {
    HANDLER_INSTANCE
    gfx->mRdp->prim_color.r += 21;
    return false;
}
static bool temporaryDraw(HANDLER_CONTEXT F3DGfx**) {
    HANDLER_INSTANCE
    auto saved = gfx->mRdp->other_mode_l;
    gfx->mRdp->other_mode_l ^= G_ZS_PRIM | ZMODE_DEC;
    gfx->GfxSpTri1(1, 3, 2, true);
    gfx->mRdp->other_mode_l = saved;
    return false;
}

// Commands must operate on the interpreter executing this stream, even if the
// externally selected interpreter changes. Reacquiring the global weak pointer
// both violates that contract and adds atomic ownership work to every command.
static void dispatchContextTest() {
    constexpr UcodeHandler table = {{1, {"tri", gfx_tri2_handler_f3dex}},
                                   {2, {"depth", gfx_set_prim_depth_handler_rdp}}};
    Fixture selected, executing;
    auto selectedOwner = std::shared_ptr<Interpreter>(&selected.gfx, [](Interpreter*) {});
    mInstance = selectedOwner;
    F3DGfx cmd{};
    auto ptr = &cmd;
    cmd.words.w1 = 0x12340000;
    dispatch(table, 2, &ptr, &executing.gfx);
    require(executing.gfx.mRdp->prim_depth == 0x1234,
            "command must update the executing interpreter, not the selected global interpreter");
    require(selected.gfx.mRdp->prim_depth == 0, "dispatch must leave the other interpreter untouched");
    cmd.words.w0 = (1 << 9) | (2 << 1);
    cmd.words.w1 = (1 << 17) | (3 << 9) | (2 << 1);
    dispatch(table, 1, &ptr, &executing.gfx);
    executing.gfx.Flush();
    require(executing.backend.triangles == 2 && selected.backend.triangles == 0,
            "triangle dispatch must use the executing backend");
    std::cout << "PASS command dispatch stays with its executing interpreter\n";
}
static void dispatchTest() {
    const UcodeHandler table = {{1, {"tri", gfx_tri2_handler_f3dex}},
                               {2, {"change", changeColor}}, {3, {"temporary", temporaryDraw}}};
    Fixture baseline, optimized;
    for (auto* f : {&baseline, &optimized}) {
        auto owner = std::shared_ptr<Interpreter>(&f->gfx, [](Interpreter*) {});
        mInstance = owner;
        configure(*f, 2);
        enable(f->gfx, f == &optimized);
        F3DGfx cmd{};
        cmd.words.w0 = (0 << 17) | (1 << 9) | (2 << 1);
        cmd.words.w1 = (1 << 17) | (3 << 9) | (2 << 1);
        auto ptr = &cmd;
        for (int i : {1, 1, 2, 1, 1, 3, 1, 1}) dispatch(table, i, &ptr, &f->gfx);
        f->gfx.Flush();
    }
    equalOutput(baseline, optimized);
    require(optimized.backend.queries == 4, "two state barriers must split triangle preparation into three runs plus temporary draw");
    std::cout << "PASS production dispatcher invalidates before/after state and temporary rectangle draws\n";
}

static void sharedSamplerTest() {
    Fixture baseline, optimized;
    for (auto* f : {&baseline, &optimized}) {
        configure(*f, 3);
        f->gfx.mRenderingState.mTextures[1] = f->gfx.mRenderingState.mTextures[0];
        f->gfx.mRdp->texture_tile[0].cms = G_TX_WRAP;
        f->gfx.mRdp->texture_tile[1].cms = G_TX_MIRROR;
        beginRun(f->gfx, f == &optimized);
        f->triangles(32);
        f->gfx.Flush();
    }
    equalOutput(baseline, optimized);
    std::cout << "PASS shared texture nodes preserve sampler transitions and flushes\n";
}

#ifdef HAS_PRELUDE_MATERIAL_BINDING_REUSE
static void materialBindingReuseTest() {
    Fixture f;
    configure(f, 1);
    auto& loaded = f.gfx.mRdp->loaded_texture[0];
    std::vector<uint8_t> pixels(2048);
    auto resource = std::make_shared<Texture>();
    resource->ImageData = pixels.data();
    resource->ImageDataSize = pixels.size();
    resource->init->Path = "alt/arbitrary-scene/water";
    loaded.addr = pixels.data();
    loaded.raw_tex_metadata.resource = resource;
    loaded.raw_tex_metadata.type = TextureType::RGBA32bpp;
    const TextureCacheKey key = {loaded.addr, {}, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, loaded.orig_size_bytes};
    auto entry = f.gfx.mTextureCache.map.emplace(key, TextureCacheValue()).first;
    entry->second.lru_location = f.gfx.mTextureCache.lru.insert(f.gfx.mTextureCache.lru.end(), {entry});
    f.gfx.mRenderingState.mTextures[0] = &*entry;
    f.gfx.mPreludeMaterialBoundResources[0] = resource.get();
    const bool used[2] = {true, false};
    require(!f.gfx.TryReusePreludeMaterialBinding(0, 0, used), "material reuse stays opt-in");
    f.gfx.mPreludeMaterialBindingReuseEnabled = true;
    require(f.gfx.TryReusePreludeMaterialBinding(0, 0, used), "actual helper reuses a compatible live binding");
    f.gfx.mRdp->textures_changed[0] = true;
    beginRun(f.gfx, false);
    f.triangles(1);
    for (size_t i = 0; i < f.backend.events.size(); i += 2)
        require(f.backend.events[i] != 13, "triangle submission must avoid import for the compatible unchanged binding");
    loaded.masked = true;
    require(!f.gfx.TryReusePreludeMaterialBinding(0, 0, used), "masked materials retain the import path");
    loaded.masked = false;
    f.gfx.mRenderingState.mTextures[0] = nullptr;
    require(!f.gfx.TryReusePreludeMaterialBinding(0, 0, used), "retired bindings must not reuse a live cache key");
    std::cout << "PASS actual material binding helper: opt-in, live cache, triangle reuse and fallback guards\n";
}
#endif

static void frameResetTest() {
    constexpr UcodeHandler table = {{1, {"tri", gfx_tri2_handler_f3dex}}};
    Fixture baseline, optimized;
    for (auto* f : {&baseline, &optimized}) {
        auto owner = std::shared_ptr<Interpreter>(&f->gfx, [](Interpreter*) {});
        mInstance = owner;
        enable(f->gfx, f == &optimized);
        F3DGfx cmd{};
        cmd.words.w0 = (0 << 17) | (1 << 9) | (2 << 1);
        cmd.words.w1 = (1 << 17) | (3 << 9) | (2 << 1);
        auto ptr = &cmd;
        for (unsigned frame = 0; frame < 3; ++frame) {
            configure(*f, 3 + 1024 * frame);
            f->gfx.mRsp->modelview_matrix_stack[0][0][0] = 1;
            f->gfx.mRsp->modelview_matrix_stack[0][1][1] = 1;
            f->gfx.mRsp->modelview_matrix_stack[0][2][2] = 1;
            f->gfx.SpReset();
            dispatch(table, 1, &ptr, &f->gfx);
            dispatch(table, 1, &ptr, &f->gfx);
            f->gfx.Flush();
        }
    }
    equalOutput(baseline, optimized);
    require(optimized.backend.queries == 3, "each frame must prepare fresh state after SpReset");
    std::cout << "PASS frame reset invalidates prepared state across clip/material changes\n";
}

static void literalVertexTest() {
    Fixture f;
    beginRun(f.gfx, true);
    f.gfx.GfxSpTri1(0, 1, 2, false);
    f.gfx.Flush();
    const float expected[] = {0, 0, .625f, 1, 0, 0, 0,
                              .125f, 0, 1.125f, 2, 3/255.f, 5/255.f, 7/255.f,
                              .25f, 0, 1.625f, 3, 6/255.f, 10/255.f, 14/255.f};
    require(f.backend.output.size() == std::size(expected), "shade-only triangle must have seven floats per vertex");
    require(memcmp(f.backend.output.data(), expected, sizeof(expected)) == 0,
            "triangle must match hand-derived position/depth/color fixture");
    std::cout << "PASS hand-derived vertex positions, D3D depth conversion and colors\n";
}

static void lodInputTest() {
    Fixture baseline, optimized;
    for (auto* f : {&baseline, &optimized}) {
        auto& r = *f->gfx.mRdp;
        r.combine_mode = combine(G_CCMUX_PRIMITIVE, 15, G_CCMUX_LOD_FRACTION, 7,
                                  G_ACMUX_PRIMITIVE, 7, G_ACMUX_LOD_FRACTION, 7);
        r.other_mode_l = G_TL_LOD | (G_BL_CLR_MEM << 20) | (G_BL_1MA << 16);
        r.prim_color = {255, 255, 255, 255};
        r.prim_lod_fraction = 91;
        f->gfx.mRsp->loaded_vertices[0].w = 1500;
        f->gfx.mRsp->loaded_vertices[1].w = 4500;
        f->gfx.mRsp->loaded_vertices[2].w = 7500;
        beginRun(f->gfx, f == &optimized);
        f->gfx.GfxSpTri1(0, 1, 2, false);
        f->gfx.GfxSpTri1(1, 2, 0, false);
        f->gfx.GfxSpTri1(2, 0, 1, false);
        f->gfx.Flush();
    }
    equalOutput(baseline, optimized);
    const auto& data = optimized.backend.output;
    const size_t stride = data.size() / 9;
    // LOD is the second shader input: the first vertex's W determines it
    // for all three vertices of that triangle, even when indices are shared.
    const float want[] = {0.f, 127.f / 255.f, 1.f};
    for (size_t tri = 0; tri < 3; ++tri) {
        for (size_t v = 0; v < 3; ++v) {
            float got;
            memcpy(&got, &data[(tri * 3 + v) * stride + 8], sizeof(got));
            require(got == want[tri], "LOD input must follow first-vertex depth on each triangle");
        }
    }
    require(optimized.backend.queries == 1, "varying per-triangle LOD must not require shader preparation");
    std::cout << "PASS triangle-dependent LOD with shared/reordered vertex indices\n";
}

static void benchmark() {
    constexpr unsigned batches = 7680, perBatch = 16;
    for (unsigned material : {0u, 1u, 3u, 15u, 127u}) {
        std::vector<double> times[2];
        for (int repeat = 0; repeat < 11; ++repeat) {
            for (int pass = 0; pass < 2; ++pass) {
                const int mode = (pass + repeat) % 2;
                Fixture f;
                configure(f, material);
                f.backend.recording = false;
                auto start = std::chrono::steady_clock::now();
                for (unsigned batch = 0; batch < batches; ++batch) {
                    beginRun(f.gfx, mode);
                    f.triangles(perBatch);
                }
                f.gfx.Flush();
                auto end = std::chrono::steady_clock::now();
                require(f.backend.triangles == batches * perBatch, "benchmark must submit every triangle");
                if (repeat) times[mode].push_back(std::chrono::duration<double, std::milli>(end-start).count());
            }
        }
        std::sort(times[0].begin(), times[0].end()); std::sort(times[1].begin(), times[1].end());
        auto off = times[0][times[0].size()/2], on = times[1][times[1].size()/2];
        std::cout << "BENCH " << material << " triangles=" << batches * perBatch << " off_ms=" << off
                  << " on_ms=" << on << " saved_pct=" << 100*(off-on)/off << '\n';
    }
}

static void commandBenchmark() {
    const UcodeHandler table = {{1, {"tri", gfx_tri2_handler_f3dex}},
                               {2, {"depth", gfx_set_prim_depth_handler_rdp}},
                               {3, {"key-r", gfx_set_key_r_handler_rdp}},
                               {4, {"key-gb", gfx_set_key_gb_handler_rdp}}};
    // Synthetic mixes approximate command/triangle counts, not scene replays.
    for (auto shape : {std::array<unsigned, 3>{7680, 8, 10}, {6250, 6, 25}}) {
      for (bool shared : {false, true}) {
        std::vector<double> times;
        for (unsigned repeat = 0; repeat < 16; ++repeat) {
            Fixture f;
            configure(f, 3);
            enable(f.gfx, true);
            f.backend.recording = false;
            auto owner = std::shared_ptr<Interpreter>(&f.gfx, [](Interpreter*) {});
            mInstance = owner;
            F3DGfx triangle{}, state{};
            triangle.words.w0 = (1 << 9) | (2 << 1);
            triangle.words.w1 = (1 << 17) | (3 << 9) | (2 << 1);
            state.words.w1 = 0x12345678;
            auto start = std::chrono::steady_clock::now();
            for (unsigned batch = 0; batch < shape[0]; ++batch) {
                for (unsigned s = 0; s < shape[2]; ++s) {
                    auto ptr = &state;
                    dispatch(table, 2 + s % 3, &ptr, &f.gfx);
                }
                for (unsigned t = 0; t < shape[1]; ++t) {
                    const unsigned first = shared ? 0 : t * 6;
                    triangle.words.w0 = (first << 17) | ((first + 1) << 9) | ((first + 2) << 1);
                    triangle.words.w1 = ((first + 1) << 17) | ((first + 3) << 9) | ((first + 2) << 1);
                    if (!shared) triangle.words.w1 = ((first + 3) << 17) | ((first + 4) << 9) | ((first + 5) << 1);
                    auto ptr = &triangle;
                    dispatch(table, 1, &ptr, &f.gfx);
                }
            }
            f.gfx.Flush();
            auto end = std::chrono::steady_clock::now();
            require(f.backend.triangles == shape[0] * shape[1] * 2, "command workload must draw every triangle");
            if (repeat) times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        std::sort(times.begin(), times.end());
        std::cout << "COMMAND_BENCH commands=" << shape[0] * (shape[1] + shape[2])
                  << " triangles=" << shape[0] * shape[1] * 2 << " shared=" << shared
                  << " median_ms=" << times[times.size()/2] << '\n';
      }
    }
}

static void grayscaleBrightnessTest() {
    constexpr UcodeHandler table = { { 1, { "gray", gfx_set_grayscale_handler_custom } } };
    for (const RGBA color : { RGBA{ 185, 104, 72, 140 }, RGBA{ 255, 159, 145, 255 }, RGBA{ 185, 205, 217, 176 },
                              RGBA{ 196, 210, 201, 176 }, RGBA{ 190, 210, 194, 176 }, RGBA{ 0, 0, 0, 255 } }) {
        Fixture f;
        configure(f, 1);
        enable(f.gfx, true);
        f.gfx.mRdp->grayscale_color = color;
        F3DGfx cmd{};
        auto ptr = &cmd;
        // Switching mode with the same vertices must invalidate packed reuse,
        // even when the shader and its attribute layout stay the same.
        constexpr unsigned modes[] = { 3u, 2u, 1u, 3u, 1u, 2u };
        for (unsigned mode : modes) {
            cmd.words.w1 = mode;
            dispatch(table, 1, &ptr, &f.gfx);
            f.gfx.mTriangleStateReuseAllowed = true;
            f.gfx.GfxSpTri1(0, 1, 2, false);
        }
        f.gfx.Flush();
        const auto stride = f.backend.output.size() / 18;
        const unsigned sum = unsigned(color.r) + color.g + color.b;
        for (unsigned draw = 0; draw < 6; ++draw) {
            float packed[4];
            std::memcpy(packed, f.backend.output.data() + draw * 3 * stride + 6, sizeof(packed));
            require(std::isfinite(packed[0]) && std::isfinite(packed[1]) && std::isfinite(packed[2]),
                    "black normalized tint must produce finite attributes");
            require(packed[3] == color.a / 255.f, "brightness normalization must preserve the interpolation factor");
            if (modes[draw] == 1) {
                require(packed[0] == color.r / 255.f && packed[1] == color.g / 255.f && packed[2] == color.b / 255.f,
                        "legacy grayscale mode must retain its exact vertex attributes after normalized tint");
            } else if (sum) {
                const float mean = (packed[0] + packed[1] + packed[2]) / 3.f;
                const float expectedMean = modes[draw] == 3 ? 1.8f : 1.f;
                require(std::abs(mean - expectedMean) < 0.000001f,
                        "winter frost must brighten dark texels; returning to autumn must preserve mean intensity");
                require(std::abs(packed[0] / packed[1] - float(color.r) / color.g) < 0.000001f,
                        "brightness normalization must preserve the chosen warm hue");
            } else {
                require(packed[0] == 0 && packed[1] == 0 && packed[2] == 0,
                        "black tint must remain black in normalized mode");
            }
        }
        cmd.words.w1 = 0;
        dispatch(table, 1, &ptr, &f.gfx);
        require(!f.gfx.mRdp->grayscale, "the scoped Off command must disable normalized tint");
    }
    std::cout << "PASS winter frost lift/autumn intensity: actual vertex submission, legacy mode, black guard and mode "
                 "barriers\n";
}

static void forestTintTraceTest() {
    Fixture f;
    configure(f, 1);
    auto& r = *f.gfx.mRdp;
    f.gfx.GfxDpSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, reinterpret_cast<const char*>(uintptr_t{ 1 }), 0, {},
                               nullptr);
    require(!r.texture_to_load.raw_tex_metadata.resource,
            "forest diagnostics must not inspect raw N64 pixel pointers as resource names");
    r.first_tile_index = 2;
    r.texture_tile[2] = r.texture_tile[0];
    r.texture_tile[2].tmem_index = 1;
    const std::string root = "scenes/nonmq/Z2_00KEIKOKU/";
    auto texture = std::make_shared<Texture>();
    texture->init->Path = root + "Z2_00KEIKOKUTex_034098";
    texture->init->Parent = std::make_shared<Ship::Archive>();
    texture->init->Parent->path = "./mm.o2r";
    r.loaded_texture[1].raw_tex_metadata.resource = texture;
    r.grayscale = true;
    r.grayscale_color = { 185, 104, 72, 255 };
    f.gfx.GfxSpTri1(0, 1, 2, false);
    f.gfx.Flush();
    require(forestDrawLogs.size() == 1, "forest triangle submission must report the actual prepared shader state");
    const auto first = forestDrawLogs.back();
    require(first.layer == "wallpaper" && first.path == texture->init->Path && first.archive == "./mm.o2r",
            "forest trace must identify the selected wallpaper resource and its actual archive");
    require(first.tile == 2 && first.tmem == 1 && first.format == 4 && first.width == 64 && first.height == 32,
            "forest trace must follow the render tile into the correct TMEM slot");
    require(first.rdpGray && first.vertexGray && first.shaderGray && first.rgba == 0xB96848FF,
            "forest trace must report the copper palette and both vertex/shader tint flags");
    bool selectedShader = false;
    for (const auto& [key, program] : f.backend.programs)
        selectedShader |=
            program.id0 == first.shader0 && program.id1 == first.shader1 && program.features.opt_grayscale;
    require(selectedShader, "forest trace shader IDs must identify the prepared backend shader");
    float packedRed;
    std::memcpy(&packedRed, &f.backend.output[6], sizeof(packedRed));
    require(packedRed == 185.f / 255.f, "reported copper red must reach submitted vertex attributes");
    for (unsigned i = 0; i < 100; ++i)
        f.gfx.GfxSpTri1(0, 1, 2, false);
    require(forestDrawLogs.size() == 1, "unchanged forest draws must not spam the log");
    r.grayscale = false;
    f.gfx.GfxSpTri1(0, 1, 2, false);
    require(forestDrawLogs.size() == 2 && !forestDrawLogs.back().vertexGray && !forestDrawLogs.back().shaderGray,
            "forest trace must expose a draw that reaches the backend without tint");
    texture->init->Path = "alt/scenes/nonmq/Z2_00KEIKOKU_scene/Z2_00KEIKOKUTex_037098";
    texture->init->Parent->path = "mods/2ship/Termina-POC4.o2r";
    texture->Type = TextureType::RGBA32bpp;
    texture->Width = 1024;
    texture->Height = 512;
    f.gfx.GfxSpTri1(0, 1, 2, false);
    require(forestDrawLogs.size() == 3 && forestDrawLogs.back().layer == "canopy" &&
                forestDrawLogs.back().archive == texture->init->Parent->path && forestDrawLogs.back().format == 1 &&
                forestDrawLogs.back().width == 1024 && forestDrawLogs.back().height == 512,
            "forest trace must also identify Alt canopy resources in the generated scene namespace");
    texture->init->Path = "objects/unrelated/Z2_00KEIKOKUTex_034098";
    f.gfx.GfxSpTri1(0, 1, 2, false);
    require(forestDrawLogs.size() == 3, "matching filenames outside Termina Field must remain quiet");
    // A real .meta alias records the payload path, not the requested native name,
    // and ResourceLoader does not populate ResourceInitData::Parent.
    texture->init->Path = "foliage/private_forest_wall.png";
    texture->init->Parent.reset();
    auto owner = std::make_shared<Ship::Archive>();
    owner->path = "mods/2ship/Termina-POC4.o2r";
    ActiveResMgr()->archives.files[texture->init->Path] = owner;
    const auto requested = root + "Z2_00KEIKOKUTex_034098";
    f.gfx.GfxDpSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_32b, 1024, requested.c_str(), 0,
                               r.loaded_texture[1].raw_tex_metadata, nullptr);
    r.loaded_texture[1].raw_tex_metadata = r.texture_to_load.raw_tex_metadata;
    f.gfx.GfxSpTri1(0, 1, 2, false);
    require(forestDrawLogs.size() == 4 && forestDrawLogs.back().layer == "wallpaper" &&
                forestDrawLogs.back().path == texture->init->Path && forestDrawLogs.back().archive == owner->path,
            "forest trace must follow requested texture identity through a private .meta payload and missing Parent");
    f.gfx.GfxDpSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_32b, 1024, nullptr, 0, r.loaded_texture[1].raw_tex_metadata,
                               nullptr);
    r.loaded_texture[1].raw_tex_metadata = r.texture_to_load.raw_tex_metadata;
    r.grayscale = true;
    for (const auto* suffix : { "034098", "037098" }) {
        texture->init->Path = root + "Z2_00KEIKOKUTex_" + suffix;
        for (unsigned i = 0; i < 64; ++i) {
            r.grayscale_color.r = uint8_t(i);
            f.gfx.GfxSpTri1(0, 1, 2, false);
        }
    }
    const auto wallpaper = std::count_if(forestDrawLogs.begin(), forestDrawLogs.end(),
                                         [](const auto& x) { return x.layer == "wallpaper"; });
    const auto canopy = forestDrawLogs.size() - wallpaper;
    require(wallpaper == 16 && canopy == 16, "forest logging must stop independently at 16 samples per layer");
    std::cout << "PASS actual forest triangle/shader trace: selected TMEM resource, archive, palette, shader flags, "
                 "Alt and bounded output\n";
}

int main(int argc, char** argv) {
    bool reference = false, bench = false;
    for (int i = 1; i < argc; ++i) {
        reference |= std::string(argv[i]) == "--reference";
        bench |= std::string(argv[i]) == "--benchmark";
    }
    if (bench) { benchmark(); commandBenchmark(); return 0; }
    if (!reference) {
        grayscaleBrightnessTest();
        forestTintTraceTest();
    }
    if (!reference) { dispatchContextTest(); packedVertexReuseTest(); packedFlushAndBoundaryTest(); }
    if (!reference) reuseTest();
#ifdef HAS_PRELUDE_MATERIAL_BINDING_REUSE
    if (!reference) materialBindingReuseTest();
#endif
    equivalenceTest();
    literalVertexTest();
    if (!reference) { dispatchTest(); sharedSamplerTest(); frameResetTest(); lodInputTest(); }
}
