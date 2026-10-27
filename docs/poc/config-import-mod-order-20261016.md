# Per-game mod load order import

| Field | Record |
| --- | --- |
| Baseline | `marsh6487/ComboShip` develop, `a82852a3a02c8cabc541cce7c124da71c5755efb` |
| Candidate | `fix/config-mod-load-order-20261016`, isolated source checkout |
| Scope | Existing first-launch JSON settings importer; preserve each game's saved mod order |
| Preservation | Shared settings still merge per leaf with SoH winning; Window remains excluded; only SoH supplies ConfigVersion; mod discovery/mounting remain unchanged |
| Evidence | Baseline reproduces seven order failures; candidate passes settings-import tests, existing shared integration regressions, formatting, and YAML validation |
| Verdict | Source/host-test verified; full game build and runtime untested; not promoted |
| Recovery | Separate branch and merge patch based on the exact baseline above |

## Cause and change

The importer already copies object blocks including CVars, graphics, controls,
and randomizer settings. It previously merged both standalone games' native
`CVars.gSettings.EnabledMods` into one value. ComboShip's MM mod loader reads
`CVars.gSettings.EnabledModsMM`, so MM's order either occupied OoT's slot or
was overwritten by SoH without populating its own slot.

The launcher now keeps SoH's native list in EnabledMods and maps the MM input's
list to EnabledModsMM before the live config is applied. An existing separated
MM list in the MM input takes priority over its legacy list or a copy in the
SoH input. Strings are copied verbatim, including an explicit empty string.
Missing mod data does not invent an order. Source JSON files are never modified.

Import still occurs only on first launch without comboship.json. This change
imports the saved ordering data; archive discovery uses the existing game folders
and filenames. Both mod loaders initialize after the imported CVars are applied.

## Verification

- `python3 tests/settings_import/run_tests.py --json-include <json-include>`:
  passed with C++20, Wall/Wextra/Werror/pedantic.
- Same test with `--baseline`: seven expected failures from the original
  production merge statements at the pinned baseline.
- Same candidate with `--sanitize`: ASan/UBSan passed with
  `ASAN_OPTIONS=detect_leaks=0`. Default LeakSanitizer could not inspect this
  environment's process tasks; no leak-check claim is made.
- `python3 scripts/diagnostics/run_combo_shared_test.py --json-include <json-include>`:
  production behavior, launcher delivery/slot checks, and plando picker passed.
- Changed C++ files pass clang-format 14.0.6; workflow YAML parses; diff has no
  whitespace errors. New settings-import tests are included in the CI gate.
- Independent code review found no actionable issues.

## Decisive runtime check

In a fresh test install with the matching packs in mods/soh and mods/2ship,
import standalone configs containing distinct nonalphabetical orders. Compare
each game's Mod Menu to its source JSON, then restart and confirm both orders
persist. Also check MM-only import leaves OoT's order independent. The shared
import dialog and game-specific pack priority still need this runtime proof.
