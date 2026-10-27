# Native MM runtime feedback implementation plan

> **For agentic workers:** Use superpowers:executing-plans for integration. Independent root-cause investigations use the dispatching-parallel-agents workflow.

**Goal:** Correct the reported native MM effect, climbing, pickup and camera regressions with isolated production-path evidence.

**Architecture:** Native MM state remains authoritative. Carry missing accepted donor behavior at the matching native seam, and use existing owner-aware resource routing for shared artwork.

**Tech Stack:** C/C++, native MM headers and rendering, Python diagnostic fixtures, existing GitHub Actions gate and Windows/Linux builds.

**Spec:** `docs/superpowers/specs/2026-09-29-mm-runtime-feedback.md`

## Global constraints

- Baseline and tested merge have identical tree `8a3dccd531d38cee6d101a71d0b9a567af563219`.
- Accepted effects/artwork and all prior integrations remain protected.
- Din's Fire cutscene is excluded; no source save or uploaded evidence is edited.
- Root integrates and publishes; workers make only scoped candidate edits.

## Review focus

- Native particle suppression must retain the accepted projectile and collision behavior, including first person and missing assets.
- Arrow replacement paths must agree on game owner, dimensions, Alt selection and fallback.
- Mitt climb entry must preserve explicit put-away, no-magic cancellation and ordinary climbing without Mitts.
- A foreign reward must keep one concrete pre-grant tier across model, icon, text and award; save/reload must not duplicate an obtained check.
- Leaf/Shovel input control must preserve real cutscenes and targeting camera behavior.

## 1. Effects

Files: native Ice Rod logic, native Ice/Light arrow draw adapters, owner routing and focused `tests/mm_nei` diagnostics as identified by trace.

- [x] Compare native active paths against accepted `soh/` paths and record exact missing behavior/resource ownership.
- [x] Add failing production-path assertions for proven omissions; run and record failures.
- [x] Apply minimal native corrections, preserving accepted Fire/Light output and existing artwork.
- [x] Run affected native tests and production-header compilation; report remaining asset/runtime dependencies.

## 2. Item use

Files: native Mitts climb/put-away seam, Leaf/Shovel logic, `tests/mm_nei` use diagnostics.

- [x] Demonstrate active Mitts are stowed by native climb entry and Leaf/Shovel set a camera-suppression flag.
- [x] Add failing climb-entry and camera-state tests including cancellation and real cutscene preservation.
- [x] Correct those native ownership seams and run relevant lifecycle/pose checks.

## 3. Foreign scales

Files: foreign item presentation, `OOT_FillItemIconInfo`, shared item and save/check delivery seams as trace requires; `tests/mm_presentation` and grant diagnostics.

- [x] Trace the earlier native swim grant, chest tier, icon owner/size and target/source save boundaries.
- [x] Add failing production-path tests only for confirmed faults, distinguishing intentional progressive advancement from stale presentation.
- [x] Correct descriptor/presentation or transaction faults without changing unrelated progression or saves.
- [x] Run icon, progression and cross-grant preservation tests; report intentional persistence behavior explicitly.

## 4. Integration and candidate

- [x] Inspect each diff, wire new focused tests into the existing regression entry points and run the cumulative affected gate.
- [x] Independently review production changes and address substantive findings; format changed eligible C/C++ and verify whitespace.
- [ ] Record exact evidence/limits, commit, update PR 24 and inspect the new remote gate/platform builds under existing authorization.
- [ ] Hand off a uniquely identified runtime candidate with a short retest list; leave acceptance and master promotion pending.
