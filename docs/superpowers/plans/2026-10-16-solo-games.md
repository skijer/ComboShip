# Solo Games Implementation Plan

> **For agentic workers:** Use the implementation tasks below with focused tests and a final independent review. Keep source recovery checkpoints.

**Goal:** Offer native vanilla and randomizer gameplay for OoT-only and MM-only within ComboShip.

**Architecture:** Use an immutable runtime game mode across the launcher and game modules. Keep the existing combined boot and save pipeline, and enter native game flows in solo sessions.

**Tech Stack:** C/C++20, libultraship/ImGui, dynamic module exports, Python regression drivers.

**Spec:** `docs/superpowers/specs/2026-10-16-solo-games-design.md`

## Global constraints

- Preserve the MQ source candidate and original donor checkout.
- Require only the selected solo game's ROM; keep shipping port modules together.
- Use `saves/oot/` and `saves/mm/` for native solo save files.
- Retain all four combinations: OoT vanilla, OoT randomizer, MM vanilla, MM randomizer.
- Combined saves and seed generation remain on their existing path.

## Review focus

- Persisted Combo options or seed paths must not hijack native solo generation or gameplay.
- MM-only initialization must never require OoT ROM data.
- Copy/erase/backups/global saves must remain within the selected solo save directory.
- Native solo goals and portal destinations must not depend on the dormant game.
- Either OoT quest alone and both quests together must retain correct native behavior.

### Task 1: Mode contract, selection, extraction and launch

**Files:** `combo/ComboGameMode.h`, `combo/ComboLaunchOptions.h`, `combo/ComboLaunchPreferences.h`, `combo/ComboShip.cpp`, `combo/ComboExtract.h`, `combo/gui/ComboExtractScreen.cpp`, `tests/solo_games/`.

**Interfaces:** `ComboGameMode` values combined=0, OoT=1, MM=2. Each game defines its own hidden `gComboGameMode`; header inline `Combo_IsCombinedGame()` returns true only for combined sessions, and false in a non-Combo build. Launcher calls `SOH_SetGameMode(int)` and `MM_SetGameMode(int)` before window initialization. UI export `ComboUI_SelectGame(int current)` returns 0/1/2 or -1 on cancel. MM export `MM_RunSolo()` reuses an existing window and enters native title/file select; launcher calls it after window-only initialization and no OoT finish-init. OoT solo uses normal SOH finish-init/run without combined callback registration.

- [x] Write/run failing mode, CLI, selected-ROM setup, and launcher-routing tests.
- [x] Implement the mode contract and mode selection screen in the existing setup UI unit.
- [x] Branch solo launch before combined callbacks/eager boot; preserve cleanup and native menus.
- [x] Run focused behavior and production-header checks; commit checkpoint.

### Task 2: Native OoT vanilla/randomizer behavior

**Files:** OoT startup, GUI, file select, SRAM, randomizer generation/spoiler/goal/settings, native save manager, and OoT portal host files as necessary. Own all changes below `soh/`.

**Consumes:** `ComboGameMode`, `gComboGameMode`, `Combo_IsCombinedGame()` from Task 1.
**Produces:** `SOH_SetGameMode(int)`; vanilla/MQ/randomizer native flow in solo; native save isolation and runtime guards for forced combined logic.

- [x] Add behavioral regression tests that fail on the baseline's forced randomizer/combined paths.
- [x] Restore native quest selection, generation/spoilers, and solo goals behind runtime mode checks.
- [x] Isolate every native save operation to `saves/oot/`; prevent cross-game portal/seed behavior.
- [x] Run tests and real-header checks available for modified units; commit checkpoint.

### Task 3: Native MM vanilla/randomizer behavior

**Files:** MM startup, GUI, file select, native save manager, randomizer generation/pools/goal, and MM portal host files as necessary. Own all changes below `mm/`.

**Consumes:** common mode contract and an existing ROM-independent shared window.
**Produces:** `MM_SetGameMode(int)`, `MM_RunSolo()`; native title/file select, vanilla/randomizer generation, MM save isolation and combined guards.

- [x] Add behavioral regression tests that fail on the baseline's combined-only flow.
- [x] Add the MM solo entry and activate its native menu when solo.
- [x] Isolate every native save operation to `saves/mm/`; restore native generation/goals and guard Combo-only portals/sharing.
- [x] Run tests and real-header checks available for modified units; commit checkpoint.

### Task 4: Integration, evidence and recoverable source

**Files:** focused tests, regression gate, `docs/poc/solo-games-20261016.md`, separate source handoff.

- [x] Review all changed callers and mode defaults; run solo, MQ/setup, native settings and cross-dungeon suites.
- [x] Run real-header compilation and repository regression gate; identify pre-existing versus new failures.
- [x] Obtain one independent review of the complete change and address required findings.
- [x] Record exact baseline/candidate revisions, checks and runtime boundaries.
- [x] Package complete tracked source, a patch relative to MQ and cumulative patch relative to PR43; verify archive checksums and persist the deliverable.
