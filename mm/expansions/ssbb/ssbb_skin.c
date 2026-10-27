#include "expansions/ssbb/ssbb_skin.h"
#include "expansions/ssbb/ssbb_anim.h"
#include "z64.h"
#include "functions.h"
#include "variables.h"
#include "z64malloc.h"
#include "mods/nei_oot_compat.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <string.h>

// ── Bone matrix storage ─────────────────────────────────────────────────────
static MtxF sBoneWorldMatrices[SSBB_MAX_SKIN_BONES];
static MtxF sCombinedMatrices[SSBB_MAX_SKIN_BONES];
static const SSBBCharacterInstance* sPoseOwner;

// ── Init / Destroy ──────────────────────────────────────────────────────────

void SSBBSkin_Init(SSBBSkinMesh* skin) {
    s32 size;

    if (!skin)
        return;

    // Skip if already initialized (shared skin mesh between companion + transform)
    if (skin->vtxBuf[0] != NULL && skin->vtxBuf[1] != NULL) {
        skin->bufIndex = 0;
        return;
    }

    size = skin->vertexCount * sizeof(Vtx);
    skin->vtxBuf[0] = ZELDA_ARENA_MALLOC_DEBUG(size);
    skin->vtxBuf[1] = ZELDA_ARENA_MALLOC_DEBUG(size);
    skin->bufIndex = 0;

    if (skin->vtxBuf[0] && skin->vtxBuf[1]) {
        s32 v;
        for (v = 0; v < skin->vertexCount; v++) {
            SSBBSkinVertex* sv = &skin->vertices[v];
            Vtx* vtx0 = &skin->vtxBuf[0][v];
            Vtx* vtx1 = &skin->vtxBuf[1][v];

            vtx0->n.ob[0] = vtx1->n.ob[0] = 0;
            vtx0->n.ob[1] = vtx1->n.ob[1] = 0;
            vtx0->n.ob[2] = vtx1->n.ob[2] = 0;
            vtx0->n.flag = vtx1->n.flag = 0;
            vtx0->n.tc[0] = vtx1->n.tc[0] = sv->texS;
            vtx0->n.tc[1] = vtx1->n.tc[1] = sv->texT;
            vtx0->n.n[0] = vtx1->n.n[0] = 0;
            vtx0->n.n[1] = vtx1->n.n[1] = 0;
            vtx0->n.n[2] = vtx1->n.n[2] = 0;
            vtx0->n.a = vtx1->n.a = sv->alpha;
        }
    }
}

void SSBBSkin_Destroy(SSBBSkinMesh* skin) {
    if (!skin)
        return;
    if (sPoseOwner && sPoseOwner->def && sPoseOwner->def->skinMesh == skin)
        sPoseOwner = NULL;
    if (skin->vtxBuf[0]) {
        ZELDA_ARENA_FREE_DEBUG(skin->vtxBuf[0]);
        skin->vtxBuf[0] = NULL;
    }
    if (skin->vtxBuf[1]) {
        ZELDA_ARENA_FREE_DEBUG(skin->vtxBuf[1]);
        skin->vtxBuf[1] = NULL;
    }
}

// ── Build local bone matrix: T(pos) × R(euler_ZYX) × S(scale) ──
// Pure math, NO OOT Matrix stack, NO FrameInterpolation hooks.
// Matches Three.js exactly: bone.matrix = T × R × S

static void SSBBSkin_BuildLocalMatrix(const SSBBBoneFrame* bf, MtxF* out) {
    f32 deg2rad = 3.14159265358979f / 180.0f;
    f32 rx = bf->rx * deg2rad;
    f32 ry = bf->ry * deg2rad;
    f32 rz = bf->rz * deg2rad;
    f32 cx = cosf(rx), sx = sinf(rx);
    f32 cy = cosf(ry), sy = sinf(ry);
    f32 cz = cosf(rz), sz = sinf(rz);
    s32 i;

    for (i = 0; i < 16; i++)
        ((f32*)out)[i] = 0.0f;

    // R(ZYX) × S — combined rotation+scale in column-major mf[col][row]
    out->mf[0][0] = cy * cz * bf->sx;
    out->mf[0][1] = cy * sz * bf->sx;
    out->mf[0][2] = -sy * bf->sx;

    out->mf[1][0] = (sx * sy * cz - cx * sz) * bf->sy;
    out->mf[1][1] = (sx * sy * sz + cx * cz) * bf->sy;
    out->mf[1][2] = sx * cy * bf->sy;

    out->mf[2][0] = (cx * sy * cz + sx * sz) * bf->sz;
    out->mf[2][1] = (cx * sy * sz - sx * cz) * bf->sz;
    out->mf[2][2] = cx * cy * bf->sz;

    // Translation in column 3
    out->mf[3][0] = bf->tx;
    out->mf[3][1] = bf->ty;
    out->mf[3][2] = bf->tz;
    out->mf[3][3] = 1.0f;
}

