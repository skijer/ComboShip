#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MMMidnaAudioEvent {
    MM_MIDNA_AUDIO_DASH,
    MM_MIDNA_AUDIO_APPEAR,
    MM_MIDNA_AUDIO_VANISH,
    MM_MIDNA_AUDIO_TARGET_NPC,
    MM_MIDNA_AUDIO_TARGET_ENEMY,
    MM_MIDNA_AUDIO_TARGET_OTHER,
    MM_MIDNA_AUDIO_CALL,
    MM_MIDNA_AUDIO_HINT,
    MM_MIDNA_AUDIO_TALK,
    MM_MIDNA_AUDIO_YAWN,
    MM_MIDNA_AUDIO_EVENT_COUNT
} MMMidnaAudioEvent;

// Load optional private clips on startup. No original sound-bank entries change.
void MMMidnaAudio_Init(void);
// False means the caller must play its original sound, with its original arguments.
// True includes an available movement cue intentionally suppressed by its cooldown.
bool MMMidnaAudio_TryPlay(MMMidnaAudioEvent event);
// One call per 20 Hz Tatl update. Only eligible stationary/visible time counts.
// Ineligible updates cancel idle playback; other cues always take priority.
void MMMidnaAudio_UpdateIdle(bool eligible);
// Adds dry, centered mono clips to the engine's 32 kHz stereo output.
void MMMidnaAudio_Mix(int16_t* interleavedStereo, size_t frameCount);
// Stop Midna's voices at scene teardown; keep clips and remaining cue intervals.
void MMMidnaAudio_Reset(void);

#ifdef __cplusplus
}

#include <string>
#include <vector>

// Menu operations use the decoded archive bank; no I/O is performed during playback.
std::vector<std::string> MMMidnaAudio_GetAvailableClips();
const char* MMMidnaAudio_GetEventLabel(MMMidnaAudioEvent event);
std::string MMMidnaAudio_GetAssignment(MMMidnaAudioEvent event);
// An empty path restores the native cue (silence for the optional idle yawn).
bool MMMidnaAudio_Assign(MMMidnaAudioEvent event, const std::string& path);
void MMMidnaAudio_ResetAssignments();
#endif
