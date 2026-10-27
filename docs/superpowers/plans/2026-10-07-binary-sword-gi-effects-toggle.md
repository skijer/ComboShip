# Binary Sword GI Effects and Asset Toggle Correction

> **For agentic workers:** Execute inline with superpowers:executing-plans and test-driven-development. Review the complete candidate before publishing its isolated branch.

**Goal:** Preserve the accepted Din binary sword framing, restore readable sword particles and shimmer, and show the authored sword GIs when the active game's Alt Assets are disabled.

**Architecture:** Keep mesh fitting and Din material layers together. Give procedural sword effects the authored award's presentation transform rather than the arbitrary binary model's coordinate conversion. Route vanilla sword roots and deferred dependencies through the existing archive-pinned `oot-gi-base` view, applying the active host's choice in native and foreign paths.

**Tech Stack:** C++20, Ship resource views, native GBI streams, ASan/UBSan regression harnesses.

**Spec:** The user's 2026-10-07 request; baseline `13901c677d0084e0305eec4672c0557f4434aea7`, the successful develop workflow 37561010298. Checkpoint 12 is recovery evidence. User reports that all Din sword models now fit; tiny effects and lingering custom models are unresolved failures.

## Global Constraints

- Preserve binary vertex decoding, model-fit constants, full-spin framing, Din core/flame layers, award identities, Four Sword palettes, and non-sword behavior.
- Keep unrelated cape, rod, weather, equipment-menu and randomizer work out of this branch.
- Save a recoverable checkout and isolated GitHub candidate; do not merge or declare runtime acceptance.

## Review Focus

- Native and foreign swords must obey the active host when the donor's mode differs.
- Base-path mods and replaced deferred dependencies must not survive vanilla selection.
- Tiny or large binary coordinate systems must not resize procedural particles.
- Human and Goron receipts, shops, and ordinary world draws must preserve their camera fits.
- Missing shipped geometry must not enter a mod-capable fallback; toggles must stay live after warmed caches.

### Task 1: Correct the shared rendering and selection boundary

**Files:** `combo/menu/ComboSwordGiFit.h`, `combo/menu/ComboForeignDrawOOT.h`, `combo/menu/ComboForeignDrawMM.h`, `soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp`, `soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc`, `mm/2s2h/Rando/NeiGiPresentation.cpp`, and their existing sword/GI regression fixtures.

**Interfaces:** Retain the existing resource-fit C ABI and pinned base owner. Add an inline award-effect fit using authored frame bounds. Asset queries remain live; no owner-global Alt state is mutated.

- [x] Add regressions exercising production render functions with a nonidentity binary fit and native/foreign asset transitions.
- [x] Record the expected shrinkage and vanilla-selection failures before correcting the production code.
- [x] Isolate the geometry fit from procedural sword effects and intrinsic presentation flames. Use the existing authored camera fit for those effects.
- [x] Route native and foreign vanilla sword GIs through the shipped base view using the active game's Alt state.
- [x] Run the focused sanitizer suite and real supplied O2R mesh tests on both hosts. Verify full native translation units and relevant standard regressions.
- [x] Review the final diff and resolve all concrete review findings with regressions.
- [ ] Confirm actual installed packs in game before any baseline promotion.

## Evidence and status

- Baseline asset-toggle fixture passes but covers MM native routing only.
- Baseline full GI/held/combo sanitizer gate passes with real dependency headers, including native translation-unit syntax checks.
- Root cause: geometry fitting encloses procedural effects in native, external and legacy/foreign render paths; the previous test's resource-fit seam always returned no correction.
- Root cause: native OoT and foreign descriptors retain mod-capable owner selection outside MM's host-aware native path.
- Whole-candidate review found a negatively cached missing vanilla recipe, a non-sword MM receipt fit regression, and Four Sword's optional-shimmer setting being overridden. All three have reproducing regressions and corrections.
- Final full GI/held/combo ASan/UBSan check passes, including all 61 serialized models, the five native MM receipt-camera configurations, packed effects and native C/C++ syntax gates.
- All ten original Din meshes across three O2Rs pass on both host paths, with model-fit constants and Din layers preserved.
- Eight full native C++ translation units pass with real dependency headers, including both foreign dispatchers' native sources.
- The 72-command combined gate passes 70 commands across the initial run and verified continuations. Wolf packaging is blocked by missing CMake; the unrelated used-FX historical comparison is blocked by unavailable commit `c77c18587a976f6d6cb5c8f91f27593286469218`. The complete broad suite is not claimed green. Leak detection is disabled because the container cannot inspect `/proc`; ASan memory bounds and UBSan remain active.
- In-game acceptance remains pending.