// ── Compute Bone World Matrices (NO OOT Matrix stack) ──
// Uses SkinMatrix_MtxFMtxFMult directly to avoid FrameInterpolation interference.
// Matches Three.js: bone.matrixWorld = parent.matrixWorld × bone.localMatrix

// Shortest-path lerp for a rotation component in degrees.
static f32 SSBBSkin_LerpAngle(f32 a, f32 b, f32 t) {
    f32 d = fmodf(b - a, 360.0f);
    if (d >= 180.0f)
        d -= 360.0f;
    else if (d < -180.0f)
        d += 360.0f;
    return a + d * t;
}

static void SSBBSkin_ComputeBoneMatricesFromAnim(void** skeleton, const struct SSBBAnim* anim, u16 frame, u16 nextFrame,
                                                 f32 blend, MtxF* parentWorld, u8 limbIdx, s32 numLimbs,
                                                 u8 neutralizeRootMotion) {
    StandardLimb* limb;
    const SSBBBoneFrame* bf;
    SSBBBoneFrame blended;
    MtxF localMat;

    if (limbIdx == LIMB_DONE || limbIdx >= numLimbs)
        return;

    limb = (StandardLimb*)skeleton[limbIdx];
    bf = SSBBAnim_GetBoneFrame(anim, frame, limbIdx);

    if (bf && blend > 0.0f && nextFrame != frame) {
        const SSBBBoneFrame* nf = SSBBAnim_GetBoneFrame(anim, nextFrame, limbIdx);
        if (nf) {
            blended.tx = bf->tx + (nf->tx - bf->tx) * blend;
            blended.ty = bf->ty + (nf->ty - bf->ty) * blend;
            blended.tz = bf->tz + (nf->tz - bf->tz) * blend;
            blended.rx = SSBBSkin_LerpAngle(bf->rx, nf->rx, blend);
            blended.ry = SSBBSkin_LerpAngle(bf->ry, nf->ry, blend);
            blended.rz = SSBBSkin_LerpAngle(bf->rz, nf->rz, blend);
            blended.sx = bf->sx + (nf->sx - bf->sx) * blend;
            blended.sy = bf->sy + (nf->sy - bf->sy) * blend;
            blended.sz = bf->sz + (nf->sz - bf->sz) * blend;
            bf = &blended;
        }
    }

    if (bf) {
        SSBBSkin_BuildLocalMatrix(bf, &localMat);

        // Neutralize root motion for movement bones (TopN=0, EyeYellowM=1, TransN=2)
        // These bones have animated translation that moves the character forward.
        // In OOT, movement is handled by Player.actor.world.pos — we only want rotation.
        // Keep bind-pose translation (from the animation frame's tx/ty/tz at frame 0).
        // For bone 2 (TransN): zero out X and Z translation, keep Y (height bobbing ok).
        if (neutralizeRootMotion && (limbIdx == 0 || limbIdx == 1)) {
            localMat.mf[3][0] = 0.0f;
            localMat.mf[3][1] = 0.0f;
            localMat.mf[3][2] = 0.0f;
        }
        if (neutralizeRootMotion && limbIdx == 2) {
            localMat.mf[3][0] = 0.0f; // No forward/back root motion
            localMat.mf[3][2] = 0.0f; // No left/right root motion
            // Keep Y (ty) for height — walk bobbing is ok
        }
    } else {
        // Identity if no animation data
        s32 i;
        for (i = 0; i < 16; i++)
            ((f32*)&localMat)[i] = 0.0f;
        localMat.mf[0][0] = localMat.mf[1][1] = localMat.mf[2][2] = localMat.mf[3][3] = 1.0f;
    }

    // boneWorld = parentWorld × localMatrix
    SkinMatrix_MtxFMtxFMult(parentWorld, &localMat, &sBoneWorldMatrices[limbIdx]);

    // Children inherit this bone's world matrix
    SSBBSkin_ComputeBoneMatricesFromAnim(skeleton, anim, frame, nextFrame, blend, &sBoneWorldMatrices[limbIdx],
                                         limb->child, numLimbs, neutralizeRootMotion);

    // Siblings inherit PARENT's world matrix
    SSBBSkin_ComputeBoneMatricesFromAnim(skeleton, anim, frame, nextFrame, blend, parentWorld, limb->sibling, numLimbs,
                                         neutralizeRootMotion);
}

