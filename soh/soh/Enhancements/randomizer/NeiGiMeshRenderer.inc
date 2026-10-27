#include "NeiElementalArrowGi.h"
#include "ComboRewardGi.h"
extern "C" int ResourceMgr_GetRewardSurfaceForGame(const char* game, const char* path, NeiGi::Mesh* mesh);
// Shared by the OoT and MM host renderers. The enclosing TU supplies native GBI/matrix APIs.
void NeiGi_ReportBlackMarketBounds(const NeiGi::Point& minimum, const NeiGi::Point& maximum) {
    if (!BlackMarketVisual::Active())
        return;
    MtxF matrix;
    Matrix_Get(&matrix);
    BlackMarketVisual::Report({ minimum.x, minimum.y, minimum.z }, { maximum.x, maximum.y, maximum.z },
                              [&](BlackMarketVisual::Point point) {
        return BlackMarketVisual::Point{ point.x * matrix.xx + point.y * matrix.xy + point.z * matrix.xz + matrix.xw,
                                         point.x * matrix.yx + point.y * matrix.yy + point.z * matrix.yz + matrix.yw,
                                         point.x * matrix.zx + point.y * matrix.zy + point.z * matrix.zz + matrix.zw };
    });
}

void NeiGi_ReportBlackMarketModel(const char* owner, const char* opaque, const char* translucent) {
    if (!BlackMarketVisual::Active() || !opaque || std::strncmp(opaque, "__OTR__", 7) != 0)
        return; // Loaded raw Gfx lists need their caller's original selected root.
    const char* roots[] = { opaque, translucent };
    float minimum[3], maximum[3];
    if (ResourceMgr_GetGiModelBoundsForGame(owner, roots, translucent ? 2 : 1, minimum, maximum))
        NeiGi_ReportBlackMarketBounds({ minimum[0], minimum[1], minimum[2] },
                                       { maximum[0], maximum[1], maximum[2] });
}

static void NeiGi_ReportBlackMarketMesh(const NeiGi::Mesh& mesh) {
    if (!BlackMarketVisual::Active() || !mesh.count || mesh.count > mesh.vertices.size())
        return;
    auto minimum = mesh.vertices[0].p, maximum = minimum;
    for (size_t i = 1; i < mesh.count; ++i) {
        const auto& point = mesh.vertices[i].p;
        minimum = { std::min(minimum.x, point.x), std::min(minimum.y, point.y), std::min(minimum.z, point.z) };
        maximum = { std::max(maximum.x, point.x), std::max(maximum.y, point.y), std::max(maximum.z, point.z) };
    }
    NeiGi_ReportBlackMarketBounds(minimum, maximum);
}

void NeiGi_ReportBlackMarketVertices(const Vtx* vertices, size_t count) {
    if (!BlackMarketVisual::Active() || !vertices || !count || count > 65536)
        return;
    NeiGi::Point minimum{ float(vertices[0].v.ob[0]), float(vertices[0].v.ob[1]), float(vertices[0].v.ob[2]) };
    auto maximum = minimum;
    for (size_t i = 1; i < count; ++i) {
        const NeiGi::Point point{ float(vertices[i].v.ob[0]), float(vertices[i].v.ob[1]), float(vertices[i].v.ob[2]) };
        minimum = { std::min(minimum.x, point.x), std::min(minimum.y, point.y), std::min(minimum.z, point.z) };
        maximum = { std::max(maximum.x, point.x), std::max(maximum.y, point.y), std::max(maximum.z, point.z) };
    }
    NeiGi_ReportBlackMarketBounds(minimum, maximum);
}

NeiGi::Basis NeiGi_CameraBasis(PlayState* play) {
    MtxF m;
    Matrix_Get(&m);
    auto local = [&](NeiGi::Point p) {
        return NeiGi::Point{ p.x * m.xx + p.y * m.yx + p.z * m.zx, p.x * m.xy + p.y * m.yy + p.z * m.zy,
                             p.x * m.xz + p.y * m.yz + p.z * m.zz };
    };
    const auto& b = play->billboardMtxF;
    const auto right = NeiGi::Unit(local({ b.xx, b.yx, b.zx }));
    const auto up = NeiGi::Unit(local({ b.xy, b.yy, b.zy }));
    return { right, up, NeiGi::Unit(NeiGi::Cross(right, up)) };
}

// Bitwise finite checks remain effective under the engines' fast-math flags.
// Reject a whole malformed mesh before any arena allocation or GPU submission.
static bool NeiGi_Finite(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}

static bool NeiGi_ValidScale(float scale) {
    return NeiGi_Finite(scale) && scale > 0.f && scale <= 2048.f;
}

// Both native allocators subtract the OPA tail without detecting exhaustion.
// Leave space for queued heads, aligned vertices/matrices and the debug nodes
// emitted by OPEN/CLOSE_DISPS (including each setup function's nested pair).
struct NeiGi_ArenaBudget {
    GraphicsContext* gfx = nullptr;
    size_t matrixBytes = 0, opaCommands = 0, xluCommands = 0;
};
static thread_local NeiGi_ArenaBudget NeiGi_ReservedArena;

