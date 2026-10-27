#include "global.h"
#include "mods/transformation_masks/transformation_masks.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Engine types stay real; only the port's private state subset is represented. */
static struct {
    MmPlayerTransformation currentForm;
    int goronAction, actionTimer, jumpKickPhase, gerudoQuadsActive, gerudoQuadDamage;
    int state, swimState, fastSwimActive, swimExitFlag, swimYawRate, wasOnGround;
    int swimPhase, swimPhaseCounter, swimRollSmoothed, boomerangState, comboStep;
    s16 swimPitch, swimRoll;
    f32 swimSpeedB48;
    f32 zoraPunchCollisionFrame;
    u8 comboBPressed, comboBufferTimer;
    SkelAnime formSkelAnime;
    LinkAnimationHeader *idleAnim, *fishSwim, *waterRoll, *gerudoPowerJumpMid, *fallAnim, *jumpKick, *cutterAttack;
    u8 zoraMeleeActive, jumpKickActive, punchTrailActive, wallRecoilActive, zoraJumpTrailActive;
    u8 zoraSwimTrailActive[2], punchTrailActiveR;
    s32 zoraSwimTrailIndex[2], punchTrailEffectIndex, punchTrailEffectIndexR;
} gFormState;
static PlayState play;
static Player player;
static LinkAnimationHeader clip = {{14}, NULL}, fish = {{20}, NULL}, roll = {{25}, NULL};
static f32 recordedSpeed, recordedMorph;
static int changes, ticks, done, atCalls, deletes, effectAdds, vertices;
static int effectIndices[16], effectCursor, nextEffect = SPARK_COUNT, effectFailure;
static Vec3f matrixOffset;
static EffectBlure effects[512];
#define MMFORM_ON_GROUND(p) ((p)->actor.bgCheckFlags & 1)
#define ZORA_SWIM_THRESHOLD 20.0f
#define ZORA_DEEP_THRESHOLD 68.0f
#define MMFORM_STATE_ACTIVE 2
#define MMFORM_GRAVITY_SWIM 1
#define MMFORM_GRAVITY_NORMAL 0
#define GERUDO_SLASH_DAMAGE 2
#define GERUDO_FINISHER_DAMAGE 4
#define GORON_PUNCH_DAMAGE 2
#define ZORA_PUNCH_DAMAGE 1
#define MM_NA_SE_PL_ZORA_SWIM_DASH 0
#define MM_NA_SE_IT_GORON_PUNCH_SWING 0
#define MM_NA_SE_PL_GORON_PUNCH 0
static f32 MmForm_GetGravity(int mode) { return mode ? 0 : -1.2f; }
static void MmForm_EnterSwimIdle(Player* p, PlayState* c) { gFormState.goronAction = 0; }
static void MmForm_PlaySfx(Player* p, u16 id, u16 fallback) {}
static void MmForm_PlayAttackVoice(Player* p) {}
static void MmForm_StopRootMotion(void) {}
static void MmForm_ApplyRootMotion(Player* p) {}
static void MmForm_StartRootMotion(u8 step) {}
static LinkAnimationHeader* MmForm_GetPunchEndAnim(u8 step, u8 lock) { return &clip; }
static LinkAnimationHeader* MmForm_GetPunchAttackAnim(u8 step) { return &clip; }
static u8 MmForm_ZoraBoomerangHoldReady(PlayState* p) { return 0; }
static void Player_StartZoraBoomerang(Player* p, PlayState* c) {}
typedef int MmFormWallHitResult;
#define MMFORM_WALL_HIT_GORON 1
#define MMFORM_WALL_HIT_ZORA 2
static MmFormWallHitResult MmForm_CheckWallHit(Player* p, PlayState* c, u8 step, u8 goron, f32 frame, f32 start) { return 0; }
static void MmForm_DisablePunchQuad(Player* p) { p->meleeWeaponQuads[0].base.atFlags &= ~AT_ON; }
static void MmForm_EnablePunchQuad(Player* p, PlayState* c, u8 step, u8 damage, u32 flags) {}
void MmForm_KillTrail(PlayState* p, s32* index, u8* active) {
    if (*active) { Effect_Delete(p, *index); *active = 0; *index = -1; }
}
static const u8 sZoraPunchFrames[3][2] = {{2,5}, {3,8}, {3,10}};
static const u8 sGoronPunchFrames[3][2] = {{6,8}, {12,18}, {8,14}};
static const u8 sGerudoStationaryFrames[5][2] = {{2,5}};
void LinkAnimation_Change(PlayState* p, SkelAnime* sa, LinkAnimationHeader* anim,
                          f32 speed, f32 start, f32 end, u8 mode, f32 morph) {
    ++changes; recordedSpeed = speed; recordedMorph = morph;
    sa->animation = anim; sa->curFrame = start; sa->endFrame = end;
    sa->playSpeed = speed; sa->mode = mode;
}
s16 Animation_GetLastFrame(void* anim) { return ((LinkAnimationHeader*)anim)->common.frameCount - 1; }
s32 LinkAnimation_Update(PlayState* p, SkelAnime* sa) {
    ++ticks;
    if (!done) sa->curFrame += sa->playSpeed * 1.5f;
    return done;
}
f32 Math_CosS(s16 a) { return cosf(a * (3.14159265358979323846f / 32768)); }
f32 Math_SinS(s16 a) { return sinf(a * (3.14159265358979323846f / 32768)); }
s16 Math_Atan2S(f32 x, f32 y) { return 0; }
s32 Math_StepToF(f32* value, f32 target, f32 step) { *value=target; return 1; }
s16 Math_SmoothStepToS(s16* value, s16 target, s16 scale, s16 max, s16 min) { *value=target; return 0; }
void Audio_PlayActorSound2(Actor* p, u16 sfx) {}
void Player_PlaySfx(Actor* p, u16 sfx) {}
s32 Health_ChangeBy(PlayState* p, s16 amount) { return 0; }
void EffectSsHahen_SpawnBurst(PlayState* p, Vec3f* pos, f32 burstScale, s16 unused,
                             s16 scale, s16 scaleRange, s16 count, s16 object, s16 life, Gfx* dl) {}
