# Moon's Tear grant fallthrough follow-up

| Field | Record |
| --- | --- |
| Baseline | `1bbb94e6` on `fix/shop-forest-entrances-20261016`; the October 8 Combo sidecar guard and current new-save equipment layout markers are present. |
| Scope | Stop MM dungeon-item and soul grants from also granting Moon's Tear in OoT. Preserve FC receipts, actual trade grants, native saves, legacy standalone sidecars and earlier ownership fixes. |
| Candidate | One terminating `break` in `Randomizer_Item_Give`, plus a production-code regression fixture. No current user save was supplied or modified. |
| Configuration | Headless native OoT/MM save structures, real NEI codecs and ordinary nlohmann JSON. No asset packs, rendering, game loop, scene or live network are loaded. |
| Recovery | Remove the three-line production change to restore baseline behavior. Existing save bytes are unaffected. |
| Status | Source defect reproduced and correction implemented; headless verification below. No gameplay acceptance or master promotion is implied. |

## Proven source defect

In OoT's `Randomizer_Item_Give`, 16 MM dungeon-item cases and 52 MM soul cases immediately precede `RG_MM_MOONS_TEAR` without a terminating statement. Every one therefore runs `TradeAdult_GiveItem(ITEM_MM_MOONS_TEAR)`, setting trade-wheel bit 11 (`0x800`). None of these 68 IDs belongs to the earlier `RandoGetToRandInf` fast path, so that earlier return does not protect them.

This is independent of the old sidecar importer. With MM enemy souls unshuffled, the production `GetComputedStartingItems` adds those souls to the starting list. Native grants record their FC receipts. When OoT's production `ApplyFcRegistryToNatives` materializes the incoming receipts, the missing break grants the unrelated Tear. MM's existing trade-wheel compatibility healing can subsequently create the Tear's obtained receipt and native item from that bit.

The correction ends this intended no-op inventory group. Its own FC receipt is still recorded before the switch, so souls and dungeon items continue to cross. No receipt or ownership cleanup is added; real Moon's Tear and the other trade entries keep their existing handlers.

## Reproduction and verification

The new `tests/moons_tear/run_tests.py` compiles the affected production cases, both real FC lookup/materialization paths, trade writers, trade/receipt merge blocks, new-save initializers and native NEI serializers. The MM starting-list functions are complete production bodies. MM native trade-slot writes and obtained flags execute production code. Generated shared data uses real JSON text between two native host executables.

The probe starts fresh paired states, covers all 4,096 combinations of twelve computed-start switches and all 95 explicit MM UI starting choices, then passes each state MM → OoT → MM → OoT → MM. Each receiving host repeats merge/materialization across native NEI save/reload. Independent cases exercise all 68 affected OoT grants and all nine real MM trade grants in OoT, including the actual Tear. Trade ownership and complete obtained/applied FC arrays must persist without introducing an unrequested Tear.

The fixture deliberately substitutes unrelated local item effects, progressive conversion, deterministic time RNG, presentation, hooks and unrelated shared vitals. Its 95-choice coverage proves the starting-list/FC routing into the affected boundary, not the complete gameplay effect of every starting item. Native inventory/flag snapshots accompany the NEI fields in the private paired fixture; this is not a complete `.combosav` engine codec test.

Commands use `CPATH=/workspace/scratch/60110ae7df32/analysis/deps` for existing normal dependency headers. Sanitizer runs disable LeakSanitizer process inspection with `ASAN_OPTIONS=detect_leaks=0`.

| Check | Evidence |
| --- | --- |
| New direct regression before the source change | Failed on `RG=505`, `fcId=309`: a Woodfall Small Key granted trade mask `0x00000800`. |
| Fresh-pair negative control with `--grant-ref 1bbb94e6 --paired-only` | Failed on OoT scenario 0: expected `0x00000000`, actual `0x00000800` after native sync grant. No configured starting item or stale sidecar is required. |
| Corrected direct grants | All 68 dungeon/soul grants and nine actual trade grants pass. |
| `tests/moons_tear/run_tests.py --sanitizers` | Passes with ASan/UBSan: all 4,191 scenarios complete both switch rounds and native NEI reloads, retaining the nine legitimate trade grants. |
| `tests/nei_shared_slot/run_save_tests.py --sanitizers` | Both native hosts pass, including reused-sidecar exclusion, genuine legacy layout migrations and legitimate item persistence. |
| `tests/nei_shared_slot/run_save_tests.py --host soh --standalone` | Passes; legacy standalone trade-sidecar import/export remains intact. |
| `tests/item_grant_audit/run_tests.py` | Passes with its ASan/UBSan checks, including 46 MM-host OoT dungeon cases, Moon's Tear, spells, native shared tiers and passive ownership captures. |
| Python syntax, runner shell syntax and `git diff --check` | Pass. |
| Full engine build / in-game test | Untested. |

The new fixture is registered in `scripts/diagnostics/run_combo_nei_regressions.sh`. Formatting executable availability is separate from source correctness: no `clang-format` executable is installed in this environment.

## Existing saves and runtime acceptance

This proves a current source path that produces the reported symptom. A specific user's current contaminated save cannot be attributed or safely repaired without inspecting that save and its seed/check evidence. The correction intentionally preserves ownership already persisted in `.combosav`; it cannot distinguish an earned Tear from an older phantom bit.

For the candidate executable, create a fresh pair with no configured Tear and enemy-soul shuffle off, inspect both trade wheels, switch twice, then save/close/reload. Repeat with starting MM keys/maps/compasses and soul shuffle on. No Tear should appear. Collect its actual randomized check (or explicitly configure it as a start) and confirm the legitimate Tear survives switching and reload. Existing accepted assets, entrance routes, equipment and standalone save behavior remain outside this source change.
