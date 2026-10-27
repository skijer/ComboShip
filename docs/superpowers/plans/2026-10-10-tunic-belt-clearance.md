# Tunic GI belt recovery implementation plan

> **For agentic workers:** Use superpowers:executing-plans inline. Steps use checkbox tracking.

**Goal:** Recover the previously authored belt/harness correction missing from the current ComboShip integration.

**Architecture:** Restore only the garment sampling, fitted belts and Champion rear harness from the October 3 candidate. Regenerate the three tunic GI assets and checkpoints, preserving all other current recipe, rendering and gameplay behavior.

**Tech Stack:** Python, NumPy, existing XML display-list exporter, GLB checkpoints and both-host C++ GI fixtures.

**Spec:** User reports additional tunic GI belts clipping into cloth on the latest commit; October 3 recovery record `GI_Material_Refinement_Checkpoint_20261003.txt` documents the prior correction.

## Global constraints

- Baseline: PR #41 `3abeff897e38f00de4a70d674c463212ec110575`.
- Historical fitting candidate: `dcff6120df6b8a008cf8e805947ed5b20e2b11d8`, never promoted or runtime accepted.
- Preserve current torso/sleeves, textures, materials, UVs, effective scale, equipment behavior, render routing and shimmer.
- Candidate remains isolated; no master promotion. Old GI replacement archives can override bundled resources and need equivalent review.

## Review focus

- Full belt circumference and width, including folded side/back cloth.
- Piping, buckle/cross-pin and Champion rear ribbon after vertex quantization.
- Installed GLBs/resources must carry the correction, not only the authoring recipe.
- Normal/winding and base/Alt dispatch remain correct in both game hosts.
- Unrelated geometry and asset resources remain unchanged.

## Task 1: Recover fitted tunic geometry

**Files:** `tools/nei_gi/SOURCE/equipment_revamp.py`, new `tools/nei_gi/test_tunic_clearance.py`, the three tunic directories under `soh/assets/custom/objects/nei_gi_redesign/` and `tools/nei_gi/CHECKPOINTS/`.

**Interfaces:** Existing `BUILDERS[slug]()` returns a model; the recovered `_belt(m, body, ...)` and `_rear_harness(m, body)` sample the existing torso grid.

- [x] Write independent radial ray/triangle clearance checks for recipes and installed GLBs.
- [x] Run the checks on baseline; expect cloth penetration in belts and rear harness.
- [x] Restore only `_garment_surface`, `_belt`, `_rear_harness` and their three call sites from the recovered historical recipe.
- [x] Rebuild only `spirit_breastplate`, `sages_tunic`, `champions_tunic` with `--install`.
- [x] Run clearance checks, full asset verification and relevant native/foreign GI fixtures; expect passes.
- [x] Compare all protected parts/materials/scales and unrelated resource hashes to baseline; render front-quarter/back views.
- [ ] Commit the isolated candidate and preserve the matching correction archive/source for runtime review.

## Evidence ledger

- Latest recipe and installed GLBs match each other and still contain the original fixed oval belts.
- October 3 archive differs in the three GI meshes and contains the recorded fitted geometry.
- Recovery bundle prerequisite commit is unavailable. Its pack checksum is valid; the old recipe base was reconstructed by removing the later forge dispatch and verified against blob SHA `7b4e5e83eb321fb611e41f63e0e036a0a604d393`; the exact historical recipe delta decoded successfully.
- Independent mesh tests reproduced 32 baseline failures across recipe and installed belt/piping/buckle/harness geometry. The restored belt and harness passed; the historical buckle fit left as little as 0.011 author units at a side fold. Its complete horizontal/vertical footprint is now sampled before positioning it. Both recipe tests pass.
- The clearance regression is wired into the existing ComboShip NEI regression runner.

- Final checks: four quantized recipe/checkpoint clearance tests pass; all 64 serialized GI models pass; 68 authored binding/native/common/shop/MM renderer fixtures and all five native MM form camera bounds pass; four native/Combo host material fixtures pass under ASan/UBSan; two real asset-owner/Alt/scope fixtures pass under ASan/UBSan after supplying the existing real spdlog/fmt headers. Bash syntax and git diff checks pass. Full application builds and in-game testing were not run.
- Independent reviewer found no Critical/Important issue. Exact protected geometry/normals/UV/material/scale equality and resource byte parity confirmed. Minor test-hardening opportunity deferred: the clearance helper requires at least one matching fitting part, rather than each named part; normal asset geometry verification still runs.
- Resource counts and all texture/matrix bytes are unchanged. Triangle counts increase from 5,154 to 8,382 Champion, 6,872 to 9,084 Sages, and 5,534 to 7,746 Spirit. Runtime performance remains untested.
- Runtime candidate only: preserved separately; no merge, push, promotion or implied game acceptance.