void Math_Vec3f_Copy(Vec3f* to, Vec3f* from) { *to = *from; }
void Matrix_MultVec3f(Vec3f* from, Vec3f* to) {
    to->x = from->x * .01f + matrixOffset.x;
    to->y = from->y * .01f + matrixOffset.y;
    to->z = from->z * .01f + matrixOffset.z;
}
s32 Collider_ResetQuadAT(PlayState* p, Collider* collider) { collider->atFlags &= ~AT_HIT; return 0; }
void Collider_SetQuadVertices(ColliderQuad* q, Vec3f* a, Vec3f* b, Vec3f* c, Vec3f* d) {
    q->dim.quad[0] = *a; q->dim.quad[1] = *b; q->dim.quad[2] = *c; q->dim.quad[3] = *d;
}
s32 CollisionCheck_SetAT(PlayState* p, CollisionCheckContext* ctx, Collider* c) { ++atCalls; return 0; }
void Effect_Delete(PlayState* p, s32 index) { assert(index >= SPARK_COUNT && index < TOTAL_EFFECT_COUNT); ++deletes; }
void Effect_Add(PlayState* p, s32* index, s32 type, u8 a, u8 b, void* init) {
    *index = effectFailure ? TOTAL_EFFECT_COUNT : nextEffect++; ++effectAdds;
}
void* Effect_GetByIndex(s32 index) { assert(index >= SPARK_COUNT && index < TOTAL_EFFECT_COUNT); return &effects[index]; }
void EffectBlure_AddVertex(EffectBlure* effect, Vec3f* tip, Vec3f* base) {
    effectIndices[effectCursor++] = (int)(effect - effects); ++vertices;
}
void EffectBlure_AddSpace(EffectBlure* effect) {}
#include "native_melee.inc"
#include "zora_actions.inc"

