#include "global.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "overlays/actors/ovl_En_Tanron5/z_en_tanron5.h"
#include "overlays/actors/ovl_Boss_02/z_boss_02.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "tests/test_require.h"
#include <string.h>

SaveContext gSaveContext;
static PlayState play;
static Player player;
static int enabled, threeD, killed, given;
static u8 lastItem;
static f32 sGiantModeScaleFactor = 1.0f;

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name, "gCheats.DropsDontDie")) {
        return enabled;
    }
    if (!strcmp(name, "gEnhancements.Graphics.3DItemDrops")) {
        return threeD;
    }
    return fallback;
}
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    return result;
}
void Actor_Kill(Actor* actor) {
    actor->update = NULL;
    ++killed;
}
s32 Actor_HasParent(Actor* actor, PlayState* play) {
    return actor->parent != NULL;
}
s32 Actor_OfferGetItem(Actor* actor, PlayState* play, GetItemId id, f32 xz, f32 y) {
    return false;
}
u8 Item_Give(PlayState* play, u8 item) {
    ++given;
    lastItem = item;
    return item;
}
s32 Health_ChangeBy(PlayState* play, s16 amount) {
    return true;
}
void Audio_PlaySfx(u16 id) {
}
void Flags_SetCollectible(PlayState* play, s32 flag) {
}
void Actor_SetScale(Actor* actor, f32 scale) {
    actor->scale = (Vec3f){ scale, scale, scale };
}
void Actor_MoveWithGravity(Actor* actor) {
}
void Actor_UpdateBgCheckInfo(PlayState* play, Actor* actor, f32 height, f32 wall, f32 ceiling, u32 flags) {
}
void Collider_UpdateCylinder(Actor* actor, ColliderCylinder* cylinder) {
}
s32 CollisionCheck_SetAC(PlayState* play, CollisionCheckContext* context, Collider* collider) {
    return true;
}
f32 Math_SmoothStepToF(f32* value, f32 target, f32 fraction, f32 step, f32 min) {
    *value = target;
    return 0;
}
s16 Math_SmoothStepToS(s16* value, s16 target, s16 scale, s16 step, s16 min) {
    *value = target;
    return 0;
}
f32 Math_SinS(s16 angle) {
    return sinf(angle * 3.14159265358979323846f / 32768);
}
f32 Math_CosS(s16 angle) {
    return cosf(angle * 3.14159265358979323846f / 32768);
}
f32 Rand_ZeroFloat(f32 scale) {
    return 0;
}
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) {
    *dest = *src;
}
void EnTanron5_SpawnEffectSand(TwinmoldEffect* effect, Vec3f* pos, f32 scale) {
}
void func_800B8D50(PlayState* play, Actor* actor, f32 force, s16 yaw, f32 height, u32 damage) {
}
void func_800A6650(EnItem00* item, PlayState* play) {
}
void DummyActorUpdate(Actor* actor, PlayState* play) {
}

#include "drop_lifetime_production.inc"

static EnItem00 drop(s16 params, s16 timer) {
    killed = given = 0;
    EnItem00 item = { 0 };
    item.actor.params = params;
    item.actor.update = DummyActorUpdate;
    item.actor.xzDistToPlayer = 1000.0f;
    item.actor.floorHeight = 0;
    item.unk152 = timer;
    item.unk150 = 1;
    item.unk154 = 0.02f;
    item.getItemId = GI_NONE;
    item.actionFunc = func_800A640C;
    return item;
}

