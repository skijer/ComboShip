# Starting Pendant and Moon's Tear candidate

## Scope

Baseline: combined recovery branch `35ce5f22f11c845a4ef0b28228880547e14ee92d`. Preserve that baseline and the user's original files. Diagnose the reported starting Pendant and Moon's Tear, prepare a narrow source correction and a separate repair copy, and retain runtime acceptance as a separate gate.

## Supplied evidence

- `file1_tradeitems.bin` and `file2_tradeitems.bin` each contain `00 08 00 00`: little-endian mask `0x800`, Moon's Tear only. `file3_tradeitems.bin` is zero. No supplied sidecar contains the Pendant bit `0x80000`.
- `file1(3).combosav` uses slot 0, seed 214999, runtime seed 1316389493. OoT trade ownership is `0x80800`, Pendant owned is 1, boots layout is 1, and extended ownership is `0x02df0000`. MM trade ownership is `0x800`, Pendant owned is 0, boots layout is 0, and extended ownership is `0x0fdf0000`.
- The MM mask includes modern Climb/Roc Boots and Sage's Tunic. The masks differ by `0x0d000000`: exactly the Climb Boots, Roc Boots, and Sage's Tunic bits removed by the old-layout migrations. Both hosts' new-save initializers previously zeroed the layout markers; both `ExtEquip_Init` paths treat boots layout 0 as the old Pendant/Dragon Scale layout and tunic layout 0 as the old Cape/Spirit/Champion layout.
- The seed has no MM starting items. Moon's Tear and Pendant are placed at actual randomized checks. The native MM trade inventory is empty. Whether the installed binary contains the current sidecar guard cannot be established from these attachments alone.
- `file256.combosav` is an empty wrapper for invalid slot 255 with null game saves; it has no item ownership. `global(1).sav` contains only audio, language, targeting, and version preferences.
- The actual Pendant location, `RC_WOODFALL_TEMPLE_SF_MAZE_BEEHIVE` (378), is uncollected. Moon's Tear's OoT placement, `RC_SFM_WONDER_MAZE_1` (2298), has no collected tracker entry and its randomizer pickup bit is zero. Native MM Moon/Pendant obtained bits are also zero.
- Compiling the current canonical enums gives `FCI_PENDANT_OF_MEMORIES = 40` and `FCI_MM_MOONS_TEAR = 210`. Pendant's obtained/applied receipts are already zero in both hosts. FCI 14 is Ocarina, while the separate `comboObtained[14]` is an enemy soul; neither is modified.

## Candidate boundary

New saves use current tunic and boots layout marker 1 in both hosts. Existing save-load defaults remain 0, retaining genuine legacy migration. The already present Combo-only sidecar guard remains unchanged. No generic cleanup deletes earned items from arbitrary saves.

## Verification

- Before the source change, both native host fixtures compiled and failed: `modern starting Climb Boots must not become a Pendant on first equipment initialization`.
- With the four new-save marker assignments, both native host fixtures passed with AddressSanitizer and UBSan. Cases exercise real `ExtEquip_Init`, native ownership/trade writers and native serializers after fresh grants, owned/equipped legacy loads, repeated initialization, and save/reload. Modern boots/tunics and a legitimately earned Pendant survive; genuine legacy migrations still run once.
- The same fixtures using `--init-ref 35ce5f22f11c845a4ef0b28228880547e14ee92d` failed on the first-initialization behavior in both hosts, proving the regression detects the initializer defect.
- The standalone OoT fixture passed, including its legacy sidecar import/export. Combo tests retain the existing stale-sidecar exclusion.
- `clang-format-14` dry-run passed for the two changed production files; `git diff --check` passed. Tests retain the existing fixture style (the repository format gate targets `soh`, `mm`, and `combo`).
- The supplied original save fails the optional repaired-save acceptance check in both hosts. The separate corrected copy passes native NEI load/init/save/reload with ASan/UBSan in both hosts, retaining modern equipment without either phantom item or its pending receipt.
- All runs use the regular nlohmann JSON dependency; sanitizer runs set `ASAN_OPTIONS=detect_leaks=0` because process inspection for leak detection is unavailable in this environment. No complete game build or in-game acceptance is inferred from these fixtures.

## Separate repair copy

The original supplied save SHA-256 is `6222f9019d901d251286e19628334d57d7fbce43daf6006c19ae9351cc177fae`. Recursive comparison verifies exactly eight semantic changes:

| JSON path | Before | After |
| --- | ---: | ---: |
| `/oot/sections/nei/data/tradeAdultOwned` | 526336 | 0 |
| `/oot/sections/nei/data/pendantOwned` | 1 | 0 |
| `/oot/sections/nei/data/comboObtainedFc/210` | 1 | 0 |
| `/oot/sections/nei/data/extEquipOwnedBits` | 48168960 | 266272768 |
| `/mm/newCycleSave/save/shipSaveInfo/nei/tradeAdultOwned` | 2048 | 0 |
| `/mm/newCycleSave/save/shipSaveInfo/nei/comboObtainedFc/210` | 1 | 0 |
| `/mm/newCycleSave/save/shipSaveInfo/nei/extBootsLayoutVersion` | 0 | 1 |
| `/mm/newCycleSave/save/shipSaveInfo/nei/extTunicLayoutVersion` | 0 | 1 |

The copy removes the two reported orphaned trade items and pending Moon receipts, restores the three modern equipment bits lost by migration in OoT, and marks MM's modern equipment layout to prevent re-creation of the Pendant. Seed, native inventory, collected checks, preferences, unrelated ownership and receipts remain semantically identical. No raw save or binary sidecar is committed to the repository.

## Runtime acceptance and rollback

Fully close the game before installing the separate correction as `Save/file1.combosav`, retaining a backup of the original. If using an executable that still reads the old trade sidecars, rename `file1_tradeitems.bin` and `file2_tradeitems.bin` to `.bak` names first; each still carries Moon's Tear and can contaminate a reused slot. The candidate's existing Combo-only guard makes those files irrelevant to combined ownership.

Load the corrected save and inspect the trade wheel: both items should be absent while Climb/Roc Boots and Sage's Tunic remain owned. Switch MM → OoT → MM, save, close, and reload; the items must remain absent. Create a fresh file with the candidate and modern equipment granted before its first player initialization, then verify the same absence. Collect the actual seed checks and verify the corresponding items are then awarded and survive switching/reload. Repeat with Skip Zelda's Letter on and off; this supplied seed only captures the off case.

Rollback consists of the untouched original files and source baseline `35ce5f22`. Runtime acceptance remains open until the user verifies the candidate executable.

An independent read-only code/repair review found no critical or important issue and independently confirmed the eight changes and both hashes. Fixture wording was clarified to exclude behavior cleanup, vanilla-base equipment updates and player/render refresh; those full-engine effects are not covered by the ownership fixture.

The candidate builds on the existing combined recovery branch for [draft PR #39](https://github.com/marsh6487/ComboShip/pull/39). The downloadable checkpoint carries the complete incremental patch against `35ce5f22`; full linked-build and gameplay acceptance remain separate gates.
