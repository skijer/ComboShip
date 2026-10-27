// TU glue: include after the host engine/GBI declarations, outside extern "C".
// Adds native water-tentacle geometry around the existing Morpha GI core.
#ifndef COMBO_MORPHA_GI_H
#define COMBO_MORPHA_GI_H

#include <cmath>
#include <cstdint>
#include <libultraship/libultra/gu.h>

#ifdef COMBO_MORPHA_GI_HOST_MM
#define CMG_SETUP_XLU(ctx) Gfx_SetupDL25_Xlu(ctx)
#else
#define CMG_SETUP_XLU(ctx) Gfx_SetupDL_25Xlu(ctx)
#endif

// The availability callback must query the selected resource in the given
// owner's RM, including Alt selection. It must not borrow a different host's
// shadowing resource. Owner strings must outlive deferred GBI execution.
// False submits nothing: callers retain the existing core/flame fallback.
static inline bool ComboDrawMorphaTentacleGi(PlayState* play, const char* owner,
                                             bool (*available)(const char*, const char*)) {
    if (!play || !play->state.gfxCtx || !owner || !available)
        return false;
    GraphicsContext* ctx = play->state.gfxCtx;
    if (!ctx->polyXlu.p || !ctx->polyOpa.p || !ctx->polyOpa.d)
        return false;
    static const char* const paths[41] = {
        "__OTR__objects/object_mo/gMorphaTentacleBaseDL",   "__OTR__objects/object_mo/gMorphaTentaclePart1DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart2DL",  "__OTR__objects/object_mo/gMorphaTentaclePart3DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart4DL",  "__OTR__objects/object_mo/gMorphaTentaclePart5DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart6DL",  "__OTR__objects/object_mo/gMorphaTentaclePart7DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart8DL",  "__OTR__objects/object_mo/gMorphaTentaclePart9DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart10DL", "__OTR__objects/object_mo/gMorphaTentaclePart11DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart12DL", "__OTR__objects/object_mo/gMorphaTentaclePart13DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart14DL", "__OTR__objects/object_mo/gMorphaTentaclePart15DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart16DL", "__OTR__objects/object_mo/gMorphaTentaclePart17DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart18DL", "__OTR__objects/object_mo/gMorphaTentaclePart19DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart20DL", "__OTR__objects/object_mo/gMorphaTentaclePart21DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart22DL", "__OTR__objects/object_mo/gMorphaTentaclePart23DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart24DL", "__OTR__objects/object_mo/gMorphaTentaclePart25DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart26DL", "__OTR__objects/object_mo/gMorphaTentaclePart27DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart28DL", "__OTR__objects/object_mo/gMorphaTentaclePart29DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart30DL", "__OTR__objects/object_mo/gMorphaTentaclePart31DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart32DL", "__OTR__objects/object_mo/gMorphaTentaclePart33DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart34DL", "__OTR__objects/object_mo/gMorphaTentaclePart35DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart36DL", "__OTR__objects/object_mo/gMorphaTentaclePart37DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart38DL", "__OTR__objects/object_mo/gMorphaTentaclePart39DL",
        "__OTR__objects/object_mo/gMorphaTentaclePart40DL",
    };
    for (const char* path : paths) {
        if (!available(path, owner))
            return false;
    }
    // Allocate every dynamic dependency before writing any GBI. Native scroll
    // and Hilite helpers allocate internally without an OOM contract; build
    // their same commands using this guarded, frame-owned allocation.
    struct Frame {
        Mtx nodes[41];
        Mtx incoming;
        Mtx hiliteMtx;
        LookAt lookAt;
        Hilite hilite;
        Gfx scroll[12];
    };
    if (ctx->polyXlu.d &&
        (reinterpret_cast<uintptr_t>(ctx->polyXlu.d) < reinterpret_cast<uintptr_t>(ctx->polyXlu.p) ||
         reinterpret_cast<uintptr_t>(ctx->polyXlu.d) - reinterpret_cast<uintptr_t>(ctx->polyXlu.p) < 128 * sizeof(Gfx)))
        return false;
    const size_t bytes = (sizeof(Frame) + 15u) & ~size_t(15u);
    // The unchanged core follows this optional pass. Keep room for its two
    // 12-command scroll DLs, matrix, and MM's eight-ENDDL segment cleanup,
    // including alignment slack for the native frame allocators.
    const size_t coreReserve = 32 * sizeof(Gfx) + sizeof(Mtx) + 64;
    if (reinterpret_cast<uintptr_t>(ctx->polyOpa.d) < reinterpret_cast<uintptr_t>(ctx->polyOpa.p) ||
        reinterpret_cast<uintptr_t>(ctx->polyOpa.d) - reinterpret_cast<uintptr_t>(ctx->polyOpa.p) < bytes + coreReserve)
        return false;
#ifdef COMBO_MORPHA_GI_HOST_MM
    auto* data = static_cast<Frame*>(GRAPH_ALLOC(ctx, bytes));
#else
    auto* data = static_cast<Frame*>(Graph_Alloc(ctx, sizeof(Frame)));
#endif
    if (!data)
        return false;
    MtxF incoming;
    Matrix_Get(&incoming);
    Matrix_MtxFToMtx(&incoming, &data->incoming);
    const float tau = 6.2831853071795864769f;
    const float phase = tau * float(play->gameplayFrames % 240u) / 240.f;
    static const float widths[41] = {
        3.56f, 3.25f, 2.96f, 2.69f, 2.44f, 2.21f, 2.f, 1.81f, 1.64f, 1.49f, 1.36f, 1.25f, 1.16f, 1.09f,
        1.04f, 1.01f, 1.f,   1.f,   1.f,   1.f,   1.f, 1.f,   1.f,   1.f,   1.f,   1.f,   1.f,   1.f,
        1.f,   1.f,   1.f,   1.f,   .98f,  .95f,  .9f, .8f,   .6f,   1.f,   1.f,   1.f,   1.f,
    };
    for (int i = 0; i < 41; ++i) {
        const int ring = i < 2 ? 0 : i - 2;
        const float u = float(ring) / 38.f;
        const float angle = u * tau * .65f;
        const float pulse = std::sin(phase + u * tau);
        const float x = 10.f * std::sin(angle) - 5.f;
        const float y = 54.f * u - 27.f;
        const float z = 2.f * pulse;
        const float dx = 13.f * tau * std::cos(angle);
        const float dy = 54.f;
        const float dz = 2.f * tau * std::cos(phase + u * tau);
        const float tangentLength = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float planarLength = std::sqrt(dx * dx + dy * dy);
        const float tx = dx / tangentLength, ty = dy / tangentLength, tz = dz / tangentLength;
        const float rx = dy / planarLength, ry = -dx / planarLength;
        // Native rings are post-rotated X by pi/2. These columns have that
        // orientation along the compact curve, including its gentle depth sway.
        const float bx = ry * tz, by = -rx * tz, bz = rx * ty - ry * tx;
        // Preserve native taper, with a local bulge enclosing the existing
        // core at GI origin. This is a presentation scale, not actor scale.
        const float bulge = 2.f * std::exp(-36.f * (u - .5f) * (u - .5f));
        const float scale = i < 2 ? 0.f : .003f * (widths[ring] + bulge) * (1.f + .08f * pulse);
        MtxF local = {};
        local.xx = rx * scale;
        local.yx = ry * scale;
        local.xy = bx * scale;
        local.yy = by * scale;
        local.zy = bz * scale;
        local.xz = -tx * scale;
        local.yz = -ty * scale;
        local.zz = -tz * scale;
        local.xw = x;
        local.yw = y;
        local.zw = z;
        local.ww = 1;
        MtxF posed = {};
        // Keep all incoming translation, orientation and nonuniform scale.
        const float* a = reinterpret_cast<const float*>(&incoming);
        const float* b = reinterpret_cast<const float*>(&local);
        float* out = reinterpret_cast<float*>(&posed);
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                for (int k = 0; k < 4; ++k)
                    out[column * 4 + row] += a[k * 4 + row] * b[column * 4 + k];
        Matrix_MtxFToMtx(&posed, &data->nodes[i]);
    }
    const uint32_t f = play->gameplayFrames & 2047u;
    const uint32_t secondX = (0u - f * 3u) & 2047u;
    gDPTileSync(&data->scroll[0]);
    gDPSetTileSizeLerp(&data->scroll[1], 0, f, f, f + 124, f + 124, f + 1, f + 1, f + 125, f + 125);
    gDPSetTileSizeLerp(&data->scroll[6], 1, secondX, f, secondX + 124, f + 124, int(secondX) - 3, f + 1,
                       int(secondX) + 121, f + 125);
    gSPEndDisplayList(&data->scroll[11]);
    const auto& direction = play->envCtx.dirLight1.params.dir;
    const auto& eye = play->view.eye;
    const float eyeX = eye.x == incoming.xw && eye.z == incoming.zw ? eye.x + .001f : eye.x;
    guLookAtHilite(&data->hiliteMtx, &data->lookAt, &data->hilite, eyeX, eye.y, eye.z, incoming.xw, incoming.yw,
                   incoming.zw, 0, 1, 0, direction.x, direction.y, direction.z, direction.x, direction.y, direction.z,
                   16, 16);
    static Gfx empty[8] = { gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList(),
                            gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList(), gsSPEndDisplayList() };
    OPEN_DISPS(ctx);
    CMG_SETUP_XLU(ctx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPGrayscale(POLY_XLU_DISP++, false);
    gSPComboRMPush(POLY_XLU_DISP++, owner);
    gSPSegment(POLY_XLU_DISP++, 8, reinterpret_cast<uintptr_t>(data->scroll));
    gSPSegment(POLY_XLU_DISP++, 12, reinterpret_cast<uintptr_t>(data->nodes));
    gDPSetPrimColor(POLY_XLU_DISP++, 0xFF, 0xFF, 200, 255, 255, 180);
    gDPSetEnvColor(POLY_XLU_DISP++, 0, 100, 255, 150);
    const uint16_t textureScale = uint16_t(350.f + 30.f * std::sin(phase));
    gSPTexture(POLY_XLU_DISP++, textureScale, textureScale, 0, G_TX_RENDERTILE, G_ON);
    gSPLookAt(POLY_XLU_DISP++, &data->lookAt);
    gDPSetHilite1Tile(POLY_XLU_DISP++, 1, &data->hilite, 16, 16);
    for (int i = 0; i < 41; ++i) {
        gSPMatrix(POLY_XLU_DISP++, &data->nodes[i], G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        // Defer canonical-path resolution until the owner bracket executes.
        // gSPDisplayList resolves unqualified paths through the CPU host RM.
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, paths[i], 0, G_DL_PUSH);
    }
    gSPComboRMPop(POLY_XLU_DISP++);
    static const int restoreSegments[3] = { 8, 9, 12 };
    for (int segment : restoreSegments)
        gSPSegment(POLY_XLU_DISP++, segment, reinterpret_cast<uintptr_t>(empty));
    CMG_SETUP_XLU(ctx);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, &data->incoming, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(ctx);
    return true;
}

#undef CMG_SETUP_XLU
#endif
