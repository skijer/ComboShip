# MM Black Market arrival crash — October 21, 2026

| Field | Record |
|---|---|
| Baseline source | PR #48 head `be657713ad572813b36f5095ce7404832a31bc85`, tree `c26423908e1234e11da25edca2c28dd9c5265d22` |
| Observed executable | Windows PR merge `03c928e18670c70023bf883d7553b44ef480f71f`, same source tree; built October 21 |
| Candidate / recovery | `fix/black-market-arrival-20261021`, direct child of the baseline; retain this recovery branch when adding the correction to PR #48 |
| Scope | Correct only the known legacy MM market player spawn during scene initialization |
| Preservation | Source scene/room/collision/material bytes, cached spawn, position, parameters, player form, access checks, keeper, stock, ordinary MM scene chains and OoT source |
| Verdict | Reproduced in a native sanitizer fixture, corrected, production scene-loader object compiled; configured gameplay and user acceptance pending |

## Evidence and cause

`Fleet of Harkinian(20261021-055321).log` SHA-256:
`fc674df8fc640bd5634e1857a7f065e2f6b2eb1ece21c13a87fe59412f325474`.
`Fleet of Harkinian-crash(5).dmp` SHA-256:
`d0bb44e9392c6cea4028b4870c5182ac27fdede930d6279fe8d98178de788b5c`.
The attachments remain unchanged; process memory is not published with source.

At log time `00:53:00.045`, `BlackMarketDoor` reports `native_open`, `entered`,
no failed checks, all three gates true, scene 101, room/layer zero, human form,
slot zero, seed 2255345699 and eight stock entries. At `00:53:00.537`, MM starts
entrance 4608 (`0x1200`). At `00:53:00.839`, it reads address `0x4` from a null
base in `2ship.dll`, RVA `0x67FC64`.

The dump's captured instruction bytes match player cutscene assignment: a
16-byte entry stride, camera ID at offset 4, linked ID at offset 8, global
cutscene threshold 120, and ocarina camera `-3` priority adjustment 700 to 550.
This is instruction/source matching, not PDB symbolization. The source functions
are `Play_AssignPlayerCsIdsFromScene` and its inlined cutscene lookup.

The checked-in MM market scene contains player row
`(0, -11, 0, 381, 0, -32768, 0, 3839)` and a zero-entry actor cutscene list.
The rotation is an OoT raw half-turn. MM's actual `Actor_SpawnEntry` instead
decodes its upper nine bits as 256 degrees and its lower seven as cutscene 0.
Player initialization follows that nonexistent entry and dereferences null.
The preceding controller/resource tests did not execute this initialization path.

## Correction and verification

After all scene commands, before player initialization, copy the recognized raw
market spawn and pack 180 degrees with MM's no-cutscene sentinel 127. Its normal
decoded yaw is then the intended raw half-turn. The private-scene and legacy
value guards preserve other scenes and already authored MM spawn chains. A copy
keeps archive/cache bytes intact. Runtime normalization also covers older scene
archives that override the bundled resource, without requiring an extra mod.

- New `tests/black_market/run_scene_spawn_tests.py`: exact bundled row and empty
  list, actual scene-init, actor-entry decode, cutscene manager and player
  assignment bodies with production engine types. The resource command and actor
  allocation boundaries are adapters; there is no renderer or game session.
  Before the correction, UBSan reports member access within null `ActorCutscene`.
  Afterward, ASan/UBSan pass 15 market loads across all five forms, cached-byte,
  pose/parameter/form preservation, null spawn, valid native camera chains,
  ordinary scenes, and compilation without `COMBO_BUILD`.
- Compile the complete modified `mm/2s2h/z_play_2SH.cpp` with real engine/build
  headers, C++20, `COMBO_BUILD`, `MM_BUILD_DLL`, `F3DEX_GBI_2` and `FMT_HEADER_ONLY`:
  passed. Object SHA-256
  `7e8e5b97110b59272bb1946ca665f27100025a78d0aef78171d59cdd7e6b8187`.
- Native market readiness, legacy-cache and both hosts' native transaction,
  entry/return/reload/respawn suites: passed.
- Existing October 20 preflight plus the new spawn test: entry, real-resource
  controller cycles, decoder, particles, teardown and palette checks passed.
  The forest preparation suite initially stopped because CMake was absent in this
  fresh workspace; after installing it, the full 31-test suite passed with two
  optional-input skips. All nine preflight commands have passing results.
- Clang-format 14.0.6, whitespace checks and independent code review: passed.

The new regression command is included in the existing pre-build CI gate.
Executable source changes require a rebuilt `2ship.dll`; no archive bytes were
changed. Hosted Windows/Linux builds, installed archive/load-order behavior,
keeper rendering, purchases and in-game entry/exit/re-entry remain unverified.
No accepted master is promoted.

Restore the pre-correction source from `be657713ad572813b36f5095ce7404832a31bc85`.
Restore the candidate from its named recovery branch. All cumulative NPC, dialogue,
forest, elemental, camera and particle integrations remain in its parent history.
