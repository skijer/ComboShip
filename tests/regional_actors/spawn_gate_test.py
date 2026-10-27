#!/usr/bin/env python3
"""Execute the unchanged native spawn prefix through its actual clear-room gate."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'mm/src/code/z_actor.c').read_text()
start = source.index('Actor* Actor_SpawnAsChildAndCutscene(')
end = source.index('    actor = ZeldaArena_Malloc(profile->instanceSize);', start)
prefix = source[start:end] + '    return (Actor*)1;\n}\n'
fixture = r'''
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboRegionalActors.h"
#include <assert.h>
#include <stdio.h>
static ActorProfile testProfile;
static int cleared, objectAvailable = 1;
ActorOverlay gActorOverlayTable[ACTOR_ID_MAX];
ActorProfile* Actor_LoadOverlay(ActorContext* c, s16 id) { return &testProfile; }
s32 Object_GetSlot(ObjectContext* c, s16 id) { return objectAvailable ? 0 : OBJECT_SLOT_NONE; }
s32 Flags_GetClear(PlayState* p, s32 room) { return cleared; }
void Actor_FreeOverlay(ActorOverlay* o) {}
static s16 ComboForestSkullKid_ObjectId(s16 id, s32 params, s16 native) { return native; }
/* PREFIX */
int main(void) {
    PlayState play = {0};
    testProfile.id = ACTOR_EN_RAIL_SKB;
    testProfile.type = ACTORCAT_ENEMY;
    for (cleared = 0; cleared < 2; ++cleared) {
        for (unsigned params = 0; params < 65536; ++params) {
            int reserved = params == 0x7E0F || params == 0x7E1F || params == 0x7E2F ||
                           params == 0x7E3F || params == 0x7E4F;
#ifndef COMBO_BUILD
            reserved = 0;
#endif
            int allowed = Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, testProfile.id,
                0, 0, 0, 0, 0, 0, params, 0, 0, NULL) != NULL;
            if (allowed != (!cleared || reserved)) {
                fprintf(stderr, "clear=%d params=%04X allowed=%d expected=%d\n",
                        cleared, params, allowed, !cleared || reserved);
                return 1;
            }
        }
    }
    cleared = 1;
    objectAvailable = 0;
    assert(!Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, ACTOR_EN_RAIL_SKB,
        0, 0, 0, 0, 0, 0, 0x7E0F, 0, 0, NULL));
    objectAvailable = 1;
    testProfile.id = ACTOR_EN_TEST;
    assert(!Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, testProfile.id,
        0, 0, 0, 0, 0, 0, 0x7E0F, 0, 0, NULL));
    testProfile.id = ACTOR_BOSS_05;
    assert(Actor_SpawnAsChildAndCutscene(&play.actorCtx, &play, testProfile.id,
        0, 0, 0, 0, 0, 0, 0, 0, 0, NULL));
    puts("PASS actual native spawn gate: all Stalchild params, clear/uncleared, missing object, boss exception");
}
'''.replace('/* PREFIX */', prefix)
with tempfile.TemporaryDirectory(prefix='regional-spawn-') as directory:
    path = Path(directory) / 'spawn.c'
    path.write_text(fixture)
    for combo in (False, True):
        flags = ['-std=gnu11', '-w', '-DF3DEX_GBI_2']
        if combo:
            flags.append('-DCOMBO_BUILD')
        includes = ['mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'mm', 'mm/2s2h',
                    'libultraship/include', 'combo', 'combo/menu']
        subprocess.run(['cc', *flags, *['-I' + str(ROOT / p) for p in includes],
                        str(path), '-o', directory + '/spawn'], check=True)
        subprocess.run([directory + '/spawn'], check=True)
