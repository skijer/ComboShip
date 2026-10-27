#include "mods/items/logic/item_hylias_grace.h"
#include "tests/test_require.h"
#include <cstring>

CustomItemState gCustomItemState{};
static Vec3f sFairyVelocity;
static int emitted;
static Color_RGBA8 capturedPrim, capturedEnv;

extern "C" {
f32 Rand_CenteredFloat(f32) {
    return 0.0f;
}
f32 Rand_ZeroFloat(f32) {
    return 0.0f;
}
f32 Math_SinS(s16) {
    return 0.0f;
}
f32 Math_CosS(s16) {
    return 1.0f;
}
#ifdef GRACE_PARTICLE_MM
void EffectSsGSpk_SpawnAccel(PlayState*, Actor*, Vec3f*, Vec3f*, Vec3f*, Color_RGBA8* prim, Color_RGBA8* env, s16 scale,
                             s16 life) {
    scale *= 10; // Verify the real MM compatibility macro's native scale conversion.
#else
void EffectSsKiraKira_SpawnFocused(PlayState*, Vec3f*, Vec3f*, Vec3f*, Color_RGBA8* prim, Color_RGBA8* env, s16 scale,
                                   s32 life) {
#endif
    REQUIRE((scale == 300 && life == 12) || (scale == 200 && life == 10) || (scale == 800 && life == 25) ||
            (scale == 600 && life == 20));
    capturedPrim = *prim;
    capturedEnv = *env;
    ++emitted;
}
}

#include "grace_particles.inc"

static void CheckColors(Color_RGBA8 prim, Color_RGBA8 env) {
    REQUIRE(std::memcmp(&capturedPrim, &prim, sizeof(prim)) == 0);
    REQUIRE(std::memcmp(&capturedEnv, &env, sizeof(env)) == 0);
}

int main() {
    Player player{};
    PlayState play{};
#ifdef GRACE_PARTICLE_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
    hgForcedBySpell = 0;
    HGrace_SpawnFairySparkles(&player, &play);
    REQUIRE(emitted == 3);
    CheckColors({ 255, 180, 220, 255 }, { 255, 100, 180, 255 });
    HGrace_SpawnTrailSparkles(&player, &play, 0.0f);
    REQUIRE(emitted == 3);
    HGrace_SpawnTrailSparkles(&player, &play, HGRACE_SPEED * HGRACE_SPRINT_MULT);
    REQUIRE(emitted == 9);
    CheckColors({ 255, 180, 220, 255 }, { 255, 100, 180, 255 });
    hgForcedBySpell = 1;
    HGrace_SpawnFairySparkles(&player, &play);
    REQUIRE(emitted == 12);
    CheckColors({ 255, 220, 100, 255 }, { 200, 150, 30, 255 });
    HGrace_SpawnTrailSparkles(&player, &play, HGRACE_SW97_SPEED * HGRACE_SW97_SPRINT_MULT);
    REQUIRE(emitted == 18);
    CheckColors({ 255, 200, 80, 255 }, { 200, 120, 20, 255 });
    for (int forced = 0; forced <= 1; ++forced) {
        hgForcedBySpell = forced;
        int before = emitted;
        HGrace_SpawnWarpSparkles(&play, &player.actor.world.pos);
        REQUIRE(emitted == before + 16);
        CheckColors(forced ? Color_RGBA8{ 100, 255, 150, 255 } : Color_RGBA8{ 255, 180, 220, 255 },
                    forced ? Color_RGBA8{ 0, 200, 100, 255 } : Color_RGBA8{ 255, 100, 180, 255 });
        before = emitted;
        hgTimer = 10;
        play.gameplayFrames = 12;
        TestCastingSparkles(&player, &play);
        hgTimer = 11;
        play.gameplayFrames = 13;
        TestCastingSparkles(&player, &play);
        REQUIRE(emitted == before);
        play.gameplayFrames = 16;
        TestCastingSparkles(&player, &play);
        REQUIRE(emitted == before + 1);
        CheckColors(forced ? Color_RGBA8{ 150, 255, 150, 255 } : Color_RGBA8{ 255, 180, 220, 255 },
                    forced ? Color_RGBA8{ 50, 200, 50, 255 } : Color_RGBA8{ 255, 100, 180, 255 });
        before = emitted;
        hgTimer = 4;
        TestWarpEnterSparkles(&player, &play);
        REQUIRE(emitted == before);
        hgTimer = 6;
        TestWarpEnterSparkles(&player, &play);
        REQUIRE(emitted == before + 1);
        CheckColors(forced ? Color_RGBA8{ 100, 255, 200, 255 } : Color_RGBA8{ 255, 180, 220, 255 },
                    forced ? Color_RGBA8{ 0, 150, 255, 255 } : Color_RGBA8{ 255, 100, 180, 255 });
    }
    puts("PASS production Grace idle/trail/casting/transform/revert pink; "
         "spell-forced colors and cadence preserved");
}
