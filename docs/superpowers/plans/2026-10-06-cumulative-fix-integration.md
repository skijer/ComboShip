# ComboShip Cumulative Fix Integration Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Combine the preserved equipment, native MM imports, GI follow-ups, completed sword work, Cape, autumn and owl fixes; validate and merge the cumulative fork candidate.

**Architecture:** Start from published PR #34 with PR #35's exact source tree. Retain one native-MM alias implementation, then merge the independent deltas. Keep shipped sword resource ownership and cold-owner tunic availability together at their shared boundary.

**Tech Stack:** C/C++20, libultraship, native OoT/MM DLLs, Python compiler harnesses, CMake, GitHub Actions.

**Spec:** The current user request to validate and merge all audited fixes, with Sword GI Checkpoint 12 at c86fab047c74b8349eef687e65e61450f94cc559.

## Global Constraints
- Preserve PR #34 at 92b72bf29d15b80f3bfd88402319bae09b74c895 and its existing Wolf, Water/Sand, Grace, seasons, audio, cosmetics and item behavior.
- Checkpoint 12 supplies the completed sword fit and asset selection delta; preserve exact supplied pack inputs outside Git.
- Keep 29 native MM aliases, foreign grant ownership, actual actor context and shuffled reward identity.
- MM Alt OFF uses shipped redesigned sword roots; Alt ON preserves selected custom geometry, Din layers, shimmer and particles.
- Keep tunic availability valid before OoT startup, including alias-only resource metadata.
- Retain both Cape and leaf-combiner tests in the canonical cumulative runner.
- Merge code into marsh6487/ComboShip after cumulative verification. Runtime appearance remains explicitly pending.

## Review Focus
- Deferred sword path/hash/metadata dependencies must stay in the same shipped archive and isolated cache with Alt OFF.
- Resource view pending-load destruction must drain workers before cache/mutex/archive teardown.
- Cold OoT archive ownership and MM native import tracing must survive overlapping source merges.
- Compass reward absence must suppress fabricated/unknown text while keeping assigned-boss information.
- Canonical runner and CI must execute equipment, Cape, autumn and both sword selection/view regressions.

### Task 1: Establish the exact published baseline
**Files:** isolated worktree; this plan; ignored progress ledger.
**Interfaces:** published PR #34 tree and PR #35 tree become the source baseline.
- [ ] Inspect the clean worktree and current GitHub refs.
- [ ] Import PR #35's published-equivalent tree and original remote commit identity.
- [ ] Validate source-tree equality and recover the two historical comparison commits.
- [ ] Commit the plan; record baseline state and unchanged runtime limitations.

### Task 2: Integrate the preserved source deltas
**Files:** native MM alias bridge; receipt/tunic/summer source; sword bounds/resource loader/manager/owner bridge; Cape draw; autumn material; owl draw; associated tests.
**Interfaces:** shared resource availability, MM GI draw routing, receipt renderer and canonical regression runner.
- [ ] Apply the local 082ad73b native alias commit.
- [ ] Apply only the broader GI candidate delta beyond that native-alias checkpoint.
- [ ] Apply Checkpoint 12's completed sword delta and reconcile overlaps without discarding either behavior.
- [ ] Apply Cape and autumn patches and the optional owl translucent draw delta.
- [ ] Register all focused tests in the cumulative runner/CI; preserve independent assets and fixture history.
- [ ] Check formatting, whitespace, conflicts and path collisions; commit source checkpoints.

### Task 3: Verify the cumulative tree
**Files:** canonical NEI runner, existing build gate, focused asset tests and native compiler probes.
**Interfaces:** full merged source tree consumed by all tests and platform builds.
- [ ] Run every canonical combined command, including both recovered historical comparisons.
- [ ] Run the additional full sanitized GI and actual supplied sword-pack tests.
- [ ] Run all remaining CI regression groups using actual dependency headers.
- [ ] Diagnose and fix integration failures with reproducing controls.
- [ ] Request one fresh whole-branch review and address any material finding in one verified correction pass.
- [ ] Publish a source checkpoint and verify exact-tree Windows/Linux build and archive generation.

### Task 4: Merge and preserve the result
**Files:** fork branch refs, PR, recovery package and validation handoff.
**Interfaces:** verified candidate SHA and unchanged target branch head.
- [ ] Recheck remote develop and cumulative heads before merge.
- [ ] Merge the verified cumulative PR into the fork's develop branch under the user's current authorization.
- [ ] Verify merged tree/ancestry and preserve recovery plus build references.
- [ ] Report included fixes, actual validation and remaining configuration-specific runtime checks.
