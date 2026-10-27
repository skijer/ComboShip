# MM arrival fixture CI repair — 2026-10-22

Baseline: PR #57, `poc/market-forest-followup-20261022`, commit
`3c924aba85be51719a9af103e021db7da273ee47`, tree
`5a261965a7250dbd194b92d29c88cb1a71f48bf9`.

Build Artifacts run `39812365174`, attempt 2, passed Fast source checks,
including clang-format. The entrance regression failed in
`transition_fade_behavior_test.py` while importing `mm_adapter_behavior_test.py`:
the fixture searched for the obsolete standalone `ComboFw_ApplyArrival();`
statement. Production now records that call's result in `crossFwArrival` for
title-card policy. Python raised `ValueError: substring not found` before the
transition test could compile. Fail-fast cancellation stopped the other jobs.

The fixture now locates the call independently of its assignment and ends the
entrance-recovery slice at the beginning of that line. It retains the real
arrival/recovery statements and their existing behavior assertions. Production
source, routing, title-card policy, assets and CI requirements are unchanged.

Validation on this candidate:

- Reproduced the identical Python failure before editing.
- `mm_adapter_behavior_test.py --sanitize`: passed, including 24 native grotto
  return/start modes and ordinary resume controls.
- `transition_fade_behavior_test.py --sanitize`: passed all four OoT/MM native
  transition and registered handoff fixtures.
- `boss_recovery_behavior_test.py --sanitize`: passed OoT and MM recovery,
  foreign frozen resume and cold reload cases.
- `run_mm_title_card_tests.py --sanitize`: passed combined and solo modes,
  including all native scene rows (222 titled, 117 untitled).
- Real-header syntax checks passed for both `z_play.c` hosts (MM retains 14
  compiler warnings; OoT has zero).
- `git diff --check`: passed.

Full executable builds and the complete CI matrix remain pending. This is a
test-fixture correction, not configured gameplay acceptance or promotion of a
runtime master. Restore the prior source from the immutable baseline above;
the isolated correction is recorded by this commit's diff. No archive, pack,
load-order or save migration is required.
