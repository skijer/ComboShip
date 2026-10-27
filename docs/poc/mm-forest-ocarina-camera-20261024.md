# Playable forest: native ocarina cameras in both solo modes

| Field | Record |
|---|---|
| Baseline | Cumulative candidate `e063cd5358e0b53025adcd4362bfba8ca1b2fd89`, public tree `106015b928eaab60e7e814bc500853a7f92363e6`. The missing playing camera is user-reported; no game session was available here. |
| Candidate | `mm-forest-ocarina-camera-20261024`, a bounded source change on that candidate. |
| Scope | Supply missing native ocarina metadata before player initialization in normal combined Lost Woods and the MM-only private forest. Verify the existing native OoT path in OoT-only and combined play. |
| Preservation | Existing arrival, portal, shop, Mirror and pause behavior; native/SoH/standalone-MM scenes, prologue and flashback layers; scene resources, spawn position/heading/params/day bits and save state. |
| Configuration | `COMBO_BUILD`; normal playable slot 0–2, layer zero, scene cutscene index zero, spawn zero and the forest's ordinary entrance. Apply only when the scene has no actor-cutscene entries and its player spawn has `CS_ID_NONE`. |
| Verdict | Implemented and native-source/build verified. Configured gameplay and user acceptance remain pending. No accepted master is promoted by these checks. |
| Recovery | Revert the new helper and its startup include/call to restore baseline behavior; immutable assets and generated bundles were not edited. |

## Cause and change

The prepared `comboship_forest` and `skiejer_forest` scene headers omit command
`0x1B`, the actor-cutscene list. Their player yaw field has low seven bits `127`,
which `Actor_SpawnEntry` normalizes to `CS_ID_NONE`. Consequently,
`Player_Init` → `Play_AssignPlayerCsIdsFromScene` leaves the ocarina player ID at
`-1`. The ocarina branch calls `func_808323C0`, which accepts that missing ID
without queuing or starting a cutscene: Link animates, but no playing camera is
created.

The original `mm.o2r` (SHA-256
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`)
has native ocarina records in Lost Woods, South Clock Town, Termina Field and
Woods of Mystery. Lost Woods entry zero is
`{700, -1, -3, -1, 1, 0, 255, 0, 0, 27}` in native `ActorCutscene` field order.
The helper retains that record's camera, script, priority, length, HUD, sound,
end-camera and letterbox fields. Its omitted successor chain is terminated by
changing `additionalCsId` from `1` to `CS_ID_NONE`. Native player assignment
then performs its existing ocarina priority normalization, `700` → `550`.

`ComboForestOcarina_PrepareScene` installs this static-lifetime entry and a copy
of the player spawn whose cutscene bits select entry zero. `OTRPlay_InitScene`
calls it after scene commands, before `Actor_InitContext`/`Player_Init` and the
subsequent camera focal-actor setup/storage. Basic camera structs already exist
at this point; the helper does not initialize or change them. The native
manager now queues, arbitrates, starts `CAM_SET_ITEM1`/`Camera_KeepOn4`, and stops
the ocarina cutscene normally. Nonempty authored lists are retained wholesale.

`ComboForestArrivalCamera` is unchanged. Its existing release conditions
already relinquish presentation on input, a changed mode/setting, an active
subcamera, pause, transition, or an incompatible cutscene.

## OoT-only and combined OoT host

The user's request to include both solo modes was checked against cumulative
source `5c828367c313cbb0cf3b0e220bc4f72e76107435`, which includes the MM repair
in `023e1840`. OoT has no corresponding missing-list defect. Its actual native
instrument branch calls `Player_SetTurnAroundCamera` with item type `90` for
free play or `91` for actor interaction. `Player_RequestCameraSetting` requests
`CAM_SET_TURN_AROUND`; `Camera_KeepOn4` creates the playing view. The native
playing action and camera handler also perform cancellation and restore the
previous background/follow camera.

All three prepared OoT-only forest rooms retain mesh type `2`, so
`Play_CamIsNotFixed` permits this native path. Their collision camera records
resolve to `CAM_SET_NORMAL0`. No OoT production code, animation or resource
change is needed or included.

The dedicated OoT source harness passes under ASan/UBSan for OoT-only and
combined modes, child/adult Link, both instrument camera variants, all three
prepared room metadata, repeated visits and the actual native playing/cancel
action. Views match the native outdoor control. It also preserves native
fixed-room and courtyard rejection, and runs a standalone OoT compilation.
Animation services, normal transformation-height fallback and clear collision
queries are controlled boundaries. This proves native source behavior, not
configured gameplay or actual rendered collision placement.

## Verification

The new regression failed before implementation with
`FAIL: playable forest supplies a native ocarina cutscene ID`. It now passes
under AddressSanitizer and UndefinedBehaviorSanitizer in combined and standalone
compilations. It exercises real spawn decoding, all ten player-ID assignments,
queue/promotion/start, camera allocation/settings, `Camera_KeepOn4`, and native
stop/update. All five forms and three repeated combined/MM-solo visits match a
native Lost Woods control view. The harness controls resource decoding, actor
allocation and clear collision queries; it is not a rendered game test.

Passing commands:

- `python -B tests/forest_ocarina/run_tests.py`
- `python -B tests/forest_ocarina/run_oot_tests.py` (also called by the registered runner above).
- `python -B tests/black_market/run_scene_spawn_tests.py`
- `python -B tests/lost_woods_entrance/run_portal_camera_tests.py --sanitize`
- `python -B tests/lost_woods_entrance/run_mm_camera_arrival_test.py --sanitize`
- `python -B tests/lost_woods_entrance/opening_forest_test.py --sanitize`
- `python -B tests/solo_games/mm_behavior_test.py`
- `python -B -m unittest discover -s tests/ci -v` (21 tests), plus the real CI dispatcher executing the new registered command through a selected-entry manifest.
- `python -B scripts/diagnostics/compile_solo_forest_core.py --unit mm/2s2h/z_play_2SH.cpp` with the supplied JSON/spdlog/imgui/SDL/thread-pool include roots and a separate output directory.
- Clang-format 14 validation for the new header/fixture and `git diff --check`.

The opening-forest and MM-solo harnesses use those same dependency include roots
through `CPATH`. One initial opening-forest invocation lacked spdlog headers;
the provisioned rerun passed all six cases. Existing scene-startup expectations
now require the one forest ocarina entry while continuing to check cold and
recalled arrivals, clean queues, camera storage, native-header override and
standalone MM behavior. The new native regression is registered in the required
`world` CI group. An independent source review found no Critical or Important
issues; its two minor requests were resolved by adding that registry entry and
removing an assertion that merely assigned pause state without exercising pause
handling. The same CI entry now runs both native hosts, covering MM-only and
OoT-only play without adding a separate gate. The OoT test uses the ordinary
JSON development headers installed by the CI dependency step; it needs no
additional pinned fixture header environment. Actual ocarina pause behavior
remains a runtime check.

## Remaining runtime check

With the cumulative candidate and its normal asset configuration, enter the
combined forest routes, MM-only forest and OoT-only forest. Pull out the ocarina in the playable
rooms, play a few notes and cancel. Confirm the normal playing view, native
follow-camera return, pause/unpause, re-entry and portal/Mirror arrival framing.
Repeat with the user's active Alt Assets configuration. Also retain a native
outdoor ocarina control in each host. No such gameplay result is claimed here.
