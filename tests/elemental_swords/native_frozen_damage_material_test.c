// The runner compiles the full production Actor_DrawDamageEffects function.
// Reuse its native structs/GBI and engine-boundary doubles from EnIce's fixture.
#define ICE_DAMAGE_DRAW
#include "native_ice_material_test.c"
#include "2s2h/GameInteractor/GameInteractor.h"

static RegEditor editor;
RegEditor* gRegEditor = &editor;
static MtxF current;
static int drawAllowed = 1, sounds;
static u16 lastSound;

bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    assert(flag == VB_DRAW_DAMAGE_EFFECT && result);
    return drawAllowed;
}
MtxF* Matrix_GetCurrent(void) { return &current; }
void Matrix_Put(MtxF* value) { (void)value; }
void Matrix_ReplaceRotation(MtxF* value) { (void)value; }
void Matrix_RotateYF(f32 angle, MatrixMode mode) { (void)angle;(void)mode; }
void Matrix_RotateZF(f32 angle, MatrixMode mode) { (void)angle;(void)mode; }
void Matrix_RotateXFApply(f32 angle) { (void)angle; }
f32 Rand_ZeroFloat(f32 value) { (void)value; return 0; }
void* Lib_SegmentedToVirtual(void* value) { return value; }
void Actor_PlaySfx(Actor* actor, u16 sfx) { assert(actor); ++sounds; lastSound = sfx; }

static size_t FrozenDraw(Actor* actor, int marked, int ready, u8 type) {
    swordHit = marked;
    available = ready;
    gfx.polyXlu.p = commands;
    gfx.polyXlu.d = commands + ARRAY_COUNT(commands);
    Vec3f points[] = {{1,2,3},{4,5,6},{7,8,9}};
    const int priorMatrices = matrices, priorQueries = queries, priorRequests = materialRequests;
    Actor_DrawDamageEffects(&play, actor, points, ARRAY_COUNT(points), 1, 1, .8f, type);
    assert(queries == priorQueries + (actor != NULL));
    assert(materialRequests == priorRequests + (actor != NULL && marked));
    assert(matrices == priorMatrices + 2 * ARRAY_COUNT(points));
    unsigned roots = 0, environments = 0;
    for (const Gfx* command = commands; command < gfx.polyXlu.p; ++command) {
        const unsigned op = command->words.w0 >> 24;
        if (op == G_DL) {
            const void* root = roots == 0 ? (marked && ready && actor ? (const void*)custom : gEffIceFragment2MaterialDL)
                              : roots <= 3 ? gEffIceFragment2ModelDL
                              : roots == 4 ? gFrozenSteamMaterialDL : gFrozenSteamModelDL;
            if (root == custom) assert(command->words.w1 == (uintptr_t)custom);
            else assert(strcmp((const char*)command->words.w1, root) == 0);
            ++roots;
        } else if (op == G_SETENVCOLOR && environments < 3) {
            const u32 color = command->words.w1;
            assert((color & 255) == (204 - environments * 30));
            if (marked && ready && actor) assert((color >> 8) == 0xE6FAFF);
            else assert((color >> 8) == 0xC8C8FF);
            ++environments;
        }
    }
    assert(roots == 8 && environments == 3);
    return (size_t)(gfx.polyXlu.p - commands);
}

int main(void) {
    play.state.gfxCtx = &gfx;
    play.gameplayFrames = 11;
    Actor actor = {0};
    const size_t nativeCommands = FrozenDraw(&actor, 0, 1, ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX);
    Gfx native[ARRAY_COUNT(commands)];
    memcpy(native, commands, nativeCommands * sizeof(Gfx));
    assert(FrozenDraw(&actor, 1, 1, ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX) == nativeCommands);
    assert(FrozenDraw(&actor, 1, 0, ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX) == nativeCommands);
    assert(memcmp(native, commands, nativeCommands * sizeof(Gfx)) == 0);
    assert(FrozenDraw(NULL, 1, 1, ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX) == nativeCommands);
    assert(memcmp(native, commands, nativeCommands * sizeof(Gfx)) == 0);
    FrozenDraw(&actor, 1, 1, ACTOR_DRAW_DMGEFF_FROZEN_SFX);
    assert(sounds == 1 && lastSound == NA_SE_EV_ICE_FREEZE - SFX_FLAG);

    const int priorQueries = queries, priorRequests = materialRequests;
    gfx.polyXlu.p = commands;
    Vec3f point = {0};
    Actor_DrawDamageEffects(&play, &actor, &point, 1, 1, 1, 1, ACTOR_DRAW_DMGEFF_FIRE);
    assert(queries == priorQueries && materialRequests == priorRequests);
    assert(sounds == 2 && lastSound == NA_SE_EV_BURN_OUT - SFX_FLAG);
    drawAllowed = 0;
    gfx.polyXlu.p = commands;
    Actor_DrawDamageEffects(&play, &actor, &point, 1, 1, 1, 1, ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX);
    assert(gfx.polyXlu.p == commands && queries == priorQueries && materialRequests == priorRequests);
    puts("PASS full native MM frozen damage draw -> scoped material; identical mesh, steam, alpha, sound, "
         "fallback commands, matrices and ordinary damage routing (ASan/UBSan)");
}
