# Tunic GI recovery implementation plan

> **For agentic workers:** Use superpowers:executing-plans inline for this focused repair.

**Goal:** Keep Spirit Breastplate, Sage's Tunic and Champion's Tunic visible when an incomplete legacy tunic replacement would otherwise suppress their authored GIs.

**Architecture:** The legacy override decision must verify both selected collar/body display lists, with the same per-pass MM/OOt ownership as the native fallback. Retain complete mod priority and use the existing authored presentation when that legacy recipe cannot draw.

**Tech stack:** C++20, registered Ship resource managers, native GI regression fixtures, ASan/UBSan.

**Spec:** Current user report: all three models are missing with Alt Assets on and off while shimmer renders. Preserve their existing designs, materials, sizes and shimmer. Baseline develop `13901c677d0084e0305eec4672c0557f4434aea7`, build run `37561010298`; Windows archive SHA-256 `d8a1fcdf42b564bfa25be8e710baed61c6d3ac2a59871e60c3fdc1db869babd8`. Actual shipped roots and all dependencies match current source after CRLF normalization. The prior cold-owner/alias lookup fix is already present.

## Constraints and review focus

- Only these three tunic selection decisions may change. Preserve complete custom model priority.
- Check missing body/collar, invalid resource type, empty display lists, mixed host/donor ownership, both asset modes and restored active manager state.
- Keep authored geometry/materials and all other equipment, sword, weather and item behavior byte-identical.
- Source verification is not GPU/runtime acceptance. Save a recoverable candidate; no shared-branch push, merge or master promotion.

### Task 1: Reproduce and repair the incomplete legacy recipe

**Files:** `soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp`, `soh/soh/ResourceManagerHelpers.cpp`, `soh/soh/ResourceManagerHelpers.h`, `tests/nei_gi/presentation_test.cpp`, and a focused native resource-query fixture/runner under `tests/nei_asset_priority/`.

**Interface:** `int ResourceMgr_IsGiModelAvailableForGame(const char* game, const char* path)` returns true only for a nonempty display list from the selected registered owner. It scopes nested factory loads without changing Alt mode or retaining raw pointers.

- [x] Add a regression that expects authored tunic model commands, rather than shimmer-only legacy fallback, when either legacy pass is missing or unusable.
- [x] Run `scripts/diagnostics/run_nei_gi_tests.py --combo --sanitize`; observe the selection failure before production edits.
- [x] Implement the typed owner query and use it only for these three legacy tunic recipes.
- [x] Verify incomplete-recipe recovery, complete override priority, native/MM descriptor paths and unchanged identity shimmer; exercise the actual resource query with real DisplayList/Texture types.
- [x] Run the relevant native GI/bridge/priority checks and standard combined suite; record unavailable checks accurately. Focused checks pass. The combined suite stops at an unrelated seed-settings translation unit because the workspace lacks `BS_thread_pool.hpp` (after supplying json, spdlog, ImGui and SDL declarations).
- [x] Review the focused candidate. One fresh read-only reviewer found no Critical or Important source issues and independently reran the focused sanitizer checks. Save the locally committed candidate with source, patch, metadata and validation logs in a recovery ZIP.

Runtime acceptance is pending: receive each item in MM with Alt off/on, including the installed mod stack and a warmed-cache toggle. The candidate remains isolated until that evidence exists.