static void TestRegularDrops(void) {
    const s16 params[] = { ITEM00_RUPEE_GREEN, ITEM00_RECOVERY_HEART, ITEM00_MAGIC_JAR_SMALL, ITEM00_MAGIC_JAR_BIG,
                           ITEM00_BOMBS_A,     ITEM00_ARROWS_10,      ITEM00_DEKU_NUTS_1 };
    for (threeD = 0; threeD <= 1; ++threeD) {
        for (size_t type = 0; type < ARRAY_COUNT(params); ++type) {
            enabled = 0;
            EnItem00 item = drop(params[type], 3);
            for (int frame = 0; frame < 3; ++frame) {
                EnItem00_Update(&item.actor, &play);
            }
            REQUIRE(killed == 1 && item.actor.update == NULL);
            enabled = 1;
            item = drop(params[type], 220);
            for (int frame = 0; frame < 1000; ++frame) {
                EnItem00_Update(&item.actor, &play);
            }
            REQUIRE(killed == 0 && item.unk152 == 220 && item.unk14E == 0);
            // Enabling while the old timer is blinking must not freeze an invisible drop.
            item.unk152 = 3;
            item.unk14E = 3;
            EnItem00_Update(&item.actor, &play);
            REQUIRE(killed == 0 && !(item.unk14E & item.unk150));
            enabled = 0;
            for (int frame = 0; frame < 3; ++frame) {
                EnItem00_Update(&item.actor, &play);
            }
            REQUIRE(killed == 1);
        }
    }
    enabled = 1;
    EnItem00 item = drop(ITEM00_RECOVERY_HEART, 220);
    item.actor.xzDistToPlayer = 0;
    EnItem00_Update(&item.actor, &play);
    REQUIRE(given == 1 && lastItem == ITEM_RECOVERY_HEART && item.actionFunc == func_800A6A40);
    for (int frame = 0; frame < 15; ++frame) {
        EnItem00_Update(&item.actor, &play);
    }
    REQUIRE(given == 1 && killed == 1); // collection popup must still finish
    item = drop(ITEM00_RECOVERY_HEART, -1);
    EnItem00_Update(&item.actor, &play);
    REQUIRE(killed == 0 && item.unk152 == -1); // permanent placed-heart sentinel
    item = drop(ITEM00_RECOVERY_HEART, 220);
    item.actor.gravity = -1.0f;
    item.actor.floorHeight = BGCHECK_Y_MIN - 1.0f;
    EnItem00_Update(&item.actor, &play);
    REQUIRE(killed == 1); // out-of-world cleanup still applies
    puts("PASS: 2D/3D drop timeout, live toggles, blink recovery, collection cleanup and placed hearts");
}

static void TestTwinmoldDrops(void) {
    for (int kind = 0; kind < 2; ++kind) {
        EnTanron5 item = { 0 };
        item.actor.params = TWINMOLD_PROP_TYPE_ITEM_DROP_1;
        item.actor.update = DummyActorUpdate;
        item.actor.world.pos.x = 1000.0f;
        item.itemDropType = kind;
        item.timer = 2;
        enabled = 1;
        killed = given = 0;
        for (int frame = 0; frame < 1000; ++frame) {
            EnTanron5_RuinFragmentItemDrop_Update(&item.actor, &play);
        }
        REQUIRE(killed == 0 && item.timer > 50); // both 2D and 3D draws stop blinking
        item.actor.world.pos.x = 0;
        EnTanron5_RuinFragmentItemDrop_Update(&item.actor, &play);
        REQUIRE(killed == 1 && given == 1);
        REQUIRE(lastItem == (kind ? ITEM_MAGIC_JAR_BIG : ITEM_ARROWS_10));
        item.actor.world.pos.x = 1000.0f;
        item.timer = 2;
        enabled = 0;
        killed = 0;
        EnTanron5_RuinFragmentItemDrop_Update(&item.actor, &play);
        EnTanron5_RuinFragmentItemDrop_Update(&item.actor, &play);
        REQUIRE(killed == 1);
    }
    EnTanron5 fragment = { 0 };
    fragment.actor.params = TWINMOLD_PROP_TYPE_FRAGMENT_LARGE_1;
    fragment.actor.world.pos.x = 1000.0f;
    fragment.timer = 1;
    enabled = 1;
    killed = 0;
    EnTanron5_RuinFragmentItemDrop_Update(&fragment.actor, &play);
    REQUIRE(killed == 1); // preserve debris timeout
    fragment.sinkTimer = 39;
    EnTanron5_RuinFragmentItemDrop_Update(&fragment.actor, &play);
    REQUIRE(killed == 2); // preserve sinking debris cleanup
    puts("PASS: Twinmold arrow/magic drops persist and collect; debris still expires");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    TestRegularDrops();
    TestTwinmoldDrops();
    return 0;
}
