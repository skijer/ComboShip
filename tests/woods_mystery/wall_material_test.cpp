// The load, tile and RGBA16 decode functions are extracted unchanged from
// production. Only the resource lookup and GPU upload are boundaries of this
// material probe.
#include "fast/resource/type/Texture.h"
#include "libultraship/libultra/gbi.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#define SPDLOG_ERROR(...) ((void)0)
#define SUPPORT_CHECK(value) assert(value)
#define SCALE_5_8(value) ((value)*255 / 31)

#include "wall_layout.inc"

static std::string_view GetBaseTexturePath(const std::string &path) {
  return path;
}

class Interpreter {
public:
  RDP state{};
  RDP *mRdp = &state;
  struct MaskedTexture {
    uint8_t *replacementData = nullptr;
  };
  std::map<std::string, MaskedTexture, std::less<>> mMaskedTextures;
  std::vector<uint8_t> buffer = std::vector<uint8_t>(16384);
  uint8_t *mTexUploadBuffer = buffer.data();
  std::vector<uint8_t> uploaded;
  unsigned uploadedWidth = 0, uploadedHeight = 0;

  void GfxDpSetTile(uint8_t, uint32_t, uint32_t, uint32_t, uint8_t, uint32_t,
                    uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  void GfxDpLoadBlock(uint8_t, uint32_t, uint32_t, uint32_t, uint32_t);
  void ImportTextureRgba16(int, bool);
  void UploadTextureMeasured(const uint8_t *data, uint32_t width,
                             uint32_t height) {
    uploadedWidth = width;
    uploadedHeight = height;
    uploaded.assign(data, data + width * height * 4);
  }
};

#include "wall_renderer.inc"

int main(int argc, char **argv) {
  assert(argc == 2);
  std::ifstream stream(argv[1], std::ios::binary);
  std::vector<uint8_t> image((std::istreambuf_iterator<char>(stream)), {});
  assert(image.size() == 32 * 32 * 2);
  Interpreter renderer;
  renderer.state.texture_to_load.addr = image.data();
  renderer.state.texture_to_load.siz = G_IM_SIZ_16b;
  renderer.state.texture_to_load.width = 1;
  auto &metadata = renderer.state.texture_to_load.raw_tex_metadata;
  metadata.width = metadata.height = 32;
  metadata.h_byte_scale = metadata.v_pixel_scale = 1;
#include "wall_commands.inc"
  assert(renderer.state.loaded_texture[0].size_bytes == image.size());
  renderer.ImportTextureRgba16(0, false);
  assert(renderer.uploadedWidth == 32 && renderer.uploadedHeight == 32);
  assert(renderer.uploaded.size() == 32 * 32 * 4);
  for (size_t i = 0; i < image.size() / 2; ++i) {
    const uint16_t pixel = (image[2 * i] << 8) | image[2 * i + 1];
    assert(renderer.uploaded[4 * i] == ((pixel >> 11) * 255 / 31));
    assert(renderer.uploaded[4 * i + 1] == (((pixel >> 6) & 31) * 255 / 31));
    assert(renderer.uploaded[4 * i + 2] == (((pixel >> 1) & 31) * 255 / 31));
    assert(renderer.uploaded[4 * i + 3] == ((pixel & 1) ? 255 : 0));
  }
  std::puts("PASS baked Woods wall: native32x32 load, stride, masks and exact "
            "RGBA16 pixels (ASan/UBSan)");
}
