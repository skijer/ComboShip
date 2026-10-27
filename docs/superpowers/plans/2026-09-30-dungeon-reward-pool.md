# Dungeon reward pool implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this single regression fix inline.

**Goal:** Prevent MM's optional OoT quest pool from reintroducing dungeon rewards already owned by OoT's placement policy.

**Architecture:** Keep `GeneratePools` and the existing ComboShip fill intact. Exclude only the nine standalone OoT reward additions in `COMBO_BUILD`, before starting-item removal and plentiful duplication.

**Tech Stack:** C++20, native MM pool generator, Python test runner.

**Spec:** User report and supplied `Fleet of Harkinian(9).log`: at 02:01:25–26, `RAND_INF_ZR_WONDER_NEAR_CUCCO_3` (0x714) grants MM-origin fc 226 then native OoT item 0x6e (Zora's Sapphire). Baseline PR #26, commit `456033dcae5b81927396eb8922d30b97a9fbd06f`. The log lacks seed settings/spoiler, so reproduction establishes the code path rather than recovering the exact seed.

## Global Constraints

- Preserve latest sword/audio candidate; no master promotion or save edits.
- Preserve standalone MM, other OoT quest items, MM boss remains settings, and NEI items.
- OoT's configured reward placement remains authoritative; do not delete real rewards.
- Generated seed placements persist: this source fix cannot rewrite an existing save.

## Review Focus

- All nine OoT rewards, not only Sapphire: absent from the ComboShip MM pool.
- Quest option off/on and normal/plentiful: no duplicate reward injection.
- Standalone MM: quest option still supplies its rewards.
- Starting items and MM remains: native behavior retained.
- Existing seeds: report regeneration requirement; no silent repair.

### Task 1: Stop duplicate reward injection

**Files:** `mm/2s2h/Rando/Logic/GeneratePools.cpp`, `tests/dungeon_rewards/`, `.github/workflows/build-artifacts.yml`.

**Interfaces:** Consume the complete production `Rando::Logic::GeneratePools(RandoSaveInfo&, vector<RandoCheckId>&, vector<RandoItemId>&)` and actual `FleetCombo_UnifiedPoolActive` implementation. No new production API.

- [x] Compile and run the native generator in ComboShip and standalone test fixtures. Assert nine literal reward IDs have zero ComboShip copies; standalone retains one/two when quest is on in normal/plentiful mode. Verify other quest items, starting items, NEI, and MM remains.
- [x] Observe the baseline fail for injected rewards.
- [x] Guard the nine standalone reward additions with `#ifndef COMBO_BUILD` and explain ownership.
- [x] Run `python3 tests/dungeon_rewards/run_tests.py --json-include /usr/include`; expect all scenarios pass. Run existing shared-item and cross-grant regressions, format and diff checks. Add the new runner to the existing CI regression gate.
- [ ] Commit, independently review the bounded diff, and publish a draft candidate carrying forward #26. Record runtime/seed limits.
