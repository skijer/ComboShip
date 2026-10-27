/* MM ice-arrow water-platform behavior, with unmodified native OoT iceberg assets. */
#include "z_bg_mm_icefloe.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "soh/Enhancements/Items/MmIceArrows.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <math.h>

static void BgMmIcefloe_StartMelting(BgMmIcefloe* this) {
    this->timer = 50;
    this->phase = MM_ICEFLOE_MELTING;
    Audio_PlayActorSound2(&this->dyna.actor, NA_SE_EV_ICE_MELT);
}

s32 MmIceArrows_TryFreezeWater(PlayState* play, EnArrow* arrow) {
    Actor* actor = &arrow->actor;
    WaterBox* waterBox;
    Vec3f hit = actor->world.pos;
    f32 surface = hit.y;
    f32 fraction;
    s32 actorId;
    Actor* platform;

    if ((actor->params != ARROW_ICE) || !CVarGetInteger(CVAR_ENHANCEMENT("MmIceArrows"), 0) ||
        (actor->bgCheckFlags & BGCHECKFLAG_WATER) ||
        !WaterBox_GetSurface1(play, &play->colCtx, hit.x, hit.z, &surface, &waterBox) || (hit.y >= surface)) {
        return false;
    }
    actor->bgCheckFlags |= BGCHECKFLAG_WATER;

    // Use the flight segment retained by EnArrow_Fly. Submerged/horizontal shots have no
    // surface crossing; do not extrapolate outside the segment or divide by zero.
    if ((arrow->unk_210.y < surface) || (arrow->unk_210.y <= hit.y)) {
        return false;
    }
    fraction = (surface - arrow->unk_210.y) / (hit.y - arrow->unk_210.y);
    hit.x = arrow->unk_210.x + (hit.x - arrow->unk_210.x) * fraction;
    hit.y = surface;
    hit.z = arrow->unk_210.z + (hit.z - arrow->unk_210.z) * fraction;
    // A segment can enter a water box from its side; its surface intersection must
    // belong to that same water box rather than spawning over a bank.
    if ((hit.x <= waterBox->xMin) || (hit.x >= waterBox->xMin + waterBox->xLength) || (hit.z <= waterBox->zMin) ||
        (hit.z >= waterBox->zMin + waterBox->zLength)) {
        return false;
    }
    actorId = MmIceArrows_GetActorId();
    if (actorId < 0) {
        return false;
    }
    platform = Actor_Spawn(&play->actorCtx, play, actorId, hit.x, hit.y, hit.z, 0, 0, 0, 300);
    if ((platform == NULL) || (platform->update == NULL)) {
        return false;
    }
    EffectSsGSplash_Spawn(play, &hit, NULL, NULL, 0, 300);
    EffectSsGRipple_Spawn(play, &hit, 100, 500, 0);
    EffectSsGRipple_Spawn(play, &hit, 100, 500, 4);
    EffectSsGRipple_Spawn(play, &hit, 100, 500, 8);
    if (actor->child != NULL) {
        Actor_Kill(actor->child);
    }
    Actor_Kill(actor);
    return true;
}

void BgMmIcefloe_Init(Actor* thisx, PlayState* play) {
    BgMmIcefloe* this = (BgMmIcefloe*)thisx;
    CollisionHeader* collision = NULL;
    Actor* actor;
    BgMmIcefloe* oldest = NULL;
    s32 count = 0;
    f32 extent;

    this->dyna.bgId = BG_ACTOR_MAX;
    this->resources = MmIceArrows_AcquireResources(&collision, &this->model);
    if ((this->resources == NULL) || (collision == NULL) || (this->model == NULL)) {
        Actor_Kill(thisx);
        return;
    }
    extent = MAX(collision->maxBounds.x - collision->minBounds.x, collision->maxBounds.z - collision->minBounds.z);
    if ((extent <= 0.0f) || (collision->numVertices == 0) || (collision->numPolygons == 0)) {
        Actor_Kill(thisx);
        return;
    }
    // Native MM bounds at scale .13 are 83.2 x 72.02. The native OoT iceberg
    // is 2946 x 3392 model units; uniform normalization keeps it a mini-floe.
    this->fullScale = 83.2f / extent;
    this->meshTop = collision->maxBounds.y;
    this->surfaceOffset = 10.0f;
    Actor_SetScale(thisx, 0.0f);
    thisx->world.pos.y = thisx->home.pos.y + this->surfaceOffset;
    DynaPolyActor_Init(&this->dyna, DPM_UNK);
    this->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, thisx, collision);
    if (this->dyna.bgId == BG_ACTOR_MAX) {
        Actor_Kill(thisx);
        return;
    }
    this->timer = 20;
    this->phase = MM_ICEFLOE_GROWING;
    // Actor_AddToCategory inserts at the head. Traverse the live scene list,
    // retaining no global pointers across destruction, room changes or reloads.
    for (actor = play->actorCtx.actorLists[ACTORCAT_BG].head; actor != NULL; actor = actor->next) {
        if ((actor != thisx) && (actor->id == thisx->id) && (actor->update != NULL) &&
            (((BgMmIcefloe*)actor)->phase != MM_ICEFLOE_MELTING)) {
            ++count;
            oldest = (BgMmIcefloe*)actor;
        }
    }
    if ((count >= 3) && (oldest != NULL)) {
        BgMmIcefloe_StartMelting(oldest);
    }
    Audio_PlayActorSound2(thisx, NA_SE_IT_EXPLOSION_ICE);
}

