# Spin shell readability and particle flicker candidate

Source parent: public cumulative baseline
`5f8966fae4b0810ce86021b3f88814ffd77e0224`, tree
`545292a667083fb835ccd86b078e3aac7a484cd1` (identical recovery source
`86387591cb6fba0adb2528c258c8dadd624136c1`). This is an implemented,
source/native-test-verified candidate. Corrected executable footage and user
visual acceptance remain pending. The two corrections below address separate
observations; neither is evidence that the other is visually accepted.

## Existing resources and shell routing

The matching `ComboShip_Windows_Cumulative_20261024_5f8966fa.zip` contains every
checked Ice, Light, Lightning, Arcane and Dark Energy resource. All 77 selected
resource paths in each delivered `soh.o2r` and `2ship.o2r` match the retained
source bundle byte for byte. These textures are present, including the authored
1024x512 burst pairs; missing raster data is not the identified defect.

The explicit Ice and Light styles bypassed their complete private burst graphs
and instead cloned the currently selected native display lists. That retained
the coarse native shell, secondary-tile UV shifts and native mask combination.
Lightning, Arcane and Dark Energy already used their authored private graphs.

| Selection | Previous burst route | Candidate burst route |
| --- | --- | --- |
| Explicit Ice / Light | Native shell with replaced texture references | Existing private Ice / Light disk and cylinder |
| Native, including Fire / Ice / Light imbues | Clone active native/custom shell | Unchanged |
| Fire / Lightning / Arcane / Astral / Dark Energy | Private style graph | Unchanged |
| All charge draws | Existing native/private charge policy | Unchanged |

Each original native disk/cylinder has 24 triangles; each selected private
Ice/Light disk/cylinder has 128. The private graphs already include their UVs,
texture masks, render state and both cosmetic tint channels. No generator,
bundle, texture bytes, vertex resource or shared native asset was modified.

This route intentionally selects the authored mask equation. The previous
native first-cycle alpha was `T0 + (T1 - 1) * ENV_ALPHA`; the private first-cycle
alpha is `T0 * T1`. Both retain the existing final multiplication by primitive
alpha, so actor fade and cosmetic alpha remain unchanged. The source change
must not be described as preserving the old Ice/Light shell's mask equation.

The actual private burst roots contain only the same segment-08 scroll call
already bound by both native actors (`0x08000001`). They contain no matrix
commands or segment writes, and no additional segment dependency. Their
hashed texture/vertex graphs pass the existing complete closure/cache tests.
Regular/great disks retain their approximately 1500/1650 outer radii; cylinder
outer radii agree with native within one vertex unit after rounding, and their
regular/great half-heights remain 250/500. No actor scale, collision geometry,
hitbox, radius, timing or lifetime branch changes.

## Temporal cause and correction

Both burst actors advance their texture-scroll counter by up to `2 * alpha`
per gameplay tick. That fast counter was also passed directly to particle
positions and model rotations. At alpha 1, one tick advanced Ice orbit by
4.4 radians, Ice yaw/pitch by 3.8/4.6 radians, and sparkle rotation by 4 radians.
The real frame interpolator linearly interpolates the submitted complete MtxF
axes; those large turns can shrink a shard or sparkle between game ticks.
Particle origins also jump substantial distances around the ring.

Only the burst particle motion input now uses `scroll * 0.025`, giving at most
0.05 phase units per tick, matching the existing charge preview's scale. Both
elemental particles and Dark Energy's red sparks use it. Their existing
geometry, textures, colors, alpha, deterministic placement functions and
12/24 total burst budgets remain intact. Charge's 4/8 budget and motion are
unchanged. The native particle texture clock still uses gameplay frames;
the parent ring's original UV scrolling is unchanged.

A separate native-header sampler executes the actual particle builder/model
functions for all 24 great-spin particles at alpha 1, with identity parent and
camera matrices. It compares one native counter step, 0 to 2. Midpoint axis
length is measured as `length((oldAxis + newAxis) / 2)` divided by the mean
endpoint length, matching the frame interpolator's matrix operation.

