# Door-only Cross-game Entrances Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Offer independent door-only shops and other interiors while retaining broad shuffle and merging the authorized integration.

**Architecture:** Append isolated Shop and DoorInterior pool identities without changing legacy values. Filter native catalogues by explicit ingress classifications, with broad interiors taking precedence. Extend options/seed replay, menu and headless controls together.

**Tech Stack:** C++20, Python production-body fixtures, native entrance descriptors, GitHub CI.

**Spec:** `docs/superpowers/specs/2026-10-16-door-only-cross-entrances.md`

## Global Constraints

- New options default off; broad legacy pool membership and old seed routing stay unchanged.
- Physical doors only in narrow pools, with each entrance classified separately.
- Preserve native access rules, scheduled opening, reverse routes and recovery.
- Keep the existing game portal exclusions and all progress-preserving forest work.
- User authorized merge into the current PR43 base, not an unrelated baseline.

## Review Focus

- Alternate entrances to one scene may have different kinds; Observatory passage and Curiosity hideout need separate treatment.
- Broad and narrow options together must not duplicate endpoints or change the broad layout.
- Shop-only or door-only generation must work with dungeons disabled.
- Narrow arrivals need the same death/Farore checkpoint lifetime as existing interiors.
- Old v1/v2 seeds must retain their category identities and deterministic replay.

### Task 1: Add isolated narrow pools and native selections

**Files:** `combo/rando/CrossDungeonEntrances.h`, `CrossDungeonSeed.h`, `CrossDungeonCoordinator.h`, `CrossDungeonOot.inc.h`, `CrossEntranceMmCatalogue.h`, `CrossDungeonOotRuntime.inc.h`, `CrossEntranceOotBoss.inc.h`; `combo/gui/ComboMenu.cpp`; `combo/ComboRandoHeadless.cpp`; tests under `tests/cross_dungeon/`.

**Interfaces:** New `Pool::Shop=4`, `Pool::DoorInterior=5`; `PoolOptions::shops`, `doorInteriors`; `IsInteriorPool(Pool)`; ingress kind selector shared by host catalogues. Seed v3 only when a narrow pool is present; v1/v2 continue decoding with their original categories.

- [ ] Write failing option/seed partition and native membership tests, asserting representative open entrances across both games remain excluded.
- [ ] Run fixtures and confirm assertions fail because narrow selection is absent.
- [ ] Add catalogue classifications and filtering, options, separate pool tags and versioned replay.
- [ ] Wire menu, CVar and headless flags; retain checkpoint semantics for narrow interiors.
- [ ] Run complete cross-dungeon suite, including generation cancellation, metadata safety, legacy replay and native adapters, with sanitizers.
- [ ] Check changed translation units and formatting; commit the combined setting change.

### Task 2: Review and integrate approved work

**Files:** verification docs and PR43 metadata; no new gameplay subsystem.

**Interfaces:** Input is the tested source tree containing forest work and Task 1; output is the exact tree on PR43 and a merge into `bridge/audit-merge-20261008`.

- [ ] Request independent review of narrow settings and the full integration boundary.
- [ ] Address blocking findings with a reproducing test and rerun relevant checks.
- [ ] Publish through authenticated GitHub tools with expected-head guards and verify exact tree identity.
- [ ] Run/inspect configured build and regression CI, then mark ready and merge the authorized PR after required checks pass.
- [ ] Report merge and any actual runtime limitations without treating static or preview evidence as gameplay proof.
