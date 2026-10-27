# New-save equipment layout implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this narrow correction in the current session.

**Goal:** Prevent modern starting equipment from being migrated into an unsolicited Pendant or losing its current slots.

**Architecture:** Mark both current equipment layouts during each host's new-save initialization. Keep load defaults at zero so actual legacy saves retain their one-time migration. Preserve the existing Combo-only trade-sidecar boundary.

**Tech Stack:** C/C++, native NEI structs, nlohmann JSON, Python fixture runner.

**Spec:** `docs/poc/starting-trade-items-20261008.md` records the supplied-save evidence and acceptance boundary.

## Global constraints

- Candidate baseline: `35ce5f22f11c845a4ef0b28228880547e14ee92d`; do not promote or overwrite a protected baseline.
- Keep original supplied bytes intact; any save correction is a separate copy.
- Change only new-save equipment markers; do not change item IDs, existing migration behavior, or unrelated ownership.
- Distinguish fixture verification from a complete build and in-game acceptance.

## Review focus

- Current Climb/Roc Boots granted before first player initialization must survive without creating a Pendant.
- Current Champion/Sage tunics must survive without being interpreted as the older Cape/Champion slots.
- A legacy save lacking markers must still migrate an owned or equipped Pendant slot once.
- Modern equipment and a legitimately earned Pendant must survive save/reload and repeated initialization.
- Existing standalone sidecar import/export and Combo-only isolation must remain intact.

### Task 1: Initialize current layouts and retain legacy migration

**Files:** Modify `soh/mods/nei_save.cpp`, `mm/mods/nei_save.cpp`, `tests/nei_shared_slot/run_save_tests.py`, and `tests/nei_shared_slot/save_test.cpp`; create the POC evidence record named above.

**Interfaces:** Exercise `NeiSave_Init(bool)`, `Nei_InitNewSave(void)`, `ExtEquip_Init(void)`, native JSON serializers, and real equipment/trade ownership writers. Engine rendering/actor resets remain outside this fixture.

- [x] Extend the native save fixture to run real equipment initialization after current equipment grants, legacy loads, and JSON roundtrips.
- [x] Run both hosts before implementation; observe a Pendant appearing from modern Climb Boots.
- [x] Set `extTunicLayoutVersion = 1` and `extBootsLayoutVersion = 1` in both new-save initializers; leave serializer load defaults unchanged.
- [x] Run both host fixtures with sanitizers and the standalone OoT fixture; run the old initializer as a failing control and check the final diff.
- [ ] Record exact save repair differences, original hashes, runtime acceptance steps, and deliver a durable checkpoint.
