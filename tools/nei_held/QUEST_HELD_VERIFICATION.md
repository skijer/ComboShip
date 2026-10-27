# Quest held candidate

Baseline: `3da467bc`. The candidate carries authored quest GI meshes into held
presentation. Gameplay, projectiles, collision and saves are outside its scope.
This is implemented and verified offline; gameplay fit and pack configurations
are not accepted or proven by these tests.

| Item | Held route | Canonical fit |
|---|---|---|
| Sand, Tornado, Water, Meteor, Storm, Shadow Scepter | Both hosts, selected wand mode | +Y shaft, native grip `[0,-45.5,0]`, y `-175..175` |
| Elemental Wand | Authored matching resource; physical action draws its selected mode | Same wand fit |
| Sheikah Slate | Both hosts, original separately calibrated poses | +X handle, native grip `[0,45,0]`, y `-48..48` |

Wand GI Z tilt of -14 degrees is undone before native fitting. The reference
legacy leather material has y bounds `-124..33`, center `-45.5`; the new leather
grip has author bounds `-48..22`, center `-13`. Piecewise axial scaling about
that center preserves the native grip and both end points. Crown silhouettes
retain each theme within the old x +/-36, z +/-23 envelope. The conversion
matrix is exactly 1/16, undoing mesh serialization's 16x vertex conversion.
At the existing .12 draw scale, wand length remains 42 world units.

OoT's established wand pose is untouched:
`{0,7.356,-3.218,91.034,180,111.724,.12}`. MM previously had no wand model hook;
its actual `Wand_Draw` callback now also draws the held component after its
unchanged Shadow/Storm/Wind world effects. MM uses the existing Somaria native
palm socket, child `[0,216.22,-4.5]` and adult `[0,328,77]`, with the original
body scale. Canonical +Y becomes wrist +X using Z=-90 degrees. A local Y offset
of 45.5*.12 restores the geometry's native grip center to the socket.

The Slate's legacy top grip spans y42..48, center45. It uses the source tablet
with its carrying handle and sculpted eye, fitted into the original x +/-28,
y +/-48, z +/-5 envelope. Both hosts retain every old pose number and transform
order, including MM's separate forearm-to-hand parent frame. Rune pickups
represent abilities of the same tablet; they are not separate physical devices.

All used source parts are opaque. No synthetic translucent shell, GI turntable
spin or GI shimmer is added. Legacy XLU wand entrypoints remain valid empty
built-in lists so a higher-priority pack can continue to supply that part.

Thirteen original model entrypoints remain the override points in each host's
asset tree. Built-in compatibility lists call the distinct matching meshes.
Their 26 retained OoT/MM native OPA/XLU backups compare byte-for-byte with the
baseline. Generated dependency graphs gate every required display list, matrix,
vertex and texture before selecting the new default or its retained fallback.
An original-path pack override takes precedence independently of missing new
default resources through the owner's archive-provenance predicate. OPA and XLU
are selected separately: an available mod pass keeps its legacy namespace;
its unmodified counterpart uses the pristine retained pass and complete pass
resource graph. A surviving XLU-only mod is drawn in the same calibrated hand
pose even when its native OPA fallback is absent.

The redundant 24-path hand display-list cache was removed: the resource manager
already owns its cache and current Alt selection. Missing-then-present assets,
a session with 32 paths and subsequent re-resolution are covered by production
helper tests; no unrelated equip/input/sound code was changed.

## Checks

```
CPLUS_INCLUDE_PATH=/path/to/nlohmann_json/include python tests/nei_quest_held/run_tests.py
CPLUS_INCLUDE_PATH=/path/to/nlohmann_json/include python tests/nei_held/run_hand_fit_tests.py
python tools/nei_held/verify_assets.py
python tests/nei_quest_held/mm_gi_loader_test.py
python tests/nei_quest_held/mm_gi_boot_mod_test.py
CPLUS_INCLUDE_PATH=/path/to/nlohmann_json/include python scripts/diagnostics/run_nei_gi_tests.py --combo --held
CPLUS_INCLUDE_PATH=/path/to/nlohmann_json/include python scripts/diagnostics/run_mm_nei_tests.py
python tools/nei_held/SOURCE/quest_held.py --install
```

The dedicated suite compiles actual native host drawers and hand helpers. The
MM callback body is extracted directly from production and executed, rather
than reimplemented. It checks six modes, full wrist orientation across 32
child/adult/body-scale frames, measured grip and staff length, the Slate's exact
independent pose, drawn/invalid-mode/missing-wrist gates, and world effects.
274 individually removed new/fallback resource parts verify complete-model
fallback and no partial unsafe draw. Original-path mod fixtures remain selected
when every new default resource is absent. Mod fixtures test presentation
selection at the provenance boundary; real archive provenance tests live with
the root integration, not in this graphics fixture.

Eight matching resources and serialized GLB checkpoints have exact triangle
parity and byte-identical OoT/MM installation copies. The complete current
asset verifier reports 60 GI models and 39 held components (including concurrent
equipment work). Existing OoT hand-fit regression checks also pass. Native MM
headers produce existing BTN macro redefinition warnings in standalone harnesses.

GI mod priority is tested through the production selectors on both hosts:
43 resource-backed OoT callbacks, the progressive Master's distinct Kokiri
placeholder, 19 MM-specific legacy roots including both split sword/shield and
rod passes, shared MM-local roots in foreign descriptors, and base/Alt selection.
Held-only wand overrides deliberately do not disable their authored GIs because
the old wand GI is an inline stand-in. MM's real legacy loader and Fire/Ice GI
callbacks are tested across missing-then-present resources, Alt-only roots, live
toggles and an incomplete translucent pair. Its boot recolor helpers preserve
current mod geometry/materials even after the stock recolor copy is populated.
The complete MM DrawItem.cpp ComboShip translation unit syntax-compiles using
the target's Window/json precompiled-header dependencies. Native MM renderer,
owner-routing and existing use/effect/lifecycle tests pass.

Gameplay remains untested: first/third-person visual fit, custom player packs,
full cast animations, live Alt/base/PAK priority, transitions and frame rate must
be assessed with the actual candidate build and recorded pack configuration.
No master promotion or gameplay acceptance is implied.
