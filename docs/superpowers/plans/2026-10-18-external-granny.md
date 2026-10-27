# External Granny Interior Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement the steps inline.

**Goal:** Add a regular Granny interior checkbox that requires the installed Djipi pack.

**Architecture:** Own private RAM display-list/counter copies, retaining external
resources. Attach them to the native market room in each engine and retain the
existing magical pack unchanged.

**Tech Stack:** C++20, Fast3D, native MM/OoT room shapes, Python preservation checks.

**Spec:** `docs/granny-interior-poc.md`

## Global Constraints

- No original Djipi room or textures in build, generated O2R or source checkpoint.
- Preserve the accepted POC14 branch and asset; no master promotion.
- Counter Y41; all eight offers, keeper, transactions and routing retained.

## Review Focus

- Missing maps, textures or native OoT resources must fail closed to magical mode.
- Alt Assets changes must not substitute magical or vanilla opaque resources.
- Original cached resources must remain unmodified across visits.
- Native translucent rendering, light settings and flames must remain present.
- Room reload and teardown must not leave pointers to freed RAM resources.

### Task 1: External resource graph

- [x] Add and run failing tests for missing dependencies, ownership, immutable inputs and the counter-only change.
- [x] Implement the graph and external loader; run tests against original donors.
- [x] Commit and save a recovery milestone.

### Task 2: Native option

- [x] Add tests of real native room attachment and decoration mapping in both engines.
- [x] Add the next-entry checkbox and adapters, preserving transactions and routing.
- [x] Run native fixtures and compile both complete controller objects.

### Task 3: Evidence and recovery

- [x] Render a preview from local external inputs without creating a classic O2R.
- [x] Verify counter-only visual changes and unchanged POC14; obtain a focused code review.
- [x] Commit and independently restore the final source archive without donor assets.
