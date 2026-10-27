#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#include <ship/Context.h>
#include <ship/resource/ResourceLoader.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include "soh/resource/importer/AudioSequenceFactory.h"
#include "soh/resource/importer/AudioSoundFontFactory.h"
#include "soh/resource/type/AudioSequence.h"
#include "soh/resource/type/SohResourceType.h"

// Execute the production binary importers without unrelated XML/streaming code.
#define COMBO_OWN_RM() (Ship::Context::GetRawInstance()->GetResourceManager())
namespace SOH {
#include "native_audio_importers.inc"
} // namespace SOH

class MemoryArchive final : public Ship::Archive {
  public:
    explicit MemoryArchive(const std::string& name) : Archive(name) {}
    std::map<std::string, std::vector<char>> files;

    std::shared_ptr<Ship::File> LoadFile(const std::string& path) override {
        auto found = files.find(path);
        if (found == files.end()) {
            return nullptr;
        }
        auto file = std::make_shared<Ship::File>();
        file->Buffer = std::make_shared<std::vector<char>>(found->second);
        file->IsLoaded = true;
        return file;
    }
    std::shared_ptr<Ship::File> LoadFile(uint64_t) override { return nullptr; }
    bool Open() override { return true; }
    bool Close() override { return true; }
    bool WriteFile(const std::string&, const std::vector<uint8_t>&) override { return false; }
};

static std::vector<std::shared_ptr<MemoryArchive>> globalArchives;
static size_t globalFileLoads;
static size_t nestedResourceLoads;
static std::shared_ptr<Ship::ResourceManager> manager;
static std::shared_ptr<Ship::ResourceLoader> loader;

namespace Ship {
// Archive opening, global mount lookup and the Context service are the I/O seams.
Archive::Archive(const std::string& path) : mPath(path) {}
Archive::~Archive() = default;

Context* Context::GetRawInstance() {
    static Context context;
    return &context;
}
std::shared_ptr<ResourceManager> Context::GetResourceManager() const { return manager; }
std::shared_ptr<File> ResourceManager::LoadFileProcess(const std::string& path) {
    ++globalFileLoads;
    for (auto it = globalArchives.rbegin(); it != globalArchives.rend(); ++it) {
        if (auto file = (*it)->LoadFile(path)) {
            return file;
        }
    }
    return nullptr;
}
int32_t ArchiveManager::GetFilePriority(const std::string& path) {
    for (int32_t i = static_cast<int32_t>(globalArchives.size()) - 1; i >= 0; --i) {
        if (globalArchives[i]->files.contains(path)) {
            return i;
        }
    }
    return -1;
}
std::shared_ptr<IResource> ResourceManager::LoadResourceProcess(const std::string&) {
    ++nestedResourceLoads;
    return nullptr;
}
std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() {
    static auto archiveManager = std::make_shared<ArchiveManager>();
    return archiveManager;
}
std::shared_ptr<ResourceLoader> ResourceManager::GetResourceLoader() { return loader; }
} // namespace Ship

struct OTRGlobals {
    static OTRGlobals* Instance;
    Ship::Context* context = Ship::Context::GetRawInstance();
};
OTRGlobals* OTRGlobals::Instance;
static std::shared_ptr<Ship::Archive> sMmArchive;
static std::unordered_map<std::string, std::shared_ptr<Ship::IResource>> sMmResourceCache;
#define MMASSETS_LOG(...) ((void)0)
#include "native_asset_load.inc"

static constexpr uint32_t FONT_TYPE = static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSoundFont);
static constexpr uint32_t SEQUENCE_TYPE = static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSequence);
static constexpr uint64_t HEADER_ID = 0x123456789ABCDEF0;
static constexpr const char* SEQUENCE_PATH = "audio/sequences/Sequence_0";

static void Append(std::vector<char>& bytes, uint64_t value, size_t count,
                   Ship::Endianness order = Ship::Endianness::Native) {
    for (size_t i = 0; i < count; ++i) {
        size_t shift = order == Ship::Endianness::Big ? count - 1 - i : i;
        bytes.push_back(static_cast<char>((value >> (8 * shift)) & 0xFF));
    }
}

static std::vector<char> Header(uint32_t type, Ship::Endianness order, bool custom = false) {
    std::vector<char> bytes{static_cast<char>(order), static_cast<char>(custom), 0, 0};
    Append(bytes, type, 4, order);
    Append(bytes, 2, 4, order);
    Append(bytes, HEADER_ID, 8, order);
    bytes.resize(OTR_HEADER_SIZE);
    return bytes;
}

