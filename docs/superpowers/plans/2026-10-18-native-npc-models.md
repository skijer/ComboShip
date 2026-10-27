# Native NPC Models Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add five optional native NPC packs and an archive-only fitting route.

**Architecture:** Paired private native/Alt rigs select model geometry in existing
actor adapters. A shared resolver selects model-specific fitted normal clips at
sampling time while retaining native actor animation identity and morphing.

**Tech Stack:** C23 actor adapters, C++20 resource bridge, Python/numpy/Pillow/scipy O2R tools.

**Spec:** docs/superpowers/specs/2026-10-18-native-npc-models.md

## Global Constraints

- En_Ko girl types 1,5,6,9,10 and static girl; preserve separate Fado.
- Keep both Saria choices on the same actor contract, enabled one at a time.
- Preserve native clip identity, timing and root motion, plus native interpolation/morphing.
- Do not run an expensive whole-game build for this checkpoint.
- Do not equate offline verification with runtime acceptance.

## Review Focus

- Partial packs and live Alt OFF must retain native actor resources.
- Reinitialized skeleton records must not select an earlier unrelated rig.
- Invalid private clip types/counts/channels/root motion must fall back.
- Morph snapshots must sample fitted targets without changing native identity.
- Offline visual defects must remain visible in the release limitations.

---

### Task 1: Private actor routing

**Files:** NativeNpcModels.h; En_Sa, Demo_Sa, En_Ko, En_Ma1/2/3 and En_Viewer actor files;
soh/tests/native_npc_models_routing_test.c; run_native_npc_model_tests.py.

**Interfaces:** Consume original native skeleton/head/prop paths. Produce
NativeNpcModel_SelectSkeleton, SelectEyes/SelectHead and SariaOcarina selectors.

- [x] Pin both Saria variants, all five girl IDs, both Malon ages and partial-pack fallbacks in routing tests.
- [x] Implement optional private rigs, OPA/XLU head routes, native mouth-index mapping and props.
- [x] Compile the eight real affected actor translation units and run the routing fixture.

### Task 2: Fitted animation and mesh resources

**Files:** NativeNpcPose.h/.cpp, NativeNpcPoseResources.h; z_skelanime.c;
static_story_kokiri.c; native_npc_pose_resources/bridge/sampling tests.

**Interfaces:** NativeNpcPose_ResolveAnimation(const SkelAnime*, AnimationHeader*)
returns original or a validated private clip. NativeNpcPose_SelectLimb returns
original or an optional private pose mesh.

- [x] Run the missing-contract test and confirm failure before adding the resolver.
- [x] Implement scoped resource paths, Fado's legacy alias and latest-record lookup.
- [x] Reject incorrect type/count/channel/root motion in the production bridge fixture.
- [x] Exercise the real frame sampler, fractional frames and both morph directions.
- [x] Skip the static donor overlay when a full fitted seated clip owns that pose.

### Task 3: Asset compiler and review

**Files:** scripts/npc_models native compiler, editor, verifier, preview and tests;
MIT native_assets resource writers.

**Interfaces:** build(source_path, native_path, variant, output) creates a candidate;
edit(parent, native, recipe, output, overlay=False) creates a fitted archive.

- [x] Verify correct diffuse index, seam ownership and animation channel bounds.
- [x] Export both Sarias, girl/twins, adult Malon and young Malon under private paths.
- [x] Test pose-editor roundtrip, exact parent hash, retained body bytes and rejected root edits.
- [x] Complete current archive readback and refresh offline pose/expression sheets.
- [x] Record asset-only pose-edit proof and current visual fitting limitations.

### Task 4: Source checkpoint

**Files:** docs/native-npc-models.md; source patch, verification logs and five O2Rs.

**Interfaces:** One reviewable POC bundle with exact source-base and artifact hashes.

- [x] Format changed source, inspect the complete diff and rerun focused checks.
- [x] Verify archive contents against manifests; verify the source tree against its recorded base at publication.
- [ ] Publish the isolated source checkpoint and persist the separate asset bundle without native game archives or original GLB duplicates.
