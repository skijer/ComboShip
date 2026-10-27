/**
 * four_sword_clone.c - ACTOR_NEI_FOUR_SWORD_CLONE (0x2C6)
 *
 * A Four Sword clone. It holds Link's own pose in a formation slot, takes hits meant for him, and
 * swings when he swings. Unity-#included by extended_equipment.c — NOT in CMake/vcxproj, and it
 * deliberately ships no header (a new mods/*.h forces a full CMake regen, see box_menu.c).
 */

#define FSC_MAX 3

// Slots are Link-local: +Z is his facing, +X his left. Rotated by his yaw every frame, so the shape
// turns with him instead of freezing at summon time.
typedef enum {
    FSC_FORM_TRIANGLE,
    FSC_FORM_LINE,
    FSC_FORM_GRID,
    FSC_FORM_CROSS,
    FSC_FORM_DISPERSE,
    FSC_FORM_MAX,
} FourSwordFormation;

typedef struct {
    f32 x;
    f32 z;
} FourSwordSlot;

#define FSC_R 80.0f

static const FourSwordSlot sFormationSlots[FSC_FORM_MAX][FSC_MAX] = {
    { { 0.0f, FSC_R }, { -69.3f, -40.0f }, { 69.3f, -40.0f } }, // triangle around Link
    { { 0.0f, -70.0f }, { 0.0f, -140.0f }, { 0.0f, -210.0f } }, // single file behind him — fits corridors
    { { FSC_R, 0.0f }, { 0.0f, FSC_R }, { FSC_R, FSC_R } },     // 2x2 block, Link one corner
    { { 0.0f, FSC_R }, { FSC_R, 0.0f }, { -FSC_R, 0.0f } },     // cross 1-2-1, Link the rear point
    { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } },         // disperse: slots unused
};

// Four Swords Adventures livery: Link keeps green, the clones are red, blue and purple. The fourth
// component is gDPSetGrayscaleColor's LERP — how far the tint pulls the model, not an alpha.
static const Color_RGBA8 sCloneTints[FSC_MAX] = {
    { 190, 40, 40, 190 },
    { 40, 70, 200, 190 },
    { 140, 40, 190, 190 },
};

#define FSC_SCALE 0.01f // human Link's own scale; not read off the player, who may be mid-init
#define FSC_SUMMON_FRAMES 12
#define FSC_SLOT_PULL 0.45f
#define FSC_SWING_FRAMES 8

#define FSC_WALL_HEIGHT 26.0f
#define FSC_WALL_RADIUS 10.0f
#define FSC_BGCHECK_FLAGS (UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_4)

typedef struct {
    Actor actor;
    ColliderCylinder hurtbox; // AC: a hit here kills the clone instead of reaching Link
    ColliderCylinder blade;   // AT: live while Link's own swing is live
    Vec3s pose[PLAYER_LIMB_MAX + 1];
    u8 index;
    u8 colInit;
    s16 summonTimer;
    s16 swingTimer;
} FourSwordClone;

// OC excludes TYPE_PLAYER on purpose: enemies bump into a clone, Link walks through it, so a
// formation can never wall him into a corridor.
static const ColliderCylinderInit sHurtboxInit = {
    { COL_MATERIAL_NONE, AT_NONE, AC_ON | AC_TYPE_ENEMY, OC1_ON | OC1_TYPE_1 | OC1_TYPE_2, OC2_TYPE_1,
      COLSHAPE_CYLINDER },
    { ELEM_MATERIAL_UNK0, { 0x00000000, 0x00, 0x00 }, { 0xFFFFFFFF, 0x00, 0x00 }, ATELEM_NONE, ACELEM_ON, OCELEM_ON },
    { 18, 46, 0, { 0, 0, 0 } },
};

static const ColliderCylinderInit sBladeInit = {
    { COL_MATERIAL_NONE, AT_ON | AT_TYPE_PLAYER, AC_NONE, OC1_NONE, OC2_NONE, COLSHAPE_CYLINDER },
    { ELEM_MATERIAL_UNK2,
      { DMG_SWORD, 0x00, 0x02 },
      { 0xFFCFFFFF, 0x00, 0x00 },
      ATELEM_ON | ATELEM_SFX_NORMAL,
      ACELEM_NONE,
      OCELEM_NONE },
    { 25, 60, 30, { 0, 0, 0 } },
};

