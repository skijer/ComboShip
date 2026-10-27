# Continuous Woods of Mystery monkey calls

Baseline: `2f0d77ce562210b35499951d181e21bd53bc39e2`, retaining the recovered
shop/title/regional source and the reviewed OoT inventory-page fix. Isolated
candidate: `fix/monkey-native-loop-20261024`. The user explicitly requested a
continuous native 2Ship cadence instead of the previous randomized 3–5 second
ambient gaps. No publication or develop/master promotion is performed here.

## Native reference and change

MM's `EnMnk_StartNextPath` selects animation index 1, the native
`object_mnk_Anim_009FE0` walking clip. The unmodified local MM archive's typed
animation record contains nine frames. `EnMnk_Monkey_GuideThroughWoods` advances
the looping animation and calls `EnMnk_PlayWalkSfx`, which emits voice
`NA_SE_EV_MONKEY_VO_WALK` (`0x2900`) at frame 1 of each loop. This is one call
every nine normal gameplay updates, approximately 0.45 seconds at 20 Hz.

Placed OoT monkeys now use that same continuously repeating voice cadence,
independently of their visible idle, scratching or jumping animation length.
The randomized countdown is replaced by a per-actor native-animation phase.
Integer third-frame units reproduce SoH's `R_UPDATE_RATE / 3` animation clock
without fractional accumulation drift. The event occurs at logical frame 1 of
the nine-frame loop. A successful prior emitter request remains owned even if
a later request fails, so cleanup cannot lose its retained position pointer.

The existing positional native MM bank/Sequence_0 sound remains unchanged.
There is no new audio asset, sample editing, footstep emission, global bank
change, pose change, scene edit or progression/dialogue change. Cleanup still
stops only that monkey's queued/active sound on pause, room change, transition,
cutscene or actor destruction. Resuming an eligible room restarts its cadence.
The field rename is internal transient actor state; its type/layout is retained.

## Evidence

- The strengthened existing production-viewer test first failed at the second
  expected native call on the old random-delay implementation. It now checks
  every update over ten native cycles for all three poses and separate emitters.
- The native guide reference now executes MM's actual `SkelAnime_LoopFull`,
  `Animation_OnFrameImpl`, `Animation_OnFrame`, guide action and sound helper.
  Only pose sampling, path following and audio output are fixture boundaries.
  Its ten-cycle trace establishes the nine-frame voice and footstep cadence;
  placed OoT actors emit the voice only.
- Slow animation-clock cases check hand-derived event ticks at update rates
  1 and 2. Existing pause/room/transition/cutscene, idempotent destruction and
  unavailable-resource cases remain. A new missed-request cleanup case retains
  ownership of a previously successful emitter.
- `python3 -B tests/monkey_audio/run_tests.py` passes: complete production viewer
  lifecycle/render fixture and full SoH viewer syntax, actual native MM guide
  cadence under ASan/UBSan, and actual MM bank/Sequence_0 dispatch plus queued,
  active, independent-emitter, cold-stop and re-entry cleanup under ASan/UBSan.
- The complete viewer source also passes separate Combo C syntax compilation.
  Existing native GBI pointer-signature warnings are unchanged. LeakSanitizer
  uses the existing `detect_leaks=0` executor convention; ASan/UBSan remain on.
- Changed C lines pass clang-format 14.0.6 and `git diff --check`.

This establishes source/native control-flow cadence and sound identity.
Full application/Windows compilation and configured audible acceptance remain
separate final cumulative gates. Listen to the placed monkeys with the matching
executable and `mm.o2r`, including scratch pose, two nearby monkeys, pause/resume
and room exit. The audit checkpoint and authorization scope include this change.
