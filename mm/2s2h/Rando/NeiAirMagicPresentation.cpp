#include "NeiAirMagicPresentation.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiAirMagicPolicy.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
extern "C" {
#include "functions.h"
#include "mods/nei_oot_compat.h"
}

namespace {
alignas(2) const char kWindTexture[] = "__OTR__objects/nei_air_magic/silver_wisp";
alignas(2) const char kLightningTexture[] = "__OTR__objects/nei_air_magic/lightning_filament";
const NeiGi::TextureMaterial kWindMaterial{ kWindTexture, true, false };
const NeiGi::TextureMaterial kLightningMaterial{ kLightningTexture, true, false };
bool Valid(PlayState* play, const Vec3f* p) {
    return play && play->state.gfxCtx && p && NeiAirMagic::Finite({ p->x, p->y, p->z });
}
NeiGi::Point Point(const Vec3f& p) {
    return { p.x, p.y, p.z };
}
struct Scope {
    Scope(const Vec3f* origin, int identity) {
        FrameInterpolation_RecordOpenChild(origin, identity);
        Matrix_Push();
        Matrix_Translate(origin->x, origin->y, origin->z, MTXMODE_NEW);
    }
    ~Scope() {
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
};
void Draw(PlayState* play, const NeiAirMagic::Layers& layers, const NeiGi::TextureMaterial& material) {
    if (!NeiGi_DrawTexturedMesh(play, layers.surface, material)) {
        auto fallback = layers.surface;
        for (size_t i = 0; i < fallback.count; ++i)
            fallback.vertices[i].alpha = NeiAirMagic::Alpha(fallback.vertices[i].alpha * .2f);
        NeiGi_DrawMesh(play, fallback);
    }
    NeiGi_DrawMesh(play, layers.detail);
}
} // namespace
extern "C" void NeiAirMagic_DrawLightning(PlayState* play, const Vec3f* origin, const Vec3f* direction) {
    if (!Valid(play, origin) || !direction || !NeiAirMagic::Finite(Point(*direction)))
        return;
    const Scope scope(origin, 0x41495201);
    Draw(play, NeiAirMagic::SampleLightning(play->gameplayFrames, Point(*direction), NeiGi_CameraBasis(play)),
         kLightningMaterial);
}
extern "C" void NeiAirMagic_DrawGust(PlayState* play, const Vec3f* origin, const Vec3f* direction, float length,
                                     float radius, bool blow, unsigned color) {
    if (!Valid(play, origin) || !direction || !NeiAirMagic::Finite(Point(*direction)) || !NeiAirMagic::Finite(length) ||
        !NeiAirMagic::Finite(radius) || length <= 0 || radius <= 0)
        return;
    const Scope scope(origin, 0x41495202);
    Draw(play,
         NeiAirMagic::SampleGust(play->gameplayFrames, Point(*direction), length, radius, blow, color,
                                 NeiGi_CameraBasis(play)),
         kWindMaterial);
}
extern "C" void NeiAirMagic_DrawEnvelope(PlayState* play, Player* player) {
    if (!player || !Valid(play, &player->actor.world.pos) || !NeiAirMagic::Finite(Point(player->actor.velocity)))
        return;
    const float height = Player_GetHeight(player);
    if (!NeiAirMagic::Finite(height) || height <= 0)
        return;
    const Scope scope(&player->actor.world.pos, 0x41495203);
    Draw(play,
         NeiAirMagic::SampleEnvelope(play->gameplayFrames, height, Point(player->actor.velocity),
                                     NeiGi_CameraBasis(play)),
         kWindMaterial);
}
