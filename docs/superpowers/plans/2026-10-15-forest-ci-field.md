# Forest integration and CI repair implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement these tasks inline.

**Goal:** Repair PR #43's CI failure, integrate the recovered bidirectional forest passage, and preserve the Hyrule Field texture/Prelude mesh correction.

**Architecture:** Keep published shuffle candidate `e00fb378` as host. Apply only reconstructed forest delta `c2a4add8` from base `a82852a3`, resolving shared handoff code in favor of retaining both features. Preserve Field cache correction `e4813206` byte for byte unless new evidence requires otherwise.

**Tech stack:** C/C++, Python production fixtures, GitHub Actions.

**Spec:** User request; `docs/poc/2026-10-15-lost-woods-entrance.md` from recovered delta; `docs/poc/field-resource-regression-20261014.md`.

## Global constraints

- No develop/master promotion or unrelated Demise integration.
- Preserve shuffled dungeon/boss routing, paired saves/slots, original portals, MM maze/monkey/swamp routes, and reset cancellation.
- Preserve scene archives, geometry, collision, textures and material bindings.
- Build/static evidence does not establish installed-game visual acceptance.

## Review focus

- Forest and shuffled routes must not leave one another's pending state active.
- New passage must coexist with native arrival and source-owned recovery.
- Seasonal gates must follow logical source clears, not shuffled rewards.
- Alt reload tests must exercise both textures and display lists through scene travel.
- New native files must compile in real host contexts and remain in the CI gate.

### Task 1: Restore CI fixture coverage

**Files:** `tests/seasons/run_oot_mode_tests.py`; related fixtures only if the full gate reveals a confirmed incompatibility.

**Interface:** `Combo_SOH_CrossBossLogicalClear(int id, int nativeValue)` and `BossCompletionState` from `combo/rando/CrossEntranceBoss.h`.

- [x] Reproduce the hosted missing-declaration failure locally.
- [x] Include the real cross-boss API and exercise logical completion through production `BossCompletionState`, including disabled/native behavior and source/physical mismatch.
- [ ] Run the fixture and the complete NEI gate; correct only confirmed failures.
- [ ] Checkpoint the verified fixture repair.

### Task 2: Integrate the recovered forest delta

**Files:** The recovered commit's 15-file inventory, primarily `ComboShip.cpp`, both host entry/handoff implementations, the new passage header/actor, and forest fixtures.

**Interface:** Recovered forest route/arrival contracts coexist with current cross-entrance and boss recovery contracts.

- [x] Compare patch and full-source hashes, then apply the exact delta with three-way conflict resolution.
- [x] Retain both handoff paths and update extraction fixtures to include newly shared dependencies.
- [x] Run forest, cross-dungeon, boss, launcher, shared-save and Farore suites.
- [x] Verify the Field `ResourceManager.cpp` matches `e4813206`; run native/Alt texture and display-list fixtures.
- [x] Checkpoint the integrated candidate.

### Task 3: Verify and publish

**Files:** Integration verification record and PR #43 metadata.

- [ ] Run the cumulative gate, changed-file formatting, asset collisions, whitespace and affected native compilation checks.
- [ ] Review the complete delta and resolve concrete correctness findings.
- [ ] Publish the candidate to the already authorized PR branch using an expected-head check; update its description.
- [ ] Inspect the new hosted run and resolve any further gate/build failures exposed by it.
- [ ] Report exact commit, CI state and remaining runtime checks.
