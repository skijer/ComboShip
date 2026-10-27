# Authored Lantern GI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore the authored Lantern GI when a pack replaces the generic Poe actor lantern.

**Architecture:** Change only the shared legacy GI selection table. Preserve authored-path replacement priority and incomplete-resource fallback in the native and cross-game consumers.

**Tech Stack:** C++20, native graphics headers, Python diagnostic runner.

**Spec:** `docs/poc/lantern-authored-gi-20261008.md`.

## Global Constraints

Preserve geometry, textures, glass, framing, held behavior, gameplay and unrelated replacement priorities. Baseline is `d4e6c36d91801aee4707ad5649deb2d90fd80e55`. Runtime proof remains separate from compiler evidence.

## Review Focus

Check both authored passes, absent glass fallback, intentional authored-path replacements, independent host/donor Alt settings, and model/dependency ownership in MM.

### Task 1: Restore authored Lantern selection

**Files:** Modify `soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp`; test `tests/nei_gi/presentation_test.cpp` and the existing Lantern receipt cases in `tests/mm_presentation/pickup_framing_checks.inc`; update the POC record.

**Interfaces:** Consume existing `NeiGi_Draw`, `NeiGi_DrawShop`, `OOT_GetNeiGiDrawInfoForAssets`, `MM_DescribeNeiGi` and `MM_TryDrawNeiGi`. Produce the same APIs with authored Lantern selection preserved under generic Poe replacements.

- [x] Add a regression asserting authored opaque/glass draws under generic Poe mods in either owner, base/Alt settings, both native/shop and MM descriptors/draws. Retain tests for incomplete passes and authored overrides.
- [x] Run `python3 scripts/diagnostics/run_nei_gi_tests.py --combo --held`; expect failure on authored Lantern selection before the fix.
- [x] Remove the Lantern's generic actor path from `HasLegacyGiMod`; explain that dedicated authored paths retain override priority.
- [x] Run the same renderer/held suite and its `--sanitize` variant; expect success. Run asset verification and collision checks. Verify all Lantern resource references and compare its asset hashes to baseline.
- [x] Record evidence and commit the isolated fix; package a recovery checkpoint. Keep in-game appearance unproven until the user's runtime test.
