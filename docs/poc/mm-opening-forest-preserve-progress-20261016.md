# Playable MM opening Lost Woods passage

Status: isolated source candidate; installed-game acceptance remains pending.
The supplied MM3D archive continuation is recorded in
`mm3d-opening-forest-exit-seasons-20261016.md`.

## Baseline and scope

Branch: `poc/mm-lost-woods-preserve-progress-20261016`.
Parent: `075783fd` (native entrance-transition preservation), on top of
`86582f5a` from draft PR #43. The inherited cross-dungeon, forest-camera,
Farore, generation, equipment and field corrections are retained. No accepted
master/develop ref, scene archive, mesh, collision, texture or actor resource
is replaced. No real player save was used or modified by verification.

The custom pink Great Fairy tunnel in OoT Lost Woods room 1 now selects
MM's opening Lost Woods (`ENTRANCE(LOST_WOODS, 0)`). The separate straight
tunnel to Woods of Mystery remains available. The source bounds come from
`Lost_Woods_Tunnel_POC9_Final_Export_Helper.zip`: the old Fairy floor is at
X740..860, Y0, Z360..400, surface exit slot 11, in `spot10_room_1`.
The trigger starts at Z320, before that native exit floor. This coordinate
reference is not a claim that the user's current installed pack was inspected.

## Playable visit

The route loads the existing paired MM slot through the normal combo loader.
It does not invoke title/debug new-save initialization or Skip Intro Sequence.
Arrival selects layer/spawn zero with an ordinary player start mode and free
background camera, retaining the native position/yaw and current player form.
The cached native/Alt actor entry is copied rather than modified.

Both entrance cutscene dispatch and later scripted cutscene starts are blocked
for this playable forest visit. Global player interactions remain available.
No Skull Kid intro, theft or curse script is run, and no story-completion bit
is written to simulate having watched one. Other layers/spawns used by the
native flashbacks retain their behavior. The scene's fixed morning setting
does not overwrite the current MM time. Normal pause/autosave eligibility is
available during the visit, subject to the existing dialog/action/save guards.

The native forward fall is redirected from Opening Dungeon to the ordinary
Clock Tower before prologue actors initialize. This bypasses the curse and
tutorial rooms without transforming the player or changing inventory/story,
cycle/day/time, randomizer or NEI state. The destination normalization applies
to normal, loaded ComboShip slots entering Opening Dungeon; title/debug modes
and invalid save slots are excluded.

The return arch uses the opening room's native player-spawn anchor. A new
arrival cannot immediately bounce: return arms only after leaving a radius
of 120 units, then triggers within 60 units. The shared guarded fade captures
the source slot and cancels on reset, slot changes or Play destruction. OoT
returns to room 1 at (800, 0, 160), facing out of the Fairy tunnel and clear of
the trigger. Farore's TOP respawn remains separate from arrival's RETURN point.

## Save preservation

A genuine MM portal departure now captures the active room and cycle scene
flags and writes a complete native owl-shaped `SaveContext`, even if no owl
snapshot existed previously. A smaller bare `Save` omitted current-session
fields such as `eventInf` and timers. This is a demonstrated preservation gap;
the exact cause of the historical reported file reset was not reproduced.

Portal-only `comboPortalSession` metadata restores live respawn points in the
actual loader independently of Remember Save Location. Native prefix loading
otherwise omits the live respawn array. Owl metadata changes are confined to
the snapshot; they do not make the running game quit. Native owl replacements
and dropped/malformed blobs clear the portal marker. Invalid slots and requests
outside normal active play refuse the portal write. Existing no-argument
save callers, resets, Farore departures and owl-save quit retain their policy.
The save/container version and new-file initialization gates are unchanged.

## Evidence

- The original route, spawn, cutscene and bare-save regressions failed before
  their corresponding production corrections. The Remember-off Farore case
  also failed when the fixture stopped unconditionally invoking the optional
  respawn hook; the production loader correction makes both settings pass.
- `tests/lost_woods_entrance/run_tests.py` passes: paired boot/arrival,
  lifetime/fade/handoff guards, preserved Mystery corridors and camera,
  opening passage, cutscene suppression and native JSON save round trips.
- `opening_forest_test.py --sanitize` passes against actual engine types and
  extracted production paths for all five forms, all three paired slots,
  unchanged progress bytes, native spawn/time handling, onward normalization,
  cutscene rejection and pause/autosave eligibility.
- `portal_save_test.py --sanitize` executes the production writer/loader,
  native JSON converters, checksum, cycle capture/reconstruction and optional
  respawn hook. All three slots pass with Remember Save Location off and on,
  including current scene and other-scene flags, form, time, temporary quest
  state, timers, randomizer/NEI data and Farore's point. Stale/malformed marker
  cleanup, invalid slots, title mode and inactive-play write refusal pass.
- The complete cross-dungeon runner passes under ASan/UBSan, including native
  transitions, boss recovery, generation and production affordability.
- The existing foreign-scale lifecycle/texture runner passes with the real
  default save wrapper and default writer/loader behavior retained.
- All nine changed production translation units pass syntax compilation with
  real engine and dependency headers: both handoff implementations, MM scene
  commands, save manager, save eligibility, passage renderer, Play, entrance
  cutscene dispatch and cutscene manager. The C++ checks use the target's
  `-fpermissive` policy and CMake CVar definitions. This is not a linked build.
- Independent review's two concrete findings (Remember-off respawn loss and
  the older foreign-scale fixture's overload handling) are corrected. The
  follow-up review reports no additional concrete findings.

The standalone tests bound disk/DLL dispatch, migration services and scene
resource payloads explicitly. They are not a full application build, rendered
scene traversal or evidence about the user's installed save/asset stack.

## Remaining acceptance

Build the candidate and test a backup copy of an established paired save with
the actual custom Lost Woods pack. Verify both OoT ages, all MM forms, entry
without Skull Kid/camera takeover, walking deeper, onward Clock Tower travel,
return arming, room/collision/camera placement, ordinary saving/reloading and
repeated crossings. Compare inventory, quests, dungeon/weekly/cycle flags,
day/time, health, rupees, form, randomizer/NEI ownership and Farore before and
after, with Remember Save Location off/on. Recheck the separate Woods of
Mystery, Market/Clock Tower and shuffled-dungeon/Farore routes. Promotion
requires this configuration-specific runtime proof and user acceptance.
