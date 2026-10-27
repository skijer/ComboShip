# Cumulative Final Source Integration Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task by task. Existing user authorization covers source publication and the requested cumulative merge; no further design choice is needed.

**Goal:** Integrate the latest Woods of Mystery source into the current cumulative PR, verify the resulting candidate, and complete the authorized source merge to develop with recovery refs retained.

**Architecture:** Preserve PR44 head `0864a312f9f03e4d192427d8627d4d3ae9bb905b` and develop `a82852a3a02c8cabc541cce7c124da71c5755efb`. Apply only the Woods delta after already integrated donor `dbd2f6ec54777b72e8f38a5c30642b7053e48b90` through `ef72039e4d01879a24ea58b955ff5bde6572f145`; retain all other cumulative work unchanged. Source integration is distinct from configured runtime acceptance and does not promote an external scene archive to an accepted runtime master.

**Tech Stack:** Git, C/C++, Python, Node, native resource binaries, clang-format 14.0.6, GitHub Actions.

**Spec:** User's October 19 cumulative merge request; `docs/poc/dinner-merge-audit-20261019.md`; current PR44 follow-up records for Chateau, Saria and What's New; verified Woods POC6 recovery manifest.

## Global Constraints

- Preserve accepted inputs, game/mode/Alt ownership, progression, native collision and audio behavior.
- Keep Chateau 4K baking, all Saria poses and hex aura, and What's New UI intact.
- Retain required scene/interior/NPC pack dependencies and known runtime holds.
- Preserve recovery source on GitHub before long hosted builds; verify independent reconstruction.
- Use exact source/tree/head identities and expected-head leases for publication and merge.
- Do not call source/static/build evidence in-game acceptance.

## Review Focus

- No stale whole-source donor overwrites newer forest, Mirror, Chateau, Saria or UI changes.
- Native bridge installation and vertex-patch restoration remain safe on missing/short/Alt resources.
- Private seasonal textures preserve original alpha, dimensions, scale and untinted resources.
- Ground grass, wall trim, native foliage and carried/replacement grass receive intended seasonal ownership.
- Built-in bridge resources are packaged; optional POC6 whole-scene geometry remains an explicit installation.

### Task 1: Reconcile and preserve inputs

**Files:** Read POC6 recovery manifest and source hashes; create this plan and a final merge record under `docs/poc/`.

- [ ] Verify every recovery payload and source payload hash against the exact donor commit.
- [ ] Run existing Woods ownership/effect probes on the current PR baseline.
- [ ] Record both source inputs, external archive identity, and rollback dependencies.

### Task 2: Integrate bounded Woods follow-up

**Files:** Exactly the 42 paths in `git diff --name-only dbd2f6ec5 ef72039e4`, plus integration documentation. Runtime changes are confined to Woods bridge/hex/makeover, MM scene foliage, grass actors and eight built-in bridge resources; remaining paths are authoring tools and tests.

- [ ] Apply the donor delta with three-way preservation and inspect any overlapping edits.
- [ ] Assert every changed owned path matches the donor except deliberate, documented integration corrections; assert all other source paths match the PR baseline.
- [ ] Run native assets/bridge, seasonal texture/grass and relevant existing foliage/forest regressions; reproduce the final archive from preserved inputs and verify its manifest hash.
- [ ] Check clang-format 14.0.6, conflict/whitespace checks and the custom-asset collision gate.
- [ ] Commit a bounded source checkpoint and verify an independent patch reconstruction from the preserved PR head.

### Task 3: Publish, verify, and merge

**Files:** Final source/verification receipt, PR description, protected recovery refs.

- [ ] Obtain one fresh review of the bounded integration and resolve substantive findings.
- [ ] Preserve old PR/develop refs and publish the new candidate with an expected-head lease.
- [ ] Inspect complete hosted gate and matching Windows/Linux build results; diagnose actual failures within authorized scope.
- [ ] Merge only the inspected candidate head, verify develop's resulting source and retain exact recovery refs.
- [ ] Report included source, decisive verification, separate external archive requirements and remaining runtime holds.