static std::vector<char> Font(int index, uint8_t release, bool headed = true,
                             Ship::Endianness order = Ship::Endianness::Native) {
    auto bytes = headed ? Header(FONT_TYPE, order, true) : std::vector<char>();
    Append(bytes, index, 4, order);
    Append(bytes, 2, 1); // medium
    Append(bytes, 1, 1); // cachePolicy
    Append(bytes, 0x0102, 2, order);
    Append(bytes, 0, 2, order);
    Append(bytes, 0, 2, order);
    Append(bytes, 0, 4, order); // drums
    Append(bytes, 1, 4, order); // instruments
    Append(bytes, 0, 4, order); // SFX
    Append(bytes, 1, 1); // valid instrument
    Append(bytes, 0, 1); // loaded
    Append(bytes, 0, 1); // normalRangeLo
    Append(bytes, 127, 1); // normalRangeHi
    Append(bytes, release, 1);
    Append(bytes, 1, 4, order); // envelope count
    Append(bytes, 0xFFFF, 2, order);
    Append(bytes, 0, 2, order);
    Append(bytes, 0, 1); // low note sample absent
    Append(bytes, 0, 1); // normal note sample absent
    Append(bytes, 0, 1); // high note sample absent
    return bytes;
}

static std::vector<char> Sequence(uint8_t opcode, bool headed = true,
                                 Ship::Endianness order = Ship::Endianness::Native) {
    auto bytes = headed ? Header(SEQUENCE_TYPE, order) : std::vector<char>();
    Append(bytes, 3, 4, order);
    Append(bytes, opcode, 1);
    Append(bytes, 0x20, 1);
    Append(bytes, 0xFF, 1);
    Append(bytes, 0, 1); // sequence number
    Append(bytes, 2, 1); // medium
    Append(bytes, 1, 1); // cachePolicy
    Append(bytes, 2, 4, order);
    Append(bytes, 1, 1);
    Append(bytes, 0, 1);
    return bytes;
}

static std::vector<char> Meta(const std::string& type, const std::string& target, bool custom = false) {
    std::string json = "{\"format\":\"Binary\",\"type\":\"" + type + "\",\"version\":2";
    if (!target.empty()) {
        json += ",\"path\":\"" + target + "\"";
    }
    json += ",\"isCustom\":" + std::string(custom ? "true" : "false") + "}";
    std::vector<char> bytes(json.begin(), json.end());
    bytes.push_back('\0');
    return bytes;
}

static std::shared_ptr<SOH::AudioSoundFont> CheckFont(const std::shared_ptr<Ship::IResource>& resource,
                                                   int index, uint8_t release) {
    auto font = std::dynamic_pointer_cast<SOH::AudioSoundFont>(resource);
    assert(font);
    assert(font->soundFont.fntIndex == index);
    assert(font->soundFont.numInstruments == 1);
    assert(font->soundFont.instruments[0]->releaseRate == release);
    assert(font->soundFont.instruments[0]->normalRangeHi == 127);
    assert(font->soundFont.sampleBankId1 == 1 && font->soundFont.sampleBankId2 == 2);
    return font;
}

