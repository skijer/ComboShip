# MM Grotto Return Arrival Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this single task inline. Steps use checkbox syntax for tracking.

**Goal:** Preserve the native jump out of an MM grotto after a cross-game return.
**Architecture:** Clear stale save-resume markers where title setup accepts an explicit composed arrival. Exercise the real MM routing, materialization, title selection and native player-start override together.
**Tech Stack:** C/C++, Python production-body fixtures.
**Spec:** `docs/poc/mm-grotto-return-arrival-20261017.md`.

## Global Constraints

- Preserve the exact baseline, routes, descriptors, respawn modes and day/time.
- Ordinary save resumes and no-arrival paths retain native behavior.
- No asset changes, master promotion, or full application build in this probe.

## Review Focus

- All MM hole-return descriptors, including seasonal fallback.
- A saved pause-location marker on repeated cross-game round trips.
- Owl-save state carried into an explicit arrival.
- Ordinary resume with no queued arrival or recovery.
- Existing boss recovery and explicit-arrival priority.

### Task 1: Preserve composed arrival start mode

**Files:**
- Modify `mm/src/code/title_setup.c` at the composed arrival/recovery branch.
- Modify `tests/cross_dungeon/mm_adapter_behavior_test.py` and `mm_adapter_cases.inc`.

**Interfaces:** Consume existing queued arrival/recovery APIs; preserve their entrances and respawn data. Add no runtime API.

- [x] Extend the MM adapter fixture with the real native pause-save start-mode override and all hole-return round trips.
- [x] Run `python tests/cross_dungeon/mm_adapter_behavior_test.py`; expect failure because the return selects stationary mode D.
- [x] Clear the stale pause-save marker and owl-save arrival state only in the accepted composed-arrival branch.
- [x] Re-run the focused fixture and `python tests/cross_dungeon/run_tests.py`, then the focused sanitizer fixture. All 45 suite commands completed across resumed runs after checkout dependencies were restored; see the POC evidence record.
- [x] Review the isolated diff, record verification and runtime limits, and commit a source checkpoint for the next publication.
