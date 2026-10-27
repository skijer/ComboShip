// Real LUS types, registry, scope and Blob factory. Only the resource lookup
// boundary is controlled; no replacement headers or copy/selection code exists.
#include "../../combo/NeiWolfAssetResource.h"
#include <ship/resource/factory/BlobFactory.h>
#include <ship/utils/binarytools/MemoryStream.h>
#include <cassert>
#include <fstream>
#include <iterator>
#include <map>
#include <set>

struct OwnerFixture {
    std::shared_ptr<Ship::ArchiveManager> archives;
    std::map<std::string, std::shared_ptr<Ship::IResource>> resources;
    bool alt = false;
};
static std::map<Ship::ResourceManager*, OwnerFixture> fixtures;
static std::map<Ship::ArchiveManager*, std::set<std::string>> files;
static std::shared_ptr<Ship::ResourceManager> active;
static Ship::Context* context;
static std::vector<std::pair<Ship::ResourceManager*, std::string>> loads;
namespace Ship {
class FixtureContext final : public Context {};
Context::~Context() = default;
Context* Context::GetRawInstance() {
    return context;
}
std::shared_ptr<ResourceManager> Context::GetResourceManager() const {
    return active;
}
void Context::SetResourceManager(std::shared_ptr<ResourceManager> manager) {
    active = std::move(manager);
}
ResourceManager::ResourceManager() = default;
ResourceManager::~ResourceManager() = default;
std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() {
    return fixtures[this].archives;
}
bool ResourceManager::IsAltAssetsEnabled() {
    return fixtures[this].alt;
}
std::shared_ptr<IResource> ResourceManager::LoadResource(const std::string& path, bool exact,
                                                         std::shared_ptr<ResourceInitData>) {
    assert(exact && "the selected Alt resource must never silently fall back to base");
    assert(active.get() == this && "nested resource loads must use the selected game's manager");
    loads.emplace_back(this, path);
    return fixtures[this].resources[path];
}
ArchiveManager::ArchiveManager() = default;
ArchiveManager::~ArchiveManager() = default;
bool ArchiveManager::HasFile(const std::string& path) {
    return files[this].count(path);
}
} // namespace Ship

class WrongResource final : public Ship::Resource<void> {
  public:
    using Ship::Resource<void>::Resource;
    void* GetPointer() override {
        return nullptr;
    }
    size_t GetPointerSize() override {
        return 0;
    }
};

