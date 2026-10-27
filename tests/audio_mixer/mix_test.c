/* Exercise the complete production mixer, including its platform dispatcher. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include MIXER_SOURCE

#if !defined(__SSE2__) && !defined(_M_AMD64)
#error This regression must exercise the x86 SSE2 mixer.
#endif

#ifdef TEST_MM_HILOGAIN
#include <assert.h>
static unsigned hiLoGainModeReports;
void MM_LogAudioGainMode(int enabled, int gain, int requestedBytes,
                         int processedBytes) {
  assert(enabled == 1);
  assert(gain >= 0 && gain <= 255);
  assert(processedBytes == requestedBytes);
  ++hiLoGainModeReports;
}

#endif

static unsigned failures;
static unsigned cases;
static uint32_t randomState = 0x5EED1234;

static int16_t nextSample(void) {
  randomState ^= randomState << 13;
  randomState ^= randomState >> 17;
  randomState ^= randomState << 5;
  return (int16_t)randomState;
}

/* Independent wide arithmetic oracle; -32768 is the native subtract command. */
static int16_t expectedSample(int16_t input, int16_t output, int16_t gain) {
  int64_t value =
      gain == INT16_MIN
          ? (int64_t)output - input
          : ((int64_t)output * 32767 + (int64_t)input * gain + 16384) >> 15;
  return value < INT16_MIN   ? INT16_MIN
         : value > INT16_MAX ? INT16_MAX
                             : (int16_t)value;
}

static void compare(const char *label, const int16_t *actual,
                    const int16_t *expected, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (actual[i] != expected[i]) {
      if (failures < 6) {
        fprintf(stderr, "%s sample %zu: got %d, expected %d\n", label, i,
                actual[i], expected[i]);
      }
      ++failures;
    }
  }
}

static void checkMix(uint16_t count, int16_t gain, int inPlace, int unaligned,
                     int pattern) {
  int16_t input[448], output[448], expected[448];
  size_t samples = ((count * 16 + 31) & ~31) / 2;
  uint16_t inAddr = 0x500 + (unaligned ? 2 : 0);
  uint16_t outAddr = inPlace ? inAddr : 0xA00 + (unaligned ? 2 : 0);
  const int16_t edges[] = {INT16_MIN, INT16_MAX, -1,     0,    1,
                           -30000,    30000,     -16384, 16384};
  for (size_t i = 0; i < samples; ++i) {
    if (pattern == 0) {
      input[i] = edges[i % 9];
      output[i] = edges[(i * 5 + 3) % 9];
    } else if (pattern == 1) {
      input[i] = (int16_t)(12000 * sin(i * 0.09));
      output[i] = (int16_t)(3000 * cos(i * 0.17));
    } else {
      input[i] = nextSample();
      output[i] = nextSample();
    }
    if (inPlace) {
      output[i] = input[i];
    }
    expected[i] = expectedSample(input[i], output[i], gain);
  }
  memset(&rspa, 0x5A, sizeof(rspa));
  memcpy(BUF_S16(inAddr), input, samples * sizeof(int16_t));
  memcpy(BUF_S16(outAddr), output, samples * sizeof(int16_t));
  aMixImpl(count, gain, inAddr, outAddr);
  compare("platform mix", BUF_S16(outAddr), expected, samples);
  if (BUF_S16(outAddr)[-1] != 0x5A5A || BUF_S16(outAddr)[samples] != 0x5A5A) {
    fprintf(stderr, "mixer wrote past the rounded command range\n");
    ++failures;
  }
  if (!inPlace) {
    compare("source preservation", BUF_S16(inAddr), input, samples);
  }
  memcpy(BUF_S16(inAddr), input, samples * sizeof(int16_t));
  memcpy(BUF_S16(outAddr), output, samples * sizeof(int16_t));
  aMixImplRef(count, gain, inAddr, outAddr);
  compare("scalar reference", BUF_S16(outAddr), expected, samples);
  ++cases;
}

/* MM's native reverb copies wet to dry, decays wet in place, then leaks L/R.
 * Use its real DMEM sizes and addresses over repeated feedback blocks. */