| Element | Maximum origin travel, previous → candidate | Minimum midpoint axis length, previous → candidate |
| --- | --- | --- |
| Fire | 842.383 → 273.799 local units | 100% → 100% |
| Ice | 2409.963 → 196.436 local units | 21.9821% → 99.7221% |
| Light | 842.383 → 273.799 local units | 41.6146% → 99.8750% |

These are production matrix calculations, not measured screen brightness,
projected area or corrected GPU rendering. Large turn/interpolation collapse
is a source-confirmed defect; its complete perceptual improvement still needs
an equivalent configured gameplay comparison.

## Clip evidence and its limits

The three newly supplied clips were directly inspected through contact sheets
and adjacent native-frame crops. They are 1920x1080 recordings at nominal
60 fps, approximately 16.079, 15.059 and 16.723 seconds long.

- Clip 1 (`20261024194207654`, upload suffix `1792889028177`) shows the gold
  ring and ice accents. Adjacent frames around 5.8 seconds retain the ring's
  texture while shards change position/orientation sharply.
- Clip 2 (`20261024194246726`, suffix `1792889046221`) visibly selects
  Lightning. Around 6.8 seconds, its legible blue/yellow ring remains present
  while Ice accents leap and change size between rendered frames.
- Clip 3 (`20261024194321386`, suffix `1792889141362`) includes equipment
  changes and further elemental spin examples. The solid yellow blade sweep
  is also present without the outer ring, so it was not treated as proof of a
  broken spin-shell texture or altered in this candidate.

The clips belong to the supplied build, not the corrected source candidate.
They support prioritizing temporal stability while retaining the artwork.
They do not isolate every material, depth, lighting or interpolation setting;
no blanket absence of GPU flicker, depth conflicts or other VFX defects is
claimed. The inspected authored-texture sheet is a composited source view,
not a gameplay render.

## Verification and handoff

Only `combo/render/SpinAttackDraw.h`,
`tests/spin_style/render_test.cpp` and this record change. Unrelated regional,
actor, title, held-sword, native archive and gameplay work is excluded.

| Check | Result |
| --- | --- |
| New burst graph regression against previous implementation | RED: Ice submitted cloned native lists instead of its private graph |
| New temporal regression before motion correction | RED: native one-tick particle travel exceeded its bound |
| `python3 -B scripts/diagnostics/run_spin_style_tests.py` | PASS all default gates, both hosts and standalone/Combo native actor TUs |
| Same complete runner with `--sanitize` | PASS ASan/UBSan; leak checks disabled for the executor |
| Regular/great motion regression, Fire/Ice/Light/Dark sparks | PASS: origin travel <350 local units, interpolated midpoint axes retain >98% length |
| Native default cloning, missing-resource fallback, cosmetics, alpha, budgets, combat ownership/damage | PASS existing and added real-header fixtures |
| `python3 -B tests/spin_style/test_bundled_assets.py` | PASS 3 tests; real CMake preparation test skipped because CMake is absent from this shell PATH |
| `git diff --check` on owned source/tests/document | PASS |
| Fresh executable build / corrected GPU clip / visual acceptance | Pending |

Native compile commands used the retained JSON, spdlog and SDL include
directories through CPATH. Existing MM controller-button macro redefinition
warnings remain; they are unrelated to these edits. No test failure is hidden.
Local evidence is under `analysis/spin_readability/`: RED/GREEN and full-suite
logs, delivered/segment/bounds JSON, authored texture sheet, adjacent clip
crops, and native matrix sampler source/results. Original archives were read
without modification and are excluded from publication.

The next decisive check is charge, regular/great release and fade in both
games, with equivalent packs, Alt setting, cosmetics and interpolation FPS.
Compare Ice/Light shell readability separately from all elemental particle
stability, including Dark Energy sparks, pause/resume and scene re-entry.
Preserve the accepted master until that configured result is accepted.
