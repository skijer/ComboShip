#include "z64skybox.h"
#include "global.h"
#include "BenPort.h"
#include "libultraship/bridge/consolevariablebridge.h"
#include "assets/misc/skyboxes/d2_cloud_static.h"
#include "assets/misc/skyboxes/d2_fine_static.h"
#include "assets/misc/skyboxes/d2_fine_pal_static.h"
#include "assets/2s2h_assets.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"

typedef struct SkyboxFaceParams {
    /* 0x00 */ s32 xStart;
    /* 0x04 */ s32 yStart;
    /* 0x08 */ s32 zStart;
    /* 0x0C */ s32 outerIncrVal;
    /* 0x10 */ s32 innerIncrVal;
} SkyboxFaceParams; // size = 0x14

// Converts texture coordinate values to s10.5 fixed point
#define TC(x) ((s16)((x)*32))

// Texture offsets for each face in the static segment buffer
u32 sSkybox128TexOffsets[] = {
    128 * 64 * 0, 128 * 64 * 1, 128 * 64 * 2, 128 * 64 * 3, 128 * 64 * 4, 128 * 64 * 4 + 128 * 128,
};

// Maps vertex buffer index to coordinate buffer index
u16 sSkybox128VtxBufIndices[] = {
    0, 2, 10, 12, 2, 4, 12, 14, 10, 12, 20, 22, 12, 14, 22, 24, 1, 3, 5, 6, 7, 8, 9, 11, 13, 15, 16, 17, 18, 19, 21, 23,
};

// S coordinates for all faces
s32 sSkybox128TexSCoords[] = {
    TC(62 * 0), TC(62 * 1), TC(62 * 2), TC(62 * 3), TC(62 * 4),
};

// T coordinates for top and bottom faces
s32 sSkybox128TexTCoordsXZ[] = {
    TC(62 * 0), TC(62 * 1), TC(62 * 2), TC(62 * 3), TC(62 * 4),
};

// T coordinates for side faces
s32 sSkybox128TexTCoords[] = {
    TC(62 * 0), TC(62 * 1), TC(62 * 2), TC(62 * 1), TC(62 * 0),
};

// Maps vertex index to vertex buffer index
s16 sSkybox128VtxIndices[] = {
    0,  16, 19, 18, 16, 1,  20, 19, 1,  17, 21, 20, 17, 5,  22, 21, 18, 19, 23, 2,  19, 20,
    3,  23, 20, 21, 24, 3,  21, 22, 7,  24, 2,  23, 26, 25, 23, 3,  27, 26, 3,  24, 28, 27,
    24, 7,  29, 28, 25, 26, 30, 10, 26, 27, 11, 30, 27, 28, 31, 11, 28, 29, 15, 31,
};

// 2S2H [Port] Skybox textures for our resource manager.
// The last texture is repeated for the top and bottom skybox faces.
TexturePtr sSkyboxTextures[2][6] = {
    // d2_fine_static
    { gSkyboxClear1Tex, gSkyboxClear2Tex, gSkyboxClear3Tex, gSkyboxClear4Tex, gSkyboxClear5Tex, gSkyboxClear5Tex },
    // d2_cloud_static
    { gSkyboxCloudy1Tex, gSkyboxCloudy2Tex, gSkyboxCloudy3Tex, gSkyboxCloudy4Tex, gSkyboxCloudy5Tex,
      gSkyboxCloudy5Tex },
};

// Explicit Alt paths keep the OoT companion's native CI8 sky out of this
// full-color path. The complete pack is checked once at scene initialization.
#define OOT_SKY_FACES(folder, name)                                                                       \
    {                                                                                                     \
        "__OTR__alt/textures/" folder "/" name "1Tex", "__OTR__alt/textures/" folder "/" name "2Tex",     \
            "__OTR__alt/textures/" folder "/" name "3Tex", "__OTR__alt/textures/" folder "/" name "4Tex", \
            "__OTR__alt/textures/" folder "/" name "5Tex"                                                 \
    }
