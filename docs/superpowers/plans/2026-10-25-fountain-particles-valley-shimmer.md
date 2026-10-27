# Fountain Particles and Valley Shimmer Implementation Plan

> Execute inline with the test-first and verification workflows.

**Goal:** Correct boxed snow/mist and extend the existing shimmer horizontally across the valley behind Darmani.
**Architecture:** Retain the previous local translucent-render-mode correction; change only Snowhead's OoT shimmer centers and terrain-derived visual bounds.
**Tech Stack:** C++ native GBI and shared NEI meshes; Python/C++ native regression fixtures.
**Spec:** `docs/superpowers/specs/2026-10-25-fountain-particles-valley-shimmer-design.md`

## Global Constraints

Preserve bridge geometry/collision, water, portal trigger and returns, actors, both ages/setups, snow/mist textures and placement, NEI art/count/palette, other routes and MM endpoints. No texture uploads or master/develop merge. Record implemented/native-verified separately from gameplay acceptance.

## Review Focus

Particle alpha and depth state under fog; full native setup restoration; packed shimmer containment at tilted cameras; asymmetric valley-bank coverage; unchanged MM/other-route presentation and trigger width.

### Task 1: Carry the omitted transparency correction into this candidate

Files: `soh/soh/Enhancements/Gameplay/ComboFountainSnowheadBridge.cpp`, `tests/regional_passages/{run_chill_tests.py,chill_native_test.cpp}`.

- [x] Restore the prior native setup/shader regression and run it against unchanged public production; expect failure for opaque snow/mist.
- [x] Apply only the prior two explicit fog-compatible translucent render-mode commands.
- [x] Run the full native Snowhead suite; expect passing draw/shader, geometry, collision, lifecycle and complete renderer compilation checks.
- [x] Record and commit the isolated result.

### Task 2: Fit the shimmer across the full valley

Files: `combo/entrances/FountainSnowheadBridge.h`, `combo/actors/RegionalPassagePresentation.h`, `tests/regional_passages/shimmer_coverage_test.cpp`.

- [x] Derive the bank cross-section independently in the coverage test; require complete packed floor containment, twinkles within twenty units of both banks and gaps below eighty units. Run the unchanged renderer; expect full-width coverage failure.
- [x] Give OoT Snowhead separate visual extents, distributed as ten columns/two rows with unchanged twenty twinkles, height/depth and art. Keep trigger dimensions unchanged.
- [x] Run native regional presentation, Snowhead and six material/combiner checks; inspect the narrow diff, review, and commit.
- [x] Save an independently restorable source checkpoint with exact base/head/tree, evidence and pending configured-runtime test.

## Verification and review

Both required regressions were observed RED against the public source and GREEN with their bounded changes. The full regional sanitizer, native presentation/graphics, and material suites passed. Independent review of `83eed9ff..5be36d66` found no Critical, Important or Minor issues. Recovery packaging records its exact final refs/tree and independent restore proof. Configured Windows appearance, installed executable identity, and user acceptance of the wider two-row layout remain explicitly untested; source acceptance does not resolve those dimensions. No master promotion or publication.

### Task 3: Replace Snowhead bridge owl flight with normal MM arrival

Files: `mm/2s2h/Enhancements/Gameplay/ComboRegionalPassages.cpp`, `tests/regional_passages/mm_native_test.cpp`; native pose provenance remains in `combo/entrances/MmRegionalBridgePlacements.h`.

- [x] Drive actual production spawn handling into the extracted native Player_Init decision. Confirm the unchanged Snowhead route fails the requirement that all three bridges use normal F arrival.
- [x] Queue a one-shot Snowhead-only override owned by slot, seed and entrance; consume it through the existing spawn-list callback using a private ActorEntry copy, leaving cached scene resources and ordinary owl arrivals unchanged.
- [x] Verify winter/spring, all slots/forms, unchanged dry pose/camera parameters, genuine owl behavior, reset and mismatched/cancelled handoffs; run the full regional sanitizer suite.
- [x] Review the bounded follow-up, commit, and replace the same source checkpoint with independent restoration proof and pending Windows/gameplay acceptance recorded.

Task3 result: RED native OWL selection observed before the change; GREEN normal F for every route/slot/form/season, genuine owl controls and nine cancellation/priority cases. Full regional sanitizer suite, pinned formatting and diff checks pass. Independent read-only review of `5a94994a..f6007354` found no issues; actual F movement/camera feel and winter/spring gameplay remain untested. Updated recovery package retains the particle/shimmer changes and verifies the final complete source tree independently.
