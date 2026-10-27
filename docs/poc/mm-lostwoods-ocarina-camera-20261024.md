# MM Lost Woods: retain the ocarina camera through Mirror arrival

| Field | Record |
|---|---|
| Baseline | Local `78d3e2aab95265b1c33209051850a15cfeb229d0`, tree `23bb1d9a650aee398d60378c7a682e6cea2021fb`; published equivalent `04dfcb9a5b985e3bfcc07581b2a91358a5f3f66d`. |
| Candidate | Bounded followup on that tree; only `combo/gameplay/MagicMirrorRuntime.inc.h` changes production behavior. |
| Scope | Preserve the already prepared MM player-ocarina chain when Mirror arrival changes the packed spawn heading. |
| Preservation | Fresh cap-facing heading, stationary arrival, cached scene/spawn bytes, save/respawn state, other actor cutscenes and priorities, native camera/HUD ownership, OoT arrival, all other cumulative repairs and original archives. |
| Evidence | Directly inspected user clip; native source RED/GREEN under ASan/UBSan; native MM/OoT adapter compilation; independent review and independent native replay. |
| Verdict | Implemented and native-source/build verified. Installed gameplay, the user's exact Alt configuration and user acceptance remain unproven. No accepted master is promoted. |
| Recovery | Revert this bounded production-file change and its associated tests/document; original archives and saves were not modified. |

## Supplied failure and source identity

The local clip `MedalTVScreenRecording20261024201715696-trim-1792891067164.mp4`
is 12.267 seconds, SHA-256
`4eead2e7d4c3101ce6efb524ef743e064f2cd1ecc0427f22b6e638a37b14e75f`.
It shows Link on the stump, the instrument equipped from pause, a wide camera
while the Stop action is displayed, and only the sword/B icon remaining after
cancellation while Link can walk. The stump suggests a Mirror arrival, but
neither that route nor the clip's exact build/settings can be established from
the recording alone.

The earlier metadata repair `023e1840` is already present in both this baseline
and local `86387591cb6fba0adb2528c258c8dadd624136c1`, the source equivalent of
prior public cumulative `5f8966fa`. Its startup include/call is also present.
The new report therefore cannot be dismissed as an absent source repair. This
record does not identify the newest clip as either cumulative build, nor as the
newer baseline published after the clip was supplied.

## Confirmed native cause

`OTRPlay_InitScene` first calls `ComboForestOcarina_PrepareScene`, which adds
the missing one-entry native unscripted ocarina chain and changes the copied
spawn's low seven yaw bits from sentinel `127` to chain ID `0`.

Later, immediately before `Actor_InitContext`, actual
`MagicMirrorRuntime::PrepareArrival` replaces that copied spawn's yaw with
`Landing`'s packed heading plus sentinel `127`. Native `Actor_SpawnEntry`
normalizes that sentinel to `CS_ID_NONE`. The real
`Play_AssignPlayerCsIdsFromScene` used by `Player_Init` consequently leaves
`PLAYER_CS_ID_ITEM_OCARINA` at `-1`, despite the scene manager still having the
correct one-entry chain. `func_808323C0` accepts the missing ID, so instrument
play proceeds without allocating the native `CAM_SET_ITEM1` subcamera.

The same lost chain explains the HUD symptom. Real
`Message_DisplayOcarinaStaffImpl` requests `HUD_VISIBILITY_B_ALT`, then restores
the saved `gSaveContext.hudVisibility` value while its alpha request remains
pending. Without a new subcamera, `sCameraHudVisibility` stays cached at
`HUD_VISIBILITY_ALL` (`50`). Real `Camera_UpdateInterface` sees the normal
camera request already matching that cache and never reissues the ALL request.
The native alpha updates leave the HUD hidden except B after actual
`Player_Action_63` cancellation. On the healthy path,
`Play_CreateSubCamera` invokes `Camera_Init`, resetting the cache and allowing
the native follow-camera return to restore the HUD.

The old forest fixture never called actual Mirror arrival. The separate Mirror
fixture correctly expected sentinel `127` for its metadata-free scenes. Both
could pass while their real integration lost the ocarina chain.

## Bounded change

Inside the existing MM-only validated forest-arrival path, keep source chain
ID zero only when the current spawn is zero, the scene has exactly one entry,
the source already selects zero, and the entry is the native unscripted,
terminal `ITEM_OCARINA` camera. `Landing`'s newly computed high nine heading
bits remain intact. Missing, foreign, scripted, changed/truncated lists and
other source IDs retain the previous sentinel behavior. OoT keeps its binary
yaw behavior.

