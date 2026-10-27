#include "audio/soundfont.h"
#include "../../combo/audio/MMAudioTraceBridge.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Only the resource-container boundary is replaced. The production callbacks,
// classifier and decoder below run against the actual Ogg/Vorbis libraries.
namespace Ship {
struct File {
    std::shared_ptr<std::vector<char>> Buffer;
};
struct ResourceInitData {
    std::string Path;
};
} // namespace Ship
namespace SOH {
struct AudioSample {
    Sample sample{};
    ~AudioSample() {
        delete[] sample.sampleAddr;
    }
};
} // namespace SOH
static unsigned errors;
static std::string errorPath;
static bool failVorbisRead;
static long ReadVorbis(OggVorbis_File* file, char* buffer, int length, int bigEndian, int wordSize, int isSigned,
                       int* bitstream) {
    // Exercise the decoder's response to a documented library error after a
    // real, successful open. All normal cases use the actual Vorbis decoder.
    if (failVorbisRead) {
        return OV_HOLE;
    }
    return ov_read(file, buffer, length, bigEndian, wordSize, isSigned, bitstream);
}
template <typename... Args> static void RecordError(const char*, const std::string& path, Args&&...) {
    ++errors;
    errorPath = path;
}
#define SPDLOG_ERROR(...) RecordError(__VA_ARGS__)
#define ov_read ReadVorbis
#include "audio_decoder_production.inc"
#undef ov_read

// Run the actual streamed-sample switch branch with observable audio-driver
// boundaries. The runtime runner also syntax-checks the complete synthesis.c.
static unsigned pcmLoads;
static unsigned opusLoads;
static bool cleared;
static void CheckPcmLoad(const uint8_t* source, size_t size) {
    assert(source != nullptr && size == 32);
    ++pcmLoads;
}
static void CheckOpusLoad(const uint8_t* source, int position, size_t size) {
    assert(source != nullptr && position == 2 && size == 64);
    ++opusLoads;
}
#define AudioSynth_ClearBuffer(cmd, dest, size) (cleared = true)
#define aLoadBufferExactImpl(source, dest, size) CheckPcmLoad(source, size)
#define aOPUSdecImpl(source, dest, count, state, pos, size) CheckOpusLoad(source, pos, size)
static void PlayStreamedSample(Sample* sample, int position) {
    struct {
        int samplePosInt;
    } state{ position };
    auto* synthState = &state;
    auto* sampleAddr = sample->sampleAddr;
    constexpr int SAMPLE_SIZE = 2;
    constexpr int A_CONTINUE = 0;
    int numSamplesToLoadAdj = 16;
    int numSamplesProcessed = 0;
    int dmemUncompressedAddrOffset1 = 0;
    int flags = -1;
    int skipBytes = 1;
    switch (sample->codec) {
#include "audio_stream_playback_production.inc"
        default:
            assert(false);
    }
skip:
    assert(cleared && flags == A_CONTINUE && skipBytes == 0);
    assert(numSamplesProcessed == 16 && dmemUncompressedAddrOffset1 == 16);
}
#undef AudioSynth_ClearBuffer
#undef aLoadBufferExactImpl
#undef aOPUSdecImpl

// Inject documented libopusfile failures at the driver boundary, including an
// error after a successful partial read. Execute the production mixer function.
struct OggOpusFile {};
static OggOpusFile opusState;
static bool failOpusOpen;
static bool failOpusSeek;
static unsigned opusReadCalls;
static std::vector<int> opusReadResults;
static int16_t opusOutput[16];
static OggOpusFile* op_open_memory(const void*, size_t, int*) {
    return failOpusOpen ? nullptr : &opusState;
}
static int op_pcm_seek(OggOpusFile* state, int32_t) {
    assert(state != nullptr);
    return failOpusSeek ? -1 : 0;
}
static int op_read(OggOpusFile* state, int16_t* output, int count, int*) {
    assert(state != nullptr && opusReadCalls < opusReadResults.size());
    assert(output >= opusOutput && output + count <= opusOutput + 16);
    int result = opusReadResults[opusReadCalls++];
    if (result > 0) {
        assert(result <= count);
        std::fill(output, output + result, 123);
    }
    return result;
}
#define BUF_S16(addr) (opusOutput + (addr) / 2)
#include "audio_opus_production.inc"
#undef BUF_S16