static void reset(void) {
    memset(&gFormState, 0, sizeof(gFormState)); memset(&player, 0, sizeof(player));
    gFormState.currentForm = MM_PLAYER_FORM_ZORA; gFormState.state = MMFORM_STATE_ACTIVE;
    gFormState.idleAnim = &clip; gFormState.fishSwim = &fish; gFormState.waterRoll = &roll;
    gFormState.formSkelAnime.animation = &clip;
    changes=ticks=done=atCalls=deletes=effectAdds=vertices=effectCursor=0;
    matrixOffset = (Vec3f){0};
}
int main(int argc, char** argv) {
    assert(argc == 2); reset();
    if (!strcmp(argv[1], "timing")) {
        for (int action=GORON_ACT_PUNCH_A; action<=GORON_ACT_PUNCH_C; ++action) {
            MmForm_SetAction(action, &play, &clip, 1, ANIMMODE_ONCE);
            assert(fabsf(recordedSpeed - 2.0f/3.0f) < .00001f && "Zora attacks must advance one native MM frame per tick");
            assert(recordedMorph == 0 && "MM Zora punches start without an eight-frame morph");
        }
        MmForm_SetAction(GORON_ACT_PUNCH_END, &play, &clip, 1, ANIMMODE_ONCE);
        assert(recordedSpeed == 1 && recordedMorph == 0);
        MmForm_SetAction(MMFORM_ACT_JUMP_KICK, &play, &clip, 1, ANIMMODE_ONCE);
        assert(fabsf(recordedSpeed - 2.0f/3.0f) < .00001f && recordedMorph == 0);
        gFormState.currentForm = MM_PLAYER_FORM_GORON;
        MmForm_SetAction(GORON_ACT_PUNCH_A, &play, &clip, 1, ANIMMODE_ONCE);
        assert(recordedSpeed == 1 && recordedMorph == -8);
        gFormState.currentForm = MM_PLAYER_FORM_ZORA;
        MmForm_SetAction(MMFORM_ACT_SHIELD, &play, &clip, 1, ANIMMODE_ONCE);
        assert(recordedSpeed == 1 && recordedMorph == -8);
    } else if (!strcmp(argv[1], "dolphin")) {
        gFormState.fastSwimActive=1; gFormState.swimPitch=-0x3000; gFormState.swimSpeedB48=6;
        gFormState.formSkelAnime.animation=&roll; gFormState.formSkelAnime.curFrame=7;
        player.actor.yDistToWater=2; player.actor.velocity.y=4;
        assert(MmForm_CheckDolphinJump(&player, &play));
        assert(gFormState.goronAction == MMFORM_ACT_DOLPHIN_JUMP);
        assert(changes == 0 && gFormState.formSkelAnime.animation == &roll && gFormState.formSkelAnime.curFrame == 7 && "water exit must retain the unfinished roll pose");
        player.actor.yDistToWater=-1; player.actor.velocity.y=3;
        MmForm_Action_DolphinJump(&player, &play); assert(ticks == 1 && changes == 0);
        done=1; MmForm_Action_DolphinJump(&player, &play);
        assert(ticks==2 && changes==1 && gFormState.formSkelAnime.animation==&fish);
        assert(fabsf(recordedSpeed-2.0f/3.0f) < .00001f && recordedMorph==0);
    } else if (!strcmp(argv[1], "jump")) {
        gFormState.goronAction=MMFORM_ACT_JUMP_KICK; gFormState.formSkelAnime.animation=&clip;
        gFormState.formSkelAnime.curFrame=8;
        MmForm_Action_JumpKick(&player, &play);
#ifdef ZORA_COLLISION_FIX
        MmForm_UpdateZoraMelee(&player, &play);
#endif
        assert(gFormState.zoraMeleeActive && "actual jump action must enable the missing leg collision path");
    }
#ifdef ZORA_COLLISION_FIX
    else if (!strcmp(argv[1], "melee")) {
        gFormState.goronAction=GORON_ACT_PUNCH_C; gFormState.comboStep=2; gFormState.formSkelAnime.curFrame=3;
        gFormState.zoraPunchCollisionFrame=3;
        MmForm_UpdateZoraMelee(&player, &play);
        assert(MmForm_ZoraMeleeLimb() == PLAYER_LIMB_R_SHIN);
        MmForm_DrawZoraMelee(&play, &player, PLAYER_LIMB_R_FOREARM); assert(atCalls==0);
        MmForm_DrawZoraMelee(&play, &player, PLAYER_LIMB_R_SHIN); assert(atCalls==0);
        matrixOffset.x=10; MmForm_DrawZoraMelee(&play, &player, PLAYER_LIMB_R_SHIN);
        assert(atCalls==2 && player.meleeWeaponQuads[0].info.toucher.damage==1);
        assert(player.meleeWeaponQuads[0].info.toucher.dmgFlags==DMG_SLASH_MASTER);
        assert(player.meleeWeaponQuads[0].dim.quad[1].x > 20 && "kick geometry follows right shin");
        gFormState.formSkelAnime.curFrame=11; gFormState.zoraPunchCollisionFrame=11;
        MmForm_UpdateZoraMelee(&player, &play);
        assert(!gFormState.zoraMeleeActive && !(player.meleeWeaponQuads[0].base.atFlags & AT_ON) && !(player.meleeWeaponQuads[1].base.atFlags & AT_ON));
        gFormState.goronAction=MMFORM_ACT_JUMP_KICK; gFormState.formSkelAnime.curFrame=8;
        MmForm_UpdateZoraMelee(&player, &play); assert(gFormState.jumpKickActive);
        assert(player.meleeWeaponQuads[0].info.toucher.damage==2 && player.meleeWeaponQuads[1].info.toucher.dmgFlags==DMG_JUMP_MASTER);
        gFormState.goronAction=MMFORM_ACT_SWIM_IDLE; MmForm_UpdateZoraMelee(&player, &play);
        assert(!gFormState.jumpKickActive && !player.meleeWeaponInfo[1].active && !player.meleeWeaponInfo[2].active);
        gFormState.goronAction=GORON_ACT_PUNCH_A; gFormState.comboStep=0; gFormState.formSkelAnime.curFrame=2;
        gFormState.zoraPunchCollisionFrame=2;
        MmForm_UpdateZoraMelee(&player, &play); assert(gFormState.zoraMeleeActive);
        gFormState.state=0; MmForm_UpdateZoraMelee(&player, &play); assert(!gFormState.zoraMeleeActive);
    } else if (!strcmp(argv[1], "trails")) {
        gFormState.goronAction=MMFORM_ACT_SWIM_FAST; gFormState.fastSwimActive=1;
        MmForm_UpdateZoraSwimTrails(&play); assert(effectAdds==2);
        MmForm_DrawZoraSwimTrail(&play, 0); MmForm_DrawZoraSwimTrail(&play, 1);
        assert(vertices==2 && effectIndices[0]!=effectIndices[1] && atCalls==0);
        MmForm_UpdateZoraSwimTrails(&play); assert(effectAdds==2);
        gFormState.goronAction=MMFORM_ACT_DOLPHIN_JUMP; MmForm_UpdateZoraSwimTrails(&play);
        assert(deletes==2 && !gFormState.zoraSwimTrailActive[0] && !gFormState.zoraSwimTrailActive[1]);
        gFormState.goronAction=MMFORM_ACT_SWIM_FAST; MmForm_UpdateZoraSwimTrails(&play);
        gFormState.boomerangState=2; MmForm_UpdateZoraSwimTrails(&play); assert(deletes==4);
        gFormState.boomerangState=0; MmForm_UpdateZoraSwimTrails(&play);
        gFormState.state=0; MmForm_UpdateZoraSwimTrails(&play); assert(deletes==6);
        MmForm_ClearZoraSwimTrails(NULL); assert(deletes==6);
    } else if (!strcmp(argv[1], "ticks")) {
        assert(MmForm_ActionTicksAnimation(MMFORM_ACT_DOLPHIN_JUMP));
        assert(MmForm_ActionTicksAnimation(MMFORM_ACT_SWIM_FAST));
        assert(!MmForm_ActionTicksAnimation(GORON_ACT_PUNCH_A));
    } else if (!strcmp(argv[1], "landing-trail")) {
        gFormState.goronAction=MMFORM_ACT_JUMP_KICK; gFormState.formSkelAnime.curFrame=8;
        MmForm_UpdateZoraMelee(&player, &play); assert(gFormState.punchTrailActive);
        /* Actual centralized landing clears this before the late ownership update. */
        gFormState.jumpKickActive=0; gFormState.goronAction=GORON_ACT_LAND;
        MmForm_UpdateZoraMelee(&player, &play);
        assert(deletes==1 && !gFormState.punchTrailActive && "landing must release the owned jump trail after clearing the kick flag");
    } else if (!strcmp(argv[1], "trail-allocation")) {
        gFormState.goronAction=MMFORM_ACT_SWIM_FAST; gFormState.fastSwimActive=1;
        effectFailure=1; MmForm_UpdateZoraSwimTrails(&play);
        assert(!gFormState.zoraSwimTrailActive[0] && !gFormState.zoraSwimTrailActive[1] && "native Effect_Add failure sentinel must not latch an invalid trail");
        effectFailure=0; MmForm_UpdateZoraSwimTrails(&play); assert(effectAdds==4);
        assert(gFormState.zoraSwimTrailActive[0] && gFormState.zoraSwimTrailActive[1]);
    } else if (!strcmp(argv[1], "punch-order")) {
        MmForm_SetAction(GORON_ACT_PUNCH_A, &play, &clip, 1, ANIMMODE_ONCE);
        gFormState.formSkelAnime.curFrame=1;
        MmForm_Action_Punch(&player, &play); LinkAnimation_Update(&play, &gFormState.formSkelAnime);
        MmForm_UpdateZoraMelee(&player, &play);
        assert(gFormState.formSkelAnime.curFrame==2 && !gFormState.zoraMeleeActive && "native MM latches grounded damage before the animation tick");
        MmForm_Action_Punch(&player, &play); LinkAnimation_Update(&play, &gFormState.formSkelAnime);
        MmForm_UpdateZoraMelee(&player, &play);
        assert(gFormState.formSkelAnime.curFrame==3 && gFormState.zoraMeleeActive);
        gFormState.formSkelAnime.curFrame=5;
        MmForm_Action_Punch(&player, &play); LinkAnimation_Update(&play, &gFormState.formSkelAnime);
        MmForm_UpdateZoraMelee(&player, &play);
        assert(gFormState.formSkelAnime.curFrame==6 && gFormState.zoraMeleeActive);
        MmForm_Action_Punch(&player, &play); LinkAnimation_Update(&play, &gFormState.formSkelAnime);
        MmForm_UpdateZoraMelee(&player, &play);
        assert(gFormState.formSkelAnime.curFrame==7 && !gFormState.zoraMeleeActive);
    } else if (!strcmp(argv[1], "dolphin-landing-tick")) {
        gFormState.goronAction=MMFORM_ACT_DOLPHIN_JUMP; gFormState.fastSwimActive=1;
        player.actor.bgCheckFlags=1; player.actor.velocity.y=-2; player.actor.yDistToWater=-1;
        MmForm_Action_DolphinJump(&player, &play);
        assert(ticks==1 && gFormState.goronAction==GORON_ACT_IDLE);
        MmForm_TickActionAnimation(&play, MMFORM_ACT_DOLPHIN_JUMP);
        assert(ticks==1 && "self-ticking dolphin landing cannot tick the new idle a second time");
        gFormState.goronAction=MMFORM_ACT_SWIM_FAST;
        MmForm_TickActionAnimation(&play, GORON_ACT_IDLE); assert(ticks==1);
        gFormState.goronAction=GORON_ACT_IDLE;
        MmForm_TickActionAnimation(&play, GORON_ACT_IDLE); assert(ticks==2);
    }
#endif
    else assert(!"unknown regression case");
    printf("PASS OoT Zora %s\n", argv[1]); return 0;
}
