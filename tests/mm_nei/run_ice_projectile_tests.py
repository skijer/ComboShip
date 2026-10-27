"""Execute MM's production Ice Rod flight update and its native effect requests.

Catch reintroduced visible EnIce flight clumps without replacing the update,
RNG consumption, collider dispatch or hit/lifetime transitions under test.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions

source = ROOT / 'mm/mods/items/logic/item_rod_ice.c'
production = functions(source.read_text())
prefix = r'''
#include "mods/items/custom_items.h"
#include "mods/items/logic/item_rod_ice.h"
#include "mods/items/logic/item_rod_common.h"
#include <cassert>
#include <cmath>
#include <iostream>
CustomItemState gCustomItemState{};
int randomCalls, requests, colliderUpdates, hitChecks, destroyed, sounds;
bool hit;
f32 Rand_ZeroOne() { ++randomCalls; return .5f; }
void EffectSsEnIce_Spawn(PlayState*, Vec3f* pos, f32 size, Vec3f* velocity, Vec3f* acceleration,
                        Color_RGBA8* primary, Color_RGBA8* secondary, s32 life) {
    ++requests;
    // The accepted private frost wake must remain visible without native opaque clumps.
    assert(size == 0.0f);
    assert(pos->x == 28 && pos->y == 40 && pos->z == 52);
    assert(velocity->x == 0 && velocity->y == 1 && velocity->z == 0);
    assert(acceleration->x == 0 && acceleration->y == -.5f && acceleration->z == 0);
    assert(primary->r == ICE_ROD_PRIM_R && secondary->b == ICE_ROD_ENV_B && life == 15);
}
void Math_ApproachF(f32* value, f32 target, f32 fraction, f32 step) {
    f32 delta = (target - *value) * fraction;
    *value += std::clamp(delta, -step, step);
}
static void IceRod_DestroySetColliders(RodProjSet*, PlayState*) { ++destroyed; }
static void IceRod_UpdateCollider(ColliderCylinder*, Vec3f*, f32 scale, PlayState*) {
    assert(scale >= .6f); ++colliderUpdates;
}
static u8 IceRod_CheckHit(ColliderCylinder*, Vec3f*, PlayState*, Player*) { ++hitChecks; return hit; }
void Audio_PlayActorSound2(Actor*, u16 id) { assert(id == ICE_ROD_SFX_ICE_LOOP - SFX_FLAG); ++sounds; }
'''
checks = r'''
int main() {
    Player player{}; PlayState play{}; RodProjSet set{};
    // Actual first-person and third-person flight updates share this same path.
    for (int firstPerson : {0, 1}) {
        iceRodFirstPerson = firstPerson;
        set = {}; set.active = 1; set.count = 3; set.timer = 20;
        set.scale = set.targetScale = 2;
        for (int i = 0; i < 3; ++i) { set.pos[i] = {10,20,30}; set.vel[i] = {18,20,22}; }
        set.trail[0] = {1,2,3};
        randomCalls = requests = colliderUpdates = hitChecks = sounds = 0;
        IceRod_UpdateOneSet(&set, &player, &play);
        assert(set.active && set.timer == 19 && set.rotZ == 5000 && set.scale == 2);
        assert(randomCalls == 108 && requests == 18 && sounds == 1);
        assert(colliderUpdates == 3 && hitChecks == 3);
        assert(set.trail[0].x == 28 && set.trail[1].x == 1);
        assert(set.vel[0].x == 18 && set.pos[2].z == 52);
    }
    hit = true; set.count = 1;
    // Avoid flight spark requests here so the fixture's known positions stay exact.
    set.scale = set.targetScale = .3f;
    IceRod_UpdateOneSet(&set, &player, &play);
    assert(set.timer == 0 && set.targetScale == 0 && set.vel[0].x == 0 && set.vel[2].z == 0);
    set.scale = .01f; destroyed = 0;
    IceRod_UpdateOneSet(&set, &player, &play);
    assert(!set.active && destroyed == 1);
    std::cout << "PASS native MM Ice Rod custom-only flight, RNG/allocation cadence, first-person, colliders/hit/lifetime\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-nei-ice-projectile-') as td:
    test = Path(td) / 'flight.cpp'
    binary = Path(td) / 'flight'
    test.write_text(prefix + '\n' + production['IceRod_SpawnIceSparks'] + '\n' +
                    production['IceRod_UpdateOneSet'] + '\n' + checks)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags(), '-include', 'algorithm',
                    str(test), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
