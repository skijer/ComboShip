#include "din_fire_sword.h"
#include "global.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"
#include "overlays/actors/ovl_Obj_Bombiwa/z_obj_bombiwa.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Native DMG_FIXED_DAMAGE uses a signed 1<<31. Preserve its intended bit value
 * here so C UBSan can check ApplyDamage without stopping on that existing
 * macro. */
#undef DMG_FIXED_DAMAGE
#define DMG_FIXED_DAMAGE (1u << 31)

static struct {
    MmPlayerTransformation currentForm;
    s32 goronAction, actionTimer, comboStep, boomerangState;
    u8 comboBPressed, comboBufferTimer, wallRecoilActive, punchTrailActive, punchTrailActiveR;
    s32 punchTrailEffectIndex, punchTrailEffectIndexR;
    SkelAnime formSkelAnime;
    LinkAnimationHeader *idleAnim, *cutterAttack;
    f32 savedShadowScale, zoraPunchCollisionFrame;
} gFormState;
static u8 sRollOwnsPause;
static PlayState play;
static Player player;
static ObjBombiwa boulder;
static DoorAna grotto;
static LinkAnimationHeader idle;
static s32 textState, boulderBreaks;
static s32 finishAcceptedCalls;
static const void* finishReceiver;
static const ColliderInfo* finishHit;
static uint32_t finishFlags, finishDamage;

#define MM_FORM_LIMB_COUNT 22
#define MMFORM_GRAVITY_NORMAL 0
#define MMFORM_ON_GROUND(p) ((p)->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
#define GORON_PUNCH_DAMAGE 2
#define ZORA_PUNCH_DAMAGE 1
#define GERUDO_SLASH_DAMAGE 2
#define GERUDO_FINISHER_DAMAGE 4
#define MM_NA_SE_IT_GORON_PUNCH_SWING 0
#define MM_NA_SE_PL_GORON_PUNCH 0
typedef int MmFormWallHitResult;
enum { MMFORM_WALL_HIT_NONE, MMFORM_WALL_HIT_GORON, MMFORM_WALL_HIT_ZORA };
enum { MASSTYPE_IMMOVABLE, MASSTYPE_HEAVY, MASSTYPE_NORMAL };

/* Only external services are controlled. Collision, geometry and actor
 * decisions below execute the original production bodies on the real game
 * structures. */
