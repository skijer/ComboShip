#pragma once

#include <stdbool.h>

#ifdef __cplusplus
#include <cstdint>
#include <string>
#include <vector>

namespace MMMidnaAudioResources {
bool Enabled();
bool HasModel();
bool ReadClip(const char* path, std::vector<uint8_t>& bytes);
std::vector<std::string> ListClips();
std::string ReadAssignment(const char* event, const char* fallback);
void WriteAssignment(const char* event, const std::string& path);
float Gain();
} // namespace MMMidnaAudioResources

extern "C" {
#endif

// Private assets always belong to MM's manager, including dormant cross-game draws.
bool MMMidnaResources_Exists(const char* path);
void* MMMidnaResources_Load(const char* path);

#ifdef __cplusplus
}
#endif
