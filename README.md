# Fleet

Fleet runs Ship of Harkinian and 2 Ship 2 Harkinian in one process. It loads each game's shared module (`soh.dll`, `2ship.dll`) from that game's own install and talks to it through the contract in libultraship's `include/fleet/FleetGameApi.h`. Fleet does not compile either game: each game builds, ships and updates on its own.

## What is here

| Path | Content |
| --- | --- |
| `combo/rando/`, `combo/gui/ComboGenProgress.h` | The combined randomizer engine: combined fill, shared items, foreign placements, playthrough, hints, cross-dungeon and cross-entrance generation. |
| `combo/ComboRandoHeadless.cpp` | The headless seed generator. It is kept as the reference driver and is not built yet. |
| `combo/tests/combo_shared_test.cpp` | Behaviour checks for the engine. `ctest` runs them. |

The engine comes unchanged from [ComboShip PR #2](https://github.com/skijer/ComboShip/pull/2) by marsh6487, commit `ffa312d4`. It keeps the original `combo/` paths so later fixes can be synced file by file. Everything in it is headers plus nlohmann/json, with no game symbols.

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

## What the engine needs from each game

ComboShip's headless driver resolves 56 game exports by name. Under Fleet every service is a function looked up by name too: Fleet asks a game through its single export `Fleet_GetFunction`, and a game asks Fleet through the lookup it receives in `Attach`. A name that is not there is a service not offered; there is no contract version.

| Group | ComboShip exports | Under Fleet |
| --- | --- | --- |
| Logic oracle | `Combo_*_Rando_Reset`, `SetOwnedItems`, `GetReachableChecks`, `PlaceItem`, `Restore`; `Combo_SOH_Rando_GetPortalOpen` | `RandoReset`, `RandoSetOwnedItems`, `RandoGetReachableChecks`, `RandoPlaceItem`; the portal through `RandoRequest`. |
| Static data | `*_DumpRandoStaticData`, `SOH_DumpRandoHintData`, `SOH_DumpSharedItemPairs`, `SOH_GetForcedPlacements` | `DumpRandoStaticData`, and `RandoRequest` for the rest. |
| Settings | `*_DumpRandoSettings`, `*_RestoreRandoSettings`, `*_InitRandoHeadless`, `SOH_PrepRandoContext`, `SOH_Read*CVars`, `*_SetCombo*`, `*_SetCheckPrices`, the tricks calls | `RandoRequest`, one op per service. |
| Dungeons and entrances | `*_DumpCrossDungeonCatalogue`, `*_GetCrossDungeonExits`, `*_SetCrossDungeon*`, `MM_CanEnterCrossEntrance`, `SOH_ShuffleEntrancesForCombo`, the entrance and dungeon-layout calls | `RandoRequest`; switching games through `RequestSwitch` and `Arrive`. |

## fleet-seedgen

`fleet-seedgen` generates one combined seed from the installed builds, with no window:

```sh
fleet-seedgen --oot <Ship folder> --mm <2Ship folder> --seed 12345 --out spoiler.json
              [--settings other.json] [--triforce required[:total]] [--start oot|mm|random]
              [--shared all|bows,wallets,...] [--cross dungeons,expanded,interiors,shops,doorInteriors,grottos]
              [--dumps <folder>]
fleet-seedgen --oot <Ship folder> --mm <2Ship folder> --playthrough spoiler.json
```

It needs the games' `fleet-rando` branches. Fleet builds its own `libultraship.dll` from the submodule, with the options both games use, and each game binds to it. The cross-dungeon seed formats both games read live in that submodule too, under `include/fleet/rando/`.

The spoiler follows ComboShip's consolidated layout: per game its settings snapshot, placements and prices, the goal, the starting game, shared items, OoT's entrance layout and Master Quest dungeons, the cross-game entrance layout, the hints, plus the cross-game `foreign` list. `--settings` takes the settings, tricks, goal, starting game, shared items and cross pools from another spoiler instead of each game's own config; nothing is written back to the games' config files. A Random start resolves exactly as ComboShip's does. `--playthrough` rebuilds both games' logic for a finished spoiler, with its own prices and cross layout, and reports whether it is beatable. `--dumps` writes what each game handed the engine.

Served by the games: the oracle, the seed dump with everything each game confines to itself, Link's Pocket, the headless init, settings dump and restore, the combined goal (bosses or a Triforce Hunt split across both pools), the MM start, shared item families, OoT's own entrance shuffle, the cross-game dungeon, interior, shop and grotto pools, hint data and saved prices. Both games name checks and items by unique readable names.

Not served yet: cross-game boss arenas, and everything that happens while playing (the host).

See [docs/pr2-triage.md](docs/pr2-triage.md) for what else PR #2 brings and where each part goes.

## Next

1. Cross-game boss arenas: boss clears must reach both games' logic through one dispatch point.
2. The Fleet host itself: window, game selection, scene switches and cross-game warps, saves, the item mapper and shared item tiers at run time.