static const ALIGN_ASSET(2) char sOotSkyTextures[2][4][5][96] = {
    { OOT_SKY_FACES("vr_fine0_static", "gSunriseSkybox"), OOT_SKY_FACES("vr_fine1_static", "gDaySkybox"),
      OOT_SKY_FACES("vr_fine2_static", "gSunsetSkybox"), OOT_SKY_FACES("vr_fine3_static", "gNightSkybox") },
    { OOT_SKY_FACES("vr_cloud0_static", "gSunriseOvercastSkybox"),
      OOT_SKY_FACES("vr_cloud1_static", "gDayOvercastSkybox"),
      OOT_SKY_FACES("vr_cloud2_static", "gSunsetOvercastSkybox"),
      OOT_SKY_FACES("vr_cloud3_static", "gNightOvercastSkybox") },
};
#undef OOT_SKY_FACES

static s32 Skybox_HasOotTextures(void) {
    if (!CVarGetInteger("gEnhancements.Graphics.UseOotSkyTextures", 0) || !ResourceMgr_IsAltAssetsEnabled()) {
        return false;
    }
    for (s32 weather = 0; weather < 2; weather++) {
        for (s32 phase = 0; phase < 4; phase++) {
            for (s32 face = 0; face < 5; face++) {
                if (!ResourceMgr_FileExists(sOotSkyTextures[weather][phase][face])) {
                    return false;
                }
            }
        }
    }
    return true;
}

s32 Skybox_IsOotSkyActive(const SkyboxContext* skyboxCtx, s16 skyboxId) {
    return ((skyboxId == SKYBOX_NORMAL_SKY) || (skyboxId == SKYBOX_3)) && skyboxCtx->ootSkyDLists[0] &&
           skyboxCtx->ootSkyDLists[1] && CVarGetInteger("gEnhancements.Graphics.UseOotSkyTextures", 0) &&
           ResourceMgr_IsAltAssetsEnabled();
}

s32 Skybox_PrepareOot(SkyboxContext* skyboxCtx, s16 skyboxId, u16 time, u8* timeBlend) {
    u8 first = 3;
    u8 second = 3;
    u16 start = 0;
    u16 end = 0;

    if (!Skybox_IsOotSkyActive(skyboxCtx, skyboxId)) {
        return false;
    }

    // Match MM's dawn/day/sunset/night schedule, retaining full-color OoT art.
    if ((time >= CLOCK_TIME(4, 0)) && (time < CLOCK_TIME(6, 0))) {
        first = 3;
        second = 0;
        start = CLOCK_TIME(4, 0);
        end = CLOCK_TIME(6, 0);
    } else if ((time >= CLOCK_TIME(6, 0)) && (time < CLOCK_TIME(8, 0))) {
        first = 0;
        second = 1;
        start = CLOCK_TIME(6, 0);
        end = CLOCK_TIME(8, 0);
    } else if ((time >= CLOCK_TIME(8, 0)) && (time < CLOCK_TIME(16, 0))) {
        first = second = 1;
    } else if ((time >= CLOCK_TIME(16, 0)) && (time < CLOCK_TIME(17, 0))) {
        first = 1;
        second = 2;
        start = CLOCK_TIME(16, 0);
        end = CLOCK_TIME(17, 0);
    } else if ((time >= CLOCK_TIME(17, 0)) && (time < CLOCK_TIME(18, 0))) {
        first = second = 2;
    } else if ((time >= CLOCK_TIME(18, 0)) && (time < CLOCK_TIME(19, 0))) {
        first = 2;
        second = 3;
        start = CLOCK_TIME(18, 0);
        end = CLOCK_TIME(19, 0);
    }
    *timeBlend = (start == end) ? 0 : ((u32)(time - start) * 255 / (end - start));

    if ((first != skyboxCtx->ootSkyPhases[0]) || (second != skyboxCtx->ootSkyPhases[1])) {
        // Reuse the native geometry; only texture references differ. Separate
        // lists let the cloud overlay blend independently of the time transition.
        SkyboxContext layer = *skyboxCtx;
        for (s32 weather = 0; weather < 2; weather++) {
            layer.dListBuf = skyboxCtx->ootSkyDLists[weather];
            for (s32 face = 0; face < 6; face++) {
                s32 textureFace = (face == 5) ? 4 : face;
                layer.staticSegments[0][face] = (void*)sOotSkyTextures[weather][first][textureFace];
                layer.staticSegments[1][face] = (void*)sOotSkyTextures[weather][second][textureFace];
            }
            Skybox_Calculate128(&layer, 5);
        }
        skyboxCtx->ootSkyPhases[0] = first;
        skyboxCtx->ootSkyPhases[1] = second;
    }
    return true;
}