static bool NeiGi_ArenaHasRoom(PlayState* play, size_t vertexBytes, size_t matrices, size_t opaCommands,
                               size_t xluCommands) {
    if (!play || !play->state.gfxCtx)
        return false;
    const auto* gfx = play->state.gfxCtx;
    const auto remaining = [](const void* head, const void* tail, bool alignTail) {
        uintptr_t begin = reinterpret_cast<uintptr_t>(head), end = reinterpret_cast<uintptr_t>(tail);
        if (alignTail)
            end &= ~uintptr_t(15); // OoT also discards a misaligned current tail.
        return begin && end >= begin ? size_t(end - begin) : size_t(0);
    };
    const size_t opaAvailable = remaining(gfx->polyOpa.p, gfx->polyOpa.d, true);
    const size_t xluAvailable = remaining(gfx->polyXlu.p, gfx->polyXlu.d, false);
    const size_t overlayAvailable = remaining(gfx->overlay.p, gfx->overlay.d, false);
    const auto reserved = NeiGi_ReservedArena.gfx == gfx ? NeiGi_ReservedArena : NeiGi_ArenaBudget{};
    const size_t tailBytes =
        ((vertexBytes + 15) & ~size_t(15)) + matrices * ((sizeof(Mtx) + 15) & ~size_t(15)) + reserved.matrixBytes;
    return tailBytes <= opaAvailable &&
           opaCommands + reserved.opaCommands <= (opaAvailable - tailBytes) / sizeof(Gfx) &&
           xluCommands + reserved.xluCommands <= xluAvailable / sizeof(Gfx) && overlayAvailable / sizeof(Gfx) >= 2;
}

// Child meshes must leave their caller's owner-pop, shell and restore commands
// plus matrices available. The reservation changes no native arena cursors.
class NeiGi_ArenaScope {
    NeiGi_ArenaBudget previous;
    bool active;

  public:
    NeiGi_ArenaScope(PlayState* play, size_t matrices, size_t opaCommands, size_t xluCommands, size_t tailBytes = 0)
        : previous(NeiGi_ReservedArena),
          active(NeiGi_ArenaHasRoom(play, tailBytes, matrices, opaCommands, xluCommands)) {
        if (active) {
            if (NeiGi_ReservedArena.gfx != play->state.gfxCtx)
                NeiGi_ReservedArena = { play->state.gfxCtx };
            NeiGi_ReservedArena.matrixBytes +=
                matrices * ((sizeof(Mtx) + 15) & ~size_t(15)) + ((tailBytes + 15) & ~size_t(15));
            NeiGi_ReservedArena.opaCommands += opaCommands;
            NeiGi_ReservedArena.xluCommands += xluCommands;
        }
    }
    ~NeiGi_ArenaScope() {
        if (active)
            NeiGi_ReservedArena = previous;
    }
    explicit operator bool() const {
        return active;
    }
};

bool NeiGi_CanDrawLayers(PlayState* play, size_t matrices, size_t opa, size_t xlu) {
    return NeiGi_ArenaHasRoom(play, 0, matrices, opa, xlu);
}

static bool NeiGi_ValidMesh(const NeiGi::Mesh& mesh, const NeiGi::TextureMaterial* material = nullptr,
                            bool seasonSunRays = false) {
    if (!mesh.count || mesh.count > mesh.vertices.size() || mesh.count % 3)
        return false;
    const auto fits = [](float value, float scale) {
        if (!NeiGi_Finite(value))
            return false;
        const float fixed = value * scale;
        return NeiGi_Finite(fixed) && fixed >= -32768.f && fixed <= 32767.f;
    };
    for (size_t i = 0; i < mesh.count; ++i) {
        const auto& v = mesh.vertices[i];
        if (!fits(v.p.x, 16) || !fits(v.p.y, 16) || !fits(v.p.z, 16) ||
            !fits(v.u, (material || seasonSunRays ? 32 : 63) * 32) ||
            !fits(v.v, (seasonSunRays ? 31
                        : material    ? 32
                                      : 63) *
                           32))
            return false;
    }
    return true;
}

