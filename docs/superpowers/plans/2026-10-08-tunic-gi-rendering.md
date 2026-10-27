# Tunic GI rendering correction implementation plan

> **For agentic workers:** Use superpowers:executing-plans inline. The user authorized diagnosis and correction, then explicitly asked to hold the push after validation. Keep this candidate local so their other fixes can be included; do not push or create a PR.

**Goal:** Render the authored Champion's Tunic, Sage's Tunic and Spirit Breastplate GIs under both asset modes, retaining replacements of their individual models.

**Architecture:** Generic `object_gi_clothes` replacements belong to the vanilla Goron/Zora tunic recipes. They must not suppress these three distinct `nei_gi_redesign` models. Keep item-specific replacement priority, and initialize the shared legacy tunic fallback's neutral material before geometry is submitted.

**Tech stack:** C++20, real engine headers and GBI packets, production-function fixtures, ASan/UBSan, GitHub Windows/Linux CI.

**Spec:** User report on October 7: all three tunic meshes remain absent after the merged repair. Baseline develop `a82852a3a02c8cabc541cce7c124da71c5755efb` (PR #38). Supplied rotated logs cover an MM session but contain no tunic receipt or build stamp; do not infer a precise executing commit or GPU acceptance from them.

## Global constraints

- Preserve geometry, textures, framing, shimmer, equipment ownership and every unrelated item.
- Preserve replacements at each item's own redesign path. Change only generic shared-tunic precedence for these three awards.
- Keep the original baseline unchanged. Candidate branch: `fix/tunic-gi-rendering-20261008`.
- Report source/build verification separately from game visibility. The October 7 follow-up holds all pushes; preserve the local branch and recovery checkpoint.

## Review focus

- Complete generic packs must not bypass any of the three authored meshes in native OoT, native MM or foreign OoT-in-MM paths.
- Independent host/donor Alt toggles and warmed selectors must remain live.
- Replacements at the three specific redesign paths must retain their own geometry and shimmer.
- A previous GI with zero primitive alpha or black colors must not make a legacy tunic fallback invisible.
- Missing authored assets must retain a bounded fallback rather than changing inventory/progression.

### Task 1: Correct shared-tunic replacement precedence

**Files:** `soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp`, `tests/nei_gi/presentation_test.cpp`.

**Interface:** `HasLegacyGiMod` no longer treats the vanilla clothes roots as replacements for the three authored tunics. `HasRedesignGiMod` remains unchanged.

- [x] Add a regression with both generic legacy passes present, marked as replacements, and each individual GI present. Assert the native and cross-game renderers submit the specific model and its shimmer in both asset modes.
- [x] Run `scripts/diagnostics/run_nei_gi_tests.py --combo --sanitize`; observe the new generic-precedence assertion fail.
- [x] Remove only the three generic tunic entries and their now-inapplicable completeness branch; preserve all other replacement rules.
- [x] Run the GI renderer, native MM, foreign bridge and asset-priority checks. Commit the correction.

### Task 2: Make the legacy fallback independent of previous material state

**Files:** `mm/2s2h/Rando/DrawItem.cpp`, `soh/soh/Enhancements/randomizer/draw.cpp`, a focused production-function material fixture and runner.

**Interface:** Both legacy tunic drawers initialize neutral primitive/environment colors with opaque alpha before their first geometry packet. The generic tint drawer remains unchanged for other items.

- [x] Execute the actual fallback drawers into real GBI packets after zero-alpha and black-material inputs; assert opaque neutral material precedes the tunic geometry.
- [x] Watch the test fail on the baseline, implement the scoped material initialization, then verify native and Combo configurations with sanitizers.
- [x] Finish all 80 commands of the CI regression gate in staged runs and obtain independent source review with no findings. Preserve a local recovery checkpoint with exact commits and limitations. Do not push or create a PR.

Runtime acceptance: receive each tunic in MM, toggle Alt OFF/ON/OFF while its GI is visible, and confirm all three individual models plus shimmer. Test both native MM entries and foreign OoT entries. GPU visibility and installed-pack parity remain pending until demonstrated.

## Verified local candidate

Source correction: `551d23add853ba731dc5eb0f1498d86204e19596`.

- The complete-generic-pack regression and stale-material packet regression both fail on the old production functions and pass with this correction.
- Native and Combo OoT/MM renderer, foreign bridge and asset-priority fixtures pass, including ASan/UBSan runs.
- The actual three changed translation units compile under the Release Ninja configuration. This verifies these source files, not a complete linked application or packaged build.
- clang-format-14 accepts the three changed production files. The asset-collision gate and conflict-marker check pass.
- All 46 protected tunic asset, authoring and checkpoint files match baseline bytes.
- One independent read-only review found no critical, important or minor issues and reran the GI/material sanitizer checks successfully.

All 80 commands in `run_combo_nei_regressions.sh` passed across staged runs. The initial completed segment passed checks 1–51, then check 52 stopped because CMake was absent from PATH. With local CMake/dependencies configured, checks 52–80 all passed. ASan/UBSan remained enabled where specified; leak scanning was disabled because this sandbox cannot provide the required `/proc` access. The pre-existing CMake texture-scroll hook was applied during production compilation and then restored; it is not a change in this candidate.

After restoring the build hook, `run_nei_gi_tests.py --combo --sanitize` and all four configurations of `run_tunic_material_tests.py --sanitize` pass again on the clean committed production source.

Status: implemented and locally source/build verified. Runtime acceptance, installed-pack parity and full linked Windows/Linux package validation remain pending. The branch is held locally; no push, PR, baseline promotion or runtime acceptance is implied.
