# OoT actor foliage tint bridge — 2026-10-13

Status: implemented and focused checks passed. Platform build and live game
appearance are separate acceptance gates. This is a ComboShip PR #41 candidate;
no develop/master promotion or game-runtime acceptance is implied.

## Recovery and baseline

The interrupted `ComboShip Weather Tinting` chat reported passing actor checks
and an identified Hyrule Field target, then stopped during its Kokiri Forest /
Lost Woods coverage audit. Its unpublished working directory was unavailable.
This correction completes the same approved scope from the actual published
ComboShip head `63ec02733b048b87e384504b7d902c537ad2078e`, preserving its SoH 9.3,
MM, weather, NEI, audio, equipment, dialogue and CI diagnostic changes.

Cor supplied `!Hyrule Field - FINAL(1).o2r`. It is byte-identical to the earlier
`!Hyrule Field - FINAL.o2r` reference: 194,513,783 bytes, SHA-256
`6a3c2662e03598a1b3aa4eaa6d421466dac9d0a295f121af0d5c7d864b5d49ad`.
Both native and Alt namespaces contain the exact ground target
`scenes/shared/spot00_scene/s00_yuka_01_78B00___Copie`.

The archive remains unchanged. Geometry, collision, waterboxes, headers,
actor placement, material identity, UVs and scrolling are outside this correction.

## Corrected paths

- Native En_Kusa types 0/1/2 now scope Spring blush, Autumn amber and Winter
  frost around their selected opaque foliage display list. Cut stumps keep
  the original draw. Lift/throw/regrowth/update behavior is unchanged.
- En_Wood02 scopes the translucent canopy after the opaque trunk. Native
  impact leaves and black shrubs remain unchanged; ordinary green shrubs use
  the subtle grass palette. Hook-emitted randomizer crates are outside the tint.
- Draw scopes enclose the resource identity selected by the normal renderer.
  Installed native/Alt model replacements still require live rendering proof.
- Shuffled grass/blades and leaves have four separate pixel-targeted keys:
  `gameplay_field_keep/BushTex.rgba16`, `gameplay_field_keep/LeafTex.rgba16`,
  `gameplay_keep/gGrassBunchTex.rgba16`, `gameplay_keep/LeafTex.rgba16`, each
  under `objects/`. Reward flower/marker materials are not targeted.
- Ordinary actor texture-copy entries were removed so they do not receive both
  the pixel copy and the draw tint. The independent cut-leaf fragment target remains.
- Added the exact Hyrule Field ground identity and 17 missing Kokiri Forest /
  Lost Woods grass, leafy trim, ivy and mixed wall/log atlas paths. Dead foliage,
  water and greenish wooden fences/bridges are excluded. Ground remains native
  during Spring; understory uses the existing 32/255 blossom blend.

The scene/isolated-texture table now contains 103 identities. Spring canopy
remains `F6C1D9` at 224/255 blend; grass remains subtle at 32/255. Actor tinting
uses the existing MM renderer modes: Spring mean normalization, Autumn grayscale
palette and Winter frost normalization. No gameplay RNG, save or Rod ownership
is modified.

## Evidence

- The actor fixture failed on the missing seasonal scope before implementation
  and passes afterward, including actual complete native C translation-unit
  syntax, palette commands, reset, trunks, stumps, leaves and hook boundaries.
  It exercises the production draw bodies and real GBI structs; its GPU boundary
  records commands rather than executing native/Alt materials.
- Expanded production texture-cache checks failed on missing Hyrule Field and
  forest targets before the table changes. Native/Alt bindings, immutable-source
  transitions, Summer/Native restore, external reload guards and retirement pass.
- The production pixel conversion passes on 41 exact serialized OTEX resources
  from the supplied Hyrule archive and Djipi Scenes Map20 / Textures21. Source,
  alpha, non-green bark/stone/flower pixels and texture metadata remain exact.
  Source paths and payload hashes are recorded in
  `oot-foliage-targets-20261013.json`.
- Normal and ASan/UBSan actor/foliage checks pass. Local LeakSanitizer is disabled
  because this execution environment cannot inspect `/proc` tasks; address and
  undefined-behavior instrumentation remain active.
- Existing real seasonal controller/native weather-tag, lighting, sky,
  saved-Rod-state and rain checks pass. Existing particle/winter/summer renderer
  ASan/UBSan checks and exact four-leaf asset hashes pass.
- Independent read-only review found no blocking or important defect. Its
  command-stream-versus-rendering limitation is reflected above.
- Full local CMake configuration stops at the unavailable SDL2 development
  package. Full application/MSVC build status belongs to the hosted candidate
  runs; no local full-build success is claimed.

Reproduction from the repository root:

```bash
python3 -B scripts/diagnostics/run_soh_season_actor_tests.py --sanitize
python3 -B scripts/diagnostics/run_soh_season_foliage_tests.py /tmp/soh-foliage --sanitize
python3 -B scripts/diagnostics/run_soh_season_weather_tests.py
python3 -B scripts/diagnostics/run_soh_season_particle_tests.py --sanitize
```

Where spdlog headers are not installed, pass `--spdlog-include <include-path>`
to the foliage runner. Optional actual-pack checks use `--texture-fixtures`
with a JSON array of `[serialized-OTEX-file, surface-index]` pairs (Ground=0,
Canopy=1, Understory=2); authored pack bytes are not committed.

## Packaging and smallest live test

This correction changes code and targeting, with no changes in either custom
asset tree. Keep the existing Hyrule Field / Djipi scene archives and use the
updated executable with its matching bundled port archives. No replacement
scene archive or additional seasonal override pack is required.

On the exact candidate build, compare Spring / Summer / Autumn / Winter / Native
in Hyrule Field, Kokiri Forest and Lost Woods with the installed Djipi packs and
Alt Assets enabled. Inspect ordinary/lifted/shuffled grass, conical/oval/adult
trees, original trunks, cut stumps and reward markers. Verify forest grass/trim,
scene re-entry and that Summer/Native restore the source. A focused Alt-off
actor comparison remains useful. Record build, pack order, age, scene/room and
relevant randomizer settings with any runtime evidence.