static bool NeiGi_DrawMeshMaterial(PlayState* play, const NeiGi::Mesh& mesh, Kind orb,
                                   const NeiGi::TextureMaterial* material, bool seasonSunRays = false,
                                   const char* owner = nullptr, const NeiElementalGi::Palette* palette = nullptr) {
    if (!play || !play->state.gfxCtx || !NeiGi_ValidMesh(mesh, material, seasonSunRays) || int(orb) < 0 ||
        int(orb) > int(Kind::GiantsKnife))
        return false;
    // Reuse shared vertices within each 32-entry RSP cache load. A full potion
    // shop must leave room in both the OPA arena and XLU command buffer.
    struct Batch {
        size_t vertexStart, vertexCount, indexStart, indexCount;
    };
    std::array<Vtx, 1536> packed{};
    std::array<uint8_t, 1536> indices{};
    std::array<Batch, 64> batches{};
    size_t vertexCount = 0, indexCount = 0, batchCount = 0;
    Batch batch{};
    auto equal = [](const Vtx& a, const Vtx& b) {
        return std::memcmp(a.v.ob, b.v.ob, sizeof(a.v.ob)) == 0 && std::memcmp(a.v.tc, b.v.tc, sizeof(a.v.tc)) == 0 &&
               std::memcmp(a.v.cn, b.v.cn, 4) == 0;
    };
    auto finish = [&]() {
        if (batch.indexCount)
            batches[batchCount++] = batch;
        batch = { vertexCount, 0, indexCount, 0 };
    };
    for (size_t i = 0; i < mesh.count; i += 3) {
        Vtx triangle[3]{};
        for (size_t j = 0; j < 3; ++j) {
            const auto& v = mesh.vertices[i + j];
            triangle[j] = { { { static_cast<int16_t>(std::lround(v.p.x * 16)),
                                static_cast<int16_t>(std::lround(v.p.y * 16)),
                                static_cast<int16_t>(std::lround(v.p.z * 16)) },
                              0,
                              { static_cast<int16_t>(std::lround(v.u * (material || seasonSunRays ? 32 : 63) * 32)),
                                static_cast<int16_t>(std::lround(v.v *
                                                                 (seasonSunRays ? 31
                                                                  : material    ? 32
                                                                                : 63) *
                                                                 32)) },
                              { static_cast<uint8_t>(v.rgb >> 16), static_cast<uint8_t>(v.rgb >> 8),
                                static_cast<uint8_t>(v.rgb), v.alpha } } };
        }
        // Conservative reservation of three vertices keeps a complete triangle
        // in one load. Unused cache slots do not consume arena storage.
        if (batch.vertexCount > 29)
            finish();
        for (const auto& vertex : triangle) {
            size_t found = 0;
            while (found < batch.vertexCount && !equal(vertex, packed[batch.vertexStart + found]))
                ++found;
            if (found == batch.vertexCount) {
                packed[vertexCount++] = vertex;
                ++batch.vertexCount;
            }
            indices[indexCount++] = static_cast<uint8_t>(found);
            ++batch.indexCount;
        }
    }
    finish();
    size_t triangleCommands = 0;
    for (size_t b = 0; b < batchCount; ++b)
        triangleCommands += (batches[b].indexCount + 5) / 6;
    // At most 30 fixed commands: debug pairs, two setup calls, texture-load
    // expansion (seven commands), material state, one matrix and owner pair.
    const bool scrolling = material && material->scrolling;
    const size_t scrollBytes = scrolling ? 2 * sizeof(Gfx) : 0;
    if (!NeiGi_ArenaHasRoom(play, vertexCount * sizeof(Vtx) + scrollBytes, 1, 2,
                           32 + batchCount + triangleCommands + (scrolling ? 1 : 0)))
        return false;
    auto* vertices = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, vertexCount * sizeof(Vtx) + scrollBytes));
    if (vertices == nullptr)
        return false;
    std::memcpy(vertices, packed.data(), vertexCount * sizeof(Vtx));
    Gfx* scroll = nullptr;
    if (scrolling) {
        scroll = reinterpret_cast<Gfx*>(vertices + vertexCount);
        const auto offset = RewardGi_Scroll(play->gameplayFrames);
        gDPSetTileSize(scroll, G_TX_RENDERTILE, offset.s, offset.t, offset.s + 31 * 4, offset.t + 31 * 4);
        gSPEndDisplayList(scroll + 1);
    }
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPClearGeometryMode(POLY_XLU_DISP++,
                         G_LIGHTING | G_CULL_BACK | G_CULL_FRONT | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_FOG);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_2CYCLE);
    gDPSetAlphaCompare(POLY_XLU_DISP++, G_AC_NONE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    if (seasonSunRays) {
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        gDPSetTextureFilter(POLY_XLU_DISP++, G_TF_BILERP);
        gDPLoadTextureBlock(POLY_XLU_DISP++, NeiGi::SeasonSunRayTexture().data(), G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0,
                            G_TX_WRAP, G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
        // Tile coordinates are quarter texels. Keep the same average speed
        // without jumping a whole texel every four game ticks.
        const uint32_t scroll = play->gameplayFrames % 128u;
        gDPSetTileSize(POLY_XLU_DISP++, G_TX_RENDERTILE, scroll, 0, scroll + 31 * 4, 31 * 4);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA, G_CC_PASS2);
    } else if (material) {
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        gDPSetTextureFilter(POLY_XLU_DISP++, G_TF_BILERP);
        gDPLoadTextureBlock(POLY_XLU_DISP++, material->path, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                            material->repeatS ? G_TX_WRAP : G_TX_CLAMP, material->repeatT ? G_TX_WRAP : G_TX_CLAMP, 5,
                            5, G_TX_NOLOD, G_TX_NOLOD);
        // Resource RGBA multiplied by each sampled vertex's color AND alpha.
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA, G_CC_PASS2);
        if (scrolling) {
            // Execute the common material fragment directly. Scene segment
            // bindings belong to the GPU and need not equal CPU gSegments.
            __gSPDisplayList(POLY_XLU_DISP++, scroll);
        }
    } else if (orb != Kind::Neutral) {
        const auto color = palette ? NeiGi::OrbColors{palette->hot, palette->edge} : NeiGi::OrbPalette(orb);
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        gDPSetTextureFilter(POLY_XLU_DISP++, G_TF_BILERP);
        gDPLoadTextureBlock(POLY_XLU_DISP++, NeiGi::OrbTexture(orb, play->gameplayFrames).data(), G_IM_FMT_I,
                            G_IM_SIZ_8b, 64, 64, 0, G_TX_CLAMP, G_TX_CLAMP, 6, 6, G_TX_NOLOD, G_TX_NOLOD);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, color.hot >> 16, (color.hot >> 8) & 255, color.hot & 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, color.edge >> 16, (color.edge >> 8) & 255, color.edge & 255, 255);
        // I8 controls both the hot/edge gradient and alpha, exactly as previewed.
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_BLENDPE, G_CC_PASS2);
    } else {
        gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
    }
    Matrix_Push();
    Matrix_Scale(1.f / 16, 1.f / 16, 1.f / 16, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    for (size_t b = 0; b < batchCount; ++b) {
        const auto& part = batches[b];
        gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(vertices + part.vertexStart), part.vertexCount, 0);
        const auto* index = indices.data() + part.indexStart;
        for (size_t j = 0; j < part.indexCount; j += 6) {
            if (j + 3 < part.indexCount) {
                gSP2Triangles(POLY_XLU_DISP++, index[j], index[j + 1], index[j + 2], 0, index[j + 3], index[j + 4],
                              index[j + 5], 0);
            } else {
                gSP1Triangle(POLY_XLU_DISP++, index[j], index[j + 1], index[j + 2], 0);
            }
        }
    }
    Matrix_Pop();
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    return true;
}

