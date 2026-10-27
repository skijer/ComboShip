# Shared Items and SoH Integration Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for the integration. The independent Combo-owned shared-item domain is delegated under superpowers:dispatching-parallel-agents. Do not stage or commit another worker's unfinished files.

**Goal:** Deliver an identifiable ComboShip candidate containing Varuuna's Shared Items and the current SoH feature delta.

**Architecture:** Merge the two related ComboShip histories, preserving NEI additions. Apply the standalone SoH delta using its recorded prior donor as the three-way base. Reconcile host-owned resource, audio, transition and build code deliberately.

**Tech Stack:** C/C++20, Git, Python diagnostic runners, CMake, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-21-shared-items-soh-integration.md`

## Global Constraints

- Inputs are the exact commits named in the spec; no stable branch replacement.
- Native new-seed shared-items behavior; no existing-save migration.
- Preserve NEI, MM, ComboShip save/transition/runtime ownership and all relevant SoH custom features.
- Separate static/build verification from runtime acceptance.
- No edits to scene or model archives; no unrelated upstream pin bump.

## Review Focus

- Combined NEI/native sharing must not double-trim or double-credit progression.
- Inactive game grants must retain save ownership and avoid stale progressive magic state.
- Linux exports and CMake registration must include both native and newly ported features.
- Resource retention and cache refresh must remain compatible with separate OoT/MM resource contexts.
- Audio/weather, age swaps and PAK selection must preserve host transitions and donor behavior.

### Task 1: Reconcile the upstream merge

**Files:** Combo-owned generator/menu/runtime files; `mm/2s2h/BenPort.cpp`, `mm/2s2h/Rando/ConvertItem.cpp`, `mm/2s2h/Rando/Rando.h`, `soh/soh/OTRGlobals.cpp`, `soh/soh/Enhancements/randomizer/item.cpp`, and the two conflicted deviation documents.

**Interfaces:** Preserve Fleet pair export and cross-grant callbacks; add upstream shared mask, tier and tick callbacks. Generator, headless and plando callers must use the same combined semantics.

- [x] Run `git merge-tree --write-tree cac526a8b9dc08c83f4c2ad7fece0113d284513d 94eb185e4abcc2d568aa8241fa02c43cdd86c439`. Observed 13 conflict paths.
- [x] Add focused production behavior regressions for combined sharing and progressive grants; confirm the unresolved baseline cannot pass.
- [x] Resolve all 13 paths and inspect auto-merged grant bodies for duplicate exports and stale references.
- [ ] Run the shared-item/grant diagnostic runners and scan tracked source for conflict markers. Expected: all focused checks pass and no conflict markers.

### Task 2: Port the newer standalone SoH delta

**Files:** The changed `soh/` paths and corresponding diagnostic scripts/docs between `fcc86528612ecabb192fe3642acaacf7c2766869` and `6f03c439ab83cbf149c4a368b529e53724f6fc47`.

**Interfaces:** Retain ComboShip CMake, global context, per-game audio/clock and MM resource ownership. New donor actor/material/PAK functions retain their public names and signatures.

- [x] Three-way merge each changed existing file using the exact old donor blob, current host and new donor; copy new files. Report conflicts rather than selecting a whole side.
- [x] Reconcile conflicting host adaptations; port production tests and their runners together.
- [x] Run `bash scripts/diagnostics/run_stabilization_tests.sh` with its JSON include dependency, plus `python3 scripts/diagnostics/run_time_pedestal_tests.py`, `python3 scripts/diagnostics/run_pedestal_sword_selection_tests.py`, and the actor/material/PAK runners with their documented dependencies. Expected: relevant focused production checks pass; unavailable dependency checks are recorded precisely.

### Task 3: Verify and publish the candidate

**Files:** integration report; draft PR and existing GitHub Actions workflows.

- [x] Confirm protected host plumbing and donor delta coverage from actual diffs; record every integration-specific choice.
- [x] Run fresh whole-branch review using superpowers:requesting-code-review, fix material findings with focused regression evidence.
- [ ] Run `git diff --check`, source conflict scan and required formatting/asset checks. Expected: clean changed-source checks; do not claim missing local checks passed.
- [ ] Publish candidate commit/branch and draft PR targeting `integration/soh-stabilized-20260912`; inspect Windows/Linux CI and fix integration failures.
- [ ] Hand off exact commit, PR/build links, actual test results and the spec's targeted runtime checks. Keep candidate/master status explicit.