static void FourSwordClone_Init(Actor* thisx, PlayState* play);
static void FourSwordClone_Destroy(Actor* thisx, PlayState* play);
static void FourSwordClone_Update(Actor* thisx, PlayState* play);
static void FourSwordClone_Draw(Actor* thisx, PlayState* play);

static FourSwordClone* sLiveClones[FSC_MAX];
static u8 sFormation = FSC_FORM_TRIANGLE;

u8 FourSwordClone_Count(void) {
    u8 n = 0;
    for (int i = 0; i < FSC_MAX; i++) {
        if (sLiveClones[i] != NULL) {
            n++;
        }
    }
    return n;
}

u8 FourSwordClone_GetFormation(void) {
    return sFormation;
}

// Where clone `index` is standing, for anything that has to act from its position (the item
// mirror). Returns 0 when that slot has no live clone.
u8 FourSwordClone_GetPos(u8 index, Vec3f* out) {
    if ((index >= FSC_MAX) || (sLiveClones[index] == NULL) || (out == NULL)) {
        return 0;
    }
    *out = sLiveClones[index]->actor.world.pos;
    return 1;
}

void FourSwordClone_SetFormation(u8 formation) {
    if (formation < FSC_FORM_MAX) {
        sFormation = formation;
    }
}

void FourSwordClone_KillAll(void) {
    for (int i = 0; i < FSC_MAX; i++) {
        if (sLiveClones[i] != NULL) {
            Actor_Kill(&sLiveClones[i]->actor);
            sLiveClones[i] = NULL;
        }
    }
    gExtEquipBehavior.fourSwordCloneMask = 0;
}

Actor* FourSwordClone_Spawn(PlayState* play, Player* player, u8 index) {
    if ((play == NULL) || (player == NULL) || (index >= FSC_MAX) || (sLiveClones[index] != NULL)) {
        return NULL;
    }
    // The mask is the INTENT — which clones should exist. Actors die with the scene, so it is what
    // FourSwordClone_Reconcile rebuilds them from.
    gExtEquipBehavior.fourSwordCloneMask |= (1 << index);
    // Spawned INSIDE Link; the summon slides it out to its slot over FSC_SUMMON_FRAMES.
    return Actor_Spawn(&play->actorCtx, play, ACTOR_NEI_FOUR_SWORD_CLONE, player->actor.world.pos.x,
                       player->actor.world.pos.y, player->actor.world.pos.z, 0, player->actor.shape.rot.y, 0, index);
}

// Called every frame: any clone the mask says should exist but doesn't is (re)summoned. That is what
// carries the formation across a scene change — the actors die with the scene, the mask does not —
// and it self-heals if a spawn ever fails. A clone killed in combat clears its own bit, so it stays
// dead until the next R+B.
void FourSwordClone_Reconcile(PlayState* play, Player* player) {
    u8 mask = gExtEquipBehavior.fourSwordCloneMask;

    for (u8 i = 0; i < FSC_MAX; i++) {
        if ((mask & (1 << i)) && (sLiveClones[i] == NULL)) {
            FourSwordClone_Spawn(play, player, i);
        }
    }
}

static Vec3f FourSwordClone_SlotPos(Player* player, u8 index) {
    const FourSwordSlot* slot = &sFormationSlots[sFormation][index];
    f32 sin = Math_SinS(player->actor.shape.rot.y);
    f32 cos = Math_CosS(player->actor.shape.rot.y);
    Vec3f out;

    out.x = player->actor.world.pos.x + (slot->z * sin) + (slot->x * cos);
    out.y = player->actor.world.pos.y;
    out.z = player->actor.world.pos.z + (slot->z * cos) - (slot->x * sin);
    return out;
}

static void FourSwordClone_Sparkle(PlayState* play, Vec3f* at, s32 count, f32 spread, f32 rise) {
    Vec3f vel = { 0.0f, rise, 0.0f };
    Vec3f accel = { 0.0f, 0.0f, 0.0f };

    for (s32 p = 0; p < count; p++) {
        Vec3f pos = {
            at->x + Rand_CenteredFloat(spread),
            at->y + 30.0f + Rand_ZeroFloat(spread),
            at->z + Rand_CenteredFloat(spread),
        };
        vel.x = Rand_CenteredFloat(2.0f);
        vel.z = Rand_CenteredFloat(2.0f);
        // EffectSsKiraKira_SpawnSmall is a MACRO in MM: a compound literal's commas would be
        // miscounted as extra macro args (C4002), so the colors need named locals.
        Color_RGBA8 prim = { 150, 210, 255, 255 };
        Color_RGBA8 env = { 60, 110, 255, 0 };
        EffectSsKiraKira_SpawnSmall(play, &pos, &vel, &accel, &prim, &env);
    }
}

