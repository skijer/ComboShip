# Rigid equipment gameplay model candidate

Baseline: `3da467bc`. Scope: carry the already authored equipment GI art into
existing rigid gameplay frames. The user explicitly retained custom outfits and
modded models. No gameplay state, damage, ownership, collision, native grip
matrix, sheath state, shield surfing, or axe throw behavior is changed.

| GI identity | Gameplay representation | Coverage |
|---|---|---|
| Divine Shield | Legacy custom shield frame | Hand and back |
| Sheikah / Kite Shield | Legacy custom shield frame | Hand, back, and surf board; native surf child ratio retained |
| Shield of Ikana | Existing custom shield hand/back frames | Hand and back; native MM mirror shield frame retained when a mod owns either native representation |
| Cane of Byrna | Measured Somaria/Byrna Y axis, -416 to +223 | Held; original trail/charge frame retained |
| Trident | Phantom Ganon lance frame | Held; authored grip seated at native Z=2049.5, tip at original Z=8520; native collision/trail matrix retained |
| Four Sword | Existing Four Sword X-axis blade/hilt frame | Held and clone draw dispatch; native sheath remains owned by the original player renderer |
| Iron Knuckle's Axe | Original 69-vertex inline axe frame | Held, thrown EnBoom actor, and OoT Gerudo demon draw |
| Native Kokiri/Razor/Gilded/Master/True Master/Biggoron/Great Fairy Sword GIs | Original native or selected custom equipment paths | No new gameplay override; existing Din/PAK/custom equipment priority retained |
| Magic Cape | Native dynamic cloth | Retained |
| Champion's / Spirit / Sages' tunics | Original player skin/tint paths | Retained; GI shells are not attached to an animated player |
| Pegasus Anklet / Climb / Roc Boots | Existing native foot presentation | Retained |
| Pendant of Memories | Existing passive item | No new body attachment |

The current OoT baseline explicitly removed its Pegasus torus/wing attachment
(`soh/src/code/z_player_lib.c`, `extended_equipment.c`, `equip_pegasus.c`). MM
retains dormant torus/wing geometry and pendulum updates, but no current player
foot callback invokes `ExtEquip_DrawAnklet`. Both states remain unchanged.

Eight rigid components live in `objects/nei_held_redesign`. Their fitted source
models reuse the approved meshes, remove GI display lean, and derive sizing from
the actual legacy resource geometry or established weapon frame constants.
Their full graphs contain 6–14 resources each. Selection checks that complete
graph before queueing anything; Four Sword selects both new parts atomically.
Incomplete graphs retain the original representation.

Legacy resources owned by ordinary base-path mods or active Alt mods win over
these built-in defaults. Canonical deferred legacy paths preserve those assets
across Alt changes rather than reusing a cached pre-toggle native pointer.
Foreign resources use the registered owning game (`@oot:` / `@mm:`). The native
player renderer's existing PAK/custom equipment selection order is unchanged.

Verification:

- `python -B tests/nei_held/run_equipment_tests.py`: real OoT/MM graphics ABI,
  actual source-extracted native getters, original fallback, graph incompleteness,
  absent archives, atomic Four Sword parts, base-path and active Alt mod priority,
  owner routing, and separate Ikana hand/back deferred wrapper lifetime.
- `python -B tools/nei_held/verify_assets.py`: serialized resource/GLB geometry,
  winding, materials and exact parity for all current GI/held components.
- Both actual `En_Boom` source files pass host-header C syntax checks.
- MM back-equipment and OoT Din progressive hand diagnostic tests pass.

A standalone compilation of `extended_equipment.c` does not supply its native
unity-build animation/player compatibility context and reports existing alias
errors; compile the complete player unity translation unit instead. The parent
integration task owns those checks and the archive-provenance policy tests.

All previews and these checks are offline. Actual child/adult pose appearance,
material transitions, alternate asset packs, surf board alignment, clone visuals,
and throw playback still need a playable ComboShip build and visual acceptance.
This candidate is implemented and statically verified, not runtime accepted.
