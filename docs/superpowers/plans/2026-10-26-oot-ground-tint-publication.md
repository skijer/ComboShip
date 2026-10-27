# OoT Ground Tint Publication Implementation Plan

> Execute inline under superpowers:executing-plans; user has authorized publication and CI without further confirmation.

**Goal:** Publish the approved ground-tint POC on a cumulative candidate branch and deliver its matching Windows build.
**Architecture:** Apply the bounded tint/header-cache changes to PR64's current cumulative tree. Retain its complete source/assets and existing CI; append native brightness/preservation assertions to the existing foliage runner.
**Tech Stack:** C++20, Python regression runners, clang-format 14, GitHub Actions.
**Spec:** User authorization in this conversation and `docs/poc/oot-ground-tint-polish-20261026.md`.

## Global Constraints

- Parent: 9f23b84859ac545a283c155b6642aea302aad414; reconcile newer candidates before applying.
- Only default OoT ground grass/trim tint arithmetic and its cache identity change.
- Preserve material masks/table, alpha, actors, canopy/understory, explicit forest overrides, MM, and original Summer/off restoration.
- Keep develop/master unmerged; no runtime acceptance or promotion is implied.

## Review Focus

- Native packed RGBA5551 and raw HD textures retain alpha and monotonic detail.
- Dark ground gains amber/frost/blush without tinting protected stone/bark pixels.
- Explicit private-forest overrides and non-ground palettes remain byte-identical.
- Seasonal transitions retain original source allocations and restore Summer/off.
- Published commit, actual CI checkout and downloaded Windows package must match.

## Task 1: Integrate and verify the candidate

- [ ] Verify parent/latest candidate state and clean isolated checkout; run existing native foliage tests.
- [ ] Add `soh/tests/season_ground_tint_polish_test.cpp` with dark-grass brightness/hue, native alpha/protected-pixel, source-ownership and unchanged-palette assertions; verify its new ground expectations fail before applying the POC.
- [ ] Apply exact `SeasonFoliageTint.h` and `SeasonFoliage.cpp` POC changes, and add the new test to `scripts/diagnostics/run_soh_season_foliage_tests.py`.
- [ ] Run the full native foliage runner with ASan/UBSan, existing actor checks, CI-policy checks and clang-format 14. Preserve all existing regression task objects and assets.
- [ ] Review the bounded diff, commit it, and save a recoverable source checkpoint with verified restoration.

## Task 2: Publish and deliver

- [ ] Create a candidate branch/PR retaining PR64 as rollback; do not merge.
- [ ] Follow all required hosted CI jobs; repair in-scope failures and rerun until green.
- [ ] Verify the actual checkout tree and fetch/inspect the matching Windows artifact, including archive integrity and native executable/DLL presence.
- [ ] Record final refs, PR, CI, checksum and download URL; deliver with runtime appearance/acceptance pending.
