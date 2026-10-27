#pragma once
#include <stdint.h>
#include <stddef.h>

enum MMAudioTraceStage {
    MM_TRACE_DECODED,
    MM_TRACE_RESAMPLED,
    MM_TRACE_GAIN,
    MM_TRACE_FILTER,
    MM_TRACE_PROCESSED,
    MM_TRACE_STAGE_COUNT
};
typedef struct MMAudioTraceNote {
    int note, player, sequence, font, codec, gain, pitch, filter, comb, samples;
    uintptr_t sample, sequenceData;
    uint32_t bytes;
    float tuning;
} MMAudioTraceNote;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef COMBO_BUILD
int MM_AudioTraceEnabled(void);
unsigned MM_AudioTraceNoteSamples(unsigned requested);
void MM_AudioTraceSetNote(const MMAudioTraceNote* note);
void MM_AudioTracePCM(int stage, const int16_t* samples, size_t count, unsigned wouldClip);
void MM_AudioTraceEvent(const char* event, int requested, int actual);
void MM_AudioTraceResource(int sequence, uintptr_t identity, const char* resource, const char* source, int streamed);
void MM_AudioTraceAlias(int sequence, uintptr_t destination, uintptr_t source);
void MM_AudioTraceFormat(uintptr_t sample, unsigned pcmRate, unsigned channels, uint64_t frames);
#else
static inline int MM_AudioTraceEnabled(void) {
    return 0;
}
static inline unsigned MM_AudioTraceNoteSamples(unsigned n) {
    return n;
}
static inline void MM_AudioTraceSetNote(const MMAudioTraceNote* n) {
    (void)n;
}
static inline void MM_AudioTracePCM(int s, const int16_t* p, size_t n, unsigned c) {
    (void)s;
    (void)p;
    (void)n;
    (void)c;
}
static inline void MM_AudioTraceEvent(const char* e, int r, int a) {
    (void)e;
    (void)r;
    (void)a;
}
static inline void MM_AudioTraceResource(int s, uintptr_t i, const char* r, const char* p, int t) {
    (void)s;
    (void)i;
    (void)r;
    (void)p;
    (void)t;
}
static inline void MM_AudioTraceAlias(int s, uintptr_t d, uintptr_t p) {
    (void)s;
    (void)d;
    (void)p;
}
static inline void MM_AudioTraceFormat(uintptr_t s, unsigned r, unsigned c, uint64_t n) {
    (void)s;
    (void)r;
    (void)c;
    (void)n;
}
#endif
#ifdef __cplusplus
}
#endif