void NeiGi_DrawMesh(PlayState* play, const NeiGi::Mesh& mesh, Kind orb) {
    NeiGi_DrawMeshMaterial(play, mesh, orb, nullptr);
}

bool NeiGi_DrawMeshWithTail(PlayState* play, const NeiGi::Mesh& mesh, const char* owner,
                            size_t matrices, size_t opa, size_t xlu) {
    NeiGi_ArenaScope arena(play, matrices, opa, xlu);
    if (!arena)
        return false;
    return NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, nullptr, false, owner);
}

bool NeiGi_DrawTexturedMesh(PlayState* play, const NeiGi::Mesh& mesh, const NeiGi::TextureMaterial& material) {
    if (!play || !play->state.gfxCtx || !NeiGi_ValidMesh(mesh, &material) || !HasResource(material.path))
        return false;
    return NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, &material);
}

static NeiElementalGi::Palette NeiGi_ElementalPalette(PlayState* play, int profile, bool spell, bool mmOwner) {
#ifdef NEI_GI_NATIVE_MM
    constexpr bool nativeMm = true;
#else
    constexpr bool nativeMm = false;
#endif
    return NeiElementalGi::ReadPalette(profile, spell, mmOwner, [&](const char* key, uint32_t& rgb) {
        const std::string base(key);
        if (!CVarGetInteger((base + ".Changed").c_str(), 0) && !CVarGetInteger((base + ".Rainbow").c_str(), 0))
            return false;
        const std::string value = base + (mmOwner ? ".Color" : ".Value");
        Color_RGB8 color = {uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb)};
        color = CVarGetColor24(value.c_str(), color);
#ifdef COMBO_BUILD
        if (mmOwner != nativeMm) {
            using Clock = void (*)(uint32_t);
            using Sample = void (*)(const char*, uint8_t, uint8_t, uint8_t, uint8_t*);
            // Retry resolution until the donor is loaded. Sample colors afresh on every draw.
            static Clock ootClock = nullptr, mmClock = nullptr;
            static Sample ootSample = nullptr, mmSample = nullptr;
            auto& clock = mmOwner ? mmClock : ootClock;
            auto& sample = mmOwner ? mmSample : ootSample;
            if (!clock) clock = reinterpret_cast<Clock>(Combo_ResolveSym(mmOwner ? "2ship" : "soh",
                mmOwner ? "MM_SetGiCosmeticFrame" : "OOT_SetGiCosmeticFrame"));
            if (!sample) sample = reinterpret_cast<Sample>(Combo_ResolveSym(mmOwner ? "2ship" : "soh",
                mmOwner ? "MM_SampleGiCosmeticColor" : "OOT_SampleGiCosmeticColor"));
            if (clock) clock(play->gameplayFrames);
            if (sample) {
                uint8_t sampled[3] = {color.r, color.g, color.b};
                sample(value.c_str(), color.r, color.g, color.b, sampled);
                color = {sampled[0], sampled[1], sampled[2]};
            }
        }
#endif
        rgb = uint32_t(color.r) << 16 | uint32_t(color.g) << 8 | color.b;
        return true;
    });
}

static void NeiGi_RestoreElemental(PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}