u8 Skybox_GetOotCloudBlend(SkyboxContext* skyboxCtx, s16 blend) {
    // Environment_UpdateSkybox has already applied native day/story weather and
    // the optional outdoor-weather fade to these two synchronous bindings.
    s32 firstCloudy = skyboxCtx->staticSegments[0][0] == sSkyboxTextures[SKYBOX_TEXTURES_CLOUD][0];
    s32 secondCloudy = skyboxCtx->staticSegments[1][0] == sSkyboxTextures[SKYBOX_TEXTURES_CLOUD][0];
    blend = CLAMP(blend, 0, 255);
    return MMWeather_ResolveOotCloudBlend(firstCloudy * (255 - blend) + secondCloudy * blend);
}

/**
 * Build the vertex and display list data for a skybox with 128x128 and 128x64 face textures.
 *
 * While the textures are nominally 128x128 (128x64) the 4x4 (4x2) tiles that cover it are only 31x31,
 * therefore only a 125x125 (125x63) area is ever sampled (125 = 4 * 31 + 1, the additional +1 accounts for bilinear
 * filtering)
 *
 * Each texture dimension is padded to the next power of 2, resulting in a final size of 128x128 (128x64)
 */
s32 Skybox_CalculateFace128(SkyboxContext* skyboxCtx, Vtx* roomVtx, s32 roomVtxStartIndex, s32 xStart, s32 yStart,
                            s32 zStart, s32 innerIncrVal, s32 outerIncrVal, s32 faceNum) {
    s32 i;
    s32 j;
    s32 k;
    s16 uls;
    s16 m;
    s32 outerIncr;
    u16 index;
    s16 ult;
    s16 l;
    s16 vtxIndex;
    s32 innerIncr;
    s32 xPoints[5 * 5];
    s32 yPoints[5 * 5];
    s32 zPoints[5 * 5];
    s32 tcS[5 * 5];
    s32 tcT[5 * 5];
    s32 pad;

    // Collect all vertex positions for this face
    switch (faceNum) {
        case 0: // xy plane
        case 1:
            outerIncr = yStart;

            for (i = 0, k = 0; k < 25; i++) {
                innerIncr = xStart;
                for (j = 0; j < 5; j++, k++) {
                    zPoints[k] = zStart;
                    xPoints[k] = innerIncr;
                    yPoints[k] = outerIncr;
                    tcS[k] = sSkybox128TexSCoords[j];
                    tcT[k] = sSkybox128TexTCoords[i];
                    innerIncr += innerIncrVal;
                }
                outerIncr += outerIncrVal;
            }
            break;

        case 2: // yz plane
        case 3:
            outerIncr = yStart;

            for (i = 0, k = 0; k < 25; i++) {
                innerIncr = zStart;
                for (j = 0; j < 5; j++, k++) {
                    xPoints[k] = xStart;
                    yPoints[k] = outerIncr;
                    zPoints[k] = innerIncr;
                    tcS[k] = sSkybox128TexSCoords[j];
                    tcT[k] = sSkybox128TexTCoords[i];
                    innerIncr += innerIncrVal;
                }
                outerIncr += outerIncrVal;
            }
            break;

        case 4: // xz plane
        case 5:
            outerIncr = zStart;

            for (i = 0, k = 0; k < 25; i++) {
                innerIncr = xStart;
                for (j = 0; j < 5; j++, k++) {
                    yPoints[k] = yStart;
                    xPoints[k] = innerIncr;
                    zPoints[k] = outerIncr;
                    tcS[k] = sSkybox128TexSCoords[j];
                    tcT[k] = sSkybox128TexTCoordsXZ[i];
                    innerIncr += innerIncrVal;
                }
                outerIncr += outerIncrVal;
            }
            break;

        default:
            break;
    }

    // Select gfx buffer
    skyboxCtx->gfx = &skyboxCtx->dListBuf[2 * faceNum][0];

    // Generate and load Vertex structures
    for (i = 0; i < ARRAY_COUNT(sSkybox128VtxBufIndices); i++) {
        index = sSkybox128VtxBufIndices[i];

        roomVtx[roomVtxStartIndex + i].v.ob[0] = xPoints[index];
        roomVtx[roomVtxStartIndex + i].v.ob[1] = yPoints[index];
        roomVtx[roomVtxStartIndex + i].v.ob[2] = zPoints[index];
        roomVtx[roomVtxStartIndex + i].v.flag = 0;
        roomVtx[roomVtxStartIndex + i].v.tc[0] = tcS[index];
        roomVtx[roomVtxStartIndex + i].v.tc[1] = tcT[index];
        roomVtx[roomVtxStartIndex + i].v.cn[1] = 0;
        roomVtx[roomVtxStartIndex + i].v.cn[2] = 0;
        roomVtx[roomVtxStartIndex + i].v.cn[0] = 255;
    }
    gSPVertex(skyboxCtx->gfx++, &roomVtx[roomVtxStartIndex], 32, 0);
    roomVtxStartIndex += i; // += 32

    // Cull the face if not within the viewing volume
    gSPCullDisplayList(skyboxCtx->gfx++, 0, 15);

    // Draw face, load the texture in several tiles to work around TMEM size limitations
    if ((faceNum == 4) || (faceNum == 5)) {
        // top/bottom faces, 128x128 texture
        ult = 0;
        for (vtxIndex = 0, l = 0; l < 4; l++, ult += 31) {
            for (uls = 0, m = 0; m < 4; m++, uls += 31, vtxIndex += 4) {
                gDPLoadMultiTile(skyboxCtx->gfx++,
                                 (uintptr_t)skyboxCtx->staticSegments[0][faceNum] /*+ sSkybox128TexOffsets[faceNum]*/,
                                 0, G_TX_RENDERTILE, G_IM_FMT_CI, G_IM_SIZ_8b, 128, 0, uls, ult, uls + 31, ult + 31, 0,
                                 G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP,
                                 G_TX_NOMASK, G_TX_NOLOD);
                gDPLoadMultiTile(skyboxCtx->gfx++,
                                 (uintptr_t)skyboxCtx->staticSegments[1][faceNum] /*+ sSkybox128TexOffsets[faceNum]*/,
                                 0x80, 1, G_IM_FMT_CI, G_IM_SIZ_8b, 128, 0, uls, ult, uls + 31, ult + 31, 0,
                                 G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP,
                                 G_TX_NOMASK, G_TX_NOLOD);
                gSP1Quadrangle(skyboxCtx->gfx++, sSkybox128VtxIndices[vtxIndex + 1], sSkybox128VtxIndices[vtxIndex + 2],
                               sSkybox128VtxIndices[vtxIndex + 3], sSkybox128VtxIndices[vtxIndex + 0], 3);
            }
        }
    } else {
        // other faces, 128x64 texture
        ult = 0;
        for (vtxIndex = 0, l = 0; l < 2; l++, ult += 31) {
            for (uls = 0, m = 0; m < 4; m++, uls += 31, vtxIndex += 4) {
                gDPLoadMultiTile(skyboxCtx->gfx++,
                                 (uintptr_t)skyboxCtx->staticSegments[0][faceNum] /*+ sSkybox128TexOffsets[faceNum]*/,
                                 0, G_TX_RENDERTILE, G_IM_FMT_CI, G_IM_SIZ_8b, 128, 0, uls, ult, uls + 31, ult + 31, 0,
                                 G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP,
                                 G_TX_NOMASK, G_TX_NOLOD);
                gDPLoadMultiTile(skyboxCtx->gfx++,
                                 (uintptr_t)skyboxCtx->staticSegments[1][faceNum] /*+ sSkybox128TexOffsets[faceNum]*/,
                                 0x80, 1, G_IM_FMT_CI, G_IM_SIZ_8b, 128, 0, uls, ult, uls + 31, ult + 31, 0,
                                 G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP,
                                 G_TX_NOMASK, G_TX_NOLOD);
                gSP1Quadrangle(skyboxCtx->gfx++, sSkybox128VtxIndices[vtxIndex + 1], sSkybox128VtxIndices[vtxIndex + 2],
                               sSkybox128VtxIndices[vtxIndex + 3], sSkybox128VtxIndices[vtxIndex + 0], 3);
            }
        }
        ult -= 31;
        for (l = 0; l < 2; l++, ult -= 31) {
            for (uls = 0, m = 0; m < 4; m++, uls += 31, vtxIndex += 4) {
                gDPLoadMultiTile(skyboxCtx->gfx++,
                                 (uintptr_t)skyboxCtx->staticSegments[0][faceNum] /*+ sSkybox128TexOffsets[faceNum]*/,
                                 0, G_TX_RENDERTILE, G_IM_FMT_CI, G_IM_SIZ_8b, 128, 0, uls, ult, uls + 31, ult + 31, 0,
                                 G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP,
                                 G_TX_NOMASK, G_TX_NOLOD);
                gDPLoadMultiTile(skyboxCtx->gfx++,
                                 (uintptr_t)skyboxCtx->staticSegments[1][faceNum] /*+ sSkybox128TexOffsets[faceNum]*/,
                                 0x80, 1, G_IM_FMT_CI, G_IM_SIZ_8b, 128, 0, uls, ult, uls + 31, ult + 31, 0,
                                 G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP,
                                 G_TX_NOMASK, G_TX_NOLOD);
                gSP1Quadrangle(skyboxCtx->gfx++, sSkybox128VtxIndices[vtxIndex + 1], sSkybox128VtxIndices[vtxIndex + 2],
                               sSkybox128VtxIndices[vtxIndex + 3], sSkybox128VtxIndices[vtxIndex + 0], 3);
            }
        }
    }

    gSPEndDisplayList(skyboxCtx->gfx++);
    return roomVtxStartIndex;
}