void BgMmIcefloe_Destroy(Actor* thisx, PlayState* play) {
    BgMmIcefloe* this = (BgMmIcefloe*)thisx;
    if (this->dyna.bgId != BG_ACTOR_MAX) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
        this->dyna.bgId = BG_ACTOR_MAX;
    }
    MmIceArrows_ReleaseResources(this->resources);
    this->resources = NULL;
    this->model = NULL;
}

void BgMmIcefloe_Update(Actor* thisx, PlayState* play) {
    BgMmIcefloe* this = (BgMmIcefloe*)thisx;
    WaterBox* waterBox;
    if (!CVarGetInteger(CVAR_ENHANCEMENT("MmIceArrows"), 0)) {
        Actor_Kill(thisx);
        return;
    }
    if (Play_InCsMode(play)) {
        return;
    }
    if (this->phase == MM_ICEFLOE_GROWING) {
        Actor_SetScale(thisx, thisx->scale.x + this->fullScale / 20.0f);
        if (--this->timer == 0) {
            Actor_SetScale(thisx, this->fullScale);
            this->timer = 300;
            this->phase = MM_ICEFLOE_FLOATING;
        }
    } else if (this->phase == MM_ICEFLOE_FLOATING) {
        if ((--this->timer == 0) || !WaterBox_GetSurface1(play, &play->colCtx, thisx->world.pos.x, thisx->world.pos.z,
                                                          &thisx->home.pos.y, &waterBox)) {
            BgMmIcefloe_StartMelting(this);
        } else {
            this->surfaceOffset = sinf(this->timer * (M_PI / 30.0f)) * 3.0f + 10.0f;
        }
    } else {
        --this->timer;
        if ((this->timer >= 38) && !(this->timer % 2)) {
            Vec3f velocity = { 0.0f, ((this->timer - 38) / 12.0f) + 0.8f, 0.0f };
            Vec3f position = thisx->world.pos;
            Vec3f accel = { 0.0f, 0.0f, 0.0f };
            position.y += this->meshTop * thisx->scale.y + 3.0f;
            EffectSsIceSmoke_Spawn(play, &position, &velocity, &accel, 200);
        }
        if (this->timer < 25) {
            thisx->scale.x = MAX(0.0f, thisx->scale.x - this->fullScale * 0.04f);
            thisx->scale.z = MAX(0.0f, thisx->scale.z - this->fullScale * 0.04f);
        }
        thisx->scale.y = MAX(0.0f, thisx->scale.y - this->fullScale * 0.02f);
        if ((this->timer <= 0) || (thisx->scale.y <= 0.0f)) {
            Actor_Kill(thisx);
        }
    }
    // OoT's native iceberg top is +150; MM's is zero. Keep both visible and
    // physical tops at the same water-relative height throughout growth/melt.
    thisx->world.pos.y = thisx->home.pos.y + this->surfaceOffset - this->meshTop * thisx->scale.y;
}

void BgMmIcefloe_Draw(Actor* thisx, PlayState* play) {
    BgMmIcefloe* this = (BgMmIcefloe*)thisx;
    if (this->model != NULL) {
        Gfx_DrawDListOpa(play, this->model);
    }
}
