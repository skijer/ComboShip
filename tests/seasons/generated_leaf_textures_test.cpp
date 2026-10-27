#include "fast/resource/factory/TextureFactory.h"
#include "fast/resource/type/Texture.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include "../../mm/tests/test_require.h"
#include <cstdio>
#include <fstream>
#include <iterator>

int main(int argc, char** argv) {
    REQUIRE(argc == 2);
    Fast::ResourceFactoryBinaryTextureV1 factory;
    for (const char* palette : {"crimson", "orange", "gold", "copper"}) {
        auto init = std::make_shared<Ship::ResourceInitData>();
        init->Path = std::string("objects/nei_autumn/leaves/") + palette + "_tex";
        init->Format = RESOURCE_FORMAT_BINARY;
        auto file = std::make_shared<Ship::File>();
        std::ifstream source(std::string(argv[1]) + "/mm/assets/custom/" + init->Path, std::ios::binary);
        REQUIRE(source.good());
        file->Buffer = std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(source),
                                                        std::istreambuf_iterator<char>());
        auto reader = std::make_shared<Ship::BinaryReader>(std::make_shared<Ship::MemoryStream>(file->Buffer));
        reader->SetEndianness(Ship::Endianness::Little);
        REQUIRE(reader->ReadUInt32() == 0);
        REQUIRE(reader->ReadUInt32() == 0x4F544558);
        REQUIRE(reader->ReadUInt32() == 1);
        reader->Seek(64, Ship::SeekOffsetType::Start);
        file->Reader = reader;
        auto texture = std::static_pointer_cast<Fast::Texture>(factory.ReadResource(file, init));
        REQUIRE(texture && texture->Type == Fast::TextureType::RGBA32bpp);
        REQUIRE(texture->Width == 128 && texture->Height == 128);
        REQUIRE(texture->HByteScale == 4 && texture->VPixelScale == 4);
        REQUIRE(texture->Flags == TEX_FLAG_LOAD_AS_RAW && texture->ImageDataSize == 128 * 128 * 4);
        REQUIRE(file->Buffer->size() == 64 + 28 + texture->ImageDataSize);
        unsigned transparent = 0, opaque = 0;
        for (unsigned i = 0; i < 128 * 128; ++i) {
            const auto alpha = texture->ImageData[i * 4 + 3];
            transparent += alpha == 0; opaque += alpha > 200;
        }
        REQUIRE(transparent > 1000 && opaque > 1000);
        auto* pixels = texture->ImageData;
        file.reset(); reader.reset();
        REQUIRE(texture->ImageData == pixels && texture->mImageBuffer);
        printf("PASS native texture factory: generated %s, physical size/scales, alpha and buffer lifetime\n", palette);
    }
}
