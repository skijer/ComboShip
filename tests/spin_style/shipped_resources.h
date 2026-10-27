// Test-only loader for the actual private F3DEX2 command streams. Each stored
// 32-bit word pair becomes one native Gfx entry, as in the resource importer.
#include <fstream>
#include <unordered_map>

static bool shippedResources;
static std::unordered_map<std::string, std::vector<Gfx>> shippedLists;

static uint64_t ShippedResourceHash(const std::string &name) {
  uint64_t hash = UINT64_MAX;
  for (uint8_t byte : name) {
    hash ^= uint64_t(byte) << 56;
    for (int bit = 0; bit < 8; ++bit)
      hash = (hash << 1) ^ ((hash >> 63) ? UINT64_C(0x42F0E1EBA9EA3693) : 0);
  }
  return hash;
}

static bool IsShippedList(const char *name) {
  const char *leaf = std::strrchr(name, '/');
  return leaf && (!std::strncmp(leaf + 1, "disk_", 5) ||
                  !std::strncmp(leaf + 1, "cylinder_", 9) ||
                  !std::strcmp(leaf + 1, "charge"));
}

static const std::vector<Gfx> &ShippedList(const char *name) {
  auto found = shippedLists.find(name);
  if (found != shippedLists.end())
    return found->second;
#ifdef HOST_MM
  const std::string path = std::string("mm/assets/custom/") + name;
#else
  const std::string path = std::string("soh/assets/custom/") + name;
#endif
  std::ifstream stream(path, std::ios::binary);
  assert(stream && "Prepare the shipped spin resources before this test");
  stream.seekg(72);
  std::vector<Gfx> commands;
  uint32_t words[2];
  while (stream.read(reinterpret_cast<char *>(words), sizeof(words))) {
    Gfx command = {};
    command.words.w0 = words[0];
    command.words.w1 = words[1];
    commands.push_back(command);
  }
  assert(!commands.empty() && commands.back().words.w0 >> 24 == 0xdf);
  return shippedLists.emplace(name, std::move(commands)).first->second;
}

static void CheckShippedMaterial(const Gfx *drawn,
                                 const std::vector<Gfx> &authored,
                                 const std::string &texture0,
                                 const std::string &texture1) {
  const uint64_t expected[] = {ShippedResourceHash(texture0),
                               ShippedResourceHash(texture1)};
  size_t textures = 0;
  for (size_t i = 0; i < authored.size(); ++i) {
    const uint8_t op = authored[i].words.w0 >> 24;
    assert(drawn[i].words.w0 == authored[i].words.w0 &&
           drawn[i].words.w1 == authored[i].words.w1);
    if (op == 0x20) {
      assert(textures < 2 && i + 1 < authored.size());
      ++i;
      const uint64_t actual =
          uint64_t(drawn[i].words.w0) << 32 | uint32_t(drawn[i].words.w1);
      if (actual != expected[textures])
        std::fprintf(stderr, "Wrong spin material: expected %s\n",
                     (textures ? texture1 : texture0).c_str());
      assert(actual == expected[textures++]);
    } else if (op == 0x31 || op == 0x32 || op == 0x33 || op == 0x36 ||
               op == 0x37 || op == 0x39) {
      ++i;
      assert(i < authored.size() && drawn[i].words.w0 == authored[i].words.w0 &&
             drawn[i].words.w1 == authored[i].words.w1);
    }
  }
  assert(textures == 2);
}