static std::shared_ptr<SOH::AudioSequence> CheckSequence(const std::shared_ptr<Ship::IResource>& resource,
                                                       uint8_t opcode) {
    auto sequence = std::dynamic_pointer_cast<SOH::AudioSequence>(resource);
    assert(sequence);
    assert(sequence->sequence.seqDataSize == 3);
    assert(static_cast<uint8_t>(sequence->sequence.seqData[0]) == opcode);
    assert(static_cast<uint8_t>(sequence->sequence.seqData[2]) == 0xFF);
    assert(sequence->sequence.numFonts == 2);
    assert(sequence->sequence.fonts[0] == 1 && sequence->sequence.fonts[1] == 0);
    assert(sequence->sequence.resolvedFont == -1);
    return sequence;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    std::string test = argv[1];
    manager = std::make_shared<Ship::ResourceManager>();
    loader = std::make_shared<Ship::ResourceLoader>();
    assert(loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSoundFontV2>(),
                                           RESOURCE_FORMAT_BINARY, "AudioSoundFont", FONT_TYPE, 2));
    assert(loader->RegisterResourceFactory(std::make_shared<SOH::ResourceFactoryBinaryAudioSequenceV2>(),
                                           RESOURCE_FORMAT_BINARY, "AudioSequence", SEQUENCE_TYPE, 2));
    OTRGlobals globals;
    OTRGlobals::Instance = &globals;
    auto mm = std::make_shared<MemoryArchive>("mm.o2r");
    auto global = std::make_shared<MemoryArchive>("higher-priority-mod.o2r");
    sMmArchive = mm;
    globalArchives = {mm, global};
    for (int i = 0; i < 2; ++i) {
        mm->files["audio/fonts/Soundfont_" + std::to_string(i)] = Font(64 + i, 9 + i);
    }
    mm->files[SEQUENCE_PATH] = Sequence(0xFD);

    if (test == "global-font-meta") {
        for (int i = 0; i < 2; ++i) {
            std::string path = "audio/fonts/Soundfont_" + std::to_string(i);
            std::string target = "global/font_" + std::to_string(i);
            global->files[path + ".meta"] = Meta("AudioSoundFont", target);
            global->files[target] = Font(220 + i, 2 + i);
            // Control: the real parser's unscoped overload follows the global redirect.
            CheckFont(loader->LoadResource(path, mm->LoadFile(path)), 220 + i, 2 + i);
            assert(globalFileLoads > 0);
            globalFileLoads = 0;
            CheckFont(MmAssets_LoadResourceObjectFromMmArchive(path.c_str()), 64 + i, 9 + i);
            assert(globalFileLoads == 0);
        }
    } else if (test == "global-sequence-meta") {
        global->files[std::string(SEQUENCE_PATH) + ".meta"] = Meta("AudioSequence", "global/sequence");
        global->files["global/sequence"] = Sequence(0xEE);
        CheckSequence(loader->LoadResource(SEQUENCE_PATH, mm->LoadFile(SEQUENCE_PATH)), 0xEE);
        assert(globalFileLoads > 0);
        globalFileLoads = 0;
        CheckSequence(MmAssets_LoadResourceObjectFromMmArchive(SEQUENCE_PATH), 0xFD);
        assert(globalFileLoads == 0);
    } else if (test == "local-meta-headed" || test == "local-meta-headerless") {
        bool headed = test == "local-meta-headed";
        mm->files["audio/fonts/Soundfont_0.meta"] = Meta("AudioSoundFont", "mm/alias-font", true);
        mm->files["mm/alias-font"] = Font(100, 17, headed);
        mm->files[std::string(SEQUENCE_PATH) + ".meta"] = Meta("AudioSequence", "mm/alias-sequence");
        mm->files["mm/alias-sequence"] = Sequence(0xCC, headed);
        // Conflicting global sidecars and targets must not displace either local alias.
        global->files["audio/fonts/Soundfont_0.meta"] = Meta("AudioSoundFont", "global/font");
        global->files["global/font"] = Font(220, 2);
        global->files[std::string(SEQUENCE_PATH) + ".meta"] = Meta("AudioSequence", "global/sequence");
        global->files["global/sequence"] = Sequence(0xEE);
        auto font = CheckFont(MmAssets_LoadResourceObjectFromMmArchive("audio/fonts/Soundfont_0"), 100, 17);
        auto sequence = CheckSequence(MmAssets_LoadResourceObjectFromMmArchive(SEQUENCE_PATH), 0xCC);
        assert(font->GetInitData()->Path == "mm/alias-font" && font->GetInitData()->IsCustom);
        assert(sequence->GetInitData()->Path == "mm/alias-sequence");
        assert(globalFileLoads == 0);
    } else if (test == "missing-local-meta-target") {
        mm->files["audio/fonts/Soundfont_0.meta"] = Meta("AudioSoundFont", "missing/font");
        mm->files[std::string(SEQUENCE_PATH) + ".meta"] = Meta("AudioSequence", "missing/sequence");
        global->files["missing/font"] = Font(220, 2);
        global->files["missing/sequence"] = Sequence(0xEE);
        auto font = CheckFont(MmAssets_LoadResourceObjectFromMmArchive("audio/fonts/Soundfont_0"), 64, 9);
        auto sequence = CheckSequence(MmAssets_LoadResourceObjectFromMmArchive(SEQUENCE_PATH), 0xFD);
        assert(font->GetInitData()->Path == "audio/fonts/Soundfont_0");
        assert(sequence->GetInitData()->Path == SEQUENCE_PATH);
        assert(globalFileLoads == 0);
    } else if (test == "local-meta-default-path") {
        mm->files["audio/fonts/Soundfont_0.meta"] = Meta("AudioSoundFont", "", true);
        mm->files[std::string(SEQUENCE_PATH) + ".meta"] = Meta("AudioSequence", "");
        auto font = CheckFont(MmAssets_LoadResourceObjectFromMmArchive("audio/fonts/Soundfont_0"), 64, 9);
        CheckSequence(MmAssets_LoadResourceObjectFromMmArchive(SEQUENCE_PATH), 0xFD);
        assert(font->GetInitData()->Path == "audio/fonts/Soundfont_0" && font->GetInitData()->IsCustom);
        assert(globalFileLoads == 0);
    } else if (test == "native-headers") {
        for (auto order : {Ship::Endianness::Little, Ship::Endianness::Big}) {
            sMmResourceCache.clear();
            mm->files["audio/fonts/Soundfont_0"] = Font(64, 9, true, order);
            mm->files[SEQUENCE_PATH] = Sequence(0xFD, true, order);
            auto font = CheckFont(MmAssets_LoadResourceObjectFromMmArchive("audio/fonts/Soundfont_0"), 64, 9);
            auto sequence = CheckSequence(MmAssets_LoadResourceObjectFromMmArchive(SEQUENCE_PATH), 0xFD);
            for (const auto& resource : {std::static_pointer_cast<Ship::IResource>(font),
                                         std::static_pointer_cast<Ship::IResource>(sequence)}) {
                assert(resource->GetInitData()->Id == HEADER_ID);
                assert(resource->GetInitData()->ResourceVersion == 2);
                assert(resource->GetInitData()->ByteOrder == order);
            }
            assert(font->GetInitData()->IsCustom && !sequence->GetInitData()->IsCustom);
        }
        assert(globalFileLoads == 0);
    } else {
        assert(false && "unknown case");
    }
    assert(nestedResourceLoads == 0);
    std::cout << "PASS parser " << test << '\n';
}
