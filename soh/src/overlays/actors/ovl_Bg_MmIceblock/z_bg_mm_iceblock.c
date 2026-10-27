/* Temporary Octorok ice block; native OoT geometry with MM's nonlethal frozen-parent contract. */
#include "z_bg_mm_iceblock.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_En_Okuta/z_en_okuta.h"
#include "soh/Enhancements/Items/MmIceArrows.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <math.h>

static ColliderCylinderInit sCylinderInit = {
    { COLTYPE_NONE, AT_NONE, AC_ON | AC_TYPE_PLAYER, OC1_NONE, OC2_TYPE_2, COLSHAPE_CYLINDER },
    { ELEMTYPE_UNK0, { 0, 0, 0 }, { DMG_ARROW_FIRE, 0, 0 }, TOUCH_NONE, BUMP_ON, OCELEM_NONE },
    { 44, 60, 0, { 0, 0, 0 } },
};

s32 MmIceArrows_TryFreezeOctorok(PlayState* play, EnOkuta* octorok) {
    Actor* parent = &octorok->actor;
    ColliderCylinder* collider = &octorok->collider;
    Actor* attacker = collider->base.ac;
    Actor* block;
    s32 actorId;
    f32 centerY;

    if (!CVarGetInteger(CVAR_ENHANCEMENT("MmIceArrows"), 0) || (parent->params != 0) ||
        (parent->colChkInfo.health == 0) || !(collider->base.acFlags & AC_ON) || (parent->child != NULL) ||
        (attacker == NULL) || (attacker->id != ACTOR_EN_ARROW) || (attacker->params != ARROW_ICE) ||
        (collider->info.acHitInfo == NULL) || !(collider->info.acHitInfo->toucher.dmgFlags & DMG_ARROW_ICE)) {
        return false;
    }
    actorId = MmIceArrows_GetOctorokBlockActorId();
    if (actorId < 0) {
        return false;
    }
    // MM centers its 60-unit block on the Octorok's frozen head. OoT's native
    // cube starts at y=0 instead of -30, so spawn its base 30 below that center.
    centerY = parent->world.pos.y + octorok->headScale.y * 25.0f;
    if (octorok->skelAnime.jointTable != NULL) {
        centerY += octorok->skelAnime.jointTable[0].y * parent->scale.y;
    }
    block = Actor_SpawnAsChild(&play->actorCtx, parent, play, actorId, parent->world.pos.x, centerY - 30.0f,
                               parent->world.pos.z, 0, parent->home.rot.y, 0, 0);
    if (block == NULL) {
        return false;
    }
    // Preserve the native parent-link hook. A vetoed link or failed actor init
    // must fall through to the original OoT hit/death reaction without an orphan.
    if ((block->update == NULL) || (block->parent != parent) || (parent->child != block)) {
        if (parent->child == block) {
            parent->child = NULL;
        }
        if (block->parent == parent) {
            block->parent = NULL;
        }
        Actor_Kill(block);
        return false;
    }
    ((BgMmIceblock*)block)->waterHeight = parent->home.pos.y;
    return true;
}

void BgMmIceblock_Init(Actor* thisx, PlayState* play) {
    BgMmIceblock* this = (BgMmIceblock*)thisx;
    CollisionHeader* collision = NULL;
    f32 width;
    f32 height;

    this->dyna.bgId = BG_ACTOR_MAX;
    this->resources = MmIceArrows_AcquireBlockResources(&collision, &this->model);
    if ((this->resources == NULL) || (collision == NULL) || (this->model == NULL)) {
        Actor_Kill(thisx);
        return;
    }
    width = MAX(collision->maxBounds.x - collision->minBounds.x, collision->maxBounds.z - collision->minBounds.z);
    height = collision->maxBounds.y - collision->minBounds.y;
    if ((width <= 0.0f) || (height <= 0.0f) || !collision->numVertices || !collision->numPolygons) {
        Actor_Kill(thisx);
        return;
    }
    this->fullScale = (Vec3f){ 60.0f / width, 60.0f / height, 60.0f / width };
    this->meshBottom = collision->minBounds.y;
    Actor_SetScale(thisx, 0.0f);
    DynaPolyActor_Init(&this->dyna, DPM_UNK);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, thisx, collision);
    if (this->dyna.bgId == BG_ACTOR_MAX) {
        Actor_Kill(thisx);
        return;
    }
    func_8003EBF8(play, &play->colCtx.dyna, this->dyna.bgId);
    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, thisx, &sCylinderInit);
    this->colliderInitialized = true;
    this->growTimer = 20;
    this->lifetime = 450;
    this->phase = MM_ICEBLOCK_GROWING;
    Audio_PlayActorSound2(thisx, NA_SE_IT_EXPLOSION_ICE);
}