void NeiGi_DrawElementalArrowForOwner(PlayState* play, int profile, int mmOwner, int shop) {
    if (!play || !play->state.gfxCtx || !NeiArrowGi::Valid(profile))
        return;
    const auto palette = NeiGi_ElementalPalette(play, profile, false, mmOwner != 0);
    auto layers = NeiArrowGi::Sample(profile, play->gameplayFrames, NeiGi_CameraBasis(play), &palette, shop != 0);
    const size_t vertices = layers.veil.count + layers.energy.count + layers.shimmer.count;
    const size_t batches = (layers.veil.count + 29) / 30 + (layers.energy.count + 29) / 30 +
                           (layers.shimmer.count + 29) / 30;
    if (!NeiGi_ArenaHasRoom(play, vertices * sizeof(Vtx), 4, 16, 144 + batches + vertices / 3))
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 20);
    if (!arena)
        return;
    const char* texture = NeiArrowGi::Texture(profile);
    // The selected core stays in its native owner. Private aura art belongs to OoT.
    const bool textured = HasResource(texture);
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    if (textured) gSPComboRMPush(POLY_XLU_DISP++, "oot");
    CLOSE_DISPS(play->state.gfxCtx);
    const NeiGi::TextureMaterial material{texture};
    if (!textured || !NeiGi_DrawTexturedMesh(play, layers.veil, material)) {
        NeiGi_DrawMesh(play, layers.veil);
    }
    OPEN_DISPS(play->state.gfxCtx);
    if (textured) gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, layers.energy);
    NeiGi_DrawMesh(play, layers.shimmer);
    NeiGi_RestoreElemental(play);
}

void NeiGi_DrawElementalArrow(PlayState* play, int profile) {
#ifdef NEI_GI_NATIVE_MM
    NeiGi_DrawElementalArrowForOwner(play, profile, 1, 0);
#else
    NeiGi_DrawElementalArrowForOwner(play, profile, 0, 0);
#endif
}

int NeiGi_DrawElementalArrowShop(PlayState* play, int profile) {
    // OPA setup also emits nested XLU debug markers before CLOSE_DISPS.
    if (!play || !play->state.gfxCtx || !NeiArrowGi::Valid(profile) || !NeiGi_ArenaHasRoom(play, 0, 1, 12, 2))
        return false;
#ifdef NEI_GI_NATIVE_MM
    constexpr int owner = 1;
    const char* core = "__OTR__objects/object_gi_m_arrow/gGiMagicArrowAmmoDL";
#else
    constexpr int owner = 0;
    const char* core = "__OTR__objects/object_gi_m_arrow/gGiMagicArrowDL";
#endif
    // Resolve the original resource at draw time so native Alt/model selection
    // keeps working. The caller's shelf tilt, scale and origin remain intact.
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, core, 0, G_DL_PUSH);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawElementalArrowForOwner(play, profile, owner, 1);
    return true;
}

int NeiGi_CanDrawElementalSpellFallback(PlayState* play) {
    // Twelve scroll commands, eight benign seg8 restore entries and one
    // native modelview. Gfx is larger than Mtx/8 on the 64-bit host.
    return NeiGi_ArenaHasRoom(play, 20 * sizeof(Gfx), 1, 12, 24);
}

int NeiGi_DrawElementalSpell(PlayState* play, int profile, int mmOwner, int shop) {
    if (!play || !play->state.gfxCtx || !NeiArrowGi::Valid(profile))
        return false;
    const auto palette = NeiGi_ElementalPalette(play, profile, true, mmOwner != 0);
    const auto layers = NeiSpellGi::Sample(profile, play->gameplayFrames, NeiGi_CameraBasis(play), &palette, shop != 0);
    size_t vertices = 0, batches = 0;
    for (const auto* mesh : {&layers.back, &layers.veil, &layers.energy, &layers.front, &layers.shimmer}) {
        vertices += mesh->count;
        batches += (mesh->count + 29) / 30;
    }
    // All five layers or none: do not leave half a crystal when a shelf fills the arena.
    if (!NeiGi_ArenaHasRoom(play, vertices * sizeof(Vtx), 6, 20, 220 + batches + vertices / 3))
        return false;
    NeiGi_ArenaScope arena(play, 1, 2, 20);
    if (!arena)
        return false;
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, layers.back);
    const auto kind = profile == 1 ? Kind::Fire : profile == 2 ? Kind::Hylia : Kind::Ice;
    NeiGi_DrawMeshMaterial(play, layers.veil, kind, nullptr, false, nullptr, &palette);
    NeiGi_DrawMesh(play, layers.energy);
    NeiGi_DrawMesh(play, layers.front);
    NeiGi_DrawMesh(play, layers.shimmer);
    NeiGi_RestoreElemental(play);
    return true;
}