// ── Blend Vertices ──────────────────────────────────────────────────────────

static s32 SSBBSkin_IsFinite(f32 value) {
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}

static s16 SSBBSkin_PackPosition(f32 value) {
    if (!SSBBSkin_IsFinite(value))
        return 0;
    if (value >= 32767.0f)
        return 32767;
    if (value <= -32768.0f)
        return -32768;
    return (s16)value;
}

static s8 SSBBSkin_PackNormal(f32 value) {
    if (!SSBBSkin_IsFinite(value))
        return 0;
    if (value >= 127.0f)
        return 127;
    if (value <= -127.0f)
        return -127;
    return (s8)value;
}

static void SSBBSkin_BlendVertices(SSBBSkinMesh* skin) {
    Vtx* vtxBuf;
    s32 v;
    s32 j;

    if (!skin->vtxBuf[0] || !skin->vtxBuf[1])
        return;

    vtxBuf = skin->vtxBuf[skin->bufIndex];

    for (v = 0; v < skin->vertexCount; v++) {
        SSBBSkinVertex* sv = &skin->vertices[v];
        SSBBSkinWeight* sw = &skin->weights[v];
        Vec3f restPos;
        Vec3f restNorm;
        Vec3f blendedPos;
        Vec3f blendedNorm;
        Vec3f transformedPos;
        Vec3f transformedNorm;
        f32 w;
        f32 len;
        f32 savedXW, savedYW, savedZW;

        restPos.x = sv->posX;
        restPos.y = sv->posY;
        restPos.z = sv->posZ;

        restNorm.x = sv->normX;
        restNorm.y = sv->normY;
        restNorm.z = sv->normZ;

        blendedPos.x = blendedPos.y = blendedPos.z = 0.0f;
        blendedNorm.x = blendedNorm.y = blendedNorm.z = 0.0f;

        for (j = 0; j < SSBB_MAX_INFLUENCES; j++) {
            if (sw->weight[j] == 0)
                break;

            w = sw->weight[j] * (1.0f / 255.0f);

            SkinMatrix_Vec3fMtxFMultXYZ(&sCombinedMatrices[sw->boneIndex[j]], &restPos, &transformedPos);
            blendedPos.x += transformedPos.x * w;
            blendedPos.y += transformedPos.y * w;
            blendedPos.z += transformedPos.z * w;

            savedXW = sCombinedMatrices[sw->boneIndex[j]].xw;
            savedYW = sCombinedMatrices[sw->boneIndex[j]].yw;
            savedZW = sCombinedMatrices[sw->boneIndex[j]].zw;
            sCombinedMatrices[sw->boneIndex[j]].xw = 0.0f;
            sCombinedMatrices[sw->boneIndex[j]].yw = 0.0f;
            sCombinedMatrices[sw->boneIndex[j]].zw = 0.0f;

            SkinMatrix_Vec3fMtxFMultXYZ(&sCombinedMatrices[sw->boneIndex[j]], &restNorm, &transformedNorm);

            sCombinedMatrices[sw->boneIndex[j]].xw = savedXW;
            sCombinedMatrices[sw->boneIndex[j]].yw = savedYW;
            sCombinedMatrices[sw->boneIndex[j]].zw = savedZW;

            blendedNorm.x += transformedNorm.x * w;
            blendedNorm.y += transformedNorm.y * w;
            blendedNorm.z += transformedNorm.z * w;
        }

        vtxBuf[v].n.ob[0] = SSBBSkin_PackPosition(blendedPos.x);
        vtxBuf[v].n.ob[1] = SSBBSkin_PackPosition(blendedPos.y);
        vtxBuf[v].n.ob[2] = SSBBSkin_PackPosition(blendedPos.z);

        len = sqrtf(blendedNorm.x * blendedNorm.x + blendedNorm.y * blendedNorm.y + blendedNorm.z * blendedNorm.z);
        if (SSBBSkin_IsFinite(len) && len > 0.001f) {
            vtxBuf[v].n.n[0] = SSBBSkin_PackNormal(blendedNorm.x / len * 127.0f);
            vtxBuf[v].n.n[1] = SSBBSkin_PackNormal(blendedNorm.y / len * 127.0f);
            vtxBuf[v].n.n[2] = SSBBSkin_PackNormal(blendedNorm.z / len * 127.0f);
        }
    }

    skin->bufIndex ^= 1;
}

