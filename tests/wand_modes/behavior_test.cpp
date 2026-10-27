// Actual MM rod/input, collider registration/overlap, damage chart and Chuchu
// damage consumption. Asset allocation, floor geometry and reaction VFX are boundaries.
#include "behavior_fixture.inc"
#include "mods/combo_rpg.h"
#include "din_fire_sword.h"
#include "overlays/actors/ovl_En_Slime/z_en_slime.h"
#include "overlays/actors/ovl_En_Clear_Tag/z_en_clear_tag.h"

u8 gIvanPossessActive;
u8 Sm64Mario_IsReady() { return 0; }
u8 TridentChargeBall_GetFierceDamage(Actor*) { return 0; }
uint8_t DinFireSword_DamageEntry(PlayState*, Actor*, const ColliderElement*, uint32_t, uint8_t entry) { return entry; }
// Elemental finish VFX are external to these native wand damage checks.
void DinFireSword_FinishAcceptedHit(PlayState*, const void*, const ColliderElement*, uint32_t, uint32_t) {}
void DinFireSword_FinishFatal(PlayState*, Actor*) {}
// Wand projectiles are foreign to the owned enchanted-sword collider context.
uint32_t DinFireSword_OriginalDamageFlags(PlayState*, const ColliderElement* hit) {
    return hit ? hit->atDmgInfo.dmgFlags : 0;
}
int ComboRpg_IsEnabled(int) { return 0; }
uint8_t ComboRpg_ApplyPower(uint8_t damage, float) { return damage; }
f32 Rand_ZeroOne() { return 0; }
s32 FrameAdvance_IsEnabled(PlayState*) { return false; }
void Math_Vec3s_ToVec3f(Vec3f* dest, Vec3s* src) { *dest = {(f32)src->x, (f32)src->y, (f32)src->z}; }
void CollisionCheck_HitEffects(PlayState*, Collider*, ColliderElement*, Collider*, ColliderElement*, Vec3f*) {}
void EnSlime_Thaw(EnSlime*, PlayState*) {}
void EnSlime_SetupReactToBluntHit(EnSlime*) {}
void EnSlime_SetupStun(EnSlime*) {}
void EnSlime_SetupSpawnIceBlock(EnSlime*) {}
void EnSlime_Freeze(EnSlime*) {}
void EnSlime_SetupDamaged(EnSlime*, PlayState*, s32) {}
void Actor_SetDropFlag(Actor*, ColliderElement*) {}
void Enemy_StartFinishingBlow(PlayState*, Actor*) {}
void Actor_PlaySfx(Actor*, u16) {}

#include "behavior_native.inc"

static int SandParityMagic() { return gSaveContext.save.saveInfo.playerData.magic; }
static void SandParityRefill() { gSaveContext.save.saveInfo.playerData.magic = 48; }
static void ResetSandParity(Player& player, PlayState& play, u16 button, s16 yaw, float halfX) {
    ResetWorld(player, play);
    player.actor.shape.rot.y = yaw;
    slabCollision.minBounds.x = -(s16)(halfX / .05f);
    slabCollision.maxBounds.x = (s16)(halfX / .05f);
    Nei_Save()->wandMode = WAND_MODE_SAND;
    if (button == BTN_CLEFT) BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_ELEMENTAL_WAND;
    else DPAD_BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_D_UP) = ITEM_ELEMENTAL_WAND;
    player.heldItemAction = PLAYER_IA_NONE;
    ++play.gameplayFrames; Wand_TickInput(&play, &player);
    player.heldItemAction = (PlayerItemAction)PLAYER_IA_ELEMENTAL_WAND;
}

#include "sand_parity_scenario.h"

