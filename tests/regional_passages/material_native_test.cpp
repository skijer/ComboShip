#define main ForeignTextureBaselineMain
#include "../mm_presentation/foreign_texture_test.cpp"
#undef main
#include "fast/resource/factory/TextureFactory.h"
#include "render/RegionalBridgeMaterials.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include <fstream>
#include <iterator>
int main(int argc, char **argv) {
  assert(argc == 2);
  using namespace RegionalBridgeMaterials;
  Fast::ResourceFactoryBinaryTextureV1 factory;
  for (auto material :
       {Material::LakeStone, Material::SnowheadStone, Material::SnowheadSnow,
        Material::SnowheadIce, Material::ValleySnow, Material::LakeWater, Material::LakeSand}) {
    const char *path = OverridePath(material);
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path + 7;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto file = std::make_shared<Ship::File>();
    std::ifstream source(std::string(argv[1]) + "/" + (path + 7),
                         std::ios::binary);
    assert(source.good());
    file->Buffer = std::make_shared<std::vector<char>>(
        std::istreambuf_iterator<char>(source),
        std::istreambuf_iterator<char>());
    auto reader = std::make_shared<Ship::BinaryReader>(
        std::make_shared<Ship::MemoryStream>(file->Buffer));
    reader->SetEndianness(Ship::Endianness::Little);
    reader->Seek(64, Ship::SeekOffsetType::Start);
    file->Reader = reader;
    auto texture = std::static_pointer_cast<Fast::Texture>(
        factory.ReadResource(file, init));
    assert(texture);
    unsigned nw = material == Material::LakeWater ? 64 : 32,
             nh = material == Material::LakeStone ||
                          material == Material::LakeWater
                      ? 64
                      : 32;
    assert(texture->Width == nw * 4 && texture->Height == nh * 4 &&
           texture->Flags == TEX_FLAG_LOAD_AS_RAW);
    assert(texture->HByteScale == (material == Material::LakeWater ? 16 : 8) &&
           texture->VPixelScale == 4);
    Fast::host->textures[path] = texture;
    // Resolve passes the archive key to the existence API and keeps the tagged
    // signature for the real native SETTIMG handler, so no metadata is lost.
    const char *selected =
        Resolve(material, [&](const char *p) { return init->Path == p; });
    Fast::Submit(Fast::gfx_set_timg_handler_rdp, selected,
                 material == Material::LakeWater ? 4 : 0,
                 material == Material::LakeWater ? 1 : 2);
    assert(Fast::instance->lastMetadata.resource == texture &&
           Fast::instance->lastPixels == texture->ImageData);
    assert(Fast::instance->lastMetadata.width == nw * 4 &&
           Fast::instance->lastMetadata.height == nh * 4);
    assert(Fast::instance->lastMetadata.h_byte_scale == texture->HByteScale &&
           Fast::instance->lastMetadata.v_pixel_scale == 4);
    assert(Fast::instance->lastFlags == TEX_FLAG_LOAD_AS_RAW);
    file.reset();
    reader.reset();
    assert(texture->mImageBuffer && texture->ImageData[3] == 255);
  }
  puts("PASS seven replacement materials through real v1 factory and SETTIMG: "
       "dimensions, raw flags, byte/pixel scales, resource ownership and "
       "buffer lifetime");
}