SkyboxFaceParams sSkybox128FaceParams[] = {
    { -64, 64, -64, 32, -32 }, { 64, 64, 64, -32, -32 }, { -64, 64, 64, -32, -32 },
    { 64, 64, -64, 32, -32 },  { -64, 64, 64, 32, -32 }, { -64, -64, -64, 32, 32 },
};

/**
 * Computes the display list for a skybox with up to 6 faces, where the sides are 128x64 CI8 textures and the
 * top/bottom faces are 128x128 CI8 textures.
 */
void Skybox_Calculate128(SkyboxContext* skyboxCtx, s32 nFaces) {
    s32 roomVtxStartIndex = 0;
    s32 faceNum;

    for (faceNum = 0; faceNum < nFaces; faceNum++) {
        roomVtxStartIndex = Skybox_CalculateFace128(
            skyboxCtx, skyboxCtx->roomVtx, roomVtxStartIndex, sSkybox128FaceParams[faceNum].xStart,
            sSkybox128FaceParams[faceNum].yStart, sSkybox128FaceParams[faceNum].zStart,
            sSkybox128FaceParams[faceNum].outerIncrVal, sSkybox128FaceParams[faceNum].innerIncrVal, faceNum);
    }
}

void Skybox_Setup(GameState* gameState, SkyboxContext* skyboxCtx, s16 skyboxId) {
    PlayState* play = (PlayState*)gameState;
    size_t size;
    void* segment;

    skyboxCtx->rot.z = 0.0f;

    switch (skyboxId) {
        case SKYBOX_NORMAL_SKY:
            // Send a DMA request for the cloudy sky texture
            // skyboxCtx->staticSegments[0] = &D_80025D00;
            // size = SEGMENT_ROM_SIZE(d2_cloud_static);
            // segment = (void*)ALIGN8((uintptr_t)skyboxCtx->staticSegments[0] + size);
            // DmaMgr_SendRequest0(skyboxCtx->staticSegments[0], SEGMENT_ROM_START(d2_cloud_static), size);

            // 2S2h [Port] Bypass DMA request and assign each skybox texture directly to the static segment
            for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[0]); i++) {
                skyboxCtx->staticSegments[0][i] = sSkyboxTextures[SKYBOX_TEXTURES_CLOUD][i];
            }

            // Send a DMA request for the clear sky texture
            // skyboxCtx->staticSegments[1] = segment;
            // size = SEGMENT_ROM_SIZE(d2_fine_static);
            // segment = (void*)ALIGN8((uintptr_t)segment + size);
            // DmaMgr_SendRequest0(skyboxCtx->staticSegments[1], SEGMENT_ROM_START(d2_fine_static), size);
            for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[1]); i++) {
                skyboxCtx->staticSegments[1][i] = sSkyboxTextures[SKYBOX_TEXTURES_FINE][i];
            }

            // Send a DMA request for the skybox palette
            // skyboxCtx->palette = segment;
            // size = SEGMENT_ROM_SIZE(d2_fine_pal_static);
            // segment = (void*)ALIGN8((uintptr_t)segment + size);
            // ResourceMgr_LoadTexOrDListByName(gSkyboxTLUT);
            // DmaMgr_RequestSync(skyboxCtx->palette, SEGMENT_ROM_START(d2_fine_pal_static), size);
            skyboxCtx->palette = gSkyboxTLUT;
            // #endregion

            skyboxCtx->prim.r = 145;
            skyboxCtx->prim.g = 120;
            skyboxCtx->prim.b = 155;

            skyboxCtx->env.r = 40;
            skyboxCtx->env.g = 0;
            skyboxCtx->env.b = 40;

            // Inverted Stone Tower Temple and Inverted Stone Tower
            if ((play->sceneId == SCENE_F41) || (play->sceneId == SCENE_INISIE_R)) {
                skyboxCtx->rot.z = 3.15f;
            }
            break;

        case SKYBOX_2:
            // #region 2S2H [Port] In the original code, this case does nothing
            // however because we changed skyboxCtx->staticSegments to be pointers to OTR strings
            // the draw calls in func_80142440 will crash because NULL being passed to the renderer.
            // Here we set the segments to point to the empty texture to prevent the crash
            for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[0]); i++) {
                skyboxCtx->staticSegments[0][i] = gEmptyTexture;
            }
            for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[1]); i++) {
                skyboxCtx->staticSegments[1][i] = gEmptyTexture;
            }
            skyboxCtx->palette = gEmptyTexture;
            // #endregion
            break;

        default:
            break;
    }
}