There is no forced camera setting or global HUD reset. Native queue priority,
subcamera allocation, playing view, cancellation and HUD restoration remain
the owners. `FinishArrival` restores the prepared scene-spawn pointer; an
independent source trace confirmed the real `MagicMirrorPlayer_Arrive` idle
handoff does not rewrite cutscene IDs, cameras, scene metadata or HUD state.

## Native reproduction and verification

Before changing production code, the integrated replay failed with:

```text
Mirror/native spawn: mode=0 actor.csId=-1 ocarina.csId=-1 scene.csCount=1
native lost-chain reproduction: camera=0 HUD cache=50 health=0 A=0 C=0 B=255
FAIL: Mirror arrival must preserve the prepared ocarina camera and restore the HUD
```

After the bounded change, combined mode `0` and MM-solo mode `2` both retain
actor/ocarina IDs `0`. The same real staff/cancel/cache path restores hearts,
magic, A, all C/D-pad buttons, minimap, Start and B. A deliberately missing-chain
negative control still reproduces the original wide-camera/sword-only-HUD
state, proving the test is sensitive to both reported symptoms.

The expanded registered forest runner covers five forms, ordinary and Mirror
arrivals in both destinations, repeated play/cancel without reloading, three
scene visits, native priority arbitration and seven rejected-chain cases per
destination. It decodes all six prepared MM room payloads and verifies they
contain no actor-cutscene-list command; actual native room-request completion
then preserves the scene/player chain through rooms `0/1/2/0`, with playing and
HUD restoration checked after each completion. The existing authored native
Lost Woods control, scene/prologue guards, OoT-only/combined child/adult control
and standalone MM/OoT compilations also pass.

Passing checks:

- `python -B tests/forest_ocarina/run_tests.py` (ASan/UBSan, MM and OoT).
- `python -B tests/magic_mirror/run_tests.py` (ASan/UBSan, both native hosts).
- `python -B tests/magic_mirror/run_integration_tests.py` (three tests).
- `python -B tests/lost_woods_entrance/run_portal_camera_tests.py --sanitize`.
- `python -B tests/lost_woods_entrance/run_mm_camera_arrival_test.py --sanitize`.
- `python -B tests/lost_woods_entrance/opening_forest_test.py --sanitize`.
- `python -B tests/solo_games/mm_behavior_test.py`.
- Real MM and OoT `MagicMirror.cpp` translation units compiled through
  `scripts/diagnostics/compile_solo_forest_core.py`, with separate objects and
  dependency hashes. Both were also compiled with `COMBO_BUILD` removed.
- Clang-format 14 validation of production code/new forest fixture and
  `git diff --check`.

The final production SHA-256 is
`5fef15d3c98203ec40fd9aca8429086c0704db7e534ac4c2e86c56de94d25f3c`.
Independent review found no issues and independently reran the final registered
forest regression successfully.

## Limits and next configured check

These are native-source replays, not an installed game session. They execute
the real Mirror placement, spawn decoding/player-ID assignment, cutscene
manager, ocarina handler, message staff, `Player_Action_63` and HUD functions.
Scene-resource decoding/actor allocation, clear collision queries, audio/text
resources, animation completion and movement return are controlled boundaries.
Follow-camera motion is outside the fixture; its real NORMAL0 HUD preset feeds
the real interface/cache code. The HUD update uses the actual visibility switch
from `Interface_Update`, excluding unrelated world/health services. Entire
`Player_Init`, `Camera_Update` and rendered `Play_Draw` are not executed.

The native room metadata checks cover the bundled prepared resources. They do
not prove an unidentified external Alt room override, actual collision
placement, note recognition, visual parity, pause behavior or installed Windows
DLL/archive pairing. No exact active Alt room pack/settings were supplied with
the newest clip. All original asset bytes remain unchanged.

The decisive installed check is to load the exact followup executable with its
matching archives, arrive on the stump using Mirror, play notes and cancel in
combined and MM-only Lost Woods, then repeat instrument use and rooms/return.
Confirm the playing view and full HUD return with Alt Assets off and with the
user's active Alt configuration, including pause/unpause and re-entry. Those
configured checks and user acceptance remain pending.
