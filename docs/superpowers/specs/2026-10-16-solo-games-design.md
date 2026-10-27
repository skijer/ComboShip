# Solo games in ComboShip

User request: implement OoT-only and MM-only gameplay, each supporting vanilla and randomizer, after the vanilla/MQ source handoff.

## Outcome

The existing executable offers Combo, Ocarina of Time, and Majora's Mask before ROM setup. Combo remains the default. A command-line `--game=combo|oot|mm` selects the same mode without prompting; `--game-select` reopens selection. The choice is remembered separately from gameplay options. Cancelling selection does not start a game or change the remembered choice.

Each solo game uses its native file select, vanilla save creation, native randomizer generation/spoiler loading, settings, and victory conditions. Solo OoT retains either or both installed vanilla/MQ archives and native mixed-layout randomizer controls. Solo MM never initializes OoT's ROM-dependent systems. Only the selected game's ROM is required; the distributed port modules and port assets still ship together.

## Architecture

A small C-compatible game-mode header supplies the enum and a per-module immutable session mode, set by prefixed DLL exports before initialization. Runtime mode replaces compile-time Combo assumptions only where needed. The launcher reuses the existing ROM-independent window bootstrap and extraction screen. Solo paths branch before registering Combo save, seed, transition, sharing, and goal callbacks. Each game's existing menu owns its solo UI. Combo's unified menu and eager dual-game boot remain on the existing path.

Native solo saves are isolated under `saves/oot/` and `saves/mm/`; existing `.combosav` containers are untouched. All slot operations, backups, global settings, copy and erase follow the selected native save directory. Cross-game portals, entrance adapters, shared-item/goal logic and stale Combo-specific options cannot route solo progression into the other game. Native randomizer settings and native NEI features remain usable.

## Preservation and exclusions

Base: merged PR43 `1bbb94e692629c0d910f97b57dd7cd02240d3775` plus exact MQ candidate tree `78d7d3504b8d7c3a27d8e4ef978eb77a5fd76a42`, preserved as commit `3fd851dd` in this checkout. The donor checkout is unchanged. This candidate does not consolidate unrelated polish archives or promote the accepted master. It preserves scene, actor, asset, rendering and audio changes already in this source.

No automatic conversion of combined saves or foreign native spoilers. A solo game cannot silently consume a Combo seed. Vanilla here means original progression/item placement with the port's existing user-selectable enhancements.

## Evidence

Behavior checks cover selection parsing/persistence/cancel, selected-ROM extraction including MM-only, all four solo file-creation/generation paths, save-path isolation, portal/goal guards and unchanged combined behavior. Compile changed production units against real project headers where dependencies permit. Run the existing MQ/setup and cross-dungeon tests plus the repository regression gate. Package complete source, solo-only and cumulative patches, exact hashes, and verification results. A full link/package and game-runtime tests are separate evidence; report any unavailable validation explicitly.
