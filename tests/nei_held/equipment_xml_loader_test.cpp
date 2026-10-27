// Load the shipped wrappers through the actual engine XML factory. A graph
// walker alone can incorrectly accept a tag that the loader turns into PipeSync.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <tinyxml2.h>
#include <utility>

#include "fast/resource/factory/DisplayListFactory.h"
#include "fast/resource/type/DisplayList.h"

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::filesystem::path assets = argv[1];
    Fast::ResourceFactoryXMLDisplayListV0 factory;
    const std::set<std::string> models = {
        "kokiri_sword", "mm_kokiri_sword", "razor_sword", "gilded_sword", "master_sword", "true_master_sword",
        "biggoron_sword", "great_fairy_sword", "four_sword", "giants_knife", "broken_giants_knife"
    };
    const std::set<std::string> frames = { "oot_adult", "oot_child", "mm_human" };
    std::set<std::pair<std::string, std::string>> checked;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assets / "objects/nei_held_swords")) {
        if (entry.path().filename() != "held_dl") {
            continue;
        }
        auto document = std::make_shared<tinyxml2::XMLDocument>();
        assert(document->LoadFile(entry.path().string().c_str()) == tinyxml2::XML_SUCCESS);
        auto file = std::make_shared<Ship::File>();
        file->Reader = document;
        auto init = std::make_shared<Ship::ResourceInitData>();
        init->Format = RESOURCE_FORMAT_XML;
        init->Path = entry.path().string();
        auto loaded = std::dynamic_pointer_cast<Fast::DisplayList>(factory.ReadResource(file, init));
        assert(loaded);
        const std::string slug = entry.path().parent_path().parent_path().filename().string();
        const std::string frame = entry.path().parent_path().filename().string();
        assert(models.contains(slug) && frames.contains(frame));
        const std::string target = slug == "broken_giants_knife"
                                       ? "objects/nei_held_swords/broken_giants_knife/model/gi_dl"
                                       : "objects/nei_gi_redesign/" + slug + "/gi_dl";
        unsigned calls = 0;
        for (const Gfx& instruction : loaded->Instructions) {
            if ((instruction.words.w0 >> 24) == G_DL_OTR_FILEPATH) {
                assert(((instruction.words.w0 >> 16) & 0xFF) == G_DL_PUSH &&
                       "Held GI must return to the wrapper's matrix pop");
                assert(std::strcmp(reinterpret_cast<const char*>(instruction.words.w1), target.c_str()) == 0);
                ++calls;
            }
        }
        if (calls != 1) {
            std::fprintf(stderr, "FAIL: engine loader emitted %u mesh calls for %s\n", calls,
                         entry.path().string().c_str());
            return 1;
        }
        assert(checked.emplace(slug, frame).second);
    }
    assert(checked.size() == models.size() * frames.size());
    std::printf("PASS: engine XML loader emits the exact authored mesh call for all %zu held sword wrappers\n",
                checked.size());
}