static void AppendPage(std::vector<char>& result, const ogg_page& page) {
    result.insert(result.end(), page.header, page.header + page.header_len);
    result.insert(result.end(), page.body, page.body + page.body_len);
}

static std::vector<char> PacketPage(const std::vector<unsigned char>& bytes) {
    ogg_stream_state stream{};
    assert(ogg_stream_init(&stream, 17) == 0);
    ogg_packet packet{};
    packet.packet = const_cast<unsigned char*>(bytes.data());
    packet.bytes = bytes.size();
    packet.b_o_s = 1;
    packet.e_o_s = 1;
    assert(ogg_stream_packetin(&stream, &packet) == 0);
    ogg_page page{};
    std::vector<char> result;
    while (ogg_stream_flush(&stream, &page)) {
        AppendPage(result, page);
    }
    ogg_stream_clear(&stream);
    return result;
}

static std::vector<char> OpusHeader() {
    return PacketPage({ 'O', 'p', 'u', 's', 'H', 'e', 'a', 'd', 1, 1, 0, 0, 0, 125, 0, 0, 0, 0, 0 });
}

static std::vector<char> VorbisAudio() {
    vorbis_info info{};
    vorbis_info_init(&info);
    assert(vorbis_encode_init_vbr(&info, 1, 32000, 0.4f) == 0);
    vorbis_comment comment{};
    vorbis_comment_init(&comment);
    vorbis_dsp_state dsp{};
    assert(vorbis_analysis_init(&dsp, &info) == 0);
    vorbis_block block{};
    assert(vorbis_block_init(&dsp, &block) == 0);
    ogg_stream_state stream{};
    assert(ogg_stream_init(&stream, 23) == 0);
    ogg_packet identification{}, comments{}, setup{};
    assert(vorbis_analysis_headerout(&dsp, &comment, &identification, &comments, &setup) == 0);
    ogg_stream_packetin(&stream, &identification);
    ogg_stream_packetin(&stream, &comments);
    ogg_stream_packetin(&stream, &setup);
    ogg_page page{};
    std::vector<char> result;
    while (ogg_stream_flush(&stream, &page)) {
        AppendPage(result, page);
    }
    float** buffer = vorbis_analysis_buffer(&dsp, 512);
    for (int i = 0; i < 512; ++i) {
        buffer[0][i] = 0.25f * std::sin(i * 0.1f);
    }
    vorbis_analysis_wrote(&dsp, 512);
    vorbis_analysis_wrote(&dsp, 0);
    while (vorbis_analysis_blockout(&dsp, &block) == 1) {
        vorbis_analysis(&block, nullptr);
        vorbis_bitrate_addblock(&block);
        ogg_packet packet{};
        while (vorbis_bitrate_flushpacket(&dsp, &packet)) {
            ogg_stream_packetin(&stream, &packet);
            while (ogg_stream_pageout(&stream, &page)) {
                AppendPage(result, page);
            }
        }
    }
    ogg_stream_clear(&stream);
    vorbis_block_clear(&block);
    vorbis_dsp_clear(&dsp);
    vorbis_comment_clear(&comment);
    vorbis_info_clear(&info);
    return result;
}

static OggType Classify(std::vector<char> bytes) {
    OggFileData file{ bytes.data(), 0, bytes.size() };
    return GetOggType(&file);
}

