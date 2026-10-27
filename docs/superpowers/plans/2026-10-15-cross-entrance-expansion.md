# Cross-game Entrance Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans for common integration. Independent host investigations/implementations use scoped parallel-agent ownership; never edit another worker's files.

**Goal:** Add the six approved entrance/recovery areas to the isolated current ComboShip candidate.

**Architecture:** Extend the existing game-tagged routing overlay with explicit pool categories and separate boss completion/recovery metadata. Native host adapters own native entrance identity, scene context and progression; the launcher owns seed selection, cross-host propagation and slot persistence.

**Tech Stack:** C/C++20 native OoT/MM modules, JSON consolidated seeds, Python production-function fixtures.

**Spec:** `docs/superpowers/specs/2026-10-15-cross-entrance-expansion.md`

## Global Constraints

- Original POC: `poc/cross-entrance-expansion-20261015`; resumed candidate: `bridge/cross-entrance-recovery-20261015`. No develop/master promotion or gameplay acceptance is implied.
- Every new pool is opt-in and defaults off. Existing dungeon-only seeds retain their codec and deterministic routes.
- Preserve native reward identities, shared items, game-owned age/time and existing portal behavior.
- Every logical-only grotto ID must be materialized before native scene dispatch.
- Physical boss defeat, logical source completion and physical reward collection remain separate.

## Review Focus

- Native mixed/decoupled routes, both starts and simultaneous Dungeon/Boss/Interior/Grotto pools resolve exactly one composed leg.
- First/repeat MM boss warps grant physical checks once, clear the logical source and discard foreign cutscene layers.
- Farore recall, live death/void and cold reload restore the correct owned host/checkpoint.
- Shared MM grotto contents and exterior return/void positions survive native-off and cross-host routes.
- Replay rejects stale catalogue/options before mutating native seed state; disabled behavior remains native.

### Task 1: Restore current isolated baseline

- [x] Import recovered `f6c4eecc` into PR41 `913bc329`, resolve six overlapping files preserving newer ownership/lifetime changes.
- [x] Run `python3 tests/cross_dungeon/run_tests.py` and `CPLUS_INCLUDE_PATH=../build-deps/usr/include python3 tests/farores_wind/run_tests.py`.
- [x] Commit baseline integration and preserve a recoverable checkpoint (`89281cab`).

### Task 2: Typed pools, seed codec and composed routes

Files: `combo/rando/CrossDungeon{Entrances,Seed,Catalogue,Coordinator,Runtime,State,Reachability}.h`, `tests/cross_dungeon/`.

- [x] Add failing cases for independent pool permutations, expanded catalogue size, v1 identity/replay and cross-pool rejection.
- [x] Add Pool/PoolOptions and boss-specific warp/recovery metadata; compose boss escape through nonboss routes before removing shadowed aliases.
- [x] Add strict v2 codec/options while preserving exact v1 records and RNG.
- [x] Verify focused tests and commit.

### Task 3: Recovery hardening

Files: Farore shared/native bridge files, `mm/src/code/z_play.c`, new `CrossEntranceTime.h`, focused recovery fixtures.

- [x] Prove stale checkpoint on local/foreign recall and legacy migration, then add optional metadata and immediate restore APIs.
- [x] Prove Stone Tower orientation normalization at the native initialization boundary, preserve off/standalone.
- [x] Add conservative generation-time MM reset/owned-clock policy, preserving runtime clock.
- [x] Run Farore and recovery regressions; commit.

### Task 4: Native catalogues and materialization

Files: new `CrossEntranceMmCatalogue.h`, `CrossEntranceMmGrottos.inc.h`; native cross adapters and OoT native grotto materializer.

- [x] Add exact seven expanded pairs, eligible MM interiors, 26 grotto identities and native return/void descriptors.
- [x] Extend OoT catalogue by native types and recorded overrides; keep native access rules.
- [x] Wire logical MM loads/returns into oracle and runtime; force OoT special-load materialization when native shuffle is off.
- [x] Verify catalogue uniqueness, materialization, reverse mapping and context-dependent exits; commit.

### Task 5: Cross-game bosses and progression

Files: new cross-boss shared/host adapters, native completion/world-effect consumers, joint oracle and launcher persistence.

- [x] Add twelve boss records and separate reverse/escape routes; test native mixed and combined dungeon composition.
- [x] Expose physical native defeat results; propagate source logical clears without collecting another boss's checks.
- [x] Notify actual first/repeat warp use, apply owning-game world effects, persist logical completion and preserve MM cycle resets.
- [x] Cover live death, save, Giant/Oath and no-defeat escape rejection; commit.

### Task 6: UI, headless entry points and whole candidate verification

Files: `combo/gui/ComboMenu.cpp`, launcher/headless wiring, diagnostics and POC evidence.

- [x] Add independent opt-in controls for bosses, expanded destinations, interiors and grottos, saved with each seed.
- [x] Wire exact replay/preflight, mask lifetime and new host exports in both launcher and headless paths.
- [x] Run focused suites/sanitizers and available native build checks; perform a fresh whole-branch review.
- [x] Save source checkpoint and evidence; list installed-game acceptance separately.