static void FourSwordClone_Init(Actor* thisx, PlayState* play) {
    FourSwordClone* self = (FourSwordClone*)thisx;

    self->index = (u8)(self->actor.params & 3);
    if (self->index >= FSC_MAX) {
        Actor_Kill(&self->actor);
        return;
    }

    Actor_SetScale(&self->actor, FSC_SCALE);
    ActorShape_Init(&self->actor.shape, 0.0f, ActorShadow_DrawCircle, 30.0f);

    Collider_InitCylinder(play, &self->hurtbox);
    Collider_SetCylinder(play, &self->hurtbox, &self->actor, &sHurtboxInit);
    Collider_InitCylinder(play, &self->blade);
    Collider_SetCylinder(play, &self->blade, &self->actor, &sBladeInit);
    self->colInit = 1;

    self->actor.gravity = -2.0f;
    self->summonTimer = FSC_SUMMON_FRAMES;
    self->swingTimer = 0;

    sLiveClones[self->index] = self;
    FourSwordClone_Sparkle(play, &self->actor.world.pos, 6, 20.0f, 1.5f);
    Actor_PlaySfx(&self->actor, NA_SE_SY_LOCK_ON);
}

static void FourSwordClone_Destroy(Actor* thisx, PlayState* play) {
    FourSwordClone* self = (FourSwordClone*)thisx;

    if (self->colInit) {
        Collider_DestroyCylinder(play, &self->hurtbox);
        Collider_DestroyCylinder(play, &self->blade);
    }
    if ((self->index < FSC_MAX) && (sLiveClones[self->index] == self)) {
        sLiveClones[self->index] = NULL;
    }
}

static void FourSwordClone_Die(FourSwordClone* self, PlayState* play) {
    gExtEquipBehavior.fourSwordCloneMask &= ~(1 << self->index);
    FourSwordClone_Sparkle(play, &self->actor.world.pos, 10, 25.0f, 2.0f);
    Actor_PlaySfx(&self->actor, NA_SE_EN_EXTINCT);
    Actor_Kill(&self->actor);
}

static void FourSwordClone_Update(Actor* thisx, PlayState* play) {
    extern u8 FourSword_IsEquipped(void);
    FourSwordClone* self = (FourSwordClone*)thisx;
    Player* player = GET_PLAYER(play);

    if (!FourSword_IsEquipped() || (player->transformation != PLAYER_FORM_HUMAN)) {
        Actor_Kill(&self->actor);
        return;
    }

    // CollisionCheck_SetAC clears AC_HIT through sACResetFuncs, so last frame's hit has to be read
    // before re-registering.
    if (self->hurtbox.base.acFlags & AC_HIT) {
        self->hurtbox.base.acFlags &= ~AC_HIT;
        FourSwordClone_Die(self, play);
        return;
    }

    if (self->summonTimer > 0) {
        self->summonTimer--;
    }

    if (sFormation != FSC_FORM_DISPERSE) {
        Vec3f slot = FourSwordClone_SlotPos(player, self->index);
        // Emerging from inside Link: ease out over the summon window instead of snapping to the
        // slot on frame one.
        f32 pull = (self->summonTimer > 0) ? (1.0f - ((f32)self->summonTimer / FSC_SUMMON_FRAMES)) : FSC_SLOT_PULL;

        self->actor.world.pos.x += (slot.x - self->actor.world.pos.x) * pull;
        self->actor.world.pos.z += (slot.z - self->actor.world.pos.z) * pull;
        self->actor.shape.rot.y = player->actor.shape.rot.y;
        self->actor.world.rot.y = self->actor.shape.rot.y;
    }

    self->actor.speed = 0.0f; // XZ is driven above; gravity only owns Y
    Actor_MoveWithGravity(&self->actor);
    Actor_UpdateBgCheckInfo(play, &self->actor, FSC_WALL_HEIGHT, FSC_WALL_RADIUS, 0.0f, FSC_BGCHECK_FLAGS);
    // Standing still still accumulates fall speed, and the clone would then drop like a stone the
    // first time it walked off a ledge.
    if (self->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        self->actor.velocity.y = 0.0f;
    }

    Collider_UpdateCylinder(&self->actor, &self->hurtbox);
    CollisionCheck_SetAC(play, &play->colChkCtx, &self->hurtbox.base);
    CollisionCheck_SetOC(play, &play->colChkCtx, &self->hurtbox.base);

    // The blade only bites while Link's own swing is live, so the clones read as mirroring him
    // rather than flailing on their own.
    if (player->meleeWeaponState > 0) {
        self->swingTimer = FSC_SWING_FRAMES;
    } else if (self->swingTimer > 0) {
        self->swingTimer--;
    }

    if (self->swingTimer > 0) {
        Collider_UpdateCylinder(&self->actor, &self->blade);
        CollisionCheck_SetAT(play, &play->colChkCtx, &self->blade.base);
    }
}