static void MmForm_StopGoronRollSfx(void) {
}
static u8 NEI_PlayerDamageBoostActive(void) {
    return 0;
}
uint8_t DinFireSword_DamageEntry(PlayState* state, Actor* target, const ColliderInfo* hit, uint32_t flags,
                                 uint8_t entry) {
    return entry;
}
/* Elemental presentation is an observer boundary, not collision policy. */
void DinFireSword_FinishAcceptedHit(PlayState* state, const void* receiver, const ColliderInfo* hit,
                                   uint32_t flags, uint32_t damage) {
    assert(state == &play);
    ++finishAcceptedCalls;
    finishReceiver = receiver;
    finishHit = hit;
    finishFlags = flags;
    finishDamage = damage;
}
static f32 MmForm_GetGravity(s32 mode) {
    return -1.2f;
}
static void actor_update(Actor* actor, PlayState* state) {
}
static MmFormWallHitResult MmForm_CheckWallHit(Player* p, PlayState* state, u8 step, u8 goron, f32 frame, f32 start) {
    return MMFORM_WALL_HIT_NONE;
}
static void MmForm_StopRootMotion(void) {
}
static void MmForm_ApplyRootMotion(Player* p) {
}
static void MmForm_StartRootMotion(u8 step) {
}
static LinkAnimationHeader* MmForm_GetPunchEndAnim(u8 step, u8 lock) {
    return &idle;
}
static LinkAnimationHeader* MmForm_GetPunchAttackAnim(u8 step) {
    return &idle;
}
static u8 MmForm_ZoraBoomerangHoldReady(PlayState* p) {
    return 0;
}
static void Player_StartZoraBoomerang(Player* p, PlayState* state) {
}
static void MmForm_PlayAttackVoice(Player* p) {
}
static void MmForm_PlaySfx(Player* p, u16 mmSfx, u16 ootSfx) {
}
void MmForm_KillTrail(PlayState* state, s32* index, u8* active) {
    *active = 0;
}
void Player_PlaySfx(Actor* actor, u16 sfx) {
}
void EffectSsHahen_SpawnBurst(PlayState* state, Vec3f* pos, f32 burstScale, s16 unused, s16 scale, s16 scaleRange,
                              s16 count, s16 object, s16 life, Gfx* dl) {
}
s32 Math_StepToF(f32* value, f32 target, f32 step) {
    *value = target;
    return 1;
}
void LinkAnimation_Change(PlayState* state, SkelAnime* skel, LinkAnimationHeader* anim, f32 speed, f32 start, f32 end,
                          u8 mode, f32 morph) {
    skel->animation = anim;
    skel->playSpeed = speed;
    skel->curFrame = start;
    skel->endFrame = end;
    skel->mode = mode;
}
s16 Animation_GetLastFrame(void* animation) {
    return ((LinkAnimationHeader*)animation)->common.frameCount - 1;
}
s32 LinkAnimation_Update(PlayState* state, SkelAnime* skel) {
    return 0;
}
static void MmForm_UpdateBlink(void) {
}
u8 Message_GetState(MessageContext* message) {
    return textState;
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* fmt, ...) {
}
f32 Math_SinS(s16 angle) {
    return sinf(angle * (3.14159265358979323846f / 32768));
}
f32 Math_CosS(s16 angle) {
    return cosf(angle * (3.14159265358979323846f / 32768));
}
void CollisionCheck_HitEffects(PlayState* state, Collider* at, ColliderInfo* atInfo, Collider* ac, ColliderInfo* acInfo,
                               Vec3f* pos) {
}
void ObjBombiwa_Break(ObjBombiwa* actor, PlayState* state) {
    ++boulderBreaks;
}
Actor* func_80033684(PlayState* state, Actor* actor) {
    return NULL;
}
void Flags_SetSwitch(PlayState* state, s32 flag) {
    state->actorCtx.flags.swch |= 1u << flag;
}
s32 Flags_GetEnv(PlayState* state, s16 flag) {
    return 0;
}
void Actor_Kill(Actor* actor) {
    actor->update = NULL;
}
void Actor_SetScale(Actor* actor, f32 scale) {
    actor->scale = (Vec3f){ scale, scale, scale };
}
void Actor_SetClosestSecretDistance(Actor* actor, PlayState* state) {
}
void DoorAna_SetupAction(DoorAna* actor, DoorAnaActionFunc action) {
    actor->actionFunc = action;
}
void DoorAna_WaitOpen(DoorAna* actor, PlayState* state) {
}
s32 Collider_DestroyCylinder(PlayState* state, ColliderCylinder* cylinder) {
    return 1;
}
void SoundSource_PlaySfxAtFixedWorldPos(PlayState* state, Vec3f* pos, s32 duration, u16 sfx) {
}
void Sfx_PlaySfxCentered(u16 sfx) {
}
void Audio_PlaySfxGeneral(u16 sfx, Vec3f* pos, u8 token, f32* frequency, f32* volume, s8* reverb) {
}
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) {
    return result;
}
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;

#include "native_collision.inc"

/* The registration boundaries retain the live colliders, as the frame context
 * does in the game. Intersection and response are native production code. */
s32 CollisionCheck_SetAT(PlayState* state, CollisionCheckContext* ctx, Collider* collider) {
    if (collider->shape == COLSHAPE_QUAD)
        Collider_ResetQuadAT(state, collider);
    else
        Collider_ResetCylinderAT(state, collider);
    ctx->colAT[ctx->colATCount++] = collider;
    return ctx->colATCount - 1;
}
s32 CollisionCheck_SetAC(PlayState* state, CollisionCheckContext* ctx, Collider* collider) {
    Collider_ResetCylinderAC(state, collider);
    ctx->colAC[ctx->colACCount++] = collider;
    return ctx->colACCount - 1;
}
s32 CollisionCheck_SetOC(PlayState* state, CollisionCheckContext* ctx, Collider* collider) {
    Collider_ResetCylinderOC(state, collider);
    ctx->colOC[ctx->colOCCount++] = collider;
    return ctx->colOCCount - 1;
}

#include "actors.inc"
#include "form_collision.inc"

