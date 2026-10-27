// Run the native forest through production call/branch, hash resolution, color
// and triangle handlers. Archive services and triangle rasterization are seams;
// the renderer's writes into the executed GBI are deliberately NOT substituted.
#include <limits>
#include <stack>
#include <unordered_set>
#define SPDLOG_TRACE(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
namespace Ship {
#include "autumn_scene_renderer_signature.inc"
}
namespace ForestRendererFixture {
using F3DGfx = Gfx;
using F3DVtx = Vtx;
struct Override {
    size_t execDepthAtPush;
};
static std::vector<Override> g_crossRMStack;
static unsigned g_skippedBracketPushes = 0;
static constexpr size_t kBracketSentinel = std::numeric_limits<size_t>::max();
struct GfxExecStack {
    std::stack<F3DGfx*> cmd_stack;
    std::vector<const F3DGfx*> gfx_path;
    std::vector<int> disp_stack;
    void start(F3DGfx*);
    void stop();
    F3DGfx*& currCmd();
    void branch(F3DGfx*);
    void call(F3DGfx*, F3DGfx*);
    F3DGfx* ret();
} g_exec_stack;
struct Palette {
    uint8_t r, g, b, a;
};
struct Rdp {
    bool grayscale = false;
    bool grayscale_normalize = false;
    bool grayscale_frost = false;
    Palette grayscale_color{};
};
struct RawTexMetadata {
    unsigned width, height;
    float h_byte_scale, v_pixel_scale;
    Fast::TextureType type;
    std::shared_ptr<Fast::Texture> resource;
};
struct Stats {
    unsigned triangles = 0, tinted = 0, vertexLoads = 0;
    std::vector<uintptr_t> vertexPointers;
    std::vector<uint32_t> palettes;
    std::vector<unsigned> tintModes;
    std::vector<std::shared_ptr<Fast::Texture>> textures;
    std::map<std::string, std::pair<unsigned, unsigned>> materials; // total, tinted
};
struct Interpreter {
    Rdp rdp;
    Rdp* mRdp = &rdp;
    bool mMarkerOn = false;
    uintptr_t mSegmentPointers[16]{};
    std::string texture;
    Stats stats;
    void* SegAddr(uintptr_t);
    void GfxDpSetGrayscaleColor(uint8_t, uint8_t, uint8_t, uint8_t);
    void GfxSpVertex(unsigned, unsigned, const F3DVtx* vertices) {
        ++stats.vertexLoads;
        stats.vertexPointers.push_back(reinterpret_cast<uintptr_t>(vertices));
    }
    void GfxDpSetTextureImage(unsigned, unsigned, unsigned, const char* name, uint32_t, RawTexMetadata metadata,
                              void* pixels) {
        texture = name;
        assert(pixels == metadata.resource->ImageData);
        stats.textures.push_back(metadata.resource);
    }
    void GfxSpTri1(unsigned, unsigned, unsigned, bool) {
        assert(!rdp.grayscale_frost || rdp.grayscale_normalize);
        ++stats.triangles;
        auto& material = stats.materials[texture];
        ++material.first;
        const auto color = rdp.grayscale_color;
        stats.palettes.push_back(
            rdp.grayscale ? uint32_t(color.r) << 24 | uint32_t(color.g) << 16 | uint32_t(color.b) << 8 | color.a : 0);
        stats.tintModes.push_back(!rdp.grayscale ? 0 : rdp.grayscale_frost ? 3 : rdp.grayscale_normalize ? 2 : 1);
        if (rdp.grayscale && color.r == 239 && color.g == 72 && color.b == 76 && color.a == 255) {
            ++stats.tinted;
            ++material.second;
        }
    }
};
struct Manager {
    alignas(16) F3DVtx vertices[128]{};
    alignas(16) F3DVtx refreshedVertices[128]{};
    uint8_t pixels[2][4]{};
    bool alt = false;
    bool refreshed = false;
    std::map<uint64_t, std::string> paths;
    Manager() {
        for (const char* suffix : { "Tex_034098", "Tex_037098", "TLUT_038378" }) {
            const std::string path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU" + std::string(suffix);
            paths[CRC64(path.c_str())] = path;
        }
    }
    Manager* GetArchiveManager() {
        return this;
    }
    bool OtrSignatureCheck(const char* path) {
        return Ship::Context::GetRawInstance()->rm->OtrSignatureCheck(path);
    }
    const char* HashToCString(uint64_t hash) {
        const auto i = paths.find(hash);
        return i == paths.end() ? nullptr : i->second.c_str();
    }
    void* GetResourceRawPointer(uint64_t) {
        return refreshed ? refreshedVertices : vertices;
    }
    std::shared_ptr<Ship::IResource> LoadResourceProcess(const char* path) {
        const auto rm = Ship::Context::GetRawInstance()->rm;
        for (const auto& name : { (rm->alt ? "alt/" : "") + std::string(path), std::string(path) }) {
            const auto it = rm->resources.find(name);
            if (it != rm->resources.end())
                return it->second;
        }
        auto texture = std::make_shared<Fast::Texture>();
        texture->ImageData = pixels[alt];
        return texture;
    }
} manager;
static Manager* ActiveResMgr() {
    return &manager;
}
static void ReportTextureLoadFailure(const char*, const char*, const char*, uint64_t, uintptr_t) {
    assert(false && "The native forest texture binding must resolve");
}
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << width) - 1))
#include "autumn_scene_renderer_production.inc"
#undef C0
#undef C1
static Stats Draw(std::vector<Gfx>& root) {
    Interpreter gfx;
    g_exec_stack.start(root.data());
    unsigned steps = 0;
    while (!g_exec_stack.cmd_stack.empty()) {
        assert(++steps < 2000);
        auto& cmd = g_exec_stack.currCmd();
        bool jumped = false;
        switch (cmd->words.w0 >> 24) {
            case G_DL:
                jumped = gfx_dl_handler_common(&gfx, &cmd);
                break;
            case G_ENDDL:
                jumped = gfx_end_dl_handler_common(&gfx, &cmd);
                break;
            case G_MARKER:
                gfx_marker_handler_otr(&gfx, &cmd);
                break;
            case G_VTX_OTR_HASH:
                gfx_vtx_hash_handler_custom(&gfx, &cmd);
                break;
            case G_SETTIMG_OTR_HASH:
                gfx_set_timg_otr_hash_handler_custom(&gfx, &cmd);
                break;
            case G_SETGRAYSCALE:
                gfx_set_grayscale_handler_custom(&gfx, &cmd);
                break;
            case G_SETINTENSITY:
                gfx_set_intensity_handler_custom(&gfx, &cmd);
                break;
            case G_TRI2:
                gfx_quad_handler_f3dex2(&gfx, &cmd);
                break;
            case G_TRI1:
                gfx_tri1_handler_f3dex2(&gfx, &cmd);
                break;
        }
        if (!jumped)
            ++cmd;
    }
    assert(!gfx.rdp.grayscale && !gfx.rdp.grayscale_normalize);
    return gfx.stats;
}
static void CheckDraw(const Stats& stats, bool tinted, const std::string& phase = {}) {
    const auto wall = stats.materials.at("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_034098");
    const auto canopy = stats.materials.at("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_037098");
    if (stats.tinted != (tinted ? 22u : 0u)) {
        std::fprintf(stderr,
                     "FAIL rendered forest: wall %u/14 and canopy %u/8 Scarlet triangles after "
                     "untinted renderer pointer writebacks\n",
                     wall.second, canopy.second);
    }
    assert(stats.triangles == 22 && stats.vertexLoads == 3);
    const auto base = reinterpret_cast<uintptr_t>(manager.GetResourceRawPointer(0));
    const std::vector<uintptr_t> expectedVertices = { base + 0x250, base, base + 0x150 };
    if (stats.vertexPointers != expectedVertices)
        std::fprintf(stderr,
                     "FAIL forest draw [%s]: tinted copy retained old vertex addresses after source hash resolution\n",
                     phase.c_str());
    assert(stats.vertexPointers == expectedVertices);
    assert(wall.first == 14 && wall.second == (tinted ? 14u : 0u));
    assert(canopy.first == 8 && canopy.second == (tinted ? 8u : 0u));
}
static unsigned ReportCount(const char* prefix) {
    unsigned count = 0;
    for (const auto& [format, calls] : sForestReports) {
        if (format.starts_with(prefix))
            count += calls;
    }
    return count;
}
} // namespace ForestRendererFixture

