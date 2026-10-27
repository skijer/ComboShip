#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void MMWeatherAudio_SetRain(float gain);
void MMWeatherAudio_Thunder(float gain);
void MMWeatherAudio_Reset(void);
void MMWeatherAudio_Mix(int16_t* interleavedStereo, size_t frameCount);
void MMWeatherAudio_Shutdown(void);
#ifdef __cplusplus
}
#endif
