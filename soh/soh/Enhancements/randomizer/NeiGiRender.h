#pragma once

#include "NeiGiEffectPolicy.h"
#include "NeiGiFrameFit.h"
#include "NeiGiSongEffectPolicy.h"
#include "NeiGiBottleShimmerPolicy.h"
#include "NeiElementalSpellGi.h"
#include "ComboElementalArrowGi.h"
#include "ComboRewardGi.h"
#include "z64.h"
#include "../../../../combo/BlackMarketFrame.h"
namespace NeiGi {
// Private RGBA32 resources use a 32x32 logical tile; resource metadata owns
// the high-resolution scale. Path and material must have static lifetime.
struct TextureMaterial {
    const char* path;
    bool repeatS = false;
    bool repeatT = false;
    bool scrolling = false;
};
} // namespace NeiGi
extern "C" {

// Passive outside an explicit native market capture. Raw Gfx callers report
// their original selected resource paths or actual procedural vertices.
void NeiGi_ReportBlackMarketBounds(const NeiGi::Point& minimum, const NeiGi::Point& maximum);
void NeiGi_ReportBlackMarketModel(const char* owner, const char* opaque, const char* translucent = nullptr);
void NeiGi_ReportBlackMarketVertices(const Vtx* vertices, size_t count);
int ResourceMgr_GetGiModelBoundsForGame(const char* game, const char* const* paths, int count, float minimum[3],
                                        float maximum[3]);

// Shared presentation primitives. The caller owns pose, origin and scale;
// these functions do not add GI rotation or optional pickup shimmer.
NeiGi::Basis NeiGi_CameraBasis(PlayState* play);
void NeiGi_DrawMesh(PlayState* play, const NeiGi::Mesh& mesh, NeiGi::Kind orb = NeiGi::Kind::Neutral);
// World emitters share the GI shimmer's grayscale/material restore and arena
// reservation. The caller supplies its world pose, without GI rotation/scale.
void NeiGi_DrawWorldParticles(PlayState* play, const NeiGi::Mesh& mesh);
bool NeiGi_CanDrawLayers(PlayState* play, size_t matrices, size_t opa, size_t xlu);
// Reserve the caller's later casing/restore commands while the child allocates.
bool NeiGi_DrawMeshWithTail(PlayState* play, const NeiGi::Mesh& mesh, const char* owner, size_t matrices, size_t opa,
                            size_t xlu);
// False queues nothing, so the caller can keep its geometry-only fallback.
bool NeiGi_DrawTexturedMesh(PlayState* play, const NeiGi::Mesh& mesh, const NeiGi::TextureMaterial& material);
// Intrinsic weather; profiles 1..4 are individual seasons, 5 cycles on the actual rod.
void NeiGi_DrawSeasonOverlay(PlayState* play, int profile, const char* owner);
void NeiGi_DrawSongOverlay(PlayState* play, int song, const char* owner);
// Intrinsic Sage fountain; retains the native medallion materials and launch animation.
void NeiGi_DrawSagesTunicMedallions(PlayState* play, const char* owner = nullptr);
// Selected external geometry uses its resource bounds for MM receipts and
// swords. The item's identity still controls its optional particles.
void NeiGi_DrawExternalPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int shimmerKind,
                                    bool shimmer, const char* owner, bool shop = false, float tilt = 0.f,
                                    bool fit = true, int mmPickup = 0);
// Owner-aware mesh/effect composition used by both foreign GI directions.
// A separate shimmer palette never changes the authored bounds or energy identity.
void NeiGi_DrawPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int effect,
                            const float center[3], bool shimmer, const char* owner, bool shop = false, int mmPickup = 0,
                            int shimmerKind = -1);
}
