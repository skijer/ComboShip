#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifdef COMBO_BUILD
void ComboEmotes_BeginUpdate(void* play);
void ComboEmotes_ApplyPose(void* play);
int ComboEmotes_CopyPose(void* destination, size_t size);
int ComboEmotes_CopyRawBuffer(void* destination, size_t size, size_t jointOffset);
int ComboEmotes_IsActive(void);
void ComboEmotes_Reset(void);
void ComboEmotes_MixAudio(int16_t* stereo, size_t frames);
#else
static inline void ComboEmotes_BeginUpdate(void* play) {
    (void)play;
}
static inline void ComboEmotes_ApplyPose(void* play) {
    (void)play;
}
static inline int ComboEmotes_CopyPose(void* destination, size_t size) {
    (void)destination;
    (void)size;
    return 0;
}
static inline int ComboEmotes_CopyRawBuffer(void* d, size_t s, size_t o) {
    (void)d;
    (void)s;
    (void)o;
    return 0;
}
static inline int ComboEmotes_IsActive(void) {
    return 0;
}
static inline void ComboEmotes_Reset(void) {
}
static inline void ComboEmotes_MixAudio(int16_t* stereo, size_t frames) {
    (void)stereo;
    (void)frames;
}
#endif
#ifdef __cplusplus
}
#endif