// Current pose is owned by one instance; another character cannot supply Wolf's paw positions.
s32 SSBBSkin_GetBoneWorldPos(const SSBBCharacterInstance* inst, s32 boneIndex, Vec3f* out) {
    if (!out || !inst || sPoseOwner != inst || boneIndex < 0 || boneIndex >= inst->def->skinMesh->boneCount)
        return 0;
    out->x = sBoneWorldMatrices[boneIndex].xw;
    out->y = sBoneWorldMatrices[boneIndex].yw;
    out->z = sBoneWorldMatrices[boneIndex].zw;
    return 1;
}

s32 SSBBSkin_ComputePose(SSBBCharacterInstance* inst) {
    SSBBSkinMesh* skin;
    u16 frame, nextFrame;
    f32 blend = 0.0f;
    s32 b;
    sPoseOwner = NULL;
    if (!inst || !inst->initialized || !inst->def || !inst->def->skinMesh || !inst->skeleton || !inst->ssbbAnim ||
        !inst->ssbbAnim->numFrames || !inst->ssbbAnim->frames)
        return 0;
    skin = inst->def->skinMesh;
    if (!skin->invBindMatrices || !SSBBSkin_IsFinite(inst->curFrame))
        return 0;
    // Pikachu's existing 48-node skeleton has 47 weighted bones and one extra rigid
    // limb. Keep that layout: only weighted bones need animation/inverse-bind data.
    if (skin->boneCount == 0 || skin->boneCount > SSBB_MAX_SKIN_BONES || inst->def->numLimbs < skin->boneCount ||
        inst->def->numLimbs > SSBB_MAX_SKIN_BONES || inst->ssbbAnim->numBones < skin->boneCount ||
        inst->ssbbAnim->numBones > inst->def->numLimbs)
        return 0;
    frame = inst->curFrame < 0.0f                         ? 0
            : inst->curFrame >= inst->ssbbAnim->numFrames ? inst->ssbbAnim->numFrames - 1
                                                          : (u16)inst->curFrame;
    nextFrame = frame;
    if (skin->interpolateFrames && frame + 1 < inst->ssbbAnim->numFrames) {
        nextFrame = frame + 1;
        blend = CLAMP(inst->curFrame - (f32)frame, 0.0f, 1.0f);
    }
    SSBBSkin_ComputeBoneMatricesFromAnim(inst->skeleton, inst->ssbbAnim, frame, nextFrame, blend, &skin->daeToF64, 0,
                                         inst->def->numLimbs, !skin->preserveRootMotion);
    for (b = 0; b < skin->boneCount; ++b) {
        s32 component;
        SkinMatrix_MtxFMtxFMult(&sBoneWorldMatrices[b], &skin->invBindMatrices[b], &sCombinedMatrices[b]);
        for (component = 0; component < 16; ++component)
            if (!SSBBSkin_IsFinite(((f32*)&sBoneWorldMatrices[b])[component]) ||
                !SSBBSkin_IsFinite(((f32*)&sCombinedMatrices[b])[component]))
                return 0;
    }
    sPoseOwner = inst;
    return 1;
}

// ── Draw ────────────────────────────────────────────────────────────────────

