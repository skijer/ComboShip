# Confirmed seasonal palettes candidate

Baseline: `candidate/oot-ground-tint-polish-20261026` at
`fa81f7926038f2aa85c7f3e73c96fec043168b66` (PR65). Candidate branch:
`poc/confirmed-season-palettes-20261026`. User authorized all palette choices
and source implementation, including OoT grass trim and Kakariko windmill
backdrop. Runtime appearance of this candidate remains untested.

## Global constraints

Keep existing paths, native/Alt/private ownership, masks, atlas bounds,
resource lifetimes, geometry, collision, UVs, source texture bytes, alpha,
bark/stumps/flowers exclusions, actor behavior, guards, summer/off restoration,
and weather/audio unchanged. Do not increase ground exposure or shadow lift.
Change only seasonal palette choices and autumn actor tint mode. Autumn canopy/bush/grass actor
mode 2 preserves mean intensity; spring/winter retain existing mode 3. Native
impact-leaf effects retain their existing legacy hue and mode 1.

| Role | Autumn | Spring | Winter |
| --- | --- | --- | --- |
| Ground and wall grass trim | Copper Orange `#F3974F` | Sakura Blush `#F6C1D9` | Glacial Blue `#DCECF5` |
| Tree actors, replacement foliage, overhead and forest backdrops | Scarlet `#EF484C` | Sakura Blush `#F6C1D9` | Mint Frost `#E2EEE9` |
| Existing Reloaded middle/lower forest wallpaper role | Maple Red `#F26846` | Sakura Blush `#F6C1D9` | Mint Frost `#E2EEE9` |
| Cuttable bushes/grass actor foliage and OoT Understory | Maple Red `#F26846` | Sakura Blush `#F6C1D9` | Glacial Blue `#DCECF5` |
| Moon Tree imported leaf/fern and existing Termina private surrounding shrubs | Crimson `#DE496B` | Sakura, existing treatment | Glacial/Mint, existing role |
| Kakariko windmill backdrop | Scarlet `#EF484C` | Sakura Blush `#F6C1D9` | Mint Frost `#E2EEE9` |

Retain role-specific existing blend alpha and exposure. A hex specifies hue,
not a replacement of textured, lit pixels with a flat colour. Reloaded's exact
private texture name is unknown; preserve its existing draw-owner wrapper and
characterize that wrapper with arbitrary private images. A native texture pair
is evidence for that pair, not proof of the unidentified private image.

## Task 1: OoT palette and trim

Own only `soh/soh/Enhancements/Graphics/SeasonFoliageTint.h`,
`SeasonFoliageActor.h`, `SeasonFoliageMaterials.inc` (hue-only edits), and related
`soh/tests/season*` test files. No routing or mask edits are permitted. Apply table hues to
default and explicit private autumn palette roles. Preserve all existing lightScale/shadowLift and alpha.
Use Scarlet for Canopy and Maple for Understory/non-canopy actor foliage.
Winter Canopy is Mint; Ground/Understory/non-canopy actors use Glacial. The
Understory cohort includes shuffled bush/grass texture copies and therefore
must match the non-canopy actor hue, avoiding Scarlet-over-Maple treatment.
Kakariko's existing `custom/prelude/spot01_scene/recipe14_tex0` Canopy binding
must inherit Scarlet; retain it exactly. Add/update focused behavioral tests
first, observe expected RED, then implement and run texture/binding/cohort,
ground polish and actor suites. Pin identical Ground treatment on grass and
grass-trim atlas paths with protected stone unchanged. Preserve all negative
controls and restoration checks. Commit only owned files; report evidence.

## Task 2: MM palette, actors and Moon foliage

Own only `mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp`, the existing
seasonally tinted `z_en_wood02.c`, `z_en_kusa.c`, `z_obj_grass.c`,
`z_obj_grass_carry.c`, `z_obj_tree.c` (existing branching-tree canopy tint),
`AutumnSceneMaterials.inc` (role metadata only; existing
paths/order/regions/scene gates unchanged), related `mm/tests/autumn*` test files and (only if
essential) corresponding diagnostic runners. Apply table hues by existing
palette role. Preserve legacy role identifiers where selectors depend on
them. Keep spring blend alpha and all scene normalization arithmetic. Preserve
the exact native/Alt/private target sets and ForestListColor draw wrapper.
Reloaded primary colour role (`FF9F91`) becomes Maple; paired mask stays intact.
Moon's existing metadata-selected foliage and existing private Termina shrub
materials become Crimson only in autumn, using their established ownership.
Native cuttable actor and shuffled reward-model leaf children use Maple;
protected flower/stem children remain untouched. Autumn actor mode is 2,
spring/winter 3. Preserve native tree ENV and impact-leaf hue/mode-1 effects exactly; isolate
the confirmed canopy hue to its scoped grayscale treatment. Do not
add eligibility to carry actors or green Wood02 bushes. Write/update focused
behavioral tests, observe expected RED then implement; preserve comprehensive
existing negative tests. Run MM tree, grass, surface texture, and scene tests
with actual Moon pack and POC4 private brush fixtures if available. Commit only
owned files; report evidence and any unavailable runtime/configuration checks.

Task 2 implementation boundary: an explicit autumn Crimson role marker on the six existing Termina shrub entries (021650, 0216D0, 0218D0, 034898, foliage_poc3/leaves_rgba and stems_rgba) is permitted. Preserve their legacy role colours for spring/winter classification. Do not reclassify forest wallpapers 034098 or 037098 as Moon foliage. Root verified baseline tree, grass, surface and scene suites including actual Moon and POC4 fixtures; all passed before Task 2. Header dependencies and sparse actor dependencies are now present.

ObjTree clarification: its existing branching-tree canopy formula also receives Scarlet autumn with mode 2 and Mint Frost winter with existing mode 3, while spring remains Sakura mode 3. Preserve its split trunk/canopy drawing and scene/story/native/Alt eligibility. Include its existing actor suite assertions in RED/GREEN evidence.

## Task 3: Review and recovery (controller)

Review task diffs and final combined diff against this spec, record focused
test evidence, verify unchanged routing/masks and accepted input hashes. Keep
baseline/master refs unchanged. Commit bounded candidate changes and create a
deterministic recovery archive with baseline and candidate files, binary git
patch, exact refs/trees, input identities, test logs, and restoration steps.
Independently restore and compare file hashes before saving the handoff.
Deliver source/static verification separately from a native executable build
or configured in-game proof. Do not merge or publish implicitly.


## Verification and limits

The candidate retains the baseline's ground exposure and masks. Focused
OoT texture/binding/599-cohort/ground and native actor checks passed; actual
Kakariko export textures were included, with ASan/UBSan checks. MM native
canopy and grass actor suites, the real POC4 mixed atlas, and scene suite passed.
The scene suite covers 19 hash-matched native Termina materials in native/Alt
and both namespaces, five Moon leaf/fern batches (1,271 triangles), six actual
POC4 brush roots (7,455 triangles), and arbitrary private-image wrapper controls.
All 567 frozen MM case identities/triangle classifications remain unchanged;
the full 567-case optional archive run is unavailable without the full native
MM archive. Exact private Reloaded image identity and configured visuals remain
unverified. No full executable was built and no master/develop merge occurred.
Recovery checkpoint includes exact baseline/candidate source, patch, input
hashes, test reports/logs and an independent patch-restoration verifier.
