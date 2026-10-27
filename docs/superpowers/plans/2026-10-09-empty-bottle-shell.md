# Empty Bottle Casing Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for this single inline task. Steps use checkbox tracking.

**Goal:** Route the native empty-bottle GI to the approved TP/Peyron casing.

**Architecture:** Select the existing private casing in native and exported empty
recipes. Preserve selected empty mods, missing-resource fallback, owner resource
scope and two-pass rendering; mark foreign empty descriptors as live appearance.

**Tech Stack:** C native drawers, C++ foreign descriptors, production-body diagnostics.

**Spec:** `docs/poc/oot-empty-bottle-shell-20261009.md`

## Global Constraints

- Parent is `c0181856968dd6f3869be0d1aa240743091fd6a8`.
- Preserve authored swords under Alt and vanilla as reported by cor.
- Preserve existing bottle/fairy geometry, materials, animation and effects.
- Leave the shared integration branch unchanged; supply an isolated candidate.

## Review Focus

- ROM archive priority must not override the default authored selection.
- Both native and foreign empty recipes must agree on ownership and OPA/XLU.
- Whole/partial selected empty mods must retain priority.
- Missing private roots and short export buffers must have safe fallback/refusal.
- Fairy precedence/motion, casing hashes and sword routing must remain unchanged.

### Task 1: Select the authored empty recipe

**Files:**
- Modify: `soh/src/code/z_draw.c`, `mm/src/code/z_draw.c`
- Modify: `combo/menu/ComboItemDrawOOT.h`, `combo/menu/ComboItemDrawMM.h`
- Modify: `scripts/diagnostics/run_bottle_gi_tests.py`, `tests/bottle_gi/render_test.cpp`
- Modify: `scripts/diagnostics/run_fairy_bottle_tests.py`, `scripts/diagnostics/run_bottle_contents_tests.py`

**Interfaces:**
- Consumes: original empty row and `ComboFairyBottle_BundledShell` only.
- Produces: `GetItem_EmptyBottleShell(s16 drawId)` in each host; matching native/exported two-pass lists and live empty descriptors.

- [x] Execute the real empty drawer/export with private casing available and assert private roots; test missing roots, selected mods and short buffers.
- [x] Run `python3 -B scripts/diagnostics/run_bottle_gi_tests.py`; observe the parent failure because native roots are submitted instead of the private casing.
- [x] Implement direct native/exported selection and live appearance flags without inheriting fairy Blue Fire precedence.
- [x] Run focused bottle/fairy, sword and broad GI compilation/identity checks; expect pass.
- [x] Review the diff, record evidence and commit the isolated candidate.

## Execution ledger

Ruling: replace the initial alias-only approach with direct recipe selection —
active boot mounts the ROM after port assets and shadows native names — the
alias-only approach would leave the observed bottle unchanged. No alias files
remain. The final implementation keeps fairy and sword selection unchanged.
