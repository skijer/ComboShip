// Real sample decoding, resampling and envelope mixing. No native game assets
// are needed: a signed-eight-bit sine fixture supplies the input waveform.
#include "mods/sound_translator/mm_sfx_synth_backend.cpp"
#include <cassert>
#include <cmath>
#include <string>

namespace mmsfx {
AudioContext gMmSfx;
SfxChannelState sSfxChannelState[SEQ_NUM_CHANNELS];
void AudioScript_ProcessSequences(s32) {}
} // namespace mmsfx

extern "C" int CVarGetInteger(const char *, int) { return 100; }
extern "C" void MmSfxSynth_Log(const char *) {}

int main(int argc, char **argv) {
  using namespace mmsfx;
  assert(argc == 2);
  const std::string scenario = argv[1];
  const bool highRate = scenario != "normal";
  const bool firstPartEnd = scenario == "high-rate-end-first";
  const bool ending = scenario == "high-rate-end" || firstPartEnd;
  const bool looping = scenario == "high-rate-loop";
  const bool odd = scenario == "high-rate-odd";
  const bool adpcm = scenario == "high-rate-adpcm";
  const bool smallAdpcm = scenario == "high-rate-small-adpcm";
  alignas(16) static s8 pcm[32768 + 32];
  for (int i = 0; i < 32768; ++i)
    pcm[i] = static_cast<s8>(90 * std::sin(i * (6.283185307179586 / 32)));
  AdpcmLoop loop{};
  loop.loopEnd = firstPartEnd ? 640 : ending || looping ? 1024 : 32768;
  loop.sampleEnd = loop.loopEnd;
  loop.count = looping ? 1 : 0;
  Sample sample{};
  sample.codec = MM_CODEC_S8;
  sample.size = sizeof(pcm);
  sample.sampleAddr = reinterpret_cast<u8 *>(pcm);
  sample.loop = &loop;
  // A zero predictor codebook makes the encoded sine directly observable,
  // while still exercising the real four-bit/two-bit ADPCM DSP decoders.
  alignas(16) static u8 encoded[(32768 / 16) * 9 + 32];
  static s16 codeBook[16]{};
  AdpcmBook book{2, 1, codeBook};
  if (adpcm || smallAdpcm) {
    const int frameBytes = smallAdpcm ? 5 : 9;
    const int bits = smallAdpcm ? 2 : 4;
    const int perByte = 8 / bits;
    for (int frame = 0; frame < 32768 / 16; ++frame) {
      encoded[frame * frameBytes] = smallAdpcm ? 0xD0 : 0xC0;
      for (int index = 0; index < 16; ++index) {
        const f32 wave =
            std::sin((frame * 16 + index) * (6.283185307179586 / 32));
        const s32 value = smallAdpcm ? wave > 0.05f    ? 1
                                       : wave < -0.05f ? -1
                                                       : 0
                                     : static_cast<s32>(5 * wave);
        encoded[frame * frameBytes + 1 + index / perByte] |=
            (value & ((1 << bits) - 1)) << (8 - bits * (index % perByte + 1));
      }
    }
    sample.codec = smallAdpcm ? MM_CODEC_SMALL_ADPCM : MM_CODEC_ADPCM;
    sample.sampleAddr = encoded;
    sample.size = (32768 / 16) * frameBytes;
    sample.book = &book;
  }
  TunedSample tuned{&sample, 1.0f};
  Note note{};
  NoteSynthesisBuffers buffers{};
  note.synthesisState.synthesisBuffers = &buffers;
  NoteSampleState state{};
  state.bitField0.enabled = true;
  state.bitField0.needsInit = true;
  state.bitField1.hasTwoParts = highRate;
  // MM halves the fixed-point ratio when hasTwoParts is set: 0xC000
  // represents 1.5 in each part, hence 3 source samples per output frame.
  state.frequencyFixedPoint = odd ? 0xC100 : 0xC000;
  state.tunedSample = &tuned;
  state.targetVolLeft = state.targetVolRight = 1000;
  gMmSfx.notes = &note;
  gMmSfx.sampleStateList = &state;
  gMmSfx.numNotes = 1;
  int signChanges = 0;
  s16 previous = 0;
  for (int update = 0; update < (ending ? 2 : 20); ++update) {
    s32 output[176 * 2]{};
    SynthUpdate(output, 176);
    if (update == 0) {
      const int wanted = odd ? 530 : highRate ? 528 : 264;
      if (note.synthesisState.samplePosInt != wanted)
        std::fprintf(stderr, "%s: consumed %d source samples, expected %d\n",
                     argv[1], note.synthesisState.samplePosInt, wanted);
      assert(note.synthesisState.samplePosInt == wanted);
    }
    if (ending && update == 1) {
      assert(!state.bitField0.enabled &&
             "the high-rate one-shot must finish at its source end");
      if (firstPartEnd)
        for (int frame = 112; frame < 176; ++frame)
          assert(
              output[frame * 2] == 0 && output[frame * 2 + 1] == 0 &&
              "first-part completion must pad the remaining PCM with silence");
      break;
    }
    if (update > 0)
      for (int frame = 0; frame < 176; ++frame) {
        const s16 value = output[frame * 2];
        if ((value < 0 && previous >= 0) || (value >= 0 && previous < 0))
          ++signChanges;
        previous = value;
      }
  }
  if (!ending) {
    // 3344 output frames, 32-source-sample periods. Count the audible
    // waveform, independently of the cursor asserted above.
    assert(signChanges >= (odd ? 628 : highRate ? 625 : 310));
    assert(signChanges <= (odd ? 633 : highRate ? 629 : 315));
    assert(state.bitField0.enabled);
  }
  std::printf("PASS MM SFX render %s\n", argv[1]);
}
