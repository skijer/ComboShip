#include "audio/soundfont.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mm_reset(void);
void soh_reset(void);
unsigned char* mm_buffer(void);
unsigned char* soh_buffer(void);
void mm_load(Sample* sample, int position, int count);
void soh_load(Sample* sample, int position, int count);
void mm_load_native(const void* source, int bytes);
unsigned mm_opus_calls(void);
void mm_resample(int count, uint16_t pitch, int16_t* state, int init, int16_t* output);
void soh_resample(int count, uint16_t pitch, int16_t* state, int init, int16_t* output);

static unsigned failures;

static void CheckLoads(void) {
    // Cover every even tail length, including requests smaller than a DMA unit.
    const int lengths[] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 30, 32, 440, 482, 486, 576 };
    for (unsigned i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
        int bytes = lengths[i];
        for (int final = 0; final <= 1; ++final) {
            const int offset = 6;
            int count = bytes / 2 + (final ? 16 : 0);
            unsigned char* source = malloc(offset + bytes);
            assert(source != NULL);
            for (int j = 0; j < offset + bytes; ++j) {
                source[j] = 1 + j % 251;
            }
            Sample sample = { 0 };
            sample.codec = CODEC_S16;
            sample.sampleAddr = source;
            sample.size = offset + bytes;
            mm_reset();
            soh_reset();
            memset(mm_buffer(), 0xA5, 768);
            mm_load(&sample, offset / 2, count);
            soh_load(&sample, offset / 2, count);
            if (memcmp(mm_buffer(), source + offset, bytes) != 0) {
                fprintf(stderr, "FAIL PCM load: %d bytes, final=%d\n", bytes, final);
                ++failures;
            }
            int clearedBytes = ((count + 16) * 2 + 15) & ~15;
            for (int j = bytes; j < clearedBytes; ++j) {
                assert(mm_buffer()[j] == 0);
            }
            assert(mm_buffer()[clearedBytes] == 0xA5);
            // Native MM DMA still truncates to 16 bytes; do not change it globally.
            mm_reset();
            memset(mm_buffer(), 0xA5, 768);
            mm_load_native(source + offset, bytes);
            assert(memcmp(mm_buffer(), source + offset, bytes & ~15) == 0);
            for (int j = bytes & ~15; j <= bytes; ++j) {
                assert(mm_buffer()[j] == 0xA5);
            }
            free(source);
        }
    }
    Sample pending = { 0 };
    pending.size = 1024;
    for (unsigned codec = CODEC_S16; codec <= CODEC_OPUS; codec += CODEC_OPUS - CODEC_S16) {
        pending.codec = codec;
        mm_reset();
        memset(mm_buffer(), 0xA5, 512);
        mm_load(&pending, 0, 177);
        for (int j = 0; j < 400; ++j) {
            assert(mm_buffer()[j] == 0);
        }
    }
    assert(mm_opus_calls() == 0);
    unsigned char opusData[8] = { 0 };
    pending.codec = CODEC_OPUS;
    pending.sampleAddr = opusData;
    mm_load(&pending, 0, 177);
    assert(mm_opus_calls() == 1);
    puts("Checked exact PCM loads, final tails, native alignment and pending/Opus routing");
}

static void CheckResampling(const int16_t* source, int sourceCount, int rate, int variedChunks) {
    const int chunks[] = { 160, 176, 184, 192, 208 };
    uint16_t pitch = (uint16_t)((double)rate / 32000 * 32768);
    int16_t mmState[16] = { 0 };
    int16_t sohState[16] = { 0 };
    int16_t mmOutput[208];
    int16_t sohOutput[208];
    uint32_t fraction = 0;
    int position = 0;
    unsigned mismatched = 0;
    unsigned total = 0;
    unsigned nonAligned = 0;
    double squaredError = 0;
    Sample sample = { 0 };
    sample.codec = CODEC_S16;
    sample.sampleAddr = (unsigned char*)source;
    sample.size = sourceCount * sizeof(int16_t);
    mm_reset();
    soh_reset();
    for (int block = 0;; ++block) {
        int count = chunks[variedChunks ? block % 5 : 0];
        // Same 16.16 sample-count accumulator used by AudioSynth_ProcessNote.
        uint32_t fixed = pitch * count * 2 + fraction;
        int inputCount = fixed >> 16;
        fraction = fixed & 0xFFFF;
        if (position + inputCount > sourceCount) {
            break;
        }
        nonAligned += (inputCount % 8 != 0);
        mm_load(&sample, position, inputCount);
        soh_load(&sample, position, inputCount);
        mm_resample(count, pitch, mmState, block == 0, mmOutput);
        soh_resample(count, pitch, sohState, block == 0, sohOutput);
        for (int j = 0; j < count; ++j) {
            int difference = mmOutput[j] - sohOutput[j];
            mismatched += difference != 0;
            squaredError += (double)difference * difference;
        }
        total += count;
        position += inputCount;
    }
    assert(total > 32000);
    if (rate == 44100 || (rate == 48000 && variedChunks)) {
        assert(nonAligned > 0);
    }
    printf("%s %d Hz %s chunks: %u/%u different samples, error RMS %.3f, %u non-aligned chunks\n",
           mismatched ? "FAIL" : "PASS", rate, variedChunks ? "varied" : "160-sample", mismatched, total,
           sqrt(squaredError / total), nonAligned);
    failures += mismatched != 0;
}

int main(int argc, char** argv) {
    CheckLoads();
    if (argc == 2) {
        // Optional local analysis: signed 16-bit mono PCM at 44.1 kHz.
        FILE* input = fopen(argv[1], "rb");
        assert(input != NULL);
        assert(fseek(input, 0, SEEK_END) == 0);
        long bytes = ftell(input);
        assert(bytes > 88200 && bytes % 2 == 0);
        rewind(input);
        int16_t* source = malloc(bytes);
        assert(source != NULL && fread(source, 1, bytes, input) == (size_t)bytes);
        fclose(input);
        CheckResampling(source, bytes / 2, 44100, 1);
        free(source);
    } else {
        const int rates[] = { 32000, 44100, 48000 };
        for (unsigned r = 0; r < sizeof(rates) / sizeof(rates[0]); ++r) {
            int rate = rates[r];
            int16_t* source = malloc(rate * 2 * sizeof(int16_t));
            assert(source != NULL);
            for (int j = 0; j < rate * 2; ++j) {
                double t = (double)j / rate;
                source[j] = 9000 * sin(2 * 3.141592653589793 * 997 * t) + 7000 * sin(2 * 3.141592653589793 * 11003 * t);
            }
            CheckResampling(source, rate * 2, rate, 0);
            CheckResampling(source, rate * 2, rate, 1);
            free(source);
        }
    }
    return failures ? 1 : 0;
}