void Skybox_Reload(PlayState* play, SkyboxContext* skyboxCtx, s16 skyboxId) {
    size_t size;

    switch (skyboxId) {
        case SKYBOX_NORMAL_SKY:
            osCreateMesgQueue(&skyboxCtx->loadQueue, skyboxCtx->loadMsg, ARRAY_COUNT(skyboxCtx->loadMsg));

            if (play->envCtx.skybox1Index == 0) {
                // Send a DMA request for the clear sky texture
                // size = SEGMENT_ROM_SIZE(d2_fine_static);

                // DmaMgr_RequestAsync(&skyboxCtx->skybox1DmaRequest, skyboxCtx->staticSegments[0],
                //                     SEGMENT_ROM_START(d2_fine_static), size, 0, &skyboxCtx->loadQueue, NULL);

                // 2S2h [Port] Bypass DMA request and assign each skybox texture directly to the static segment
                for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[0]); i++) {
                    skyboxCtx->staticSegments[0][i] = sSkyboxTextures[SKYBOX_TEXTURES_FINE][i];
                }
            } else {
                // Send a DMA request for the cloudy sky texture
                // size = SEGMENT_ROM_SIZE(d2_cloud_static);

                // DmaMgr_RequestAsync(&skyboxCtx->skybox1DmaRequest, skyboxCtx->staticSegments[0],
                //                     SEGMENT_ROM_START(d2_cloud_static), size, 0, &skyboxCtx->loadQueue, NULL);

                for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[0]); i++) {
                    skyboxCtx->staticSegments[0][i] = sSkyboxTextures[SKYBOX_TEXTURES_CLOUD][i];
                }
            }

            osRecvMesg(&skyboxCtx->loadQueue, NULL, OS_MESG_BLOCK);
            osCreateMesgQueue(&skyboxCtx->loadQueue, skyboxCtx->loadMsg, ARRAY_COUNT(skyboxCtx->loadMsg));

            if (play->envCtx.skybox2Index == 0) {
                // Send a DMA request for the clear sky texture
                // size = SEGMENT_ROM_SIZE(d2_fine_static);

                // DmaMgr_RequestAsync(&skyboxCtx->skybox2DmaRequest, skyboxCtx->staticSegments[1],
                //                     SEGMENT_ROM_START(d2_fine_static), size, 0, &skyboxCtx->loadQueue, NULL);

                for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[1]); i++) {
                    skyboxCtx->staticSegments[1][i] = sSkyboxTextures[SKYBOX_TEXTURES_FINE][i];
                }
            } else {
                // Send a DMA request for the cloudy sky texture
                // size = SEGMENT_ROM_SIZE(d2_cloud_static);

                // DmaMgr_RequestAsync(&skyboxCtx->skybox2DmaRequest, skyboxCtx->staticSegments[1],
                //                     SEGMENT_ROM_START(d2_cloud_static), size, 0, &skyboxCtx->loadQueue, NULL);

                for (size_t i = 0; i < ARRAY_COUNTU(skyboxCtx->staticSegments[1]); i++) {
                    skyboxCtx->staticSegments[1][i] = sSkyboxTextures[SKYBOX_TEXTURES_CLOUD][i];
                }
            }

            osRecvMesg(&skyboxCtx->loadQueue, NULL, OS_MESG_BLOCK);
            osCreateMesgQueue(&skyboxCtx->loadQueue, skyboxCtx->loadMsg, ARRAY_COUNT(skyboxCtx->loadMsg));

            // size = SEGMENT_ROM_SIZE(d2_fine_pal_static);

            // Send a DMA request for the skybox palette
            // DmaMgr_RequestAsync(&skyboxCtx->paletteDmaRequest, skyboxCtx->palette,
            //                     SEGMENT_ROM_START(d2_fine_pal_static), size, 0, &skyboxCtx->loadQueue, NULL);

            osRecvMesg(&skyboxCtx->loadQueue, NULL, OS_MESG_BLOCK);

            skyboxCtx->palette = gSkyboxTLUT;

            // 2S2H [Port] Since the skybox static segments point to OTR strings, we need to re-create the skybox
            // display lists to have the new textures loaded
            Skybox_Calculate128(skyboxCtx, 5);
            // #endregion

            break;

        default:
            break;
    }
}