void BgMmIceblock_Destroy(Actor* thisx, PlayState* play) {
    BgMmIceblock* this = (BgMmIceblock*)thisx;
    if ((thisx->parent != NULL) && (thisx->parent->child == thisx)) {
        thisx->parent->child = NULL;
    }
    thisx->parent = NULL;
    if (this->dyna.bgId != BG_ACTOR_MAX) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
        this->dyna.bgId = BG_ACTOR_MAX;
    }
    if (this->colliderInitialized) {
        Collider_DestroyCylinder(play, &this->collider);
        this->colliderInitialized = false;
    }
    MmIceArrows_ReleaseResources(this->resources);
    this->resources = NULL;
    this->model = NULL;
}

static void BgMmIceblock_StartMelting(BgMmIceblock* this) {
    this->phase = MM_ICEBLOCK_MELTING;
    this->collider.base.acFlags &= ~(AC_ON | AC_HIT);
    Audio_PlayActorSound2(&this->dyna.actor, NA_SE_EV_ICE_MELT);
}

void BgMmIceblock_Update(Actor* thisx, PlayState* play) {
    BgMmIceblock* this = (BgMmIceblock*)thisx;
    Actor* parent = thisx->parent;
    f32 horizontal;
    f32 waterDelta;

    if ((parent == NULL) || (parent->update == NULL) || (parent->id != ACTOR_EN_OKUTA) || (parent->params != 0) ||
        (parent->child != thisx) || !CVarGetInteger(CVAR_ENHANCEMENT("MmIceArrows"), 0)) {
        Actor_Kill(thisx);
        return;
    }
    if (Play_InCsMode(play)) {
        return;
    }
    waterDelta = parent->home.pos.y - this->waterHeight;
    thisx->home.pos.y += waterDelta;
    parent->world.pos.y += waterDelta;
    this->waterHeight = parent->home.pos.y;
    if ((this->phase != MM_ICEBLOCK_MELTING) && (this->collider.base.acFlags & AC_HIT) &&
        (this->collider.info.acHitInfo != NULL) && (this->collider.info.acHitInfo->toucher.dmgFlags & DMG_ARROW_FIRE)) {
        BgMmIceblock_StartMelting(this);
    }
    this->collider.base.acFlags &= ~AC_HIT;
    if (this->phase == MM_ICEBLOCK_GROWING) {
        this->progress = 1.0f - (--this->growTimer / 20.0f);
        if (this->growTimer == 0) {
            this->phase = MM_ICEBLOCK_SOLID;
            func_8003EC50(play, &play->colCtx.dyna, this->dyna.bgId);
        }
    } else if (this->phase == MM_ICEBLOCK_SOLID) {
        if (--this->lifetime == 0) {
            BgMmIceblock_StartMelting(this);
        }
    } else {
        // Obj_Iceblock's accelerating vertical melt, normalized from native .1 scale.
        this->meltSpeed = MIN(this->meltSpeed + (1.0f / 600.0f), 1.0f / 60.0f);
        this->progress = MAX(0.0f, this->progress - this->meltSpeed);
        if (this->progress == 0.0f) {
            Actor_Kill(thisx);
        }
    }
    horizontal = this->progress;
    if (this->phase == MM_ICEBLOCK_MELTING) {
        horizontal = this->progress >= 0.75f ? 1.0f : sinf(this->progress * (2.0f * M_PI / 3.0f));
    }
    thisx->scale =
        (Vec3f){ this->fullScale.x * horizontal, this->fullScale.y * this->progress, this->fullScale.z * horizontal };
    thisx->world.pos.y = thisx->home.pos.y - this->meshBottom * thisx->scale.y;
    // Match MM's cube radius: its circumscribed fire-arrow cylinder includes all four corners.
    this->collider.dim.radius = MAX(1, (s16)(44.0f * horizontal));
    this->collider.dim.height = MAX(1, (s16)(60.0f * this->progress));
    Collider_UpdateCylinder(thisx, &this->collider);
    if ((this->phase != MM_ICEBLOCK_MELTING) && (thisx->update != NULL)) {
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
    }
}

void BgMmIceblock_Draw(Actor* thisx, PlayState* play) {
    BgMmIceblock* this = (BgMmIceblock*)thisx;
    if (this->model != NULL) {
        Gfx_DrawDListOpa(play, this->model);
    }
}
