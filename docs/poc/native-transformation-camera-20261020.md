# Native transformation camera correction

Candidate: `fix/native-transformation-camera-20261020`.
Source baseline: `613802447cf8a4fbc3e058ba084f9e3dd7b9f7ae`, the current
PR #45 source checkpoint. Apply this correction on top of that checkpoint;
do not replace the cumulative integration with the older transformation kit.

## Evidence and scope

cor supplied `MedalTVScreenRecording20261020142925213.mp4` (29.81 seconds),
SHA256 `2c546ebc98d0c7cabd996a9992e1f5ac245b4cfea832688749a0414ce0d9e5ec`.
Extracted frames show distant rear/side framing during the mask sequences.
cor reports that the SFX are correct. The recording does not independently
identify its executable commit or optional pack/load-order configuration.

The baseline's `MmForm_UpdateNativeMaskCamera` supplies one fixed 60-degree
shot aimed at a fraction of the standing body height. It contains none of
MM's `Camera_Demo4` / `Camera_Demo5` camera progression. Its existing fixture
discards camera coordinates and FOV, so the visual mismatch passes that test.

The candidate adapts those two donor camera state machines to OoT's
`CAM_SET_FREE0` manual subcamera:

- Mask on: native close-up, face tracking during the crouch/struggle,
  lateral sway and roll, the 80-to-32-degree lens change and pitched face shot.
- Mask off: its separate initial face shot and timed rolling pullback.
- Capture the real previous eye/at direction, distance, pitch and FOV.
  Keep the player facing that shot throughout camera ownership.
- Return to the donor's reveal state under the existing white flash.
  A skip enters the donor's `999` state before returning to the saved view.

Only `soh/mods/transformation_masks/mm_native_transform.inc.h` changes
production behavior. Animation clips, playback rate, phase/flash timing,
mask/veil geometry, lighting and all SFX IDs, schedules and dispatcher paths
are preserved. The MM game continues to use its native camera implementation.
Instant and missing-resource fallbacks retain their existing behavior.

## Verification

The added regression failed on the unchanged baseline camera. The corrected
controller passes 116 scenarios across solo SoH and ComboShip OoT under
AddressSanitizer and UndefinedBehaviorSanitizer, with the Windows `near` macro
active. Camera comparisons execute the unchanged MM donor function bodies
extracted from this checkout, rather than deriving expected values from the
candidate. Both forms of the camera are compared through their whole sequence
for all four masks, child/adult, three view directions and both sway signs.
Skip, player-facing ownership, previous-view preservation and the existing
scene/death/resource/equipment/audio lifetime cases are also covered.

The donor fixture adapts MM's ABI and geometric helpers to the OoT test host
and supplies synthetic face positions. This proves the controller against
the donor state machines; it does not prove the game's rendered face tracking.
Independent code review found no substantive issue and reran the native suite.

Focused verification commands:

```sh
python3 -B scripts/diagnostics/run_oot_native_transformation_tests.py
python3 -B tests/goron_audio/run_tests.py
python3 -B scripts/diagnostics/run_mask_transformation_tests.py
python3 -B scripts/diagnostics/run_mask_progression_tests.py
python3 -B scripts/diagnostics/run_mm_fast_transformation_tests.py
```

These checks pass, including the real eight-cue MM SFX dispatcher/lifetime
tests. The affected production header passes clang-format 14 and whitespace
checks. Native object compilation and the broader solo-games boundary suite
are recorded with the accompanying checkpoint's validation results; they are
separate from full game builds and runtime acceptance.

## Runtime verdict and decisive test

Implemented and source-test verified; configured runtime acceptance is pending.
No accepted master or input archive is replaced. No native game archive or
optional model pack is modified or included in the source checkpoint.

On the next candidate executable, use the same courtyard and accepted packs
from the supplied recording. Put on and remove Deku, Zora, Goron and Fierce
Deity normally, then skip one repeat. Verify that the face and mask remain in
the native shot through the struggle, flash and form reveal, and that normal
camera/control return works. Retain the accepted audible cues. Check adult
and solo SoH separately before claiming those configurations visually proven.
The normal sequence still needs the existing MM asset loader and `mm.o2r`,
with `gMods.TransformMasks.InstantTransform=0`.

Rollback: revert this candidate commit or apply its patch in reverse. The
parent checkpoint and original footage remain unchanged.
