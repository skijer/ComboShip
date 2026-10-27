# Ikana, fairy and bottle receipt correction plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Correct the upside-down Ikana GI, restore the bottled fairy core and visible pink motes, and provide content-specific receipt dialogue for every bottle content.

**Architecture:** Retain the cumulative PR #41 implementation. Correct the shared pose and fairy helpers, extend the shared receipt catalog, and preserve each host's native message bank when available.

**Tech Stack:** C/C++, Python production-function regression harnesses.

**Spec:** User's October 11 screenshots and instruction to include all potions, blue fire and every other bottle content.

## Global constraints

- Preserve baseline `2212ea8c3429ad0cfc94f6d342532e083456153e` and unrelated work.
- Do not change worn shields, companion fairy actors, bottle casing size, inventory or grants.
- Report source/build checks separately from runtime acceptance.

## Review focus

- Upright and nearly square shields retain their current pose.
- The native fairy core applies camera rotation exactly once in either host.
- Missing donor banks still yield localized, content-specific lessons.
- Shared MM bottle names retain the MM native receipt when available.
- Full and refill aliases do not fall back to generic names.

### Task 1: Ikana pose

**Files:** `combo/NeiGiModelBounds.h`, `tests/ikana_gi/model_pose_test.cpp`.

**Interface:** Keep `ShieldRollZ()` and all owner-pinned consumers unchanged.

- [x] Reproduce the native worn decal's negative-X chin in the geometry test; run `python -B tests/ikana_gi/run_tests.py` and observe the chin-up failure.
- [x] Reverse the detected sideways quarter-turn, retaining the tilt and ambiguity threshold.
- [x] Run the same suite and its `--sanitize` variant; expect exit zero.
- [x] Commit the verified correction.

### Task 2: Fairy core and motes

**Files:** `combo/menu/ComboFairyBottleDraw.h`, `soh/soh/Enhancements/randomizer/NeiGiBottleShimmerPolicy.h`, `tests/fairy_bottle`, `tests/bottle_gi/polish_policy_test.cpp`, corresponding runners.

**Interface:** Keep existing native/foreign draw recipes and resource owners.

- [x] Reproduce double billboard rotation using each real engine `Matrix_ReplaceRotation`; require a camera-facing core with preserved position/scale. Require mote radius and opacity sufficient to separate them from the core. Observe failures before implementation.
- [x] Reset only glow-limb rotation before its native DL applies the billboard. Widen and brighten the four GI-local motes.
- [x] Run fairy and bottle-polish suites; expect exit zero.
- [x] Commit the verified correction.

### Task 3: Every bottle receipt

**Files:** `combo/menu/ComboVanillaItemReceiptText.h`, both host receipt handlers and `tests/item_receipts`.

**Interface:** Preserve `Find`, `Body`, native bank controls, icons and read-only export ABI.

- [x] Extend warm-bank and cold-bank production tests to every catalog bottle, all 21 content types, empty/half milk and refill aliases. Observe missing fairy/potion receipt failures.
- [x] Add localized content lessons, exact aliases and explicit native MM bottle precedence.
- [x] Run item-receipt and native syntax suites; expect exit zero.
- [x] Review the entire diff, commit and preserve the candidate for configuration-specific runtime acceptance.


## Verification record

The pre-fix runs reproduced the reversed sideways roll, double fairy-core billboard, unreadably small motes, missing bottle receipts, native MM receipt/model aliases, and live English donor overriding German/French bottle lessons. Each corresponding final check passed.

Passed commands:

- `python -B tests/ikana_gi/run_tests.py`
- `python -B tests/ikana_gi/run_tests.py --sanitize`
- `python -B scripts/diagnostics/run_fairy_bottle_tests.py`
- `python -B scripts/diagnostics/run_bottle_polish_tests.py`
- `python -B scripts/diagnostics/run_bottle_gi_tests.py`
- `python -B scripts/diagnostics/run_bottle_contents_tests.py`
- `python -B scripts/diagnostics/run_mm_item_receipt_tests.py`
- `python -B scripts/diagnostics/run_receipt_syntax_tests.py` (19 production translation units)
- Final syntax check of both changed receipt translation units after the review fix.
- `git diff --check`

An independent read-only review found one Important issue: a live donor could override localized missing-bank bottle lessons. The new regression failed before the MM-local language selection and passed afterward with the full receipt suite. No Critical or deferred Minor findings.

All 21 inventory contents have lessons: Ruto's Letter, Big Poe, Blue Fire, Blue/Red/Green Potion, Fairy, Fish, Bugs, Poe, Milk, Gold Dust, Hot Spring Water, Deku Princess, Seahorse, Spring Water, Zora Egg, Hylian Loach, Obaba's Drink, Chateau Romani and Magic Mushroom. Empty bottles, half milk and refill/captured-name aliases are covered separately. Native capture interactions retain their existing host receipt banks.

Runtime acceptance remains pending: recheck the shield's chin direction under the selected mod stack, fairy core visibility at the screenshot's camera angle, and pink mote readability. Source, command-boundary, sanitizer and header checks are not a substitute for those visual checks.


The first remote follow-up attempt stopped at the formatting gate. Clang-format 14.0.6 reformatted only the receipt catalog and OoT receipt handler. Its `--dry-run --Werror` check passes on all six changed production C/C++ files; the full receipt suite and both changed receipt translation units were revalidated after formatting.
