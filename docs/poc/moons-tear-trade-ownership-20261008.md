# Moon's Tear trade ownership candidate

## Scope and baseline

- Baseline: `a82852a3a02c8cabc541cce7c124da71c5755efb` (develop).
- Candidate branch: `fix/trade-sidecar-ownership-20261008`.
- Hypothesis: an unversioned `Save/file<N>_tradeitems.bin` from another playthrough can inject trade-wheel ownership into a ComboShip save using the same file number.
- Change only the legacy OoT trade-sidecar boundary. ComboShip must load ownership from its combined save and use FleetSync; standalone SoH retains its existing binary bridge.
- Preserve legitimate earned trade items, Zelda's Letter, Net/bottle behavior, inventory layout, assets, and existing FleetSync receipt/healing behavior.

## Evidence

The supplied combined save contains Moon's Tear's trade bit (`0x800`, bit 11) in both games. MM has one obtained Moon's Tear receipt (`FCI_MM_MOONS_TEAR`, 210) and zero applied receipts. OoT has zero obtained/applied Moon receipts. The MM native pickup flag is unset, the native trade slot is empty, and the seed does not configure Moon's Tear as a starting item. Net ownership is zero.

The saved configuration has `StartingZeldasLetter = 0` and `ShuffleZeldasLetter = 1`. Zelda's Letter is a distinct trade entry (bit 22, `0x400000`); the save does not establish that changing the Skip Zelda's Letter option caused the phantom item.

`NeiSave_Load` previously ORed the binary sidecar's mask into the freshly loaded OoT JSON mask. The path identified only a file number, without a seed or playthrough identity. FleetSync's existing MM compatibility healing can then turn a mirrored trade bit into an obtained receipt. A fixture with a real four-byte Moon's Tear sidecar reproduced the first step against a clean save containing only Zelda's Letter.

The user's original sidecar was not supplied. The reproduced importer and the supplied save's pending-heal state support this cause, but the exact file read in that session was not observed.

## Implementation

In ComboShip builds, `TradeItems_SyncRead` and `TradeItems_SyncWrite` are no-ops. Their original bodies remain under `#ifndef COMBO_BUILD` for standalone SoH. No migration clears existing earned trade items, and no item ID or texture is changed.

The save fixture now extracts the real production path/read/write functions instead of replacing trade I/O with stubs. Its application Save directory is a private temporary directory. Cases cover empty combined saves, Zelda's Letter alone, legitimately saved Moon's Tear plus Letter, save/reload, no creation or mutation of the standalone sidecar, and standalone import/export.

## Verification

- Before the implementation change, the new real-sidecar regression failed: `a combined save with Zelda's Letter must not acquire Moon's Tear from an old sidecar`.
- `ASAN_OPTIONS=detect_leaks=0 CPATH=/workspace/scratch/d65f4483e60f/dependencies python3 -B tests/nei_shared_slot/run_save_tests.py --sanitizers` passed for MM and OoT, including existing native serializer/shared-slot cases. AddressSanitizer and UBSan were enabled. LeakSanitizer's process-inspection support was unavailable in this environment, so leak detection was disabled for this run.
- `CPATH=/workspace/scratch/d65f4483e60f/dependencies python3 -B tests/nei_shared_slot/run_save_tests.py --host soh --standalone` passed, including real legacy sidecar import/export.
- `git diff --check` passed.
- `scripts/diagnostics/run_combo_nei_regressions.sh` was attempted but stopped when the seed-settings fixture could not find `spdlog/spdlog.h` and `imgui.h`. The full suite is not verified.
- No complete game build or in-game run was performed. Runtime acceptance and promotion remain open.

## Preserved repair copy

A separate copy of the supplied save removes only the two orphaned Moon's Tear trade bits and the MM pending healing receipt. Recursive comparison verified exactly these three semantic differences:

| JSON path | Before | After |
| --- | ---: | ---: |
| `/oot/sections/nei/data/tradeAdultOwned` | 2048 | 0 |
| `/mm/newCycleSave/save/shipSaveInfo/nei/tradeAdultOwned` | 2048 | 0 |
| `/mm/newCycleSave/save/shipSaveInfo/nei/comboObtainedFc/210` | 1 | 0 |

The original bytes remain unchanged. The private save files are delivered separately and are not committed to the repository.

## Runtime acceptance

With the candidate executable, load the repaired file from a fully closed session, inspect MM's trade wheel and bottle row, and switch to OoT and back. Moon's Tear should stay absent until its actual seed check is collected; Net should remain absent because this save has no Net. Save, close, and reload to verify ownership persists. A reused slot with a stale Moon's Tear sidecar should also leave a fresh combined save empty. Check Skip Zelda's Letter both on and off in fresh seeds, since this supplied save captures only the off configuration.

Rollback is the unchanged original save plus the baseline commit. When trying the corrected save with an older executable, first rename an existing `Save/file1_tradeitems.bin` to `file1_tradeitems.bin.bak` so the old importer cannot add the item again.