void NeiGi_DrawSagesTunicMedallions(PlayState* play, const char* owner) {
    static constexpr const char* faces[] = {
        "__OTR__objects/object_gi_medal/gGiForestMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiFireMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiWaterMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiSpiritMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiShadowMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiLightMedallionFaceDL",
    };
    static constexpr const char* body = "__OTR__objects/object_gi_medal/gGiMedallionDL";
    // This intrinsic pass must leave the caller's model, owner-pop and restore
    // tail intact. Missing original medal resources retain the robe alone.
    if (!HasResource(body) || !NeiGi_ArenaHasRoom(play, 0, 6, 48, 2))
        return;
#ifdef NEI_GI_NATIVE_MM
    const uint32_t frame = play->state.frames;
#else
    const uint32_t frame = play->gameplayFrames;
#endif
    const uint32_t bits = (frame * 2u) & 0xFFFFu;
    const float spin = (bits >= 0x8000u ? int32_t(bits) - 0x10000 : int32_t(bits)) * .01f;
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPush(POLY_OPA_DISP++, owner);
    gSPGrayscale(POLY_OPA_DISP++, false);
    for (int i = 0; i < 6; ++i) {
        if (!HasResource(faces[i]))
            continue;
        const float t = (frame + i * 7u) % 40u;
        const float theta = 1.5707963267948966f + i * 1.0471975511965976f;
        float scale = .35f;
        if (t < 4.f)
            scale *= t / 4.f;
        else if (t >= 31.f)
            scale *= (39.f - t) / 8.f;
        if (scale <= .001f)
            continue;
#ifdef NEI_GI_NATIVE_MM
        Gfx_SetupDL26_Opa(play->state.gfxCtx);
#else
        Gfx_SetupDL_26Opa(play->state.gfxCtx);
#endif
        Matrix_Push();
#ifdef NEI_GI_ROTATE_Y
        NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
        Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
        Matrix_Translate(std::cos(theta) * 1.5f * t, (std::sin(theta) * 1.5f + 1.5f) * t - .5f * .07f * t * t, 14.f,
                         MTXMODE_APPLY);
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef NEI_GI_ROTATE_Y
        NEI_GI_ROTATE_Y(frame * .09f + i, MTXMODE_APPLY);
#else
        Matrix_RotateY(frame * .09f + i, MTXMODE_APPLY);
#endif
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD);
        gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, faces[i], 0, G_DL_PUSH);
        gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, body, 0, G_DL_PUSH);
        Matrix_Pop();
    }
    if (owner)
        gSPComboRMPop(POLY_OPA_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
}

void NeiGi_DrawPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int effect,
                            const float center[3], bool shimmer, const char* owner, bool shop, int mmPickup,
                            int shimmerKind) {
    if (!play || !play->state.gfxCtx || !opa || !center || !NeiGi_ValidScale(scale) || !NeiGi_Finite(center[0]) ||
        !NeiGi_Finite(center[1]) || !NeiGi_Finite(center[2]) || effect < 0 || effect > int(Kind::GiantsKnife) ||
        shimmerKind < -1 || shimmerKind > int(Kind::Last))
        return;
    const Kind kind = static_cast<Kind>(effect);
    const uint32_t bits = (uint32_t(play->gameplayFrames) * 2u) & 0xFFFFu;
    const float spin = (bits >= 0x8000u ? int32_t(bits) - 0x10000 : int32_t(bits)) * .01f;
    const auto* bounds = NeiGi::FindFrameBounds(opa);
    if (!bounds || bounds->effect != kind)
        return;
    NeiGi_ArenaScope arena(play, 3, 16, 24);
    if (!arena)
        return;
    Matrix_Push();
    const auto fit = NeiGi::FrameFit(*bounds, scale, shop, mmPickup);
    Matrix_Translate(0.f, fit.lift, 0.f, MTXMODE_APPLY);
    Matrix_Scale(fit.scale, fit.scale, fit.scale, MTXMODE_APPLY);
    if (std::strcmp(bounds->slug, "sages_tunic") == 0)
        NeiGi_DrawSagesTunicMedallions(play, owner);
    OPEN_DISPS(play->state.gfxCtx);
    if (owner) {
        gSPComboRMPush(POLY_OPA_DISP++, owner);
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    }
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPGrayscale(POLY_XLU_DISP++, false);
    Matrix_Push();
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef NEI_GI_ROTATE_Y
    NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
    Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
    NeiGi_ReportBlackMarketBounds(bounds->minimum, bounds->maximum);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, opa, 0, G_DL_PUSH);
    // Child mesh renderers advance the same streams. Publish these cursors
    // before invoking them, then reopen after all procedural submissions.
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
    Matrix_Push();
#ifdef NEI_GI_ROTATE_Y
    NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
    Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
    const auto camera = NeiGi_CameraBasis(play);
    Matrix_Push();
    Matrix_Translate(center[0], center[1], center[2], MTXMODE_APPLY);
    if (NeiGi::IsRod(kind) || NeiGi::IsSpell(kind)) {
        NeiGi_DrawMesh(play, NeiGi::SampleOrb(kind, camera), kind);
        NeiGi_DrawMesh(play, NeiGi::SampleEnergy(kind, play->gameplayFrames, camera));
    }
    if (NeiGi::IsSpecial(kind))
        NeiGi_DrawMesh(play, NeiGi::SampleSpecial(kind, play->gameplayFrames, camera, 1.f / fit.scale));
    Matrix_Pop();
    if (shimmer)
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, camera,
                                                  shimmerKind < 0 ? kind : static_cast<Kind>(shimmerKind)));
    Matrix_Pop();
    if (kind == Kind::SeasonCycle)
        NeiGi_DrawSeasonOverlay(play, 5, nullptr);
    OPEN_DISPS(play->state.gfxCtx);
    if (xlu) {
        Matrix_Push();
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef NEI_GI_ROTATE_Y
        NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
        Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, xlu, 0, G_DL_PUSH);
        Matrix_Pop();
    }
    CLOSE_DISPS(play->state.gfxCtx);
    if (xlu && NeiGi::IsSpell(kind)) {
        Matrix_Push();
#ifdef NEI_GI_ROTATE_Y
        NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
        Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
        NeiGi_DrawMesh(play, NeiGi::SampleCrystalSheen(play->gameplayFrames, NeiGi_CameraBasis(play)));
        Matrix_Pop();
    }
    OPEN_DISPS(play->state.gfxCtx);
    if (owner) {
        gSPComboRMPop(POLY_OPA_DISP++);
        gSPComboRMPop(POLY_XLU_DISP++);
    }
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}

