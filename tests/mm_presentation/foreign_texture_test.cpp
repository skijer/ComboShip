// Execute production SETTIMG handlers; resource lookup and GPU state are the
// only doubles, with logger output captured for the missing-texture contract.
#include "fast/lus_gbi.h"
#include "fast/resource/type/Texture.h"
#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

struct CapturedError {
  std::string format;
  std::vector<std::string> fields;
};
static std::vector<CapturedError> errors;
template <typename T> static std::string LogField(const T &value) {
  std::ostringstream field;
  field << value;
  return field.str();
}
template <typename... Args>
static void CaptureError(const char *format, const Args &...args) {
  errors.push_back({format, {LogField(args)...}});
}
#define SPDLOG_ERROR(...) CaptureError(__VA_ARGS__)
namespace Ship {
class ResourceManager {
public:
  bool OtrSignatureCheck(const char *);
  bool IsAltAssetsEnabled() { return altAssets; }
  std::shared_ptr<IResource> LoadResourceProcess(const char *path) {
    ++loads;
    lastPath = path;
    auto it = textures.find(path);
    return it == textures.end() ? nullptr : it->second;
  }
  std::map<std::string, std::shared_ptr<Fast::Texture>> textures;
  std::string lastPath;
  int loads = 0;
  bool altAssets = false;
};
class CrossRMRegistry {
public:
  static std::map<std::string, std::shared_ptr<ResourceManager>> managers;
  static std::shared_ptr<ResourceManager> Get(const std::string &owner) {
    auto it = managers.find(owner);
    return it == managers.end() ? nullptr : it->second;
  }
};
std::map<std::string, std::shared_ptr<ResourceManager>>
    CrossRMRegistry::managers;
#include "texture_signature.inc"
} // namespace Ship
namespace Fast {
#include "texture_metadata.inc"
class Interpreter {
public:
  uintptr_t mSegmentPointers[16]{};
  void *SegAddr(uintptr_t address);
  void GfxDpSetTextureImage(uint32_t fmt, uint32_t siz, uint32_t width,
                            const void *path, uint32_t flags,
                            RawTexMetadata metadata, void *pixels) {
    ++sets;
    lastFormat = fmt;
    lastSize = siz;
    lastWidth = width;
    lastPath = path;
    lastFlags = flags;
    lastMetadata = metadata;
    lastPixels = pixels;
  }
  int sets = 0;
  uint32_t lastFormat = 0, lastSize = 0, lastWidth = 0, lastFlags = 0;
  const void *lastPath = nullptr;
  void *lastPixels = nullptr;
  RawTexMetadata lastMetadata{};
};
static auto instance = std::make_shared<Interpreter>();
static std::weak_ptr<Interpreter> mInstance = instance;
static auto host = std::make_shared<Ship::ResourceManager>();
static std::shared_ptr<Ship::ResourceManager> ActiveResMgr() { return host; }
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#include "foreign_texture_production.inc"

using Handler = bool (*)(Interpreter *, F3DGfx **);
static void Submit(Handler handler, const char *path, uint32_t format = 0,
                   uint32_t size = 3) {
  F3DGfx commands[2]{};
  commands[0].words.w0 = (format << 21) | (size << 19) | 31;
  commands[0].words.w1 = reinterpret_cast<uintptr_t>(path);
  const auto submitted = commands[0];
  auto *cursor = commands;
  assert(!handler(instance.get(), &cursor));
  assert(cursor == commands); // Both one-word forms preserve the following command.
  if (handler == gfx_set_timg_otr_filepath_handler_custom) {
    assert(commands[0].words.w0 == submitted.words.w0 &&
           commands[0].words.w1 == submitted.words.w1);
  }
}
static auto MakeTexture(unsigned char value) {
  auto texture = std::make_shared<Fast::Texture>();
  texture->ImageData = new uint8_t[4096]{};
  texture->ImageData[0] = value;
  texture->Width = texture->Height = 32;
  texture->Type = TextureType::RGBA32bpp;
  texture->HByteScale = texture->VPixelScale = 1;
  return texture;
}

static void TestMissingTextureDiagnostics(
    const std::shared_ptr<Ship::ResourceManager> &otherManager,
    const char *validPath) {
  const auto handler = gfx_set_timg_otr_filepath_handler_custom;
  const char *missing = "__OTR__objects/missing/texture";
  Submit(handler, validPath);
  auto sets = instance->sets;
  const auto pixels = instance->lastPixels;
  const int initialLoads = host->loads;
  for (int frame = 0; frame < 1985; ++frame)
    Submit(handler, missing);
  assert(errors.size() == 1); // A visible missing texture must not log every frame.
  assert(errors[0].fields.size() == 3 && errors[0].fields[0] == missing);
  assert(errors[0].format.find("activeRM") != std::string::npos &&
         errors[0].fields[1] == LogField(static_cast<const void *>(host.get())));
  assert(errors[0].format.find("alt") != std::string::npos &&
         errors[0].fields[2] == "0");
  assert(host->loads == initialLoads + 1985); // Suppression never suppresses lookups.
  assert(instance->sets == sets && instance->lastPixels == pixels);

  host->altAssets = true;
  Submit(handler, missing);
  Submit(handler, missing);
  assert(errors.size() == 2 && errors.back().fields[2] == "1");
  host->altAssets = false;
  auto originalHost = host;
  host = otherManager;
  Submit(handler, missing);
  Submit(handler, missing);
  assert(errors.size() == 3 && errors.back().fields[1] ==
                                   LogField(static_cast<const void *>(host.get())));
  host = originalHost;
  Submit(handler, missing); // Returning to a reported context stays quiet.
  assert(errors.size() == 3);

  Submit(handler, "__OTR__objects/another/missing");
  const char *badRoute = "__OTR__@missing:objects/missing/texture";
  Submit(handler, badRoute);
  assert(errors.size() == 5 && errors.back().fields[0] == badRoute);
  for (int path = 0; path < 123; ++path) {
    std::string name = "__OTR__objects/missing/" + std::to_string(path);
    Submit(handler, name.c_str());
  }
  assert(errors.size() == 128);
  Submit(handler, missing);
  assert(errors.size() == 128); // A duplicate at the limit is not an overflow.
  for (int path = 123; path < 300; ++path) {
    std::string name = "__OTR__objects/missing/" + std::to_string(path);
    Submit(handler, name.c_str());
  }
  assert(errors.size() == 129);
  assert(errors.back().format.find("suppress") != std::string::npos);
  assert(instance->sets == sets && instance->lastPixels == pixels);

  // Recovery and successful rendering remain available after log suppression.
  host->textures[missing] = host->textures.at(validPath);
  Submit(handler, missing);
  assert(instance->sets == sets + 1 && instance->lastPixels == pixels);
  assert(errors.size() == 129);
  host->textures.erase(missing);
  std::cout << "PASS missing texture diagnostics: requested path, active RM/Alt "
               "contexts, repeated-frame suppression, 128-entry cap and recovery\n";
}

int main() {
  auto owner = std::make_shared<Ship::ResourceManager>();
  Ship::CrossRMRegistry::managers["oot"] = owner;
  Ship::CrossRMRegistry::managers["mm"] = host;
  const char *scale =
      "__OTR__textures/icon_item_static/gItemIconScaleSilverTex";
  const char *routed =
      "__OTR__@oot:textures/icon_item_static/gItemIconScaleSilverTex";
  const char *cane = "__OTR__textures/icon_item_custom/gItemIconCaneTex";
  const char *routedCane =
      "__OTR__@oot:textures/icon_item_custom/gItemIconCaneTex";
  auto silver = MakeTexture(0x53), hostShadow = MakeTexture(0xFE),
       custom = MakeTexture(0xA1);
  owner->textures[scale] = silver;
  host->textures[scale] = hostShadow;
  owner->textures[cane] = custom;
  TestMissingTextureDiagnostics(owner, scale);
  for (auto handler :
       {gfx_set_timg_handler_rdp, gfx_set_timg_otr_filepath_handler_custom}) {
    auto sets = instance->sets, hostLoads = host->loads;
    Submit(handler, routed);
    assert(instance->sets == sets + 1 &&
           instance->lastPixels == silver->ImageData);
    assert(host->loads == hostLoads && owner->lastPath == scale);
    assert(instance->lastMetadata.width == 32 &&
           instance->lastMetadata.height == 32);
    assert(instance->lastSize == 3 &&
           instance->lastMetadata.type == TextureType::RGBA32bpp);
    Submit(handler,
           scale); // Route must not leak into the next ordinary texture.
    assert(instance->lastPixels == hostShadow->ImageData);
    Submit(handler, routedCane);
    assert(instance->lastPixels == custom->ImageData);
    Submit(handler, routed, 3,
           1); // The command format/size remains authoritative for IA8 draws.
    assert(instance->lastFormat == 3 && instance->lastSize == 1);
    sets = instance->sets;
    hostLoads = host->loads;
    for (const char *bad :
         {"__OTR__@missing:textures/icon_item_static/gItemIconScaleSilverTex",
          "__OTR__@oot:textures/absent", "__OTR__@oot",
          "__OTR__@:textures/test"}) {
      Submit(handler, bad);
      assert(instance->sets == sets && host->loads == hostLoads);
    }
  }
  alignas(8) unsigned char raw[128]{};
  Submit(gfx_set_timg_handler_rdp, reinterpret_cast<char *>(raw));
  assert(instance->lastPixels == raw &&
         instance->lastMetadata.h_byte_scale == 1);
  auto sets = instance->sets;
  Submit(gfx_set_timg_handler_rdp, reinterpret_cast<char *>(0x08000000));
  assert(instance->sets == sets);
  std::cout
      << "PASS production texture owner routing: scale RGBA32, custom/IA8, "
         "host isolation, invalid routes and raw/segment paths\n";
  return 0;
}
} // namespace Fast
int main() { return Fast::main(); }
