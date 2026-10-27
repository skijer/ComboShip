# MM Black Market: native OoT shop camera candidate

cor requested the OoT Bazaar camera and browsing behavior after rejecting the
MM Fixed1 emulation in the newest build. The reference is the supplied
16.454-second OoT Bazaar recording (20261024223520932); the rejected MM recording
is 20261024222804730. These recordings establish framing and browsing expectations,
not evidence that this candidate has run in the game.

Baseline: local `bb6f927eda710812daceb73f9124e0d536701918`, tree
`292685496dd311b2b1b97c0847eea537988b6c5a`, identical to public PR61 head
`602e430613ce3045f782f1ec9b76bd5878879784`. Candidate branch:
`poc/mm-oot-shop-camera-20261024`. No develop/master merge is authorized by this
POC. The accepted original asset archives and the OoT host stay unchanged.

## Scope and implementation sequence

1. Compile a failing test of the actual MM actor: the OoT seven-update shelf pan
   must keep one camera owner and a fixed eye. Preserve existing lifecycle,
   foreign-owner, receipt, and ordinary-shop tests.
2. Port the actual OoT `Camera_Data4` algorithm into a private appended MM camera
   setting. Keep existing setting IDs and ordinary MM behavior. Compare the port
   to the actual donor source, including form-dependent camera inputs.
3. Use OoT's +/-30-degree shelf turns and return timing through the private
   EnOssan adapter. Keep MM interaction and acknowledged receipt ownership. Remove
   the item-driven camera lift and nonlinear vertical item endpoint.
4. Fit only the MM shelves, item anchors, and display envelope to the fixed-eye
   camera and straight native item motion. Check actual room/fixture geometry,
   all eight item paths, cursors, letterbox/textbox clearance, and legibility.
5. Regenerate deterministic MM presentation resources, run source integration
   and asset checks, independently review the candidate, preserve a recoverable
   source checkpoint, then publish the isolated candidate and build it.

Engine worker owns camera implementation/declarations/data and new donor tests.
Root owns actor adapter, MM presentation, integration tests, assets and handoff.
The geometry worker investigates framing read-only. Stage only owned changes.

## Acceptance and recovery

Source checks and compilation are separate from configured gameplay proof.
Runtime acceptance requires the matching executable and generated MM assets:
keeper greeting, both bank pans, all cursor positions, select/cancel/reselect,
rejected/accepted purchase, receipt/tutorial/continue, exit and re-entry. Compare
against the supplied OoT reference. Keep normal MM shops and the OoT Black Market
as regression controls. Do not mark native gameplay accepted without that proof.

Recovery starts from the public PR61 head above and the isolated candidate's
bounded commits. Preserve exact asset hashes and generation commands with the
published checkpoint; verify its independent restoration before handoff.

## Implemented candidate

The private MM scene uses the actual OoT Data4 port, with one owned camera for
the complete keeper/browse interaction. The eye is `(0,43,160)`, native FOV60,
scene pitch728 and native +/-30-degree turns. The previous eye was around
Z310; the keeper stays at Z86. The new camera is close to the counter. Fractional
yaw and the donor's seven-update shelf/return timing are preserved. Cursor
movement and item confirmation do not move the camera. Selected stock follows
the native straight tween; the old vertical camera lift, nonlinear item path
and decorative item bob are removed for this MM presentation.

MM-only shelves have rows52/84, a short front lip and .18 display scale. Their
room translation is Y-6/Z-70; selected endpoints are `(38,53,96)` and
`(-38,53,96)` to clear the keeper. The full authored item/seal envelope is
derived from production FrameFit and BuildLock, including thin/off-center
models, not just a centered sample. Twelve private room resources retain the
original texture/triangle ownership. Four complete left-cauldron components
move `(-35,0,60)` to clear the close view and shelf post; original/shared/OoT
resources remain byte-exact.

cor also reported climbing onto the counter and ending a purchase on a shelf,
and explicitly required continued shopkeeper talk access. A collision-only
wall at Z130 spans X+/-170 and Y-16..300. It has no floor/top, uses native
no-ledge WALL_TYPE_1, and is ignored by camera collision. It closes against the
room walls and leaves the customer aisle/entrance open. Real native talk offers
still succeed after wall displacement at tested radii10/20/40 (Z140/150/170).

Shopping begins only on the grounded static customer floor within the existing
100-unit talk range. A receipt can restore an unsafe ending position to that
validated starting floor, with a fresh floor query and consistent position,
velocity, fall and contact state. Restoration requires the same play/player,
receipt ticket and main-camera identity. Observed foreign event/cutscene/camera ownership
permanently retires the saved position; release cannot revive a delayed
teleport. A read-only native camera query verifies the active pointer and UID
for the player's own item cutscene, including deferred Stop after the player
clears the get-item flag. Completion waits for that cutscene to end.
Cancellation/destruction/transition cleanup never teleports Link.

## Source verification and limits

- 5,616 actual OoT/ported MM camera frame comparisons under ASan/UBSan, using
  MM height inputs; this is not a test of transformation dispatch. Five deliberate
  camera mutations were rejected. Entire camera C and controller C++ units compile.
- Actual MM EnOssan/EnGirlA/cutscene integration: both turns/return, all8 cursors,
  selection/cancel, immediate error, receipt/tutorial/continue, all5 customer
  form eligibility, position recovery and foreign-owner release/UID/pool cases.
- All8 straight take/return paths, including between-frame interpolation,
  clear 7,919 prepared room and 144 fixture triangles with the complete reserved
  item/seal envelope. Actual native fallback Gal's 32 poses were separately
  checked. Arbitrary external models/Alt packs are outside this envelope proof.
- Actual native collision and talk functions verify barrier blocking,
  no ledge/floor/ceiling, camera pass-through, preserved aisle/exit/floors,
  capacity failure and repeated lifecycle. This excludes climb-anywhere cheats.
- Real scene factories/readiness, deterministic preparation, private graph
  closure and original OoT bytes are checked. Preparation yields141 OoT and153
  MM resources. Use matching generated 2ship.o2r with the candidate executable.

Offline previews use actual room/fixture and native fallback Gal geometry with
illustrative stock and approximate materials; they are not gameplay screenshots
or proof of the user's external custom keeper. Original bottom shelf foundations
can intersect the counter they rest on; stock, upper boards and relocated
cauldron are checked separately. Windows/Linux CI and installed gameplay remain
separate acceptance stages; no configured gameplay acceptance is claimed here.

## Checkpoint hold requested October 25

Independent review found no remaining blockers after the event-release,
main-camera dialogue/door, foreign item-camera and deferred-Stop regressions.
cor requested that this completed source candidate wait for other sources before
the combined build. Preserve the isolated commit and recoverable source archive;
do not start the combined build, open a build-triggering PR, or merge this
candidate until those sources are supplied and the next scope is established.
