// The real loader, private resource cache, resource destructors and instrument
// lookup run here. Archive parsing and engine startup are service boundaries.
#include "soh/resource/type/AudioSoundFont.h"
#include "soh/resource/type/AudioSequence.h"
#undef BE16SWAP_CONST
#include "mods/sound_translator/mm_sfx_synth_loader.cpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
std::shared_ptr<SOH::AudioSoundFont> globalFonts[2], residentFonts[2];
std::shared_ptr<SOH::AudioSequence> globalSequence, residentSequence;
bool assetsAvailable = true, failFont = false, engineReady = false, globalMetaAlias = false;
int engineStarts = 0;

std::shared_ptr<SOH::AudioSoundFont> makeFont(uint8_t releaseRate) {
    auto resource = std::make_shared<SOH::AudioSoundFont>();
    for (int i = 0; i < 4; ++i) {
        auto instrument = new SOH::Instrument{};
        instrument->releaseRate = releaseRate;
        instrument->envelope = new SOH::AdsrEnvelope[1]{};
        resource->instrumentAddresses.push_back(instrument);
    }
    resource->soundFont.numInstruments = resource->instrumentAddresses.size();
    resource->soundFont.instruments = resource->instrumentAddresses.data();
    return resource;
}

std::shared_ptr<SOH::AudioSequence> makeSequence(char firstByte) {
    auto resource = std::make_shared<SOH::AudioSequence>();
    resource->sequence = {};
    resource->sequence.seqData = new char[3]{firstByte, 0x20, char(0xFF)};
    resource->sequence.seqDataSize = 3;
    resource->sequence.numFonts = 2;
    resource->sequence.fonts[0] = 1;
    resource->sequence.fonts[1] = 0;
    return resource;
}
}

// The archive and resource factory return parsed resources; the included
// production cache must retain them after all external owners are released.
struct Archive {
    auto LoadFile(const std::string& path) { return std::make_shared<std::string>(path); }
};
struct ResourceLoader {
    std::shared_ptr<Ship::IResource> LoadResource(const std::string& path,
                                               const std::shared_ptr<std::string>& file,
                                               std::shared_ptr<Ship::ResourceInitData> = nullptr,
                                               std::shared_ptr<Archive> archive = nullptr) {
        assert(path == *file);
        // The production parser can follow global .meta aliases unless the
        // caller supplies the explicitly selected archive. Model that service
        // contract here; the separate native parser regression covers parsing.
        if (globalMetaAlias && !archive) {
            if (path == "audio/sequences/Sequence_0") return globalSequence;
            return globalFonts[path.ends_with("_1") ? 1 : 0];
        }
        if (path == "audio/sequences/Sequence_0")
            return residentSequence;
        if (path == "audio/fonts/Soundfont_0")
            return residentFonts[0];
        assert(path == "audio/fonts/Soundfont_1");
        return residentFonts[1];
    }
};
struct ResourceManager {
    auto GetResourceLoader() { return std::make_shared<ResourceLoader>(); }
};
struct Context {
    auto GetResourceManager() { return std::make_shared<ResourceManager>(); }
};
struct OTRGlobals {
    inline static OTRGlobals* Instance;
    std::shared_ptr<Context> context = std::make_shared<Context>();
};
static std::shared_ptr<Archive> sMmArchive = std::make_shared<Archive>();
static std::unordered_map<std::string, std::shared_ptr<Ship::IResource>> sMmResourceCache;
#define MMASSETS_LOG(...) ((void)0)
#include "resident_load.inc"