static void CheckStormHits() {
    // Land squashes a Chuchu to scale.y = 0.0074399994f, giving a 26-unit
    // cylinder. A free shot at height 30 must still hit it on level ground.
    for (bool locked : {true, false}) {
        for (s16 yaw : {0, 0x2000, 0x4000, -0x2000}) {
            for (float distance : {12.0f, 40.0f, 100.0f, 300.0f, 600.0f}) {
                Player player{}; PlayState play{}; EnSlime chu{};
                ResetWorld(player, play);
                player.actor.update = LiveActor;
                player.actor.shape.rot.y = yaw;
                chu.actor.id = ACTOR_EN_SLIME; chu.actor.category = ACTORCAT_ENEMY;
                chu.actor.update = LiveActor; chu.actor.params = EN_SLIME_TYPE_GREEN;
                chu.actor.world.pos = {Math_SinS(yaw) * distance, 0, Math_CosS(yaw) * distance};
                chu.actor.focus.pos = {chu.actor.world.pos.x, 15, chu.actor.world.pos.z};
                chu.actor.colChkInfo.damageTable = &sDamageTable; chu.actor.colChkInfo.health = 1;
                Collider_InitCylinder(&play, &chu.collider);
                Collider_SetCylinder(&play, &chu.collider, &chu.actor, &sCylinderInit);
                chu.collider.dim.height = 26; // native Land scale, independently calculated
                chu.collider.dim.pos = {(s16)chu.actor.world.pos.x, 0, (s16)chu.actor.world.pos.z};
                if (locked) {
                    player.focusActor = &chu.actor;
                    player.stateFlags3 |= PLAYER_STATE3_HOSTILE_LOCK_ON;
                }
                assert(Wand_Cast(&player, &play, WAND_MODE_STORM));
                for (int frame = 0; frame < 40 && chu.actor.colChkInfo.health; ++frame) {
                    play.colChkCtx.colATCount = 0;
                    WandStorm_Tick(&play, &player);
                    if (play.colChkCtx.colATCount) {
                        Collider* at = play.colChkCtx.colAT[0];
                        CollisionCheck_AC_CylVsCyl(&play, &play.colChkCtx, at, &chu.collider.base);
                        CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &chu.collider.base, &chu.collider.elem);
                        EnSlime_UpdateDamage(&chu, &play);
                    }
                }
                if (chu.actor.colChkInfo.health) {
                    std::cerr << "FAIL Storm " << (locked ? "lock-on" : "free aim")
                              << " distance=" << distance << " yaw=" << yaw << "\n";
                }
                assert(chu.actor.colChkInfo.health == 0 && "Storm must damage a low Chuchu without lock-on");
            }
        }
    }
    std::cout << "PASS native Storm collision and Chuchu damage: free/locked, four headings, five distances\n";
}

static void CheckSandPath() {
    for (u16 button : {BTN_CLEFT, BTN_DUP}) {
        for (s16 yaw : {0, 0x2000, 0x4000, -0x2000}) {
            Player player{}; PlayState play{};
            ResetWorld(player, play); player.actor.shape.rot.y = yaw;
            Nei_Save()->wandMode = WAND_MODE_SAND;
            if (button == BTN_CLEFT) BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_ELEMENTAL_WAND;
            else DPAD_BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_D_UP) = ITEM_ELEMENTAL_WAND;
            ++play.gameplayFrames; Wand_TickInput(&play, &player); // establish held item
            play.state.input[0].press.button = play.state.input[0].cur.button = button;
            ++play.gameplayFrames; Wand_TickInput(&play, &player);
            assert(SandFloorAt(actors, player.actor.world.pos));
            play.state.input[0].press.button = 0;
            for (int frame = 0; frame < 100; ++frame) {
                // MM refreshes BG dynapoly before the player category. A slab
                // spawned in player input cannot support movement until next frame.
                const auto ready = actors;
                SandUpdateReady(play, ready, SandFloorAt(ready, player.actor.world.pos));
                ++play.gameplayFrames; Wand_TickInput(&play, &player);
                player.actor.world.pos.x += Math_SinS(yaw) * 6.0f;
                player.actor.world.pos.z += Math_CosS(yaw) * 6.0f;
                if (!SandFloorAt(ready, player.actor.world.pos)) {
                    std::cerr << "FAIL Sand walked off before next floor was ready: frame=" << frame + 1 << "\n";
                }
                assert(SandFloorAt(ready, player.actor.world.pos) && "held Sand must build before Link walks off the edge");
            }
            assert(actors.size() > 8 && "a long hold must keep creating platforms beyond the ring capacity");
            assert(gSaveContext.save.saveInfo.playerData.magic == 48 - (int)actors.size() * 2 &&
                "each successful Sand placement must pay the same two magic as OoT");
        }
    }
    for (Actor* actor : actors) std::free(actor);
    actors.clear();
    std::cout << "PASS held Sand fixture path: C/D-pad, four headings, 600 units, 60-unit mesh, speed 6, crumble and BG refresh\n";
}

int main(int argc, char** argv) {
    if (argc > 1 && !std::strcmp(argv[1], "sand-parity")) {
        CheckSandParity();
        for (Actor* actor : actors) std::free(actor);
        actors.clear();
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "storm-hits") && std::strcmp(argv[1], "sand-path")) return 2;
    if (argc == 1 || !std::strcmp(argv[1], "storm-hits")) CheckStormHits();
    if (argc == 1 || !std::strcmp(argv[1], "sand-path")) CheckSandPath();
}