static s32 SSBBSkin_ArenaHasSpace(const TwoHeadGfxArena* arena, size_t bytes) {
    uintptr_t head = (uintptr_t)arena->p;
    uintptr_t tail = (uintptr_t)arena->d;
    // Integer subtraction avoids undefined pointer arithmetic for null or
    // already-overlapping native heads; no arena is touched on rejection.
    return head && tail >= head && tail - head >= bytes;
}

s32 SSBBSkin_Draw(SSBBCharacterInstance* inst, PlayState* play, Vec3f* pos, Vec3s* rot) {
    return SSBBSkin_DrawWithReserve(inst, play, pos, rot, NULL);
}

s32 SSBBSkin_DrawWithReserve(SSBBCharacterInstance* inst, PlayState* play, Vec3f* pos, Vec3s* rot,
                             const SSBBSkinDrawReserve* reserve) {
    SSBBSkinMesh* skin;
    f32 s;
    s32 b;
    Mtx* worldMtx;

    if (!inst || !inst->initialized || !inst->def || !inst->def->skinMesh || !play || !play->state.gfxCtx || !pos ||
        !rot)
        return 0;

    skin = inst->def->skinMesh;
    if (!skin->vtxBuf[0] || !skin->vtxBuf[1])
        return 0;
    if (!inst->ssbbAnim)
        return 0;

    // Outer Open/CloseDisps plus the nested SetupDL25_Opa markers consume
    // 9 opaque packets (10 with material). Xlu/overlay temporarily need two
    // packets even though CloseDisps restores their heads. Vertices/DLs live
    // in persistent buffers, so the only tail allocation is one aligned Mtx.
    if (!SSBBSkin_ArenaHasSpace(&play->state.gfxCtx->polyOpa, (skin->materialDL ? 10 : 9) * sizeof(Gfx) +
                                                                  ALIGN16(sizeof(Mtx)) +
                                                                  (reserve ? reserve->opaBytes : 0)) ||
        !SSBBSkin_ArenaHasSpace(&play->state.gfxCtx->polyXlu, 2 * sizeof(Gfx) + (reserve ? reserve->xluBytes : 0)) ||
        !SSBBSkin_ArenaHasSpace(&play->state.gfxCtx->overlay, 2 * sizeof(Gfx) + (reserve ? reserve->overlayBytes : 0)))
        return 0;

    if (!SSBBSkin_ComputePose(inst))
        return 0;
    if (CVarGetInteger("gExpansions.SSBB.SkinDebug", 0) == 1) {
        MtxF identity;
        s32 i;
        sPoseOwner = NULL; // Bind-pose debug drawing has no sampled animation pose.
        for (i = 0; i < 16; ++i)
            ((f32*)&identity)[i] = 0.0f;
        identity.xx = identity.yy = identity.zz = identity.ww = 1.0f;
        for (b = 0; b < skin->boneCount; ++b)
            sCombinedMatrices[b] = identity;
    }

    // ── 3. Blend all vertices (CPU skinning) ──
    SSBBSkin_BlendVertices(skin);

    // ── 4. Draw ──
    OPEN_DISPS(play->state.gfxCtx);

    // Base RDP state (ensures consistent state regardless of what drew before)
    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0C, (uintptr_t)gCullBackDList);

    // Material DL: loads texture + sets combiner/geometry/render mode.
    // Must come AFTER Gfx_SetupDL25_Opa to override the default combiner.
    if (skin->materialDL) {
        gSPDisplayList(POLY_OPA_DISP++, skin->materialDL);
    }

    // World matrix: pos/rot × renderScale (vertices already in polygon0 space)
    Matrix_SetTranslateRotateYXZ(pos->x, pos->y, pos->z, rot);
    {
        f32 skinScale = CVarGetFloat("gExpansions.SSBB.SkinScale", 0.0f);
        s = (!skin->useDefinitionScale && skinScale > 0.001f) ? skinScale : inst->def->scale;
    }
    Matrix_Scale(s, s, s, MTXMODE_APPLY);

    worldMtx = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));
    MATRIX_TOMTX(worldMtx);
    gSPMatrix(POLY_OPA_DISP++, worldMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gSPSegment(POLY_OPA_DISP++, 0x08, (uintptr_t)skin->vtxBuf[skin->bufIndex ^ 1]);
    gSPDisplayList(POLY_OPA_DISP++, skin->displayList);

    CLOSE_DISPS(play->state.gfxCtx);
    return 1;
}
