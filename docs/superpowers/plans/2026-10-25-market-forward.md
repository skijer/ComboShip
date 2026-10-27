# Black Market Refinement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring merchandise forward, remove rear visual obstructions and repair optional-room/counter regressions.

**Architecture:** Keep the native controller and camera. Change private MM display geometry and its anchors together; retain strict resource ownership for Granny and the counter collision.

**Tech Stack:** C/C++20, Python asset recipes, native ASan/UBSan fixtures.

**Spec:** docs/superpowers/specs/2026-10-25-market-forward-design.md

## Global Constraints

- Preserve fixed shop eye, keeper pose/position, native receipts, seeded stock and talk range.
- Keep original archives, compressed texture pools and develop/master unchanged.
- Configured gameplay acceptance is separate from source/native proof.

## Review Focus

- Granny must retain strict scene/camera/cutscene and original-room identity checks.
- Climbing powers/cheats must not turn the invisible shop barrier into a ladder.
- Forward displays must clear the counter, UI and every selection/return interval.
- Remove complete rear cauldrons without affecting neighboring geometry or OoT.
- Lifetime/reset/re-entry must never reuse an owned shape or collision from a retired scene.

### Task 1: Optional room and counter behavior

**Files:** MM BlackMarket.cpp/.h, BlackMarketCounterBarrier.h, z_bgcheck.c and native scene/collision fixtures.

**Interfaces:** Consumes existing Granny attachment and native shop readiness. Produces an exact optional-room identity path and a barrier-owned polygon check if climbing overrides reproduce the report.

- [ ] Add actual-resource readiness cases for the owned Granny shape, mismatched original shape, and foreign replacement.
- [ ] Reproduce the counter failure with the real native wall flags under climbing powers/cheats; retain ordinary-wall controls.
- [ ] Implement the smallest identity/lifetime and collision-policy corrections.
- [ ] Run `python3 tests/black_market/native_mm_scene_runtime_tests.py` and `python3 tests/black_market/run_counter_barrier_native_tests.py`, then existing Granny attachment/native shop checks. Expected: all required cases pass under sanitizers.
- [ ] Commit the bounded correction and record recovery state.

### Task 2: Closer displays and unobstructed shelves

**Files:** BlackMarketMmShopLayout.h, MM BlackMarket.cpp, BlackMarketInteriorEffects.h, mm_shop_presentation.py, matching display/effects tests and POC report.

**Interfaces:** Consumes Task 1's unchanged native shop/collision interfaces. Produces the translated private MM display, complete rear-pot removal and scoped vapor suppression.

- [ ] Add a connected-triangle regression that rejects either remaining rear cauldron or orphaned contents/attachments while preserving other faces.
- [ ] Translate shelf vertices/collision/stock together; remove only the audited rear components and corresponding MM steam.
- [ ] Run native camera/selection paths, shelf geometry, room asset and effects suites. Expected: all eight items clear room/shelves/UI, retained geometry is exact, resource graph closes, OoT stays unchanged.
- [ ] Record exact source/asset evidence, commit, perform independent whole-branch review, and save/restore-verify a durable recovery artifact.
