# Prelude monkey room audio — October 19, 2026

The three existing OoT En_Viewer monkey entries now emit MM's native
`NA_SE_EV_MONKEY_VO_JOY` (`0x2901`) through `MmSfx_PlayAtPos`, the existing
MM sound-bank / native Sequence_0 bridge. No replacement recording, altered
sample, pitch override, new asset pack or MM quest controller is introduced.
The existing `mm.o2r` donor supplies the sound as well as the monkey model.

| OoT En_Viewer params | Existing pose | Audio |
| --- | --- | --- |
| `0x7E0E` | Idle | Repeating native MM call |
| `0x7E1E` | Scratching | Repeating native MM call |
| `0x7E2E` | Jumping up and down | Repeating native MM call |

Each initialized actor submits a native one-shot, then waits 60–100 native
20 Hz update ticks before the next attempt (approximately 3–5 seconds).
Timing belongs to the actor, not the draw/render rate or a global sound loop.
The hook requires the actor's authored room to be the current room and excludes
pause, cutscenes and outgoing transitions when the actor update runs. Normal
pause also suspends actor updates and follows the existing MM audio pause policy.

`MmSfx_StopAtPos` removes queued requests and active bank references for that
position and ID under the existing mixer mutex. Room/transition suppression
and actor destruction release that emitter without globally stopping peers.
The bank stop also safely handles cold initialization, a null position and an
invalid bank. Pose/root motion, authored transforms, blink, collision, dialogue,
actor parameters, other NPCs, MM's native quest/path monkeys and progression
are preserved. The dialogue remains “Ook! Ook! The Woods of Mystery are just ahead!”

## Source and recovery

- Initial candidate baseline: PR44 `b3049a6e285b6e3a0536cc8ac3e33f71b653e07f`.
- Cumulative PR subsequently advanced to `9f3b7f32fde49c33b957ee55e9e8186f45fa74ff`;
  its two fixture/documentation paths do not overlap this change.
- Isolated recovery branch: `poc/monkey-room-audio-20261019`.
- Restore by reverting the bounded monkey-audio commit. No scene export,
  accepted asset archive, donor sound resource or master is replaced.

## Verification and limits

- The actor regression first failed because no MM call was submitted. It now
  exercises all three poses, repeat gaps, independent emitters, room return,
  pause/cutscene/transition guards, repeated destruction and unavailable resources.
- The actual MM bank, dispatcher and new locked stop wrapper pass ASan/UBSan:
  native Sequence_0 index dispatch, multiple actors, active and queued pointer
  release, unrelated sounds, cold stop and later room re-entry.
- The full OoT viewer translation unit compiles with real headers in both
  `COMBO_BUILD` and standalone configurations.
- All 28 existing Goron/transformation sound lifetime cases pass in the native
  sequence and retained direct-output fixtures.
- Formatting uses clang-format 14.0.6 on changed C/C++ lines.
- The complete forest-actor regression and both solo-forest placement/dialogue
  regressions pass with the existing real spdlog/JSON include dependencies.
- Independent source review found no Critical or Important defect; its minor
  pause-evidence qualification is reflected above. The reviewer independently
  reran the native bank/actor suite successfully.

These are source, compiled-fixture and object-build checks. Listening in the
installed game, Windows/full executable builds and configured runtime acceptance
remain unverified. The decisive runtime check is to visit a room with each pose,
listen through several calls, then leave/re-enter and confirm no outgoing-room
chatter, hanging voice or lost animation/dialogue with the usual donor/Alt setup.

Reproduce with `python3 -B tests/monkey_audio/run_tests.py` or the existing
`scripts/diagnostics/run_forest_actor_tests.py` entry, which now includes it.