int main(int argc, char** argv) {
  assert(argc == 2 || argc == 3);
  std::ifstream input(argv[1], std::ios::binary);
  auto envelope = std::make_shared<std::vector<char>>(
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  assert(envelope->size() >= 68);
  auto metadata = std::make_shared<Ship::ResourceInitData>();
  metadata->Path = NeiWolfAsset::kResourcePath;
  metadata->Format = RESOURCE_FORMAT_BINARY;
  auto file = std::make_shared<Ship::File>();
  file->Buffer = envelope;
  file->Reader = std::make_shared<Ship::BinaryReader>(
      std::make_shared<Ship::MemoryStream>(envelope, 64));
  std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader)
      ->SetEndianness(Ship::Endianness::Little);
  Ship::ResourceFactoryBinaryBlobV0 factory;
  auto real = std::dynamic_pointer_cast<Ship::Blob>(
      factory.ReadResource(file, metadata));
  assert(real && real->Data.size() == envelope->size() - 68 + 16);
  assert(NeiWolfAsset::ResourcePayloadSize(
             real->Data.data(), real->Data.size()) == envelope->size() - 68);
  auto invalid = std::make_shared<Ship::Blob>(metadata);
  invalid->Data = real->Data;
  invalid->Data[0] = 'X';
  auto wrong = std::make_shared<WrongResource>(metadata);
  auto mm = std::make_shared<Ship::ResourceManager>(),
       oot = std::make_shared<Ship::ResourceManager>();
  fixtures[mm.get()].archives = std::make_shared<Ship::ArchiveManager>();
  fixtures[oot.get()].archives = std::make_shared<Ship::ArchiveManager>();
  const std::string base = NeiWolfAsset::kResourcePath, alt = "alt/" + base;
  files[fixtures[mm.get()].archives.get()].insert(base);
  files[fixtures[oot.get()].archives.get()].insert(base);
  fixtures[mm.get()].resources[base] = real;
  fixtures[oot.get()].resources[base] = real;
  Ship::CrossRMRegistry::Register("mm", mm);
  Ship::CrossRMRegistry::Register("oot", oot);
  Ship::FixtureContext nativeContext;
  context = &nativeContext;
  active = oot;
  size_t size = 0;
  const char *owner = nullptr;
  assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == 1);
  assert(std::string(owner) == "mm" && active == oot &&
         loads.back().first == mm.get() && loads.back().second == base);
  std::vector<uint8_t> copied(size);
  assert(NeiWolfAsset::CopyResource("mm", copied.data(), copied.size(), &size,
                                    nullptr) == 1);
  assert(NeiWolfAsset::Validate(copied));
  assert(NeiWolfAsset::CopyResource("mm", copied.data(), 1, &size, nullptr) ==
             -1 &&
         active == oot);
  fixtures[mm.get()].alt = true;
  files[fixtures[mm.get()].archives.get()].insert(alt);
  fixtures[mm.get()].resources[alt] = invalid;
  size_t before = loads.size();
  assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == -1);
  assert(loads.size() == before + 1 && loads.back().second == alt &&
         active == oot);
  fixtures[mm.get()].resources[alt] = wrong;
  assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == -1 &&
         loads.back().second == alt);
  files[fixtures[mm.get()].archives.get()].erase(alt);
  files[fixtures[mm.get()].archives.get()].insert(alt + ".meta");
  fixtures[mm.get()].resources[alt] = nullptr;
  before = loads.size();
  assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == -1);
  assert(loads.size() == before + 1 && loads.back().second == alt &&
         active == oot);
  fixtures[mm.get()].resources[alt] = real;
  assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == 1 &&
         loads.back().second == alt);
  fixtures[mm.get()].alt = false;
  assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == 1 &&
         loads.back().second == base);
  if (argc == 3) {
    std::ifstream hdInput(argv[2], std::ios::binary);
    auto hdEnvelope = std::make_shared<std::vector<char>>(
        std::istreambuf_iterator<char>(hdInput),
        std::istreambuf_iterator<char>());
    auto hdFile = std::make_shared<Ship::File>();
    hdFile->Buffer = hdEnvelope;
    hdFile->Reader = std::make_shared<Ship::BinaryReader>(
        std::make_shared<Ship::MemoryStream>(hdEnvelope, 64));
    std::get<std::shared_ptr<Ship::BinaryReader>>(hdFile->Reader)
        ->SetEndianness(Ship::Endianness::Little);
    const auto hd = std::dynamic_pointer_cast<Ship::Blob>(
        factory.ReadResource(hdFile, metadata));
    assert(hd &&
           NeiWolfAsset::ResourcePayloadSize(hd->Data.data(), hd->Data.size()));
    const std::string hdPath = NeiWolfAsset::kHDResourcePath,
                      hdAlt = NeiWolfAsset::kAltHDResourcePath;
    const char *key = nullptr;
    files[fixtures[mm.get()].archives.get()].insert(hdPath);
    files[fixtures[oot.get()].archives.get()].insert(hdPath);
    fixtures[mm.get()].resources[hdPath] = hd;
    fixtures[oot.get()].resources[hdPath] = hd;
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true,
                                      &key) == 1);
    assert(std::string(key) == hdPath && std::string(owner) == "mm" &&
           loads.back().first == mm.get());
    copied.resize(size);
    assert(NeiWolfAsset::CopyResource("mm", copied.data(), copied.size(), &size,
                                      nullptr, true) == 1);
    assert(std::equal(copied.begin(), copied.end(), hd->Data.begin()) &&
           NeiWolfAsset::Validate(copied));
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, false,
                                      &key) == 1 &&
           std::string(key) == base);
    fixtures[mm.get()].alt = true;
    files[fixtures[mm.get()].archives.get()].insert(hdAlt);
    fixtures[mm.get()].resources[hdAlt] = invalid;
    before = loads.size();
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true,
                                      &key) == -1);
    assert(loads.size() == before + 1 && std::string(key) == hdAlt &&
           active == oot);
    fixtures[mm.get()].resources[hdAlt] = wrong;
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true) ==
           -1);
    fixtures[mm.get()].resources[hdAlt] = hd;
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true,
                                      &key) == 1 &&
           std::string(key) == hdAlt);
    fixtures[mm.get()].alt = false;
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true,
                                      &key) == 1 &&
           std::string(key) == hdPath);
    files[fixtures[mm.get()].archives.get()].erase(hdPath);
    files[fixtures[mm.get()].archives.get()].insert(hdPath + ".meta");
    fixtures[mm.get()].resources[hdPath] = nullptr;
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true) ==
           -1);
    files[fixtures[mm.get()].archives.get()].erase(hdPath + ".meta");
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true,
                                      &key) == 1 &&
           std::string(key) == base && std::string(owner) == "mm");
    files[fixtures[mm.get()].archives.get()].clear();
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner, true,
                                      &key) == 1 &&
           std::string(key) == hdPath && std::string(owner) == "oot");
    std::puts("PASS native HD/standard choice, per-model Alt, exact copied HD "
              "Blob, legacy fallback and rejection");
  }
    files[fixtures[mm.get()].archives.get()].clear();
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == 1);
    assert(std::string(owner) == "oot" && loads.back().first == oot.get() && active == oot);
    Ship::CrossRMRegistry::Unregister("mm");
    Ship::CrossRMRegistry::Unregister("oot");
    before = loads.size();
    assert(NeiWolfAsset::CopyResource("mm", nullptr, 0, &size, &owner) == 0);
    assert(loads.size() == before && active == oot && "unregistered owners must never fall back to active Context");
    std::puts("PASS real Blob factory; exact native/donor owner, Alt selection/rejection, scope restoration, owned "
              "copy and bounds");
}
