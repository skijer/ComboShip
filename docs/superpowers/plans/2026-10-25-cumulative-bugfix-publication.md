# Cumulative ComboShip Bugfix Publication Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Publish the six completed source checkpoints as one isolated, fully checked cumulative candidate with a matching complete Windows package.

**Architecture:** Start from the passing 83eed9ff snapshot. Verify each input through a separate Git index before bounded patch integration; preserve baseline/rollback refs. Publish an exact verified source tree, retaining the existing required CI matrix and asset pool identities.

**Tech Stack:** C/C++, Python 3, Git, CMake, GitHub Actions, lossless compressed source assets.

**Spec:** docs/poc/cumulative-bugfix-spec-20261025.md

## Global Constraints

- Public repository: marsh6487/ComboShip.
- Keep develop/master unmerged; final promotion requires separate user acceptance.
- Keep the passing 83eed9ff candidate and the previous working installation recoverable.
- Preserve required assertions and all six regression groups.
- Lossless texture part limits: 96 KiB decoded and 128 KiB encoded.
- Deliver a package only after every required check passes for its final source tree.
- Build verification does not establish configured gameplay acceptance.

## Review Focus

- A narrow input must not replace newer shared code or regress accepted scene behavior.
- Solo/combined and native/Alt resource ownership must survive GI/icon and scene transitions.
- Counter/Granny changes must preserve shopkeeper interaction and ordinary surface behavior.
- Unknown silver groups must keep native pickup/count/despawn behavior without invalid save indexing.
- Missing, failed or cancelled required CI results must prevent package delivery.

### Task 1: Verify and inventory the exact inputs

**Files:** docs/poc/cumulative-bugfix-inputs-20261025.json; docs/poc/cumulative-bugfix-spec-20261025.md.
**Interfaces:** Consumes unmodified source ZIPs and baseline 83eed9ff; produces exact input hashes, source commits, restore trees and protected asset identities.

- [x] Verify ZIP CRC, original snapshot hashes and each bounded patch in a fresh index.
- [x] Compare silver-hook production baseline bytes against the selected public baseline.
- [x] Record superseded inputs and all pre-existing runtime holds.
- [x] Run baseline CI-policy tests; Expected: all existing tests pass.
- [x] Commit the inventory/spec/plan as a recoverable planning checkpoint.

### Task 2: Integrate the completed bounded checkpoints

**Files:** Only paths named by each verified patch; scripts/ci/regressions.json.
**Interfaces:** Consumes Task 1 patches/identities; produces cumulative production source and unchanged required regression commands.

- [x] Apply each patch with index verification; commit each bounded source input.
- [x] Resolve only actual overlaps, preserving the intended final checkpoint bytes.
- [x] Add the existing Domain silver-hook sanitizer regression to a required group.
- [x] Verify every original registry command remains present and all changed source files match inputs.
- [x] Run each checkpoint's focused native/menu/asset tests; Expected: all pass.
- [x] Save and independently restore a durable cumulative source recovery checkpoint before long builds/publication.

### Task 3: Validate assets, formatting and independent review

**Files:** docs/poc/cumulative-bugfix-20261025.md; existing asset generators and tests.
**Interfaces:** Consumes Task 2 source; produces verified source tree, reconstruction receipts and final review.

- [x] Run the actual hosted preflight commands locally with pinned tools.
- [x] Compare every compressed source part with baseline; Expected: identical reuse unless an input explicitly changes one.
- [x] Reconstruct market/spin/other required source resources and check path collisions.
- [x] Run independent whole-branch review against 83eed9ff and this spec; correct material findings with focused regression evidence.
- [x] Commit final in-scope corrections and checkpoint the exact source.

### Task 4: Publish isolated candidate/cumulative source and complete CI

**Files:** Remote candidate/cumulative refs and draft PR; publication receipts.
**Interfaces:** Consumes Task 3 exact tree; produces public commit/tree and passing exact-tree CI.

- [ ] Publish only the candidate refs and draft PR; retain all rollback refs.
- [ ] Verify the full remote source tree and unchanged develop/master state.
- [ ] Run Windows/Linux and all six required groups with the final gate.
- [ ] Diagnose in-scope failures without weakening assertions and rerun on the corrected head.
- [ ] Verify final passing CI checkout tree equals the public candidate tree.

### Task 5: Verify the matching package and hand off

**Files:** Final inclusion ledger, installation notes, hashes, Windows package, recovery ZIP.
**Interfaces:** Consumes Task 4 fully passing CI and exact-tree identity; produces reviewable installable candidate and rollback instructions.

- [ ] Download the Windows artifact only after all required checks pass.
- [ ] Match GitHub digest; CRC/hash every package member and required port resources; verify x64 PE files.
- [ ] Record complete package and key executable/DLL/port archive hashes.
- [ ] Save final ledger/install/recovery artifacts and link the passing package.
- [ ] List focused remaining gameplay checks; state candidate status and keep develop/master unmerged.
