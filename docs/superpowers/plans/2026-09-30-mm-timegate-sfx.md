# MM Time Gate sound cleanup implementation plan

**Goal:** Stop Time Gate's casting sounds when native MM completes or cancels the action.

**Baseline:** PR #24, `d70557f37f24e1e1901d16d807d4163621a1c81d`. Its Time Gate source is identical to reported runtime build `cb69b63cd9f37ece758eea32ebee8457e35e0a7d`. Candidate: `poc/mm-timegate-sfx-20260930`.

**Cause:** Native `AudioSfx_PlaySfx` queues the flagged warp sounds, but none of the Time Gate exits stops them. MM swaps its adult model live, without OoT's scene reload.

**Scope:** Only the MM Time Gate audio lifetime. Preserve magic cost, Yes/No behavior, model visibility, animation, camera, unrelated sounds, and all current PR #24 changes.

- [x] Add a native MM regression that runs the production Time Gate handler and sound bank through confirmation, No/B/timeout cancellation, damage, unequipping, queued cancellation, and repeat use. Verify a second source playing the same sound survives.
- [x] Observe baseline sound-lifetime failure.
- [x] Stop Time Gate's warp and hover sounds by exact source position and sound ID on its existing exits.
- [x] Run the regression, affected native item tests, and formatting/diff checks. Keep audible gameplay unverified until user testing.
- [x] Preserve the verified patch on the isolated local candidate branch; leave the master unchanged.

**Runtime check:** Use Time Gate both directions, then choose No and B on separate casts. The portal sound must end with the action, normal sounds/music must continue, and a later cast must still have sound.


Verification record:
- Baseline regression failed at `Time Gate sound survives Yes` before any production edit.
- `python3 -B tests/mm_nei/run_timegate_audio_tests.py`: PASS with ASan/UBSan; complete production Time Gate module and native MM sound engine. Six exits plus repeated activation and pending portal-request cancellation. Another source's warp/hover sounds and Link's unrelated sword sound survive. Confirmation consumes exactly 48 magic and toggles once; cancellation consumes none and does not toggle.
- `python3 -B tests/mm_nei/run_use_tests.py`: PASS, 18 native item lifecycle groups plus integration checks.
- `python3 -B tests/mm_nei/run_action_tests.py`: PASS, Mitts climb and ordinary tool camera/activation/reset groups.
- clang-format 14.0.6 applied to changed C files; `git diff --check` PASS.
- Independent review: no critical or important findings; independently reran the native audio regression successfully.
- Existing native controller-macro and animation-const warnings remain. No full local game build or audio-device/gameplay proof is claimed. The cumulative CI runner now includes this regression.
- Production diff: 13 added lines in `mm/mods/items/logic/item_time_gate.c`; no assets or other game behavior changed.

Publication: The user explicitly approved publishing this isolated fix to `marsh6487/ComboShip` and starting its build on 2026-09-30. Publish as a draft candidate stacked on PR #24; no merge or master promotion. Gameplay remains unverified.