extern "C" uint8_t MmAssets_IsAvailable(void) { return assetsAvailable; }
extern "C" ::SoundFont* ResourceMgr_LoadAudioSoundFontByName(const char* path) {
    int index = std::string(path).ends_with("_1") ? 1 : 0;
    return failFont ? nullptr : reinterpret_cast<::SoundFont*>(globalFonts[index]->GetPointer());
}
extern "C" ::SoundFont* MmSfx_LoadFont(int32_t index) {
    const char* path = index == 1 ? "audio/fonts/Soundfont_1" : "audio/fonts/Soundfont_0";
    return failFont ? nullptr : static_cast<::SoundFont*>(MmAssets_LoadFromMmArchive(path, nullptr));
}
extern "C" SequenceData* ResourceMgr_LoadSeqPtrByName(const char*) {
    return reinterpret_cast<SequenceData*>(globalSequence->GetPointer());
}
extern "C" void MmSfxBridge_PatchFontSamples(::SoundFont*, const char*) {}

namespace mmsfx {
AudioContext gMmSfx{};
void MmSfxSynth_InitEngine(void) {
    ++engineStarts;
    for (auto& channel : gMmSfx.seqPlayers[0].channels)
        channel = &gMmSfx.sequenceChannelNone;
}
void AudioScript_ResetSequencePlayer(SequencePlayer*) {}
void MmSfxSynth_MarkReady(bool ready) { engineReady = ready; }

// Filled by the runner from the production playback and seqplayer functions.
#include "instrument_read.inc"
}

int main(int argc, char** argv) {
    assert(argc == 2);
    OTRGlobals globals;
    OTRGlobals::Instance = &globals;
    std::string test = argv[1];
    globalMetaAlias = test == "meta-owner";
    for (int index = 0; index < 2; ++index) {
        globalFonts[index] = makeFont(3); // Global same-name resource may be OoT/mod-owned.
        residentFonts[index] = makeFont(9);
    }
    globalSequence = makeSequence(0);
    residentSequence = makeSequence(char(0xFD));

    if (test == "missing-assets") {
        assetsAvailable = false;
        assert(MmSfxSynth_Init() == 0);
        assert(engineStarts == 0 && !engineReady && !MmSfxSynth_IsReady());
    } else {
        if (test == "retry") {
            failFont = true;
            assert(MmSfxSynth_Init() == 0);
            assert(!engineReady && !MmSfxSynth_IsReady());
            failFont = false;
        }
        assert(MmSfxSynth_Init() == 1 && engineReady && MmSfxSynth_IsReady());
        if (test == "evicted-font") {
            // RegisterMmFonts calls ResourceUnloadByName after the SFX loader
            // has copied the font. Destroy the actual resource owner, then read
            // instrument 2 through the same production functions as the crash.
            globalFonts[0].reset();
            std::weak_ptr<SOH::AudioSoundFont> resident = residentFonts[0];
            residentFonts[0].reset();
            mmsfx::SequenceChannel channel{};
            channel.fontId = 0;
            mmsfx::Instrument* instrument = nullptr;
            mmsfx::AdsrSettings adsr{};
            assert(mmsfx::AudioScript_GetInstrument(&channel, 2, &instrument, &adsr) == 4);
            assert(instrument && adsr.decayIndex == 9 && adsr.envelope == instrument->envelope);
            assert(!resident.expired());
        } else if (test == "evicted-sequence") {
            globalSequence.reset();
            std::weak_ptr<SOH::AudioSequence> resident = residentSequence;
            residentSequence.reset();
            assert(*mmsfx::gMmSfx.seqPlayers[0].scriptState.pc == 0xFD);
            assert(!resident.expired());
        } else if (test == "native-owner" || test == "meta-owner") {
            for (int index = 0; index < 2; ++index) {
                assert(mmsfx::gMmSfx.soundFontList[index].instruments ==
                       reinterpret_cast<mmsfx::Instrument**>(residentFonts[index]->soundFont.instruments));
            }
            assert(mmsfx::gMmSfx.seqPlayers[0].seqData ==
                   reinterpret_cast<uint8_t*>(residentSequence->sequence.seqData));
        }
        int starts = engineStarts;
        assert(MmSfxSynth_Init() == 1 && engineStarts == starts);
    }
    std::printf("PASS %s\n", argv[1]);
}
