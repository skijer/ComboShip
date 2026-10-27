# Sword GI Recovery Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to finish this recovered plan inline.

**Goal:** Complete the recovered sword GI clipping and live Alt Assets selection candidate.

**Architecture:** Preserve fitting commit `2f447200`. MM vanilla mode must route redesigned sword roots and their deferred dependencies to the shipped OoT GI archive; Alt mode retains selected custom geometry. A separate resource view preserves donor settings and isolates the cache.

**Tech Stack:** C++20, native Ship/Fast resource APIs, Python diagnostic harnesses, ASan/UBSan.

**Spec:** `/workspace/scratch/d92b161704c0/ComboShip_Sword_GI_Checkpoint11_20261006/READ_FIRST.txt` and the user's sword-only instructions.

## Global Constraints

- Preserve shimmer, particles, actual selected custom models, and the verified binary-array fitting fix.
- MM vanilla must use the authored redesigned sword GIs, including when the OoT donor has Alt Assets enabled.
- Keep unrelated dungeon/bottle, equipment, weather, audio, and actor work outside this candidate.
- No push, merge, or master promotion; user will bundle other fixes.
- Source/harness verification does not establish in-game acceptance.

## Review Focus

- Deferred root, vertex, display-list, texture, and hash lookups retain shipped-archive ownership.
- `.meta` aliases and base-path mods cannot replace vanilla geometry or dependencies.
- Repeated host/donor toggles and warmed caches preserve the selected model.
- Missing shipped resources fail without borrowing a mod or corrupting donor state.
- Shared workers, registry teardown, and temporary resource views have safe lifetimes.

---

### Task 1: Finish archive-pinned sword resource selection

**Files:** ResourceManager.cpp/.h; ResourceLoader.cpp/.h if alias scoping needs it; ResourceManagerHelpers.cpp; MM/OoT NeiGiPresentation.cpp; NeiResourceRouting.cpp; NeiGiFrameFit.h; sword resource-view/asset-toggle/receipt tests and diagnostic runners.

**Interfaces:** `ResourceManager::CreateResourceView(const std::shared_ptr<Archive>&)`; `OOT_NeiEnsureGiBaseOwner()`; `OOT_GetNeiGiDrawInfoForAssets(const char*, int32_t, CwItemDrawInfo*)`.

- [x] Replace the mock-only view test with a production loader/archive regression; observe the archive-routing failure before changing code.
- [x] Carry archive ownership through disk reads, aliases, hash dependencies, and cache keys without changing ordinary donor selection.
- [x] Verify owner initialization and teardown, missing resources, host/donor toggles, and framing namespace handling.
- [x] Run focused production renderer/receipt/real-pack/native-header checks; inspect each result and resolve failures.
- [x] Obtain one whole-candidate review, fix important findings with regressions, and commit the isolated candidate locally.

### Task 2: Preserve the complete candidate

- [ ] Save source patches/files, a self-contained git bundle, exact commit/hash metadata, checks, and a short runtime test list in a recovery ZIP.
- [ ] Verify ZIP integrity and recovery against the recorded baseline; save the checkpoint and deliver it.

**Expected validation:** Passing native loader ownership, asset-toggle, receipt, model-fit and GI regression commands; `git diff --check` clean. Runtime remains pending for Human/Goron pickups and full-spin/Alt transitions with the actual installed pack stack.
