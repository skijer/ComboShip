# Parallel ComboShip CI implementation plan

> **For agentic workers:** Use superpowers:executing-plans for inline implementation. Track completion below.

**Goal:** Reduce feedback time while preserving the existing regression and platform checks.

**Architecture:** A short preflight releases six independent regression groups and the existing Windows/Linux builds. The stable `gate` job aggregates every required result; download announcements require overall success. A timed command manifest retains the original fixtures, flags and baseline pins.

**Tech Stack:** GitHub Actions, Bash, Python standard library, existing CMake/MSVC/GCC pipelines.

**Spec:** cor approved the in-chat design on October 21, 2026. Baseline: `6effafdeaa41d0e851395c19665f11f2b24f3145`.

## Global constraints

- Preserve all distinct regression commands, environment flags and fixed baseline comparisons.
- Preserve build workflow filename/name, public artifact names and `comborando-windows` consumption.
- Preserve platform generators, build targets, memory limits, packaging and seed smoke tests.
- No game code, asset bytes, source integration or release promotion changes.
- Work on `fix/parallel-ci-gate-20261021`; baseline remains recoverable from develop.

## Review focus

- Failure, cancellation, missing output and unexpected skips must never produce a green final gate.
- Documentation-only PRs may skip expensive jobs; test/build-tool changes must trigger them.
- Extracted commands must keep their environment and per-step dependencies.
- Parallel groups must have independent checkouts and required fixed history/headers.
- A failed overall run must preserve the prior successful download announcement.

### Task 1: Preserve and split regression execution

- [x] Write behavior tests for command execution, strict failure propagation, timing and unknown groups; observe failures.
- [x] Extract original steps plus the NEI umbrella into six groups with exact command/environment preservation.
- [x] Add a standard-library runner with per-command GitHub log groups and timing summaries.
- [x] Compare the expanded before/after commands and verify local runner tests.

### Task 2: Parallel workflow and final verdict

- [x] Write failing verdict tests for every required job, docs-only skips and malformed/missing outputs.
- [x] Move cheap checks/path detection into preflight; launch groups and both platforms from preflight.
- [x] Add strict final `gate` aggregation and success-only artifact announcements.
- [x] Refresh Windows cache keys and separate dependencies; allow scoped Linux PR compiler-cache saves.
- [x] Validate workflow syntax, commands, failure cases and preserved artifact interfaces.

### Task 3: Review and hosted evidence

- [x] Run the focused CI tests, preservation comparison and workflow linter.
- [x] Commit only owned CI/test/documentation paths and obtain independent review.
- [ ] Publish the recoverable candidate branch/PR and run hosted CI.
- [ ] Record measured results or explicit pending/blocked checks without claiming in-game proof.

## Recovery

Restore baseline workflows from `6effafdeaa41d0e851395c19665f11f2b24f3145`, or revert the candidate CI commit. This change owns only `.github/workflows`, `scripts/ci`, `tests/ci` and this plan. The remote candidate commit and this record provide exact source recovery; no game archives are modified.

## Independent review follow-up

- Preserve fail-closed routing: native merge-parent diff, explicit documentation allowlist, no rename collapse, no coercion of missing output into `false`.
- Deploy the publisher guard alone to the default branch before publishing the parallel candidate PR, because `workflow_run` executes the default-branch definition.
- No changes to the preserved 167 NEI commands or 36 outer regression steps.

Publisher guard deployed to develop as `71af839fd7cd9c8aef36199486e054d3de349267` after cor explicitly approved all proposed CI workflow changes. Local CI helper/routing suite: 14 tests passed. Actionlint 1.7.12, all 203 command syntax checks, exact coverage comparison and asset-collision check passed.
