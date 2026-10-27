// Inspect the actual serialized pixels supplied in a user's scene/texture pack.
// These optional fixtures are kept outside the repository; no authored scene is edited.
#include "soh/Enhancements/Graphics/SeasonFoliageTint.h"
#include "test_require.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

static unsigned U32(const std::vector<char>& bytes, size_t at) {
    REQUIRE(at + 4 <= bytes.size());
    return static_cast<unsigned char>(bytes[at]) | static_cast<unsigned char>(bytes[at + 1]) << 8 |
           static_cast<unsigned char>(bytes[at + 2]) << 16 | static_cast<unsigned char>(bytes[at + 3]) << 24;
}

int main(int argc, char** argv) {
    using namespace SeasonFoliage;
    REQUIRE(argc >= 3);
    for (int file = 1; file < argc;) {
        const char* name = argv[file++];
        REQUIRE(file < argc);
        const auto surface = static_cast<Surface>(std::stoi(argv[file++]));
        Region region;
        if (file < argc && std::strcmp(argv[file], "--region") == 0) {
            REQUIRE(file + 4 < argc);
            ++file;
            region = { static_cast<unsigned>(std::stoul(argv[file++])), static_cast<unsigned>(std::stoul(argv[file++])),
                       static_cast<unsigned>(std::stoul(argv[file++])),
                       static_cast<unsigned>(std::stoul(argv[file++])) };
        }
        std::ifstream input(name, std::ios::binary);
        REQUIRE(input.good());
        const std::vector<char> bytes{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
        REQUIRE(bytes.size() >= 80 && U32(bytes, 4) == 0x4F544558);
        const unsigned version = U32(bytes, 8);
        REQUIRE(version <= 1);
        const size_t offset = version ? 92 : 80;
        auto init = std::make_shared<Ship::ResourceInitData>();
        init->Path = name;
        auto source = std::make_shared<Fast::Texture>(init);
        source->Type = static_cast<Fast::TextureType>(U32(bytes, 64));
        source->Width = U32(bytes, 68);
        source->Height = U32(bytes, 72);
        source->Flags = version ? U32(bytes, 76) : 0;
        if (version) {
            std::memcpy(&source->HByteScale, bytes.data() + 80, 4);
            std::memcpy(&source->VPixelScale, bytes.data() + 84, 4);
        }
        source->ImageDataSize = U32(bytes, offset - 4);
        REQUIRE(bytes.size() == offset + source->ImageDataSize);
        source->mImageBuffer = std::make_shared<std::vector<char>>(bytes.begin() + offset, bytes.end());
        source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
        const auto original = *source->mImageBuffer;
        const bool packed = source->Type == Fast::TextureType::RGBA16bpp && source->Flags == 0;
        const size_t stride = packed ? 2 : 4;
        size_t greenPixels = 0;
        for (int season : { 0, 1, 2, 3 }) {
            const auto tint = Palette(season, surface);
            const auto copy = BuildTexture(source, tint, region);
            if (!tint.color) {
                REQUIRE(!copy);
                continue;
            }
            size_t green = 0, changed = 0;
            for (size_t at = 0; at < source->ImageDataSize; at += stride) {
                const auto* before = source->ImageData + at;
                const auto* after = copy ? copy->ImageData + at : before;
                const unsigned native = packed ? unsigned(before[0]) << 8 | before[1] : 0;
                const unsigned r = packed ? native >> 11 : before[0];
                const unsigned g = packed ? (native >> 6) & 31 : before[1];
                const unsigned b = packed ? (native >> 1) & 31 : before[2];
                const unsigned x = (at / stride) % source->Width, y = (at / stride) / source->Width;
                const bool inside = x * 16 >= source->Width * region.left && x * 16 < source->Width * region.right &&
                                    y * 16 >= source->Height * region.top && y * 16 < source->Height * region.bottom;
                if (inside && g > r && g > b) {
                    ++green;
                } else {
                    REQUIRE(!std::memcmp(before, after, stride) && "protected atlas/bark/stone/flower pixels changed");
                }
                REQUIRE(packed ? (before[1] & 1) == (after[1] & 1) : before[3] == after[3]);
                changed += std::memcmp(before, after, stride) != 0;
            }
            REQUIRE(!green || changed);
            greenPixels = green;
            REQUIRE(*source->mImageBuffer == original);
            if (copy) {
                REQUIRE(copy->GetInitData() == source->GetInitData() && copy->Type == source->Type &&
                        copy->Flags == source->Flags && copy->Width == source->Width &&
                        copy->Height == source->Height && copy->HByteScale == source->HByteScale &&
                        copy->VPixelScale == source->VPixelScale);
            }
        }
        std::cout << "PASS " << name << " pixels=" << source->Width * source->Height << " green=" << greenPixels
                  << " source/alpha/non-green/metadata exact\n";
    }
}