static void reset(MmPlayerTransformation form) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&boulder, 0, sizeof(boulder));
    memset(&grotto, 0, sizeof(grotto));
    memset(&gFormState, 0, sizeof(gFormState));
    gFormState.currentForm = form;
    gFormState.idleAnim = &idle;
    idle.common.frameCount = 25;
    gFormState.savedShadowScale = 90;
    textState = boulderBreaks = sRollOwnsPause = 0;
    finishAcceptedCalls = 0;
    player.actor.update = actor_update;
    player.actor.colChkInfo.mass = 50; /* Player_UpdateCommon's live mass */
    player.cylinder.base.actor = &player.actor;
    player.cylinder.base.ocFlags1 = OC1_ON | OC1_TYPE_ALL;
    player.cylinder.base.ocFlags2 = OC2_TYPE_PLAYER;
    player.cylinder.base.acFlags = AC_ON | AC_TYPE_ENEMY;
    player.cylinder.base.shape = COLSHAPE_CYLINDER;
    player.cylinder.info.ocElemFlags = OCELEM_ON;
    player.cylinder.dim = (Cylinder16){ 24, 62, 0, { 0, 0, 0 } };
    boulder.actor.id = ACTOR_OBJ_BOMBIWA;
    boulder.actor.update = actor_update;
    boulder.actor.colChkInfo.mass = MASS_IMMOVABLE;
    boulder.actor.world.pos.z = 70;
    boulder.actor.xzDistToPlayer = 70;
    boulder.actor.params = 3;
    Collider_SetCylinder(&play, &boulder.collider, &boulder.actor, &boulderCylinderInit);
    Collider_UpdateCylinder(&boulder.actor, &boulder.collider);
    grotto.actor.update = actor_update;
    grotto.actor.params = 0x200;
    grotto.actor.world.pos.z = 5;
    Collider_SetCylinder(&play, &grotto.collider, &grotto.actor, &grottoCylinderInit);
    Collider_UpdateCylinder(&grotto.actor, &grotto.collider);
    player.meleeWeaponQuads[0].base.actor = &player.actor;
    player.meleeWeaponQuads[0].base.shape = COLSHAPE_QUAD;
    player.meleeWeaponQuads[0].dim.acDist = FLT_MAX;
}

static void collide_body(void) {
    Collider_UpdateCylinder(&player.actor, &player.cylinder);
    assert(!CollisionCheck_Incompatible(&player.cylinder.base, &boulder.collider.base) &&
           "An interrupted Goron pound must restore body OC before normal walking");
    CollisionCheck_OC_CylVsCyl(&play, &play.colChkCtx, &player.cylinder.base, &boulder.collider.base);
}

static void expect_solid(void) {
    collide_body();
    assert((player.cylinder.base.ocFlags1 & OC1_HIT) &&
           "Walking must physically collide with a nearby bombable boulder");
    assert(player.actor.colChkInfo.displacement.z < 0 && "Native OC must push Link away from the immovable rock");
    assert(boulder.actor.colChkInfo.displacement.z == 0);
}

