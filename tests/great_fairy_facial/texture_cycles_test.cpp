// Exercise the actual MM importer, native parameter structs and LUS readers.
// All paths are owned by the returned resource after its input buffer is gone.
#include "2s2h/resource/importer/TextureAnimationFactory.h"
#include "2s2h/resource/type/TextureAnimation.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#define CHECK(condition)                                                         \
    do {                                                                         \
        if (!(condition)) {                                                      \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition);     \
            std::exit(1);                                                        \
        }                                                                        \
    } while (0)

using Paths = std::vector<std::vector<std::string>>;

static void integer(std::vector<char>& out, unsigned value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; ++i) out.push_back(static_cast<char>(value >> (i * 8)));
}

static std::vector<char> payload(const Paths& paths) {
    std::vector<char> bytes;
    integer(bytes, paths.size(), 4);
    for (size_t cycle = 0; cycle < paths.size(); ++cycle) {
        integer(bytes, cycle + 1 == paths.size() ? -(int)(cycle + 1) : cycle + 1, 1);
        integer(bytes, 5, 2);
        integer(bytes, paths[cycle].size(), 2);
        integer(bytes, paths[cycle].size(), 4);
        for (const auto& path : paths[cycle]) {
            integer(bytes, path.size(), 4);
            bytes.insert(bytes.end(), path.begin(), path.end());
        }
        for (size_t i = 0; i < paths[cycle].size(); ++i) integer(bytes, i, 1);
    }
    return bytes;
}

static std::shared_ptr<SOH::TextureAnimation> load(std::vector<char> bytes, size_t offset = 0) {
    auto file = std::make_shared<Ship::File>();
    file->Buffer = std::make_shared<std::vector<char>>(std::move(bytes));
    file->BufferOffset = offset;
    auto reader = std::make_shared<Ship::BinaryReader>(std::make_shared<Ship::MemoryStream>(file->Buffer, offset));
    reader->SetEndianness(Ship::Endianness::Little);
    file->Reader = reader;
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = "objects/object_dy_obj/gGreatFairyAppearenceTexAnim";
    init->Format = RESOURCE_FORMAT_BINARY;
    SOH::ResourceFactoryBinaryTextureAnimationV0 factory;
    auto result = std::dynamic_pointer_cast<SOH::TextureAnimation>(factory.ReadResource(file, init));
    CHECK(result != nullptr);
    // file/reader/buffer are released on return; result must own the strings.
    return result;
}

static void verify(const std::shared_ptr<SOH::TextureAnimation>& resource, const Paths& paths) {
    CHECK(resource->anims.size() == paths.size());
    for (size_t cycle = 0; cycle < paths.size(); ++cycle) {
        const auto& animation = resource->anims[cycle];
        CHECK(animation.type == 5);
        auto* params = static_cast<SOH::AnimatedMatTexCycleParams*>(animation.params);
        CHECK(params->keyFrameLength == paths[cycle].size());
        for (size_t i = 0; i < paths[cycle].size(); ++i) {
            const auto wanted = "__OTR__" + paths[cycle][i];
            const auto actual = std::string(static_cast<const char*>(params->textureList[i]));
            if (actual != wanted) {
                std::fprintf(stderr, "cycle %zu texture %zu: expected %s, got %s\n", cycle, i,
                             wanted.c_str(), actual.c_str());
            }
            CHECK(actual == wanted);
            CHECK(params->textureIndexList[i] == i);
        }
    }
}

static Paths fountains() {
    Paths paths;
    for (const auto& kind : { "Charcoal", "Accent", "Emission", "Stocking" }) {
        paths.emplace_back();
        for (const auto& fountain : { "ClockTown", "Snowhead", "Woodfall", "GreatBay", "Ikana" }) {
            paths.back().push_back(std::string("objects/object_dy_obj/GreatFairyMM_") + fountain + "_" + kind + "Tex");
        }
    }
    return paths;
}

static std::vector<char> read(const char* path) {
    std::ifstream input(path, std::ios::binary);
    CHECK(input.good());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

int main(int argc, char** argv) {
    Paths single = { { "objects/object_dy_obj/native_clock", "objects/object_dy_obj/native_snow" } };
    verify(load(payload(single)), single);
    std::puts("PASS original single texture cycle and path ownership");

    const auto paths = fountains();
    verify(load(payload(paths)), paths);
    std::puts("PASS four independent fountain cycles retain their own paths and indices");

    // Short strings expose relocation of earlier c_str() pointers when later
    // cycles grow the container. Mixed lengths also expose index resets.
    Paths shortPaths = { { "a", "b" }, { "c" }, { "d", "e", "f" }, { "g" } };
    auto shortResource = load(payload(shortPaths));
    verify(shortResource, shortPaths);
    auto another = load(payload(paths));
    verify(shortResource, shortPaths);
    another.reset();
    verify(shortResource, shortPaths);
    std::puts("PASS short-string pointers, unequal cycles and independent resource lifetime");

    if (argc > 1) {
        auto candidate = load(read(argv[1]), 64);
        verify(candidate, paths);
        const int segments[] = { 1, 3, 4, -5 };
        for (size_t i = 0; i < 4; ++i) CHECK(candidate->anims[i].segment == segments[i]);
        std::puts("PASS actual POC2 archive selector through native importer: segments 8/10/11/12");
    }
    if (argc > 2) {
        auto native = load(read(argv[2]), 64);
        CHECK(native->anims.size() == 2 && native->anims[0].type == 5 && native->anims[1].type == 2);
        auto* eyes = static_cast<SOH::AnimatedMatTexCycleParams*>(native->anims[0].params);
        auto* colors = static_cast<SOH::AnimatedMatColorParams*>(native->anims[1].params);
        CHECK(eyes->keyFrameLength == 5 && colors->keyFrameLength == 5);
        CHECK(native->anims[0].segment == 1 && native->anims[1].segment == -3);
        for (size_t i = 0; i < 5; ++i) {
            CHECK(eyes->textureIndexList[i] == i);
            CHECK(std::string(static_cast<const char*>(eyes->textureList[i])).starts_with("__OTR__objects/object_dy_obj/"));
        }
        std::puts("PASS original MM eye/color appearance selector remains compatible");
    }
    return 0;
}
