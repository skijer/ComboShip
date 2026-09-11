/**
 * equip_roc_boots.c - Roc Boots (Extended Boots Slot 3)
 *
 * Takes over the slot the deleted Water Dragon Scale used to hold (its Zora swim became the ZORA
 * TUNIC's permanent effect — see equip_dragonscale.c / Nei_IsZoraSwim).
 *
 * BEHAVIOR:
 *   - WALK ON WATER: feet at/below a water surface while not swimming → Link is pinned to the
 *     surface and it counts as flat floor (z_player.c Player_ProcessSceneCollision, the same snap
 *     Deku Link's water hops use). Jumping off it works; coming back down lands on it again.
 *   - WALK ON LAVA: the sink/burn/void-out floor types and the timed hot floors read as plain
 *     floor (z_player.c floor-type override, the family the Hover Boots float over).
 *   - HALF GRAVITY: every downward acceleration is halved. The z_player.c gravity integrators go
 *     through RocBoots_MoveWithGravity (gravity scaled for the call only) and the Deku Leaf's fixed
 *     glide descent is halved too. Jumps (Roc's Feather included) go higher for free.
 *
 * Included by ext_equip_behavior.c (unity build).
 */

u8 RocBoots_IsWorn(void) {
    return ExtEquip_IsEnabled() && (gExtEquipState.currentExtBoots == 3);
}

// Actions set actor.gravity once and keep it, so scaling the field itself would compound every
// frame — scale it around the engine integrator instead.
void RocBoots_MoveWithGravity(Player* p, void (*integrate)(Actor*)) {
    f32 gravity = p->actor.gravity;

    if (RocBoots_IsWorn() && (p->transformation == PLAYER_FORM_HUMAN)) {
        p->actor.gravity *= 0.5f;
    }
    integrate(&p->actor);
    p->actor.gravity = gravity;
}

static u8 sRocOnWater = 0;

// 0 = not on water; 1 = pinned to the surface; 2 = pinned, first frame (the landing).
u8 RocBoots_WalksOnWater(Player* p) {
    u8 wasOnWater = sRocOnWater;

    sRocOnWater = RocBoots_IsWorn() && (p->transformation == PLAYER_FORM_HUMAN) &&
                  !(p->stateFlags1 & PLAYER_STATE1_8000000) && (p->actor.depthInWater >= 0.0f) &&
                  (p->actor.velocity.y <= 0.0f);
    if (!sRocOnWater) {
        return 0;
    }
    return wasOnWater ? 1 : 2;
}

u8 RocBoots_OnWater(void) {
    return sRocOnWater;
}

// Per-frame behavior while the Roc Boots are the equipped ext boots (the effects are z_player gates).
static void RocBoots_Behavior(Player* player, PlayState* play) {
    (void)player;
    (void)play;
}

// Called when the Roc Boots are unequipped (restore anything the behavior forced).
static void RocBoots_Cleanup(void) {
    sRocOnWater = 0;
}