static std::shared_ptr<SOH::AudioSample> Decode(std::vector<char> bytes) {
    auto resource = std::make_shared<SOH::AudioSample>();
    auto file = std::make_shared<Ship::File>();
    file->Buffer = std::make_shared<std::vector<char>>(std::move(bytes));
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = "custom/samples/stress-test.ogg";
    OggDecoderWorker(resource, file, init);
    return resource;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    std::string test = argv[1];
    if (test == "invalid-header") {
        assert(Classify(std::vector<char>(4096, 'x')) == OggType::None);
    } else if (test == "empty-header") {
        assert(Classify({}) == OggType::None);
    } else if (test == "truncated-header") {
        auto data = OpusHeader();
        data.resize(20);
        assert(Classify(data) == OggType::None);
    } else if (test == "bad-checksum") {
        auto data = OpusHeader();
        data.back() ^= 1;
        assert(Classify(data) == OggType::None);
    } else if (test == "short-packet") {
        assert(Classify(PacketPage({ 'O' })) == OggType::None);
    } else if (test == "valid-vorbis") {
        assert(Classify(VorbisAudio()) == OggType::Vorbis);
    } else if (test == "valid-opus") {
        assert(Classify(OpusHeader()) == OggType::Opus);
    } else if (test == "worker-invalid" || test == "worker-invalid-vorbis" || test == "worker-read-error") {
        auto data = std::vector<char>(4096, 'x');
        if (test == "worker-invalid-vorbis") {
            data = PacketPage({ 1, 'v', 'o', 'r', 'b', 'i', 's', 0 });
        } else if (test == "worker-read-error") {
            data = VorbisAudio();
            failVorbisRead = true;
        }
        auto resource = Decode(data);
        assert(resource->sample.sampleAddr == nullptr);
        assert(errors == 1 && errorPath == "custom/samples/stress-test.ogg");
        failVorbisRead = false;
        // A rejected sample must not poison subsequent valid resources.
        assert(Decode(OpusHeader())->sample.sampleAddr != nullptr);
    } else if (test == "worker-vorbis") {
        auto resource = Decode(VorbisAudio());
        assert(resource->sample.sampleAddr != nullptr && errors == 0);
        auto* pcm = reinterpret_cast<int16_t*>(resource->sample.sampleAddr);
        assert(std::any_of(pcm, pcm + 512, [](int16_t value) { return value != 0; }));
    } else if (test == "worker-opus") {
        auto bytes = OpusHeader();
        auto resource = Decode(bytes);
        assert(resource->sample.codec == CODEC_OPUS && resource->sample.sampleAddr != nullptr && errors == 0);
        assert(std::memcmp(resource->sample.sampleAddr, bytes.data(), bytes.size()) == 0);
    } else if (test == "callback-bounds") {
        char data[] = { 1, 2, 3, 4 };
        char out[4]{};
        OggFileData file{ data, 0, sizeof(data) };
        assert(VorbisReadCallback(out, 0, 2, &file) == 0 && file.pos == 0);
        assert(VorbisSeekCallback(&file, -1, SEEK_SET) == -1 && file.pos == 0);
        assert(VorbisSeekCallback(&file, -2, SEEK_END) == 0 && file.pos == 2);
        assert(VorbisReadCallback(out, 1, 4, &file) == 2 && out[0] == 3 && out[1] == 4);
        assert(VorbisSeekCallback(&file, 1, SEEK_CUR) == -1 && file.pos == 4);
    } else if (test == "playback-null-s16" || test == "playback-null-opus") {
        Sample sample{};
        sample.codec = test == "playback-null-s16" ? CODEC_S16 : CODEC_OPUS;
        sample.size = 64;
        sample.fileSize = 64;
        PlayStreamedSample(&sample, 0);
        PlayStreamedSample(&sample, 2);
        assert(pcmLoads == 0 && opusLoads == 0);
    } else if (test == "playback-ready") {
        uint8_t data[64]{};
        Sample sample{};
        sample.sampleAddr = data;
        sample.size = sizeof(data);
        sample.fileSize = sizeof(data);
        sample.codec = CODEC_S16;
        PlayStreamedSample(&sample, 2);
        assert(pcmLoads == 1 && opusLoads == 0);
        sample.codec = CODEC_OPUS;
        PlayStreamedSample(&sample, 2);
        assert(pcmLoads == 1 && opusLoads == 1);
    } else if (test.compare(0, 5, "opus-") == 0) {
        failOpusOpen = test == "opus-open-failure";
        failOpusSeek = test == "opus-seek-failure";
        opusReadResults = test == "opus-read-error" ? std::vector<int>{ 4, -3, 0 }
                          : test == "opus-eof"      ? std::vector<int>{ 4, 0 }
                                                    : std::vector<int>{ 4, 4 };
        uint8_t bytes[64]{};
        OggOpusFile* state = nullptr;
        aOPUSdecImpl(bytes, 0, 16, &state, 0, sizeof(bytes));
        assert(opusReadCalls == (failOpusOpen || failOpusSeek ? 0u : 2u));
        if (!failOpusOpen && !failOpusSeek) {
            assert(opusOutput[0] == 123 && opusOutput[3] == 123);
            assert(opusOutput[4] == (test == "opus-ready" ? 123 : 0));
        }
    } else {
        return 2;
    }
    std::printf("PASS MM Ogg decoder: %s\n", test.c_str());
}
