# MM pause and equipment regressions implementation plan

> **For agentic workers:** Execute the pause/transfer tasks inline; the independent equipment and Four Sword tasks have dedicated agents. Use systematic debugging, TDD and verification-before-completion.

**Goal:** Restore MM item access and shared item button transfers, hide unearned equipment, and restore the authored Four Sword pickup/held model.

**Architecture:** Keep the native pause state machine and departure-only equip synchronization. Fix each failing boundary without changing progression or global renderer behavior.

**Tech Stack:** Native C/C++, Python production-function fixtures, CMake and GitHub CI.

**Spec:** `docs/poc/mm-pause-equipment-regressions-20261008.md` and the user's runtime report.

## Global constraints

- Baseline: PR39 head `35ce5f22f11c845a4ef0b28228880547e14ee92d`; runtime build `f069ea64a87450f9be621fee7f0100d50342d324` is its test merge.
- Preserve seasonal weather, cross-game Farore recall, earned ownership, item checks, other Din sword variants, upright pickup framing and shop shelves.
- Changes are candidates until demonstrated in the user's configuration.
- Latest user steering: generate the recovery ZIP and hold the push while other recovery changes are being merged cumulatively. Preserve a mergeable full-index patch against the recorded baseline.

## Review focus

- Empty inventory page with cursor on either arrow must not trap L switching.
- Open wheel, equip animation or description must retain input ownership.
- Custom u16 buttons must retain full IDs and native slot conventions in both directions.
- Destination-only items and unowned items must not be overwritten or granted by button transfer.
- Starting equipment visibility must reflect ownership; Four Sword must have a live authored draw path under both asset modes.

### Task 1: MM inventory access

**Files:** MM `z_kaleido_item.c`; `tests/mm_pause_inventory/run_tests.py`.

- [x] Extract the real cursor function into a native fixture and reproduce empty-page arrow entry/L failures.
- [x] Handle L before grid-only cursor logic; allow arrow entry on an empty page while retaining populated-page selection behavior.
- [x] Verify both arrows, page cooldown, wheel/description guards and same-frame item identity.

### Task 2: Shared C/D-button transfers

**Files:** Both `FleetSync.cpp` and `FleetComboIds.h`; `tests/combo_button_transfer/run_tests.py`.

- [x] Reproduce a wand backed by NEI ownership and a u16 Rod of Seasons transfer through production extraction/application functions.
- [x] Preserve full supported shared IDs, find NEI slots, and keep button ID/shadow/slot fields coherent.
- [x] Verify both directions, shared MM C-slot convention, unsupported local items, missing ownership and malformed input.

### Task 3: Equipment availability

**Owner:** equipment_ownership agent; equipment-specific code/tests only.

- [x] Distinguish pre-grant save state from debug grants; reproduce unearned icons/selection using real production branches.
- [x] Align display and selection with ownership without clearing saves.
- [x] Verify vanilla and additional pages and earned items.
- [x] Repair sync publisher ownership leaks and verify incoming shield starter/skin/base coherence without consuming pending grants.

### Task 4: Four Sword

**Owner:** four_sword agent; sword/GI/held-specific code/tests only.

- [x] Trace authored pickup and held resource ownership; reproduce replacement/invisibility.
- [x] Restore authored Four Sword priority and a visible held route while preserving other custom swords and shelves.
- [x] Verify both game routes and asset modes with production dispatch tests.

### Task 5: Integration and recovery

- [x] Review combined diff; run focused checks, required formatting and the repository regression gate.
- [x] Preserve source candidate and rollback as commits/patches; create a recovery package.
- [x] Report verified dimensions, remaining runtime checks and exact candidate identity.