void NeiGi_DrawExternalPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int shimmerKind,
                                    bool shimmer, const char* owner, bool shop, float tilt, bool fit, int mmPickup) {
    if (!play || !play->state.gfxCtx || !opa || !NeiGi_ValidScale(scale) || shimmerKind < 0 ||
        shimmerKind > int(Kind::Last))
        return;
    NeiGi_ArenaScope arena(play, 2, 16, 16);
    if (!arena)
        return;
    const uint32_t bits = (uint32_t(play->gameplayFrames) * 2u) & 0xFFFFu;
    const float spin = (bits >= 0x8000u ? int32_t(bits) - 0x10000 : int32_t(bits)) * .01f;
    const bool sword = NeiGi::IsSword(static_cast<Kind>(shimmerKind));
    if (sword)
        tilt = ComboSwordGi_SelectedTilt(tilt, shop);
    Matrix_Push();
    if (fit && sword)
        ComboSwordGi_ApplyPresentationSize(shop, mmPickup);
    if (fit && mmPickup && !sword) {
        const char* paths[] = { opa, xlu };
        ComboSwordGi_ApplyModelsFit(owner ? owner : "oot", paths, xlu ? 2 : 1, scale, tilt, shop, mmPickup);
    }
    Matrix_Push();
    if (fit && sword) {
        const char* paths[] = { opa, xlu };
        ComboSwordGi_ApplyModelsFit(owner ? owner : "oot", paths, xlu ? 2 : 1, scale, tilt, shop, mmPickup);
    }
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef NEI_GI_ROTATE_Y
    NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
    Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
    if (tilt != 0.f) {
#ifdef NEI_GI_ROTATE_Z
        NEI_GI_ROTATE_Z(tilt, MTXMODE_APPLY);
#else
        Matrix_RotateZ(tilt, MTXMODE_APPLY);
#endif
    }
    NeiGi_ReportBlackMarketModel(owner ? owner : "oot", opa, xlu);
    OPEN_DISPS(play->state.gfxCtx);
    if (owner) {
        gSPComboRMPush(POLY_OPA_DISP++, owner);
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    }
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, opa, 0, G_DL_PUSH);
    if (xlu) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPGrayscale(POLY_XLU_DISP++, false);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, xlu, 0, G_DL_PUSH);
    }
    if (owner) {
        gSPComboRMPop(POLY_OPA_DISP++);
        gSPComboRMPop(POLY_XLU_DISP++);
    }
    CLOSE_DISPS(play->state.gfxCtx);
    if (tilt != 0.f && NeiGi::IsSword(static_cast<Kind>(shimmerKind)))
        ComboDinSwordGi_DrawLayers(play, owner ? owner : "oot", opa);
    Matrix_Pop();
    // Sword particles belong to the awarded sword, including mod geometry.
    if (sword) {
        ComboSwordGi_ApplyEffectFit(static_cast<Kind>(shimmerKind), shop, mmPickup);
        NeiGi_DrawMesh(play, NeiGi::SampleSpecial(
                                 static_cast<Kind>(shimmerKind), play->gameplayFrames, NeiGi_CameraBasis(play),
                                 ComboSwordGi_ParticleScale(static_cast<Kind>(shimmerKind), shop, mmPickup, fit)));
    }
    if (shimmer)
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, NeiGi_CameraBasis(play),
                                                  static_cast<Kind>(shimmerKind)));
    Matrix_Pop();
}

void NeiGi_DrawSeasonOverlay(PlayState* play, int profile, const char* owner) {
    if (!play || !play->state.gfxCtx || profile < 1 || profile > 6)
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena)
        return;
    const bool stormSong = profile == 6;
    const bool sun = NeiGi::SeasonProfile(play->gameplayFrames, profile) == 2;
    if (sun) {
        Matrix_Push();
        // Record the billboard through the native matrix interpolator. Baking
        // its inverse parent rotation into vertices snaps at game ticks while
        // the held GI's parent spin continues between those ticks.
        Matrix_ReplaceRotation(&play->billboardMtxF);
    }
    const auto camera = sun ? NeiGi::Basis{} : NeiGi_CameraBasis(play);
    auto mesh = NeiGi::SampleSeason(play->gameplayFrames, stormSong ? 1 : profile, camera);
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, mesh, Kind::Neutral);
    if (profile <= 4)
        NeiGi_ReportBlackMarketMesh(mesh);
    if (sun) {
        const auto rays = NeiGi::SampleSeasonSunRays(play->gameplayFrames, profile, camera);
        if (NeiGi_DrawMeshMaterial(play, rays, Kind::Neutral, nullptr, true) && profile <= 4)
            NeiGi_ReportBlackMarketMesh(rays);
        Matrix_Pop();
    }
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}

