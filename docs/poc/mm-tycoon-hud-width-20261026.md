# 2Ship Tycoon wallet: three digits at 999 capacity

## Candidate scope

Parent: public cumulative PR65 `fa81f7926038f2aa85c7f3e73c96fec043168b66`, tree `40658cee090ddf0b55e234aa7bbc7e436723377e`.
Local branch: `poc/mm-tycoon-hud-width-20261026`.
User report: 2Ship displays `0999` after obtaining Tycoon while OoT displays `999`; the shared wallet cannot exceed 999. User requests the bank reward fix preserved and source prepared for later merge.

This candidate changes one production function and extends its existing regression fixture. No public push, merge, capacity change, game executable or runtime acceptance is included.

## Implementation trace

- `mm/2s2h/FleetShipCombo/FleetSync.cpp`, `ApplyShared`: when a combo is active, shared wallet level 3 has capacity 999, matching OoT.
- `mm/2s2h/Rando/MiscBehavior/TycoonWallet.cpp`, `InitTycoonWallet`: native randomizer Tycoon registration sets standalone capacity to 5000 and replaces the icon/counter draw hooks for wallet level 3. This registration and capacity logic are unchanged.
- `DrawTycoonRupeeCounter` passed a hardcoded minimum of four to `RupeeCounter_BuildDigits`, so the active 999-capacity wallet drew `0999`. The earlier safe-digit change in public commit `8cf71afc532917d62f1be61e4a7800dd3c48e0c2` retained the pre-existing four-digit Tycoon layout while replacing unsafe decimal handling.
- The native MM draw path in `mm/src/code/z_parameter.c` already selects a three-digit minimum for upgraded wallets. OoT source is unchanged by this candidate.
- `mm/2s2h/Enhancements/Timesavers/AutoBankDeposit.cpp`, `HandleWalletOverflow` and `GrantBankerReward`: excess currency deposits to the bank and marks randomized reward checks eligible, or grants the native rewards. The bank cap remains 5000. Default reward thresholds remain 200/1000/5000; lowered thresholds remain 100/500/1000.
- `mm/2s2h/Rando/MiscBehavior/MiscBehavior.h`, `BankRewardSourceSuffix`, and its `CheckQueue.cpp` consumers identify the three bank reward check IDs in the existing item receipts/notifications. These bank paths and the grant/save/queue code remain unchanged.

## Narrow correction

Read the live `CUR_CAPACITY(UPG_WALLET)` when choosing the Tycoon counter's minimum width: three below 1000, four otherwise. Continue using the unchanged safe formatter, which expands for the full nonnegative signed16 balance and displays negative data as zero without writing to the save.

Expected displays:

| Capacity | Stored balance | Display |
| --- | --- | --- |
| 999 | 999 | `999` |
| 999 | 7 | `007` |
| 999 | 0 or negative | `000` |
| 999 | restored 1667 | `1667` |
| 999 | restored 32767 | `32767` |
| 5000 | 999 | `0999` |
| 5000 | 5000 | `5000` |

Icon, live cosmetic color, full-wallet green color, shadow, digit coordinates, HUD editor handling, hook eligibility, capacities and bank behavior are preserved.

## Evidence and limitations

- RED: added actual-draw fixture fails on unchanged production with `balance=999 wallet=3 path=Tycoon expected=999 drawn=0999`.
- GREEN: the real native and Tycoon draw bodies pass ASan/UBSan/bounds for all 65,536 signed16 values in four native wallet levels and Tycoon capacities 999/5000 (393,216 exhaustive draw cases, plus literal boundary cases). The fixture checks drawn decimal textures/positions and unchanged currency, upgrade and accumulator values; the added checks also retain the capacity table.
- The complete native `mm/src/code/z_parameter.c` syntax check passes with inherited header/const warnings.
- `python3 -B tests/mm_wallet/run_tests.py` does **not** fully pass in this local environment: its final complete Tycoon C++ translation-unit syntax step stops at missing external `nlohmann/json.hpp`. The same blocker occurs on the parent. The runner and its checks are unchanged; no substitute or skipped compile is reported as a pass.
- Independent read-only review found no actionable issues and independently confirmed the same sanitizer/native-syntax results and missing-header limitation.
- Whitespace checks and preservation comparisons cover unchanged bank, grant, sync, safe formatter, native/MM and OoT paths. The handoff package contains exact hashes and test logs.

Full build/link, configured gameplay, live bank rewards and HUD editor override rendering remain untested for this candidate. The production HUD editor branch is unchanged. This source is ready for later integration with the stated compile/runtime checks still required.

## Focused runtime check after integration

1. In the existing combined save, use the level-3 wallet at capacity 999. Confirm 2Ship draws `999` and OoT still draws `999`; spend/pick up rupees and check ordinary three-digit padding and full-wallet color.
2. With Auto Bank Deposit active and the bank just below an existing reward threshold, overflow the full wallet. Confirm the deposit and the existing bank-attributed reward presentation still occur once. Verify a manual deposit and the existing skip-cutscene notification setting as relevant to that save.
3. If standalone 2Ship is tested, enable its native Tycoon option and confirm the real 5000-capacity wallet retains four digits.

Keep the accepted installation and parent source unchanged until configured runtime acceptance. Revert only this candidate to recover the original display; bank code does not need restoration.