void Skybox_Init(GameState* gameState, SkyboxContext* skyboxCtx, s16 skyboxId) {
    skyboxCtx->shouldDraw = false;
    skyboxCtx->rot.x = skyboxCtx->rot.y = skyboxCtx->rot.z = 0.0f;
    skyboxCtx->ootSkyDLists[0] = skyboxCtx->ootSkyDLists[1] = NULL;
    skyboxCtx->ootSkyPhases[0] = skyboxCtx->ootSkyPhases[1] = 255;

    Skybox_Setup(gameState, skyboxCtx, skyboxId);

    if (skyboxId != SKYBOX_NONE) {
        skyboxCtx->dListBuf = THA_AllocTailAlign16(&gameState->tha, 12 * 150 * sizeof(Gfx));

        if (skyboxId == SKYBOX_CUTSCENE_MAP) {
            // Allocate enough space for the vertices for a 6 sided skybox (cube)
            skyboxCtx->roomVtx = THA_AllocTailAlign16(&gameState->tha, 6 * 32 * sizeof(Vtx));
            Skybox_Calculate128(skyboxCtx, 6);
        } else {
            // Allocate enough space for the vertices for a 5 sided skybox (bottom is missing)
            skyboxCtx->roomVtx = THA_AllocTailAlign16(&gameState->tha, 5 * 32 * sizeof(Vtx));
            Skybox_Calculate128(skyboxCtx, 5);
        }
        if (((skyboxId == SKYBOX_NORMAL_SKY) || (skyboxId == SKYBOX_3)) && Skybox_HasOotTextures()) {
            for (s32 weather = 0; weather < 2; weather++) {
                skyboxCtx->ootSkyDLists[weather] = THA_AllocTailAlign16(&gameState->tha, 12 * 150 * sizeof(Gfx));
            }
        }
    }
}