// All shimmer palettes preserve the same owner, grayscale and matrix boundary.
static void NeiGi_DrawShimmerMesh(PlayState* play, const NeiGi::Mesh& mesh, const char* owner) {
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, mesh, Kind::Neutral);
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}

void NeiGi_DrawWorldParticles(PlayState* play, const NeiGi::Mesh& mesh) {
    if (!play || !play->state.gfxCtx)
        return;
    NeiGi_DrawShimmerMesh(play, mesh, nullptr);
}

// A single two-color mesh keeps the Chateau's gold and burgundy motes distinct.
// Native C and C++ draw paths share the same bounded, frame-derived sampler.
void NeiGi_DrawChateauShimmer(PlayState* play) {
    if (!play || !play->state.gfxCtx)
        return;
    const auto mesh = NeiGi::SampleShimmer(play->gameplayFrames, true, NeiGi_CameraBasis(play), Kind::ChateauRomani);
    NeiGi_DrawShimmerMesh(play, mesh, nullptr);
}

// Shared by native and foreign mask/remains and progressive sword overlays.
// Reuse the exact optional NEI shimmer sampler and renderer; only the requested
// transformation/remains palette changes. Ordinary items retain NEI's palette.
static void NeiGi_DrawColorShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner,
                                        bool explicitTint) {
    if (!play || !play->state.gfxCtx || !color)
        return;
    const bool ordinary = !explicitTint && color[0] == 220 && color[1] == 225 && color[2] == 240;
    const bool dark = color[0] == 0 && color[1] == 0 && color[2] == 0;
    auto mesh = NeiGi::SampleShimmer(play->gameplayFrames, true, NeiGi_CameraBasis(play));
    if (!ordinary) {
        const uint32_t rgb = uint32_t(color[0]) << 16 | uint32_t(color[1]) << 8 | color[2];
        for (size_t i = 0; i < mesh.count; ++i) {
            auto& vertex = mesh.vertices[i];
            vertex.rgb = vertex.rgb == 0xFFFFFF ? (dark ? 0x505060 : 0xFFFFFF) : rgb;
        }
    }
    NeiGi_DrawShimmerMesh(play, mesh, owner);
}

void NeiGi_DrawShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner) {
    NeiGi_DrawColorShimmerOverlay(play, color, owner, false);
}

extern "C" void NeiGi_DrawTintedShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner) {
    NeiGi_DrawColorShimmerOverlay(play, color, owner, true);
}

// All nine rewards share this material and scroll fragment. Identity selects
// only the periodic energy image and hex tint; the selected model supplies
// the triangles. Missing/unsupported resources leave the native draw visible.
void NeiGi_DrawRewardMaterial(PlayState* play, int profile, const char* surfacePath, const char* settingPath,
                              const char* owner) {
    uint8_t color[4];
    if (!play || !owner || !surfacePath || !RewardGi_ShimmerColor(profile, color)) return;
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena) return;
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi::Mesh mesh{};
    constexpr const char* metal = "__OTR__objects/nei_reward_gi/metal";
    if (settingPath && HasResource(metal) && ResourceMgr_GetRewardSurfaceForGame(owner, settingPath, &mesh)) {
        // Keep the original lighting beneath a restrained brushed-gold layer.
        for (size_t i = 0; i < mesh.count; ++i) {
            mesh.vertices[i].rgb = 0xE6C780;
            mesh.vertices[i].alpha = 92;
        }
        const NeiGi::TextureMaterial material{metal, true, true};
        NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, &material, false, "oot");
    }
    const char* energy = RewardGi_Texture(profile);
    if (HasResource(energy) && ResourceMgr_GetRewardSurfaceForGame(owner, surfacePath, &mesh)) {
        for (size_t i = 0; i < mesh.count; ++i) {
            mesh.vertices[i].rgb = RewardGi_Color(profile);
            mesh.vertices[i].alpha = 220;
        }
        const NeiGi::TextureMaterial material{energy, true, true, true};
        NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, &material, false, "oot");
    }
    NeiGi_DrawShimmerOverlay(play, color, owner);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}

// Four faint GI-local motes for the accepted bundled fairy. Sampling uses
// only the active host frame; the caller has restored its incoming bottle pose.
void ComboBottleShimmer_DrawMotes(PlayState* play, int profile) {
    if (!play || !play->state.gfxCtx || profile != CW_SHIMMER_FAIRY)
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena)
        return;
    const auto mesh = NeiGi::SampleBottleMotes(profile, play->gameplayFrames, true, NeiGi_CameraBasis(play));
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, mesh, Kind::Neutral);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}

void NeiGi_DrawSongOverlay(PlayState* play, int song, const char* owner) {
    if (!play || !play->state.gfxCtx || song < 0 || song >= CW_SONG_COUNT)
        return;
    if (song == CW_SONG_STORMS) {
        NeiGi_DrawSeasonOverlay(play, 6, owner);
        return;
    }
    if (ComboSongAlwaysShimmers(song)) {
        uint8_t color[4];
        ComboSongShimmerColor(song, color);
        NeiGi_DrawShimmerOverlay(play, color, owner);
    }
    const auto mesh = NeiGi::SampleSong(song, play->gameplayFrames, NeiGi_CameraBasis(play));
    if (!mesh.count)
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, mesh);
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}