static void collide_attack(Collider* at, Collider* ac) {
    if (!(at->atFlags & AT_ON) || !(ac->acFlags & AC_ON))
        return;
    if (at->shape == COLSHAPE_QUAD)
        CollisionCheck_AC_QuadVsCyl(&play, &play.colChkCtx, at, ac);
    else
        CollisionCheck_AC_CylVsCyl(&play, &play.colChkCtx, at, ac);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    reset(MM_PLAYER_FORM_GORON);
    if (!strcmp(argv[1], "walk")) {
        ObjBombiwa_Update(&boulder.actor, &play);
        assert(play.colChkCtx.colOCCount == 1 && play.colChkCtx.colOC[0] == &boulder.collider.base);
        expect_solid();
        assert(boulderBreaks == 0 && boulder.actor.update != NULL);
    } else if (!strcmp(argv[1], "punch")) {
        ObjBombiwa_Update(&boulder.actor, &play);
        gFormState.goronAction = GORON_ACT_PUNCH_A;
        gFormState.formSkelAnime.animation = &idle;
        gFormState.formSkelAnime.curFrame = 5;
        MmForm_Action_Punch(&player, &play);
        collide_attack(&player.meleeWeaponQuads[0].base, &boulder.collider.base);
        assert((boulder.collider.base.acFlags & AC_HIT) && "Real Goron punch quad must hit a nearby bombable rock");
        ObjBombiwa_Update(&boulder.actor, &play);
        assert(boulderBreaks == 1 && boulder.actor.update == NULL);
    } else if (!strcmp(argv[1], "pound-grotto") || !strcmp(argv[1], "pound-window")) {
        DoorAna_WaitClosed(&grotto, &play);
        if (!strcmp(argv[1], "pound-grotto"))
            production_pound_0(&player);
        else
            production_pound_1(&player);
        collide_attack(&player.cylinder.base, &grotto.collider.base);
        DoorAna_WaitClosed(&grotto, &play);
        assert(grotto.actionFunc == DoorAna_WaitOpen && "Goron ground pound must open a hammer/bomb hidden grotto");
        assert(!(player.cylinder.info.toucher.dmgFlags & DMG_EXPLOSIVE));
        grotto.actor.params = 0x100; /* Song of Storms requirement remains native. */
        grotto.actionFunc = DoorAna_WaitClosed;
        grotto.collider.base.acFlags = AC_HIT;
        DoorAna_WaitClosed(&grotto, &play);
        assert(grotto.actionFunc == DoorAna_WaitClosed && (grotto.actor.params & 0x100));
    } else if (!strcmp(argv[1], "pound-damage")) {
        DamageTable table = { 0 };
        table.table[6] = DMG_ENTRY(2, 1);  /* native hammer swing */
        table.table[30] = DMG_ENTRY(4, 2); /* native hammer jump */
        boulder.actor.colChkInfo.damageTable = &table;
        boulder.collider.base.acFlags &= ~AC_HARD;
        production_pound_0(&player);
        collide_attack(&player.cylinder.base, &boulder.collider.base);
        CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &boulder.collider.base, &boulder.collider.info);
        assert(finishAcceptedCalls == 1 && finishReceiver == &boulder.collider.base &&
               finishHit == &player.cylinder.info && finishFlags == boulder.collider.info.bumper.dmgFlags &&
               finishDamage == 4);
        assert(boulder.actor.colChkInfo.damage == 4 && boulder.actor.colChkInfo.damageEffect == 2 &&
               "Adding grotto compatibility must preserve the pound's existing "
               "jump-hammer damage table row");
        ObjBombiwa_Update(&boulder.actor, &play);
        assert(boulderBreaks == 1);
    } else if (!strcmp(argv[1], "textbox") || !strcmp(argv[1], "post-textbox-punch")) {
        production_pound_0(&player);
        gFormState.goronAction = GORON_ACT_GORON_ROLL_POUND;
        player.stateFlags1 |= PLAYER_STATE1_INPUT_DISABLED;
        player.actor.bgCheckFlags |= 0x800;
        textState = TEXT_STATE_DONE;
        production_textbox(&player, &play);
        assert(gFormState.goronAction == GORON_ACT_IDLE);
        textState = TEXT_STATE_NONE;
        production_textbox(&player, &play);
        expect_solid();
        assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
        assert(!(player.cylinder.base.atFlags & AT_ON));
        if (!strcmp(argv[1], "post-textbox-punch")) {
            ObjBombiwa_Update(&boulder.actor, &play);
            gFormState.goronAction = GORON_ACT_PUNCH_A;
            gFormState.formSkelAnime.curFrame = 5;
            MmForm_Action_Punch(&player, &play);
            collide_attack(&player.meleeWeaponQuads[0].base, &boulder.collider.base);
            ObjBombiwa_Update(&boulder.actor, &play);
            assert(boulderBreaks == 1 && "After the interruption, a new Goron punch "
                                         "must break the rock normally");
        }
    } else if (!strcmp(argv[1], "restore")) {
        production_pound_0(&player);
        production_restore_body(&player);
        assert(!(player.cylinder.base.atFlags & AT_ON) && "A human body must not retain the Goron pound attack");
        player.actor.world.pos.z = 10; /* Human radius 12 reaches the rock at distance 60. */
        expect_solid();
    } else if (!strcmp(argv[1], "controls")) {
        for (int form = MM_PLAYER_FORM_GORON; form <= MM_PLAYER_FORM_HUMAN; ++form) {
            reset(form);
            expect_solid();
            MmForm_EnablePunchQuad(&player, &play, 0, 1, DMG_SLASH_MASTER);
            collide_attack(&player.meleeWeaponQuads[0].base, &boulder.collider.base);
            ObjBombiwa_Update(&boulder.actor, &play);
            assert(boulderBreaks == 0 && boulder.actor.update != NULL &&
                   "Ordinary sword/Zora damage cannot break hammer rocks");
        }
        reset(MM_PLAYER_FORM_GORON);
        MmForm_SetRollAttack(&player, DMG_HAMMER_SWING, 1, 25);
        expect_solid();
        collide_attack(&player.cylinder.base, &boulder.collider.base);
        ObjBombiwa_Update(&boulder.actor, &play);
        assert(boulderBreaks == 1);
    } else {
        assert(0 && "Unknown test case");
    }
    printf("PASS %s\n", argv[1]);
    return 0;
}
