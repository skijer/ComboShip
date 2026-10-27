# Lost Woods room 7 to Woodfall entrance

| Field | Record |
| --- | --- |
| Source baseline | `develop` at `6effafdeaa41d0e851395c19665f11f2b24f3145` |
| Candidate | `feat/lost-woods-woodfall-shortcut-20261021` |
| Scope | Fixed, two-way Combo mode passage from room 7's east tunnel to the normal Woodfall exterior entrance |
| Verdict | Source tests, native collision-data checks and individual native object compilation pass; configured gameplay and visual proof remain untested |

Cor chose Woodfall after considering Graveyard. The destination must retain the
exterior's traversal, rather than arriving beside the owl or temple. The return
is a separate plain opening near the starting platform. This candidate does not
require a particular previous arrival to make that opening usable.

## Route and preservation

| Endpoint | Placement |
| --- | --- |
| OoT departure | Lost Woods scene `0x5B`, room 7, east tunnel; claim beyond X1840 before the native exit floor begins at X2000 |
| MM arrival | `ENTRANCE(WOODFALL, 0)`, native spawn zero `(-41,12,-1353)`, native start mode D |
| MM return | Small angled wall left/back of the starting platform; anchor approximately `(-201.863,12,-1481.536)`, yaw `-7818` |
| OoT return | Room 7 at `(1780,0,-2400)`, yaw `-16384`, native slow walk into the clearing |

Route value 4 is private to the existing paired handoff path. The Woodfall owl
spawn 4 is not the destination. Arrival preserves loaded inventory, form, cycle,
time, weekly/dungeon/randomizer progress and Farore's Wind state. It activates
neither the owl nor the temple. The original Woodfall swamp exit and other Lost
Woods tunnels retain their ownership. No native collision, scene headers, actor
placements or cached resource payloads are edited.

The shortcut is a fixed additional connection, not an entrance-shuffle registry
entry. Explicit shuffled dungeon/boss recovery and Farore arrivals supersede its
queued route. Slot changes, Play destruction and reset cancel pending passage
state. During an armed OoT departure, the narrowly scoped player exit guard
prevents walking onward into the native Kokiri/shuffled exit during the fade.
Original game modes retain their existing behavior.

## Native geometry evidence

The native Woodfall scene has 459 vertices and 648 collision polygons. Wall
polygons 158/159 run from `(-150,12,-1436)` to `(-251,12,-1530)`, rising to Y408.
Dry approach triangle 184 is `(-206,12,-1313)`, `(-150,12,-1436)`,
`(-251,12,-1530)`, surface 3, exit 0. Original exit polygons 181/182 use surface 8,
exit 1, in the separate corridor at X[-109,11]. The basin water surface is Y1.

The return frame is 128 wide and 133 high. Its face is two units in front of the
wall; the black mouth is one unit behind that frame and still in front of the
native wall. The centered trigger is 40 wide and reaches 30 units in front of
the frame. This leaves clearance beyond Fierce Deity's 27-unit wall radius.
The normal spawn walk along X=-41 cannot enter the trigger. Nearby grass is
removable; a dry approach also avoids it without clearing it.

The controller copies spawn-zero metadata and decodes the native packed yaw.
It casts through the wall relative to that spawn, then validates both frame
feet, top corners and the narrow approach against static, level, dry, non-exit
collision. Changed geometry that fails those checks disables the opening for
that visit. It does not guess a substitute deeper into Woodfall. In particular,
forward probes 220/300/380 from the entrance miss the platform and are rejected;
entity-ignored Y0 basin polygons are not valid dry floors.

The saved native OoT room 7 mesh spans X1200..2400 and Z-3200..-2000. Its east
tunnel approach polygons 1671/1672 are non-exit floors at X1800..2000. Exit
polygon 1679 begins at X2000 and extends to `(2400,0,-2400)`, confirming the
departure captures the right tunnel before its native exit.

| Read-only input | SHA-256 |
| --- | --- |
| Native `mm.o2r` | `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59` |
| Native `oot.o2r` | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |

These archives are verification inputs, not replacement outputs. The newer
user-authored Lost Woods export remains separate and has not been overwritten.
The source baseline does not include the separate
`fix/lost-woods-fountain-return-20261021` branch at
`34d6f1f3266bafdee86a144749ba66faf52abb0e`. Combine that work separately and
repeat the relevant entrance checks on the composed tree.

## Verification and remaining proof

`tests/lost_woods_entrance/run_woodfall_tests.py --sanitize` executes production
arrival and handoff bodies with native save/player types. It covers all paired
slots/forms, progress preservation, arrival priorities, cancellation, right
tunnel isolation, native yaw packing, dry/static/non-exit requirements, cached
resource safety and no automatic return. Its geometry service models the native
entrance wall and apron rather than a generic corridor.

Add `--mm-archive /path/to/mm.o2r` to substitute all 648 native polygons and their
entity-ignore/exit flags. That complete-mesh run also passes with AddressSanitizer
and UndefinedBehaviorSanitizer. The scene-reader's camera/chest command support
is extended only to parse these native verification inputs.

`tests/lost_woods_entrance/run_tests.py` passes, covering existing forest routes,
paired save/handoff behavior, native renderer boundaries, opening-forest and
camera behavior, authored return packs and export/repair controls. Individual
native object compilation passes for `BenPort.cpp`, `OTRGlobals.cpp`,
`ComboWoodfallPassage.cpp`, `ComboLostWoodsPassage.cpp`, `z_scene_2SH.cpp` and
OoT's `z_player.c`. These are compilation probes with native headers, not a
linked or packaged application. No full application build or PR build was
started for this candidate.

The next decisive check is a configured in-game round trip: room 7 right tunnel
to the ordinary Woodfall entrance, inspect the plain return opening on the
left/back wall, then return into room 7 without an immediate bounce. Check normal
swamp departure, all forms, uncleared/cleared Woodfall, entrance shuffle/Farore
priority and the active mod/Alt Assets configuration. Visual fit, camera framing
and runtime transitions remain unproven until that check. This candidate is not
a promoted or runtime-accepted master.

## Recovery

Preserve this source branch and its baseline ref. A separate recovery receipt
records the exact implementation commit/tree and hashes of changed source and
verification logs. Restore by fetching the source candidate from
`marsh6487/ComboShip`, checking out the receipt's exact implementation commit,
and initializing the recorded submodule commits. Supply the native game
archives separately. The candidate adds its new MM compilation unit through
the existing CMake recursive source glob; no extra scene pack is required.
