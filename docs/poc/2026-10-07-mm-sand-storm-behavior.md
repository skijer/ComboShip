# MM Sand Scepter parity and Storm collision candidate

Baseline: `develop` at `13901c677d0084e0305eec4672c0557f4434aea7`.
Candidate branch: `poc/mm-sand-storm-behavior-20261007`.
Status: implementation and isolated regression checks verified; full runtime build,
actual archive geometry, in-game acceptance and promotion remain outstanding.

## Reference and change

The authoritative Sand donor is the OoT side of this same baseline:
`soh/mods/items/logic/wand/wand_sand.c`, with held input and billing in
`soh/mods/items/logic/item_elemental_wand.c`. These were read directly.
The user's latest requirement is to preserve this behavior in MM, including its
magic policy. It supersedes the earlier MM stationary-drain customization.

| Sand rule | Previous MM behavior | Candidate, matching OoT |
| --- | --- | --- |
| First placement | Center under Link | Ahead by 85% of the measured slab reach |
| Further placement | Ahead | Ahead, including after an empty ring |
| Start of a hold | Wait six frames | Check coverage immediately |
| Continuing hold | Independent drain every six frames | Check coverage every six frames |
| Magic | Drain even when no new slab appears | Base two magic only on successful placement, through `Wand_Cast` |
| Covered step | Drain without placement | No placement or cost while coverage is solid |
| Crumbling slab | Excluded from coverage | Preserved; a replacement may cost magic even when stationary |
| Slab lifetime | Crumble after standing on it | Preserved: 24 frames, full collision until death, draw-only shrink |
| Scale and overflow | 0.05 scale, eight-slot ring | Preserved |

MM keeps the native persistent-object readiness checks, dynamic collision bgId
validation, rotation-based switch bypass, room persistence and destroy wrapper.
The input driver continues resetting held coverage on MM's blocked, menu, wheel,
stow and scene/lifetime paths. Held placements do not restart the cast pose.
These are explicit host adapters around the donor gameplay.

Storm's free shot flies at height 30. Its original cylinder started at height 30,
above a landed Chuchu's 26-unit collider. Lock-on aimed the ray down and could hit,
matching the user's observation. Center the existing 30-unit cylinder on the bolt
with `yShift = -15`; retain its damage flags, damage value, trajectory and lightning
drawing. Grass collision and the existing MM free-cast behavior remain intact.

## Verification

The same current expectations were run against baseline `13901c67` and the candidate.

| Check | Baseline | Candidate |
| --- | --- | --- |
| OoT/MM Sand comparison | First cast differs: under Link versus ahead | 2,224 observations match |
| Held Sand path fixture | Walks off before new collision is ready at frame six | Eight C/D-pad and heading combinations cover 600 units |
| Native Storm/Chuchu collision and damage | Lock-on succeeds; first free shot misses at distance 12 | All 40 free/locked, heading and distance combinations damage the Chuchu |
| Full wand regression suite | Prior suite passed before new expectations | All existing modes, ownership, input gates, magic, object rejection, lifecycle, cast poses, Water, Shadow and Meteor checks pass |
| MM player unity source | Baseline syntax accepted | GNU C syntax accepted with existing header/unity warnings |
| Air magic visuals | Existing lightning source | Geometry policy at O2/Ofast and native MM renderer fixture pass |
| Whitespace | Clean baseline | `git diff --check` passes |

The Sand comparison compiles each game's actual input, magic, placement and slab
functions with its native headers. It runs one shared scenario across C-left/D-up,
four headings and two rectangular mesh bounds, covering walking, a pause, a turn,
pressed overflow, draw shrink and death. Fixture setup first establishes a stowed
native input frame so both engines begin with the same draw latch. Dynamic floor
ties choose the lowest live bgId, matching native MM traversal.

The path fixture uses a synthetic 60-unit square at scale 0.05 and movement at six
units per frame. It includes background collision becoming ready on the following
frame. This proves that scenario; it does not establish the dimensions or triangles
of the user's real resource, nor all movement modifiers, forms, jumps or terrain.
The human Hylian boot row has a 5.5 run cap before modifiers; this fixture's speed
is an explicit test value, not a claim about every native movement state.

AddressSanitizer and UndefinedBehaviorSanitizer were enabled for the wand and parity
checks. Leak detection was disabled because LeakSanitizer cannot operate under the
host's ptrace environment. The fixture frees its final scenario allocations.
Native C enum conversions use a C++ fixture exception for enum-range checking.
An independent read-only review and sanitized parity/behavior reruns found no
blocking issue.

Primary reproduction commands from the repo root:

```sh
ASAN_OPTIONS=detect_leaks=0 python3 tests/wand_modes/run_tests.py --sanitize
ASAN_OPTIONS=detect_leaks=0 python3 tests/wand_modes/run_tests.py --case sand-parity --sanitize
ASAN_OPTIONS=detect_leaks=0 python3 tests/wand_modes/run_tests.py --check sand-path --sanitize --source-ref 13901c677d0084e0305eec4672c0557f4434aea7
ASAN_OPTIONS=detect_leaks=0 python3 tests/wand_modes/run_tests.py --check storm-hits --sanitize --source-ref 13901c677d0084e0305eec4672c0557f4434aea7
```

The two baseline-focused commands intentionally fail. Add `--source-ref` to the
parity command to reproduce the first-placement mismatch.

## Runtime acceptance and recovery

Build the isolated candidate with the user's normal ComboShip assets/settings.
Compare the OoT and MM sides while holding Sand from stationary ground and while
walking over a gap, then while turning, jumping and using the user's movement
modifiers. Check that slabs appear ahead, solid coverage costs nothing, new and
replacement slabs charge once, and their crumble/collision lifetimes agree.
Release, shield, stow, change mode and leave/re-enter the scene to check MM gates.
For Storm, shoot a Chuchu on level ground both freely and locked on at close and
long range; check grass cutting and the accepted lightning appearance.

No sacred/master baseline is promoted by this checkpoint. The recovery archive
contains the complete candidate patch, commit/baseline identifiers, verification
logs and hashes. Source files and donor history stay in Git. Apply its patch to the documented baseline with
`git apply --check` followed by `git apply`; reverse the same patch to roll back.