static void checkReverb(void) {
  enum {
    CHANNEL = 208,
    STEREO = 416,
    WET = 0xC70,
    DRY = 0x930,
    SCRATCH = 0x710
  };
  int16_t feedback[STEREO] = {0}, dry[STEREO], savedLeft[CHANNEL];
  const int16_t decayGains[] = {-32768, -26624, -20480, -8192, -6144};
  for (size_t preset = 0; preset < sizeof(decayGains) / sizeof(decayGains[0]);
       ++preset) {
    memset(&rspa, 0, sizeof(rspa));
    memset(feedback, 0, sizeof(feedback));
    for (int block = 0; block < 80; ++block) {
      for (int i = 0; i < STEREO; ++i) {
        int16_t excitation =
            (int16_t)(4000 * sin((block * CHANNEL + i) * 0.07));
        feedback[i] = clamp16(feedback[i] + excitation);
        dry[i] = expectedSample(feedback[i], 0, 0x7FFF);
      }
      memcpy(BUF_S16(WET), feedback, sizeof(feedback));
      aClearBufferImpl(DRY, sizeof(dry));
      aMixImpl(sizeof(dry) >> 4, 0x7FFF, WET, DRY);
      compare("reverb wet to dry", BUF_S16(DRY), dry, STEREO);
      aMixImpl(sizeof(feedback) >> 4, decayGains[preset], WET, WET);
      for (int i = 0; i < STEREO; ++i) {
        feedback[i] =
            expectedSample(feedback[i], feedback[i], decayGains[preset]);
      }
      memcpy(savedLeft, feedback, sizeof(savedLeft));
      aDMEMMoveImpl(WET, SCRATCH, sizeof(savedLeft));
      aMixImpl(sizeof(savedLeft) >> 4, 0x1400, WET + sizeof(savedLeft), WET);
      aMixImpl(sizeof(savedLeft) >> 4, 0x1400, SCRATCH,
               WET + sizeof(savedLeft));
      for (int i = 0; i < CHANNEL; ++i) {
        feedback[i] =
            expectedSample(feedback[i + CHANNEL], savedLeft[i], 0x1400);
        feedback[i + CHANNEL] =
            expectedSample(savedLeft[i], feedback[i + CHANNEL], 0x1400);
      }
      compare("reverb decay and stereo leak", BUF_S16(WET), feedback, STEREO);
      ++cases;
    }
  }
}

#ifdef TEST_MM_HILOGAIN
/* Preserve every byte outside the command's rounded byte span, including the
 * adjacent synthesis workspace. Compare UQ4.4 gain using wide independent
 * arithmetic. */
static void checkHiLoGain(void) {
  const uint16_t lengths[] = {1, 16, 31, 32, 33, 64, 352, 384, 416};
  int16_t original[DMEM_BUF_SIZE / sizeof(int16_t)];
  int16_t expected[DMEM_BUF_SIZE / sizeof(int16_t)];
  {
    for (unsigned gain = 0; gain <= 255; ++gain) {
      for (size_t n = 0; n < sizeof(lengths) / sizeof(lengths[0]); ++n) {
        const size_t span =
            ((lengths[n] + 31) & ~31) / sizeof(int16_t);
        const size_t offset = (0x3B0 - 0x330) / sizeof(int16_t);
        for (size_t i = 0; i < sizeof(original) / sizeof(original[0]); ++i)
          original[i] = nextSample();
        memcpy(expected, original, sizeof(expected));
        for (size_t i = offset; i < offset + span; ++i) {
          int64_t v = ((int64_t)original[i] * gain) >> 4;
          expected[i] = v < INT16_MIN   ? INT16_MIN
                        : v > INT16_MAX ? INT16_MAX
                                        : (int16_t)v;
        }
        memcpy(rspa.buf.as_s16, original, sizeof(original));
        aHiLoGainImpl((uint8_t)gain, lengths[n], 0x3B0);
        compare("MM HiLoGain span and adjacent workspace", rspa.buf.as_s16,
                expected, sizeof(expected) / sizeof(expected[0]));
        ++cases;
      }
    }
  }
  assert(hiLoGainModeReports == 1);
}
#endif

int main(void) {
  /* Gain zero must preserve every lane of a smooth existing signal. */
  checkMix(26, 0, 0, 0, 1);
  for (int gain = INT16_MIN; gain <= INT16_MAX; ++gain) {
    checkMix(2, (int16_t)gain, 0, 0, 0);
    checkMix(2, (int16_t)gain, 1, 0, 0);
  }
  const uint16_t counts[] = {0, 1, 2, 3, 13, 26, 51, 52};
  const int16_t gains[] = {INT16_MIN, -30000, -26624, -20480, -8192,    -1,
                           0,         1,      0x1400, 0x3000, INT16_MAX};
  for (size_t c = 0; c < sizeof(counts) / sizeof(counts[0]); ++c) {
    for (size_t g = 0; g < sizeof(gains) / sizeof(gains[0]); ++g) {
      for (int mode = 0; mode < 4; ++mode) {
        for (int pattern = 0; pattern < 3; ++pattern) {
          checkMix(counts[c], gains[g], mode & 1, mode >> 1, pattern);
        }
      }
    }
  }
  checkReverb();
#ifdef TEST_MM_HILOGAIN
  checkHiLoGain();
#endif
  printf("%s native SSE2 mixer: %u cases, %u mismatched samples\n", GAME_NAME,
         cases, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
