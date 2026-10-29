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

The headless driver resolves 56 game exports by name. Under Fleet these become calls on `FleetGameApi`:

| Group | Exports today | Under Fleet |
| --- | --- | --- |
| Logic oracle | `Combo_*_Rando_Reset`, `SetOwnedItems`, `GetReachableChecks`, `PlaceItem`, `Restore`; `Combo_SOH_Rando_GetPortalOpen` | The `Rando*` fields already in the contract. |
| Static data | `*_DumpRandoStaticData`, `SOH_DumpRandoHintData`, `SOH_DumpSharedItemPairs`, `SOH_GetForcedPlacements` | `DumpRandoStaticData` plus a generic request call. |
| Settings | `*_DumpRandoSettings`, `*_RestoreRandoSettings`, `*_InitRandoHeadless`, `SOH_PrepRandoContext`, `SOH_Read*CVars`, `*_SetCombo*`, `*_SetCheckPrices`, the tricks calls, `SOH_NormalizeComboGraceFromMM` | Generic request call. |
| Dungeons and entrances | `*_DumpCrossDungeonCatalogue`, `*_GetCrossDungeonExits`, `*_SetCrossDungeon*`, `MM_CanEnterCrossEntrance`, `SOH_ShuffleEntrancesForCombo`, `SOH_ApplyEntranceOverridesForCombo`, `SOH_DumpEntranceOverrides`, the dungeon-layout calls | Generic request call; `ResolveExit` at run time. |

The proposal is one appended field, `char* (*RandoRequest)(const char* op, const char* json)`, keyed by the export names above. That keeps the contract small and lets the game side grow without bumping its version.

## Next

1. Add `RandoRequest` to the contract and implement it in the Unbound hosts.
2. Port `ComboRandoHeadless.cpp` from `LoadLibrary` and named exports to `Fleet_GetGameApi`.
3. Build the Fleet host itself: window, game selection, scene switches, saves and the item mapper.
