// Prelude's generated recipe IDs change on import. Material labels and the
// archive that actually owns each draw must select the foliage instead.
static nlohmann::json MoonTreeShape(const std::string& root, const std::vector<std::string>& labels) {
    auto textures = nlohmann::json::array();
    auto paths = nlohmann::json::array();
    for (size_t i = 0; i < labels.size(); ++i) {
        textures.push_back({ { "label", labels[i] } });
        paths.push_back(root + "_tex" + std::to_string(i));
    }
    return { { "newDlPath", root }, { "stored", { { "textures", textures }, { "texPaths", paths } } } };
}

static void SetMoonTreeProject(const std::shared_ptr<Ship::Archive>& owner, const nlohmann::json& shapes) {
    const nlohmann::json project = {
        { "codec", 2 },
        { "edits", { { "Z2_00KEIKOKU", { { { "kind", "geometry" }, { "data", { { "shapes", shapes } } } } } } } }
    };
    const auto json = project.dump();
    auto file = std::make_shared<Ship::File>();
    file->Buffer = std::make_shared<std::vector<char>>(json.begin(), json.end());
    owner->rawFiles["prelude/project/edits.json"] = file;
}

static void CheckPreludeMoonTreeFoliage() {
    const std::string prefix = "custom/prelude/Z2_00KEIKOKU/recipe";
    for (bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        rm->alt = alt;
        auto owner = rm->archive->defaultOwner;
        auto shapes = nlohmann::json::array();
        std::map<std::string, std::shared_ptr<Fast::DisplayList>> leaves;
        std::map<std::string, std::vector<Gfx>> originals;
        const std::string assetPrefix = alt ? "alt/" : "";
        size_t id = 6661300910966087;
        for (const char* label : { "sou_ha_01", "sou_ha_02", "sou_ha_03", "sou_sida_01", "sou_wood_01",
                                   "s01_kusa_01_4FE00", "s01_iekabe_04_21480" }) {
            const auto root = prefix + std::to_string(id++);
            shapes.push_back(MoonTreeShape(root, { label, "sou_wood_01" }));
            auto list = List((root + "_tex0").c_str());
            auto bark = MakeMaterial((root + "_tex1").c_str());
            list->Instructions.insert(list->Instructions.end() - 1, bark.begin(), bark.end());
            const auto path = assetPrefix + root;
            // Prelude exports direct resources; bark images need not be loaded.
            rm->archive->files[path] = list;
            rm->resources[path] = list;
            leaves[path] = list;
            originals[path] = list->Instructions;
        }
        SetMoonTreeProject(owner, shapes);
        // Another loaded pack can shadow the global project path. Selection
        // must read from the owner of each recipe, not that unrelated project.
        auto unrelated = std::make_shared<Ship::Archive>();
        SetMoonTreeProject(unrelated, nlohmann::json::array());
        rm->archive->files["prelude/project/edits.json"] = std::make_shared<Ship::IResource>();
        rm->archive->owners["prelude/project/edits.json"] = unrelated;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_AUTUMN, SEASON_OFF }) {
            season = selected;
            MMAutumnSceneFoliage_Update(&play);
            size_t index = 0;
            for (const auto& [path, list] : leaves) {
                const bool foliage = index++ < 4;
                const bool active = foliage && selected != SEASON_SUMMER && selected != SEASON_OFF;
                if (!active) {
                    assert(Equal(list->Instructions, originals.at(path)));
                    continue;
                }
                if ((list->Instructions[0].words.w0 >> 24) != G_DL)
                    std::fprintf(stderr, "FAIL Prelude Moon tree foliage not seasonally targeted: %s\n", path.c_str());
                assert((list->Instructions[0].words.w0 >> 24) == G_DL);
                const auto& changed = CurrentVariants().at(list.get()).commands;
                assert(Equal(RemoveTint(changed), originals.at(path)));
                const auto root = path.substr(assetPrefix.size());
                const auto color = selected == SEASON_SPRING   ? 0xF6C1D9E0
                                   : selected == SEASON_WINTER ? 0xE2EEE9FF
                                                               : 0xDE496BFF;
                assert(CountMaterialTriangles(changed, (root + "_tex0").c_str(), color) == 1);
                assert(CountMaterialTriangles(changed, (root + "_tex1").c_str(), color) == 0);
            }
            const auto reads = owner->reads;
            for (int frame = 0; frame < 5; ++frame)
                MMAutumnSceneFoliage_Update(&play);
            assert(owner->reads == reads && unrelated->reads.empty() && rm->loads == 0);
        }
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        play.sceneId = SCENE_24KEMONOMITI;
        MMAutumnSceneFoliage_Update(&play);
        for (const auto& [path, list] : leaves)
            assert(Equal(list->Instructions, originals.at(path)));
        ClearCache();
    }
    season = SEASON_AUTUMN;
    std::puts("PASS Prelude Moon tree: four leaf/fern labels, generated IDs, owning-pack metadata, missing bark, "
              "mixed bark protection, native/Alt, three palettes and exact restoration");
}

