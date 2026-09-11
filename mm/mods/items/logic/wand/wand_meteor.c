/**
 * wand_meteor.c — Meteor Rod (Skijer's NEI).
 *
 * A real En_Bom, thrown instead of set down, and red. It keeps EnBom's horizontal behaviour — the
 * wall reflection, the friction, the fuse, the explosion — but its vertical motion is written as a
 * decaying sine arc rather than left to gravity, so it skips instead of dropping.
 */

#include "overlays/actors/ovl_En_Bom/z_en_bom.h" // EnBom.timer — never a hand-computed offset
#include <math.h>                                // sinf / fabsf — the arc is written, not simulated

// Launched from Link's feet: the arc starts at floor level (sin 0), so spawning it any higher only
// buys a snap downward on the first frame.
#define METEOR_SPAWN_DIST 30.0f
#define METEOR_LAUNCH_SPEED 9.0f

// How close an enemy has to be for the bomb to cook off against it. The vanilla AT collider only
// exists once the bomb is ALREADY exploding, so contact has to be answered by proximity.
#define METEOR_ENEMY_TRIGGER_RADIUS 34.0f

// Long enough to let the four bounces happen before it cooks off on its own.
#define METEOR_FUSE_FRAMES 170

// EnBom_Init leaves the bomb at scale 0 and the frame that grows it is timer == 67 exactly
// (z_en_bom.c:469). With a fuse this long that frame is a hundred away, so the bomb would spend its
// whole flight invisible — it has to be sized here instead.
#define METEOR_BOMB_SCALE 0.01f

// The skip. Vertical motion is NOT left to gravity: a bomb keeps 0.3 of its impact speed off the
// floor, which is two hops and done. The arc is written directly instead — |sin| over one hop
// period, with the peak falling as 1/(1 + k*n). That 1/n decay is the point: the bounces get
// shorter but never suddenly, so the bomb keeps skipping the length of a corridor.
#define METEOR_BOUNCES_REQUIRED 4
#define METEOR_HOP_PERIOD 16.0f // frames per arc
#define METEOR_HOP_HEIGHT 46.0f // peak of the first arc, world units
#define METEOR_HOP_DECAY 0.3f   // higher = the arcs flatten out faster

// Frames since launch, on home.rot.x — untouched by En_Bom, and the same "state on a hijacked actor
// field" idiom the Somaria summons use. There is no room for it in EnBom itself.
#define METEOR_HOP_TIME(actor) ((actor)->home.rot.x)

#define METEOR_TINT_R 235
#define METEOR_TINT_G 45
#define METEOR_TINT_B 30
#define METEOR_TINT_MIX 255

static const u8 sMeteorEnemyCats[2] = { ACTORCAT_ENEMY, ACTORCAT_BOSS };

static ActorFunc sMeteorBombDraw = NULL;
static ActorFunc sMeteorBombUpdate = NULL;

// Wrapping rather than replacing EnBom_Draw matters — that draw also billboards the bomb and runs
// Collider_UpdateSpheres, which is what gives the explosion its hitbox.
static void WandMeteor_TintDraw(Actor* thisx, PlayState* play) {
    if (sMeteorBombDraw == NULL) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);
    gDPSetGrayscaleColor(POLY_OPA_DISP++, METEOR_TINT_R, METEOR_TINT_G, METEOR_TINT_B, METEOR_TINT_MIX);
    gSPGrayscale(POLY_OPA_DISP++, true);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, METEOR_TINT_R, METEOR_TINT_G, METEOR_TINT_B, METEOR_TINT_MIX);
    gSPGrayscale(POLY_XLU_DISP++, true);
    CLOSE_DISPS(play->state.gfxCtx);

    sMeteorBombDraw(thisx, play);

    OPEN_DISPS(play->state.gfxCtx);
    // Both buffers, or everything drawn after the bomb this frame comes out red too.
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPGrayscale(POLY_XLU_DISP++, false);
    CLOSE_DISPS(play->state.gfxCtx);
}

