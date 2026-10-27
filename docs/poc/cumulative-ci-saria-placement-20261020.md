# Cumulative CI correction and authored Saria placement

Baseline: merged PR44, `develop` commit
`63d806b9c4675232e918400e00c0a53277259c8f`, tree
`1039d84e8d99d3edfbb1a30dd0367c76b0fc38d2`.
Candidate branch: `fix/cumulative-ci-saria-placement-20261020`.

## CI failure and bounded corrections

Both PR run `39344892266` and merged-develop run `39345413373` fail
the gate step **2Ship weather, item-color, scene-randomization and
streamed-audio regressions**. The merged job `118376698638` links the
standalone item-color fixture without `NeiGi_ReportBlackMarketModel`,
which the current production item drawers call. The same link error was
reproduced locally before editing.

The fixture now supplies that passive reporting boundary and the native
draw declarations; its isolated Double Defense C++ compilation also sees
the reporting declaration. Game draw code is unchanged by this repair.

Replaying the remainder of the failed step exposed three further fixture
dependencies. These are corrected before asking CI to spend another run:

- The HUD fixture now extracts the complete production cosmetic registry,
  preserving multiline macro arguments and build guards. Its old one-line
  filter cut off the Black Market shelf option.
- The native item-visual fixture observes the dedicated Chateau shimmer
  callback and checks native dispatch plus suppression for a missing model.
  Actual shimmer geometry remains covered by the existing renderer suites.
- The dungeon-key fixture supplies the passive reporting boundary and the
  native MM `GID_CHATEAU = 0x73` enum value used by the shared draw producer.

No production audio, material, item grant, weather or renderer code changes
are part of these fixture repairs.

## Song Giver Saria reference

cor supplied `1 - Termina Field Saria Song Giver.prelude.o2r` as the intended
game placement. SHA-256:
`0c63a6f396cf1648f1707f143672bcea961ca58109450fcfba7a0b0c1d9fab7f`.
The export is unchanged and is not bundled by this source correction.

The project edit and binary room resource identify the seated song giver:

| Field | Authored value |
| --- | --- |
| Scene / room / setup | `Z2_00KEIKOKU` / 0 / 0 |
| Actor / params | En Viewer, ID 28 / `0x7E33` |
| Position | X 975, Y -99, Z 2101 |
| Packed MM rotation | `[7, -30593, 127]` |
| Native yaw | 272 degrees, binang `0xC16C` |

The automatic Termina Field spawn now uses the authored position and yaw
instead of the provisional southwest placement. It retains a floor-existence
probe but preserves the authored seated Y instead of snapping her onto a
different surface. Native `Actor_SpawnEntry` decodes the exported Y rotation
as `(-30593 >> 7) & 0x1FF`, then applies `DEG_TO_BINANG`.

Preserved: authored song-giver placements take precedence; no duplicate
spawn; seated pose; green hex draw; donor/Alt model routing; one-time shared
song claim and native receipt; slot/mode/cutscene/pause/transition guards;
the existing Lost Woods automatic placement and its floor snap.

## Verification and release boundary

- Item-color fixture reproduced RED (undefined reporting symbol), then
  passed all 92 rendering cases and actual-header C/C++ syntax checks.
- The new Saria transform assertion failed on the baseline before the
  production placement edit. The complete forest actor suite then passed,
  including native monkey audio, all Saria poses, resource validation,
  lifecycle/Alt handling, field/solo-MM routing, authored overrides, Epona,
  reward duplication/claim persistence and actual JSON save roundtrip.
- The complete affected MM step from item-color through its final Gfx patch
  command passes. This includes HUD/rainbow, drop lifetimes, unrestricted
  items, item visuals, dungeon-key assets, normal and ASan/UBSan key bridges,
  and all 13 Gfx patch cases.
- Every later gate regression group passed locally: seasons/winter,
  foreign souls, static equipment bridge, magic jars, key/heart cosmetics,
  mask/remains shimmer, mod fonts, render recorder, triangles, texture and
  display-list addresses, vertex batching (normal and sanitized), back
  equipment/sky, transformation cosmetics and young Epona.
- Full repository `run-clang-format.sh` ran with clang-format 14.0.6;
  only the intentional concurrent dungeon-key fixture edits remained in
  its comparison. Those edits were subsequently committed and the tracked
  tree is clean. Asset-collision and whitespace checks pass.
- Local dependency headers were supplied through `CPATH` (real spdlog
  1.12.0 and nlohmann JSON); the default local include path lacks them.
  The hosted gate installs its real dependency headers before these tests.
- Independent review found no Critical or Important issue and independently
  reran item-color, HUD, item-visual and dungeon-key fixtures successfully.

## Platform build follow-up

The first correction was published as PR45 head
`68a9f901e8149a088931bb8a1f5bd2c7bd5fa340`. Hosted run
`39353353156` passed the complete gate in 33m03s, then exposed two separate
platform failures:

- Windows job `118412021255` stopped compiling SoH. The SDK's empty `near`
  macro erased a native-transformation lighting member/local name, and the
  forest renderer passed `__FILE__` to the legacy mutable `char*`
  `Matrix_NewMtx` filename parameter. It never reached the seed tests.
- Linux job `118412021290` compiled both games and all launcher/port targets,
  then the seed smoke test failed loading the game libraries. Both native
  modules reproduced the failure locally: anonymous-namespace
  `FrameInterpolation_RecordOpenChild` / `RecordCloseChild` imports had no
  definitions. Display-list macros declare these helpers at block scope;
  an outer global header alone does not supply linkage in that namespace.

The follow-up renames only `near` identifiers to `fogNear`, uses the existing
legacy filename cast pattern, and anchors both interpolation helper
declarations to each host's native C interface in the four affected forest
and Black Market controllers. Draw behavior, materials, geometry, audio and
Saria's reference placement are unchanged by this follow-up.

The headless loader now reports the specific module and native loader error.
Windows captures its last-error value before stream operations can change it.
Early gate regressions enforce the Windows macro/filename boundaries and
inspect full production rendering objects for exact native C imports.

Follow-up verification:

- Both Windows compile regressions reproduced RED and then passed. Native
  transformation checks cover solo/combined hosts with ASan/UBSan and the
  actual Windows `near` macro expansion; forest draw/routing checks pass.
- All four affected production rendering objects failed the extended ABI
  test before correction; all five rendering objects, including the existing
  passage control, now pass with real native headers/macros.
- Full native SoH, MM and `comborando` build succeeds. Both corrected game
  libraries resolve all relocations; both uncorrected libraries fail actual
  headless loading on the anonymous `RecordCloseChild` symbol.
- Standard CI seed `ci-linux-smoke` exits 0: 198 cross-game placements,
  completable as configured. Door/shop-only CI seed `ci-door-interiors` also
  exits 0 with 200 cross-game placements; 406 advancement candidates and
  407 oracle replays pass the playthrough/hint pare-down.
- Real ELF missing-library, unresolved-symbol and successful-load tests pass;
  Windows last-error preservation reproduces RED then GREEN when stream
  output overwrites the simulated thread error.
- Independent final review found no material issue. Changed production files
  pass clang-format 14 and whitespace checks.

Local game objects used `-O0` after the Release flags to speed diagnosis.
The scratch dependency build suppresses obsolete duplicate libc prototypes
in StormLib's C++ objects; no repository or dependency source was changed
for that workaround. This is local native evidence, not a hosted Release
or native Windows build result.

A new hosted run, complete Windows build/smoke results and configured in-game
visual/interaction acceptance remain pending. No develop merge or
runtime-master promotion is implied.