static void CheckPreludeMoonTreeOwnerOverride() {
    ClearCache();
    auto rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->rm = rm;
    const std::string root = "custom/prelude/Z2_00KEIKOKU/recipe1029384756";
    auto native = List((root + "_tex0").c_str());
    auto alt = List((root + "_tex0").c_str());
    const auto original = native->Instructions;
    auto replacementOwner = std::make_shared<Ship::Archive>();
    SetMoonTreeProject(rm->archive->defaultOwner, nlohmann::json::array({ MoonTreeShape(root, { "sou_ha_02" }) }));
    SetMoonTreeProject(replacementOwner, nlohmann::json::array({ MoonTreeShape(root, { "sou_wood_01" }) }));
    rm->archive->files[root] = native;
    rm->archive->files["alt/" + root] = alt;
    rm->archive->owners["alt/" + root] = replacementOwner;
    rm->resources[root] = native;
    rm->resources["alt/" + root] = alt;
    PlayState play{};
    play.sceneId = SCENE_00KEIKOKU;
    for (bool enabled : { false, true, false }) {
        rm->alt = enabled;
        MMAutumnSceneFoliage_Update(&play);
        assert(Equal(alt->Instructions, original));
        assert(Equal(native->Instructions, original) == enabled);
    }
    ClearCache();
    // A direct recipe and a sidecar alias can come from different packs.
    // Without resolving the alias target, neither metadata owner proves which
    // material the renderer will use. Leave that ambiguous draw unselected.
    rm->archive->files[root + ".meta"] = native;
    rm->archive->owners[root + ".meta"] = replacementOwner;
    rm->alt = false;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(native->Instructions, original));
    ClearCache();
    rm->archive->files.erase(root + ".meta");
    rm->archive->owners.erase(root + ".meta");
    // A meta-only Alt alias can point back to another pack's native bark.
    // Its sidecar owner's leaf label must not tint that cached aliased draw.
    SetMoonTreeProject(rm->archive->defaultOwner, nlohmann::json::array({ MoonTreeShape(root, { "sou_wood_01" }) }));
    SetMoonTreeProject(replacementOwner, nlohmann::json::array({ MoonTreeShape(root, { "sou_ha_02" }) }));
    rm->archive->files.erase("alt/" + root);
    rm->archive->files["alt/" + root + ".meta"] = alt;
    rm->archive->owners["alt/" + root + ".meta"] = replacementOwner;
    rm->alt = true;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(native->Instructions, original) && Equal(alt->Instructions, original));
    ClearCache();
    rm->archive->files.erase("alt/" + root + ".meta");
    rm->archive->owners.erase("alt/" + root + ".meta");
    rm->alt = false;
    // Missing or malformed project metadata must leave all source commands alone.
    for (const std::string json : { "", "{", "null", "{\"codec\":2,\"edits\":[]}",
                                    "{\"codec\":2,\"edits\":{\"Z2_00KEIKOKU\":[{\"data\":{\"shapes\":[null]}}]}}" }) {
        auto file = std::make_shared<Ship::File>();
        file->Buffer = std::make_shared<std::vector<char>>(json.begin(), json.end());
        rm->archive->defaultOwner->rawFiles["prelude/project/edits.json"] = file;
        MMAutumnSceneFoliage_Update(&play);
        assert(Equal(native->Instructions, original) && Equal(alt->Instructions, original));
        ClearCache();
    }
    std::puts("PASS Prelude Moon tree: conflicting Alt ownership and malformed/missing metadata stay uncolored");
}

struct ActualMoonTreeDraw {
    std::string path;
    std::vector<Gfx> commands;
    std::vector<std::string> textures;
    bool foliage;
};

static void CheckActualPreludeMoonTree(const char* projectPath, const std::vector<ActualMoonTreeDraw>& draws) {
    ClearCache();
    auto rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->rm = rm;
    std::ifstream project(projectPath, std::ios::binary);
    assert(project.good());
    auto file = std::make_shared<Ship::File>();
    file->Buffer =
        std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(project), std::istreambuf_iterator<char>());
    rm->archive->defaultOwner->rawFiles["prelude/project/edits.json"] = file;
    for (const auto& draw : draws) {
        auto list = std::make_shared<Fast::DisplayList>();
        list->Instructions = draw.commands;
        rm->resources[draw.path] = list;
        rm->archive->files[draw.path] = list;
    }
    PlayState play{};
    play.sceneId = SCENE_00KEIKOKU;
    size_t leafBatches = 0, protectedBatches = 0;
    unsigned leafTriangles = 0;
    for (int selected : { SEASON_AUTUMN, SEASON_WINTER, SEASON_SPRING, SEASON_SUMMER, SEASON_OFF }) {
        season = selected;
        MMAutumnSceneFoliage_Update(&play);
        for (const auto& draw : draws) {
            auto list = std::dynamic_pointer_cast<Fast::DisplayList>(rm->resources.at(draw.path));
            if (!draw.foliage || selected == SEASON_SUMMER || selected == SEASON_OFF) {
                assert(Equal(list->Instructions, draw.commands));
                if (selected == SEASON_AUTUMN)
                    ++protectedBatches;
                continue;
            }
            assert((list->Instructions[0].words.w0 >> 24) == G_DL);
            const auto& commands = CurrentVariants().at(list.get()).commands;
            assert(Equal(RemoveTint(commands), draw.commands));
            const auto color = selected == SEASON_SPRING   ? 0xF6C1D9E0
                               : selected == SEASON_WINTER ? 0xE2EEE9FF
                                                           : 0xDE496BFF;
            unsigned triangles = 0;
            for (const auto& texture : draw.textures) {
                const auto expected = CountMaterialTriangles(draw.commands, texture.c_str());
                assert(CountMaterialTriangles(commands, texture.c_str(), color) == expected);
                triangles += expected;
            }
            assert(triangles > 0);
            if (selected == SEASON_AUTUMN) {
                ++leafBatches;
                leafTriangles += triangles;
            }
        }
        assert(rm->loads == 0); // Neither missing bark nor full texture images are needed.
    }
    ClearCache();
    season = SEASON_AUTUMN;
    std::printf("PASS actual Prelude export: %zu leaf batches / %u triangles, %zu protected bark/building/grass "
                "batches, three seasonal palettes and exact Summer/Off commands\n",
                leafBatches, leafTriangles, protectedBatches);
}