static void CheckRenderedForestTransitions() {
    using namespace ForestRendererFixture;
    // ArchiveManager::LoadFile(hash) inserts a null archive for a missing Alt.
    // HasFile then reports the key as present, even though native fallback is
    // the resource that actually rendered. A null .meta entry behaves likewise.
    for (const char* missingSuffix : { "", ".meta" }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = manager.alt = true;
        const std::string path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0241B0";
        const std::string missingAlt = "alt/" + path + missingSuffix;
        auto forest = std::make_shared<Fast::DisplayList>();
        forest->Instructions = kActualForestCommands;
        const auto original = forest->Instructions;
        rm->archive->files[path] = forest;
        rm->resources[path] = forest;
        rm->archive->files[missingAlt] = nullptr;
        assert(rm->archive->HasFile(missingAlt));
        assert(rm->archive->GetArchiveFromFile(missingAlt) == nullptr);
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        CheckDraw(Draw(forest->Instructions), true, std::string("missing-Alt") + missingSuffix);
        assert(rm->loads == 0); // Do not retry the known missing Alt every frame.
        season = SEASON_OFF;
        MMAutumnSceneFoliage_Update(&play);
        CheckDraw(Draw(forest->Instructions), false);
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        CheckDraw(Draw(forest->Instructions), true);
        ClearCache();
        assert(SameCommand(forest->Instructions[0], original[0]));
    }
    std::puts("PASS missing Alt/null archive and .meta entries retain the rendered native forest, without reloads");
    // A list already rendered before its first seasonal snapshot is a useful
    // control: the unpatched code can work when its pointer words stay stable.
    {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0241B0";
        auto forest = std::make_shared<Fast::DisplayList>();
        forest->Instructions = kActualForestCommands;
        CheckDraw(Draw(forest->Instructions), false);
        rm->archive->files[path] = forest;
        rm->resources[path] = forest;
        // The requested asset can exist while a winning .meta alias loads a
        // different resource path. Diagnostics must look up the loaded target.
        const std::string aliasTarget = "alt/scenes/nonmq/Z2_00KEIKOKU/forest_owner_override";
        forest->initData = std::make_shared<Ship::ResourceInitData>();
        forest->initData->Path = aliasTarget;
        rm->archive->files[aliasTarget] = forest;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        for (const int selected : { SEASON_AUTUMN, SEASON_OFF, SEASON_AUTUMN }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), selected == SEASON_AUTUMN);
            assert(rm->archive->archiveLookup == aliasTarget);
        }
        ClearCache();
    }
    for (const char* namespaceSuffix : { "", "_scene" }) {
        for (bool alt : { false, true }) {
            ClearCache();
            sForestReports.clear();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            rm->alt = manager.alt = alt;
            const std::string path =
                "scenes/nonmq/Z2_00KEIKOKU" + std::string(namespaceSuffix) + "/Z2_00KEIKOKU_room_00DL_0241B0";
            const std::string resolved = alt ? "alt/" + path : path;
            auto forest = std::make_shared<Fast::DisplayList>();
            forest->Instructions = kActualForestCommands;
            rm->archive->files[resolved] = forest;
            rm->resources[resolved] = forest;
            PlayState play{};
            play.sceneId = SCENE_00KEIKOKU;
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true);
            const auto branch = forest->Instructions[0];
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            assert(Equal(forest->Instructions, kActualForestCommands));
            CheckDraw(Draw(forest->Instructions), false);
            unsigned writebacks = 0;
            for (size_t i = 0; i < kActualForestCommands.size(); ++i) {
                if (!SameCommand(forest->Instructions[i], kActualForestCommands[i])) {
                    const auto opcode = kActualForestCommands[i].words.w0 >> 24;
                    assert(opcode == G_VTX_OTR_HASH || opcode == G_SETTIMG_OTR_HASH);
                    assert(forest->Instructions[i].words.w0 == kActualForestCommands[i].words.w0);
                    assert(forest->Instructions[i].words.w1 > 0xFFFFF);
                    ++writebacks;
                }
            }
            assert(writebacks == 7); // Three vertex and four texture pointer caches.
            const auto resolvedCommands = forest->Instructions;
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true); // The baseline fails here, 0/14 + 0/8.
            assert(SameCommand(forest->Instructions[0], branch));
            // Texture hashes are resolved again when Alt pixels change. Keep the
            // native/Alt draw owner fixed to distinguish this from a list reload.
            for (unsigned pass = 0; pass < 8; ++pass) {
                season = SEASON_OFF;
                MMAutumnSceneFoliage_Update(&play);
                manager.alt = !manager.alt;
                CheckDraw(Draw(forest->Instructions), false);
                season = SEASON_AUTUMN;
                MMAutumnSceneFoliage_Update(&play);
                CheckDraw(Draw(forest->Instructions), true);
                assert(sVariants.size() == 1 && rm->loads == 0);
            }
            play.sceneId = SCENE_TOWN;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), false);
            play.sceneId = SCENE_00KEIKOKU;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true);
            for (size_t i = 1; i < resolvedCommands.size(); ++i) {
                const auto opcode = resolvedCommands[i].words.w0 >> 24;
                if (opcode != G_SETTIMG_OTR_HASH)
                    assert(SameCommand(forest->Instructions[i], resolvedCommands[i]));
                if (opcode == G_SETTIMG_OTR_HASH || opcode == G_VTX_OTR_HASH || opcode == G_MARKER)
                    ++i;
            }
            assert(ReportCount("[AutumnForest] bound") == 1);
            assert(ReportCount("[AutumnForest] accepted") == 1);
            assert(ReportCount("[AutumnForest] refusing") == 0);
            ClearCache();
            CheckDraw(Draw(forest->Instructions), false);
        }
    }
    std::puts("PASS production renderer forest transitions: 14/14 wallpaper + 8/8 canopy stay Scarlet after Off, "
              "scene reset and texture-pointer changes; seven real writebacks, both namespaces and Alt");

    // The copy can resolve an address before the source has ever drawn Off.
    // Re-resolving that source against a refreshed vertex resource must carry
    // the actual new address into the tinted copy as well as preserve color.
    for (const char* drawName : { "0241B0", "0241B1" }) {
        for (bool offDraw : { true, false }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            const std::string path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_" + std::string(drawName);
            const std::string caseName = std::string(drawName) + (offDraw ? "/Off-draw" : "/no-Off-draw");
            auto forest = std::make_shared<Fast::DisplayList>();
            forest->Instructions = kActualForestCommands;
            rm->archive->files[path] = forest;
            rm->resources[path] = forest;
            PlayState play{};
            play.sceneId = SCENE_00KEIKOKU;
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true, caseName + "/initial");
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            manager.refreshed = true;
            if (offDraw)
                CheckDraw(Draw(forest->Instructions), false, caseName + "/Off");
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true, caseName + "/resumed");
            // A no-Off-draw source still holds offsets, so resetting those
            // same words would do nothing. First render and observe real source
            // caches before testing their subsequent clear.
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), false, caseName + "/cache-warm");
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true, caseName + "/cache-observed");
            // Clearing existing native pointer caches must refresh the copied
            // addresses too. The material variant inserts commands, so its vertex
            // slots have different indices from the whole-list forest wrapper.
            for (const size_t index : { 2u, 28u, 53u })
                forest->Instructions[index].words.w1 = kActualForestCommands[index].words.w1;
            manager.refreshed = false;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true, caseName + "/cache-clear");
            ClearCache();
            manager.refreshed = false;
        }
    }
    std::puts("PASS resumed tint follows new vertex resources and cleared caches; whole-list and material copies, "
              "with and without a rendered Off frame");

    // Index zero is temporarily our branch. Restoring a VTX-first source also
    // restores its serialized offset, so a later resolution is a fresh cache
    // write rather than an edit to a previously resolved vertex pointer.
    for (const char* drawName : { "0241B0", "0241B1" }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        const std::string path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_" + std::string(drawName);
        auto forest = std::make_shared<Fast::DisplayList>();
        forest->Instructions = kActualForestCommands;
        forest->Instructions.erase(forest->Instructions.begin(), forest->Instructions.begin() + 2);
        assert((forest->Instructions[0].words.w0 >> 24) == G_VTX_OTR_HASH);
        rm->archive->files[path] = forest;
        rm->resources[path] = forest;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        CheckDraw(Draw(forest->Instructions), true, std::string(drawName) + "/VTX-first-initial");
        for (const bool refreshed : { true, false }) {
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            // Only the branch-covered vertex cache is refreshed. Other
            // resolved vertex pointers remain stable and must remain intact.
            manager.refreshed = refreshed;
            auto stats = Draw(forest->Instructions);
            assert(stats.tinted == 0 &&
                   stats.vertexPointers[0] == reinterpret_cast<uintptr_t>(manager.GetResourceRawPointer(0)) + 0x250);
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            stats = Draw(forest->Instructions);
            if (stats.tinted != 22)
                std::fprintf(stderr, "FAIL %s VTX-first: restored source offset was treated as a pointer edit\n",
                             drawName);
            assert(stats.tinted == 22 &&
                   stats.vertexPointers[0] == reinterpret_cast<uintptr_t>(manager.GetResourceRawPointer(0)) + 0x250);
        }
        ClearCache();
        manager.refreshed = false;
    }
    std::puts("PASS VTX-first copies reactivate after their original first-command offset is restored");

    // A hash payload that resembles a G_VTX command is data, not another
    // mutable pointer slot. Actual binding/alpha/geometry edits still win.
    alignas(16) Vtx alternateVertices[128]{};
    for (unsigned kind = 0; kind < 5; ++kind) {
        ClearCache();
        sForestReports.clear();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0241B0";
        auto forest = std::make_shared<Fast::DisplayList>();
        forest->Instructions = kActualForestCommands;
        if (kind == 0)
            forest->Instructions[19] = Command(0x32008010, 0);
        const auto original = forest->Instructions;
        rm->archive->files[path] = forest;
        rm->resources[path] = forest;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        if (kind == 4) {
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), false);
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            CheckDraw(Draw(forest->Instructions), true);
        }
        const size_t index = kind == 0 ? 19 : kind == 1 ? 26 : kind == 2 ? 30 : 28;
        forest->Instructions[index].words.w1 = kind == 0   ? reinterpret_cast<uintptr_t>(manager.vertices)
                                               : kind == 3 ? 16
                                               : kind == 4 ? reinterpret_cast<uintptr_t>(alternateVertices)
                                                           : 0x80909055;
        const auto changed = forest->Instructions[index];
        MMAutumnSceneFoliage_Update(&play);
        if (!SameCommand(forest->Instructions[0], original[0]))
            std::fprintf(stderr, "FAIL forest edit control %u: replayed a stale tint after another feature's edit\n",
                         kind);
        assert(SameCommand(forest->Instructions[0], original[0]));
        assert(SameCommand(forest->Instructions[index], changed) && sActive.empty());
        for (unsigned repeat = 0; repeat < 12; ++repeat)
            MMAutumnSceneFoliage_Update(&play);
        assert(ReportCount("[AutumnForest] refusing") == 1);
    }
    ClearCache();
    std::puts("PASS real hash-payload, alpha, triangle, vertex-offset and resolved-vertex edits disable stale tint; "
              "forest diagnostics stay bounded");
}