// Copying the live Player wholesale and overriding only transform and pose is what makes the sword,
// shield, sheath and tunic render — a bare SkelAnime_DrawFlexOpa draws the naked skeleton.
static Player sCloneDrawTemplate;

static void FourSwordClone_Draw(Actor* thisx, PlayState* play) {
    extern void PlayerTunic_BindLocalColor(PlayState * play);
    FourSwordClone* self = (FourSwordClone*)thisx;
    Player* player = GET_PLAYER(play);

    if ((player->skelAnime.skeleton == NULL) || (player->transformation != PLAYER_FORM_HUMAN)) {
        return;
    }
    if ((player->actor.objectSlot < 0) || !Object_IsLoaded(&play->objectCtx, player->actor.objectSlot)) {
        return;
    }

    // Link's pose into our own buffer: the limb override writes back into rot[] (leg IK) and must
    // never touch the real joint table.
    s32 limbCount = player->skelAnime.limbCount;
    for (s32 j = 0; j <= limbCount; j++) {
        self->pose[j] = player->skelAnime.jointTable[j];
    }

    Player* dp = &sCloneDrawTemplate;
    *dp = *player;
    dp->actor.world.pos = self->actor.world.pos;
    dp->actor.shape.rot = self->actor.shape.rot;
    dp->skelAnime.jointTable = self->pose;
    // The sword trail is keyed on meleeWeaponEffectIndex, which the copy shares with Link — every
    // clone would drive HIS blur. Zeroing the state skips the whole trail path in the post-limb
    // draw (the same guard HarpoonDummyPlayer needs).
    dp->meleeWeaponState = 0;

    // Ground it the way Actor_Draw does, or the model hovers.
    f32 yOff = dp->actor.shape.yOffset * dp->actor.scale.y;
    void* seg06 = play->objectCtx.slots[player->actor.objectSlot].segment;
    const Color_RGBA8* tint = &sCloneTints[self->index];

    OPEN_DISPS(play->state.gfxCtx);

    if (seg06 != NULL) {
        gSPSegment(POLY_OPA_DISP++, 0x06, (uintptr_t)seg06);
        gSPSegment(POLY_XLU_DISP++, 0x06, (uintptr_t)seg06);
    }
    gSPSegment(POLY_OPA_DISP++, 0x0C, (uintptr_t)gCullBackDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, (uintptr_t)gCullBackDList);
    PlayerTunic_BindLocalColor(play);

    gSPGrayscale(POLY_OPA_DISP++, true);
    gDPSetGrayscaleColor(POLY_OPA_DISP++, tint->r, tint->g, tint->b, tint->a);

    Matrix_SetTranslateRotateYXZ(dp->actor.world.pos.x, dp->actor.world.pos.y + yOff, dp->actor.world.pos.z,
                                 &dp->actor.shape.rot);
    Matrix_Scale(dp->actor.scale.x, dp->actor.scale.y, dp->actor.scale.z, MTXMODE_APPLY);

    Player_DrawImpl(play, dp->skelAnime.skeleton, dp->skelAnime.jointTable, dp->skelAnime.dListCount, 0,
                    PLAYER_FORM_HUMAN, dp->currentBoots, dp->actor.shape.face, Player_OverrideLimbDrawGameplayDefault,
                    Player_PostLimbDrawGameplay, &dp->actor);

    gSPGrayscale(POLY_OPA_DISP++, false);

    CLOSE_DISPS(play->state.gfxCtx);
}

ActorProfile FourSwordClone_Profile = {
    /**/ ACTOR_NEI_FOUR_SWORD_CLONE,
    /**/ ACTORCAT_MISC,
    /**/ (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED),
    /**/ GAMEPLAY_KEEP,
    /**/ sizeof(FourSwordClone),
    /**/ FourSwordClone_Init,
    /**/ FourSwordClone_Destroy,
    /**/ FourSwordClone_Update,
    /**/ FourSwordClone_Draw,
};
