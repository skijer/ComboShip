#pragma once

// Resource-only bridge. No native engine structs cross this boundary.
#ifdef __cplusplus
extern "C" {
#endif
int NeiResource_Available(const char* path);
int NeiResource_IsMod(const char* path);
// Register OoT's existing shipped-only view before deferred base graph draws.
int NeiResource_EnsureGiBaseOwner(void);
// Process-lifetime path for deferred G_DL_OTR_FILEPATH commands only.
// Textures use an explicit OoT resource-manager bracket and the original path.
const char* NeiResource_Route(const char* path);
#ifdef __cplusplus
}
#endif
