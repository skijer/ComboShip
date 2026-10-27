// Compile the shipped ramp XML with the real engine factory, then submit its
// commands beside the real native stump DL. File I/O replaces archive lookup.
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <tinyxml2.h>
#include "fast/resource/factory/DisplayListFactory.h"
#include "fast/resource/type/DisplayList.h"
#include "ship/utils/binarytools/MemoryStream.h"

static constexpr const char* kCore = "custom/comboship/native_forest/";
static std::filesystem::path assets;
static std::map<std::string, std::shared_ptr<Fast::DisplayList>> loaded;

static std::shared_ptr<Fast::DisplayList> Load(const std::string& path) {
    if (auto found = loaded.find(path); found != loaded.end())
        return found->second;
    std::ifstream input(assets / path, std::ios::binary);
    assert(input && "Test input is an actual prepared resource");
    auto bytes = std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(input),
                                                     std::istreambuf_iterator<char>());
    auto file = std::make_shared<Ship::File>();
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    std::shared_ptr<Ship::IResource> resource;
    if (bytes->at(0) == '<') {
        auto document = std::make_shared<tinyxml2::XMLDocument>();
        assert(document->Parse(bytes->data(), bytes->size()) == tinyxml2::XML_SUCCESS);
        file->Reader = document;
        init->Format = RESOURCE_FORMAT_XML;
        Fast::ResourceFactoryXMLDisplayListV0 factory;
        resource = factory.ReadResource(file, init);
    } else {
        assert(bytes->size() >= 72 && std::string(bytes->data() + 4, 4) == "TLDO");
        auto stream = std::make_shared<Ship::MemoryStream>(bytes, 64);
        auto reader = std::make_shared<Ship::BinaryReader>(stream);
        reader->SetEndianness(Ship::Endianness::Little);
        file->Reader = reader;
        init->Format = RESOURCE_FORMAT_BINARY;
        Fast::ResourceFactoryBinaryDisplayListV0 factory;
        resource = factory.ReadResource(file, init);
    }
    auto result = std::dynamic_pointer_cast<Fast::DisplayList>(resource);
    assert(result);
    loaded[path] = result;
    return result;
}

struct State {
    uint32_t high = G_CYC_2CYCLE;
    uint32_t geometry = G_FOG;
    unsigned renderTile = 0;
    unsigned tile1Format = 0, tile1Size = 0;
    unsigned scrollCalls = 0, rampTriangles = 0, stockTriangles = 0;
};

static void Draw(const std::string& path, State& state) {
    const auto list = Load(path);
    const bool ramp = path == std::string(kCore) + "mirror_access_root";
    const bool stock = path == std::string(kCore) + "geometry/Z2_LOST_WOODS_room_00DL_002548";
    for (size_t index = 0; index < list->Instructions.size(); ++index) {
        const auto& command = list->Instructions[index];
        const uint32_t a = command.words.w0;
        const uintptr_t b = command.words.w1;
        const unsigned opcode = a >> 24;
        if (opcode == uint8_t(G_DL_OTR_FILEPATH)) {
            Draw(reinterpret_cast<const char*>(b), state);
        } else if (opcode == G_DL) {
            assert(b == 0x08000001 && "Ramp must call the authored dynamic segment, not a filename");
            ++state.scrollCalls;
        } else if (opcode == G_SETOTHERMODE_H) {
            const unsigned count = (a & 255) + 1;
            const unsigned shift = 32 - ((a >> 8) & 255) - count;
            const uint32_t mask = count == 32 ? 0xFFFFFFFFU : ((1U << count) - 1) << shift;
            state.high = (state.high & ~mask) | (uint32_t(b) & mask);
        } else if (opcode == G_GEOMETRYMODE) {
            state.geometry = (state.geometry & (a & 0xFFFFFF)) | uint32_t(b);
        } else if (opcode == G_TEXTURE) {
            state.renderTile = (a >> 8) & 7;
        } else if (opcode == G_SETTILE && ((b >> 24) & 7) == 1) {
            state.tile1Format = (a >> 21) & 7;
            state.tile1Size = (a >> 19) & 3;
        } else if (opcode == G_TRI1 || opcode == G_TRI2 || opcode == G_TRI1_OTR) {
            if (ramp) {
                assert((state.high & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_1CYCLE);
                assert((state.high & (3U << G_MDSFT_TEXTLUT)) == G_TT_NONE);
                assert(state.renderTile == 1 && state.tile1Format == G_IM_FMT_I && state.tile1Size == G_IM_SIZ_8b);
                assert(state.scrollCalls > 0 && "Bind the authored 32x64 scrolling layer before drawing");
                state.rampTriangles += opcode == G_TRI2 ? 2 : 1;
            } else if (stock) {
                assert((state.high & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE &&
                       "One-cycle ramp state corrupts the stock two-texture bark combiner");
                assert((state.high & (3U << G_MDSFT_TEXTLUT)) == G_TT_NONE);
                assert(state.geometry & G_FOG);
                assert(state.renderTile == 0);
                state.stockTriangles += opcode == G_TRI2 ? 2 : 1;
            }
        } else if (opcode == G_MARKER || opcode == G_VTX_OTR_HASH || opcode == G_SETTIMG_OTR_HASH) {
            // Binary stock hash commands occupy two Gfx words.
            ++index;
        }
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    assets = argv[1];
    const auto ramp = std::string(kCore) + "mirror_access_root";
    const auto stock = std::string(kCore) + "geometry/Z2_LOST_WOODS_room_00DL_002548";
    for (bool rampFirst : { false, true }) {
        State state;
        Draw(rampFirst ? ramp : stock, state);
        Draw(rampFirst ? stock : ramp, state);
        Draw(stock, state); // Subsequent/re-entry draw cannot retain the ramp pipeline.
        if (state.rampTriangles != 28 || state.stockTriangles == 0 || state.scrollCalls != 1)
            std::fprintf(stderr, "material loader counts: ramp=%u stock=%u scroll=%u\n",
                         state.rampTriangles, state.stockTriangles, state.scrollCalls);
        assert(state.rampTriangles == 28 && state.stockTriangles > 0 && state.scrollCalls == 1);
    }
    std::puts("PASS real XML/binary loaders: ramp tile-one segment-8 scroll, native TLUT/two-cycle/fog restored across draw orders");
}
