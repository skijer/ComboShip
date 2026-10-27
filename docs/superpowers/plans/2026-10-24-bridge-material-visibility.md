# Bridge Material and Visibility Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for the material repair and the existing parallel Snowhead investigation. Track the steps below.

**Goal:** Correct unstable bridge materials and resolve or concretely diagnose the missing Fountain/Snowhead provider.

**Architecture:** Keep the existing bridge geometry and scene resources. Repair material selection at the draw boundary and test production combiner consumption; investigate the separate Fountain readiness chain against actual resource layouts.

**Tech Stack:** C++20, native GBI, libultraship interpreter, Python fixture runners.

**Spec:** `docs/poc/bridge-material-visibility-20261024.md`.

## Global Constraints

Preserve accepted geometry, collision, waterboxes, shared child/adult scene, native Ice Cavern clearance, adult icebergs and manual actors. Keep develop/master unmerged. Runtime acceptance is distinct from compilation/native checks.

## Review Focus

- Host scene texture changes with culling must not affect bridge materials.
- Water and stone must use their intended slot under the actual two-cycle setup.
- Alt paths and cached resources must not silently disable the shared-age endpoint.
- Cold load, Alt toggle and scene re-entry must retain correct resource ownership.
- Foreign geometry must not be destructively patched to force availability.

### Task 1: Stable material consumption

Files: `tests/regional_passages/lake_render_test.cpp`, `run_lake_bridge_tests.py`, and the Lake/Fountain bridge and valley draw providers.

- [x] Execute emitted CaveDraw commands with production GenerateCC and assert `usedTextures[0] && !usedTextures[1]` for stone and water.
- [x] Run the regression against the baseline and record its expected failure.
- [x] Change repeated first-cycle modulation to second-cycle `G_CC_PASS2` in the affected draw providers, preserving the first cycle and render mode.
- [x] Run `python tests/regional_passages/run_lake_bridge_tests.py --sanitize`, material and relevant Fountain suites; record results and commit the bounded repair.

### Task 2: Fountain readiness

Files: `ComboFountainValley.cpp`, `ComboFountainSnowheadBridge.cpp`, their geometry headers and native fixtures.

- [x] Trace resource lookup, guard rejection, actor registration and both Get dependencies against available native/Alt resource evidence.
- [x] Reproduce any identified rejection with an actual native fixture before changing it; preserve guards against foreign geometry.
- [x] Repair only a demonstrated faulty condition and verify both ages, cached/cold resources and re-entry, or report the exact missing evidence and add useful bounded readiness diagnostics.
- [x] Record what the new source proves and what still needs the user's configured game.

### Task 3: Reviewable recovery

- [x] Run the relevant integrated regional suites and inspect the complete diff.
- [x] Obtain independent review, preserve a source checkpoint and verify independent restoration to the recorded tree.
- [ ] Publish only the authorized candidate changes using verified individual blobs; preserve the accepted baseline.

Task 1 evidence: baseline shared combiner regression fails all six sites;
corrected actual-draw and shared regressions pass under sanitizers. Task 2
evidence: foreign active resource owner regression fails before pinning and
passes after. The integrated regional suite and native presentation syntax
suite both exited 0 on the combined candidate. See the linked POC records for
the explicit boundary between native verification and configured runtime.

Ruling: retain the authored inlet geometry while correcting its material,
because the user requested verification of its intent; no geometry removal was
requested. Its post-repair appearance still needs the configured game.
Ruling: preserve the backdrop guard, repair proven resource ownership and add
bounded readiness logging; collision alone cannot prove a render-only tree
screen is absent. This may leave the reported current-pack absence unresolved,
which is stated explicitly in the handoff.

Latest steering: the second clip rejected the strip's presentation; the bounded
follow-up design and RED/GREEN evidence are in
`docs/poc/lake-flowing-inlet-20261024.md`. This authorizes channel/intake changes
while preserving the main dry route and native scene. The original material and
ownership checkpoint `834bc321` was independently restored and saved before
that revision. Final integrated regional sanitizers passed after review fixes.