static u8 WandMeteor_TouchingEnemy(Actor* thisx, PlayState* play) {
    return TargetSelect_FindNearest(play, sMeteorEnemyCats, ARRAY_COUNT(sMeteorEnemyCats), NULL, &thisx->world.pos,
                                    METEOR_ENEMY_TRIGGER_RADIUS) != NULL;
}

static s16 WandMeteor_HopIndex(Actor* thisx) {
    return (s16)((f32)METEOR_HOP_TIME(thisx) / METEOR_HOP_PERIOD);
}

/**
 * One arc of the skip, stamped onto the position after the vanilla update has moved the bomb
 * horizontally and refreshed its floor. EnBom's own gravity is left to run underneath and simply
 * overwritten, which is what makes every arc identical instead of dependent on impact speed.
 *
 * velocity.y is zeroed with it so the vanilla floor bounce never fires and cannot fight the curve.
 */
static void WandMeteor_Hop(Actor* thisx) {
    f32 phase = (f32)METEOR_HOP_TIME(thisx) / METEOR_HOP_PERIOD;
    f32 peak = METEOR_HOP_HEIGHT / (1.0f + (METEOR_HOP_DECAY * (f32)WandMeteor_HopIndex(thisx)));

    thisx->world.pos.y = thisx->floorHeight + (fabsf(sinf(M_PI * phase)) * peak);
    thisx->velocity.y = 0.0f;
    METEOR_HOP_TIME(thisx)++;
}

static void WandMeteor_Update(Actor* thisx, PlayState* play) {
    // The wall bit is read BEFORE the vanilla update: EnBom_Move consumes and clears it while
    // bouncing (z_en_bom.c:227), so afterwards there is nothing left to see.
    u8 hitWall = (thisx->bgCheckFlags & BGCHECKFLAG_WALL) != 0;

    // Only while it is still a body: once it is the explosion, params changed and the timer means
    // something else entirely.
    if (thisx->params == BOMB_TYPE_BODY) {
        // An enemy sets it off on contact whatever the count. A wall only does once the four
        // bounces are spent, so a shot down a corridor still skips instead of dying on the first
        // thing it grazes.
        if (WandMeteor_TouchingEnemy(thisx, play) ||
            (hitWall && (WandMeteor_HopIndex(thisx) >= METEOR_BOUNCES_REQUIRED))) {
            ((EnBom*)thisx)->timer = 0;
        }
    }

    sMeteorBombUpdate(thisx, play);

    // Over a pit there is no floor to arc above, so the bomb is handed back to gravity and falls.
    if ((thisx->params == BOMB_TYPE_BODY) && (thisx->floorHeight > BGCHECK_Y_MIN)) {
        WandMeteor_Hop(thisx);
    }
}

u8 WandMeteor_Cast(Player* player, PlayState* play) {
    s16 yaw = player->actor.shape.rot.y;
    f32 sn = Math_SinS(yaw);
    f32 cs = Math_CosS(yaw);
    Actor* bomb = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_BOM, player->actor.world.pos.x + (sn * METEOR_SPAWN_DIST),
                              player->actor.world.pos.y, player->actor.world.pos.z + (cs * METEOR_SPAWN_DIST), 0, yaw,
                              0, BOMB_TYPE_BODY);

    if (bomb == NULL) {
        return 0;
    }

    // Captured from the only place they are guaranteed to still be EnBom's own.
    if (sMeteorBombUpdate == NULL) {
        sMeteorBombUpdate = bomb->update;
        sMeteorBombDraw = bomb->draw;
    }
    bomb->update = WandMeteor_Update;
    bomb->draw = WandMeteor_TintDraw;

    ((EnBom*)bomb)->timer = METEOR_FUSE_FRAMES;
    Actor_SetScale(bomb, METEOR_BOMB_SCALE);
    METEOR_HOP_TIME(bomb) = 0;
    bomb->speed = METEOR_LAUNCH_SPEED;
    bomb->world.rot.y = yaw;

    // A throw, not NA_SE_IT_BOMB_IGNIT: every vanilla caller plays the fuse as `- SFX_FLAG`, the
    // continuous variant re-issued every frame, so started raw it would never stop.
    Audio_PlaySoundGeneral(NA_SE_PL_THROW, &bomb->world.pos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    return 1;
}
