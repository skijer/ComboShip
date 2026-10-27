# Final cumulative gate follow-up — October 19, 2026

## Woods release scope

cor clarified that only these Woods of Mystery integrations belong in the bundled build:

1. Green hex shimmer at the OoT/2Ship bridge.
2. The exported wood tunnel replacing the original doorway, with its minimal native wall/canopy fit support.
3. Seasonal tint routing, including private grass, wall-trim, foliage and grass-actor resources.

The complete Woods makeover assets are not bundled. The normal stock archive input contains exactly eight files under `mm/assets/custom/custom/cor_woods_bridge/`; it contains no Woods scene archive or full makeover textures. Full-scene authoring tools and recovery recipes remain source records outside the stock asset tree.

## Transition fixture link correction

The exact published candidate `b3049a6e285b6e3a0536cc8ac3e33f71b653e07f` passed hosted formatting and the repaired native weather/winter preflight. Its [Build Artifacts gate](https://github.com/marsh6487/ComboShip/actions/runs/39338675119) then reproduced a local transition-fixture link failure: the copied MM host block includes the unrelated `MM_ApplyComboPortalArrival` entrypoint, which references `MagicMirror_QueueArrival`.

The cross-dungeon test exercises native fades and handoff hooks, not that arrival entrypoint. Its MM handoff compiler command now uses function/data sections and linker section collection, matching the existing standalone forest fixture. Executed handoff code still links and runs under address/undefined sanitizers; no game implementation, callback or arrival state is changed. Actual Mirror arrival behavior retains its separate production runtime/forest fixture coverage.

The original command failed with the missing Mirror callback before this correction. The focused MM handoff sanitizer command and the complete four-variant transition command pass afterward. The remaining cumulative gate and exact-head full application builds are separate pending checks, not inferred from this fixture result.

Recovery inputs and the complete 45-workstream inclusion remain recorded in `final-cumulative-merge-20261019.md` and `final-cumulative-checkpoint-20261019.json`. This source follow-up does not claim configured gameplay acceptance or promotion of any external scene master.

## Remaining fixture dependencies

The broader gate exposed additional fixture maintenance after combining the accepted workstreams:

- The shared OoT key/silver-rupee cosmetic fixture now includes the actual pure-data `BlackMarketShelves.h` dependency used by the production editor's default shelf tint. Full key/heart and SoH compatibility groups pass, including sanitizer cases.
- The foreign-soul runner now extracts the actual native `SkelAnime_ApplyNpcSkin` helper along with its draw functions. The fixture explicitly models boss souls with no NPC skin plan, executes the real helper's null-plan path, and rejects unexpected skin blending. The complete foreign-soul/imported-boss group passes in normal and sanitizer configurations; NPC replacement skinning has its own separate coverage.
- The young-Epona actor fixture supplies the combined-game mode and fails if forest-only presentation services are unexpectedly called in its native young-horse cases. Its player compiler also includes the existing `combo/menu` header path, required by elemental-sword receipts. The complete Epona actor/player/save/cosmetic group passes, including full player/horse translation-unit checks.

Each missing dependency was reproduced before correction. These changes affect test fixtures and their compiler commands only. No game implementation, item behavior, scene resources or asset packaging changes are made in this follow-up. Exact-head hosted full application builds remain a separate gate.

## Standalone resource logging dependency

The combined source checkpoint `a0fd97c155c0c0641fe8b257c5823eaaa2170931` passed the complete Windows renderer/UI/launcher preflight and every preceding hosted regression group. Its [NEI gate](https://github.com/marsh6487/ComboShip/actions/runs/39340883540) then reached the real Saria resource fixture and exposed unresolved external `fmt::v9` symbols. Ubuntu's spdlog headers select an external fmt implementation; the standalone resource and save fixtures did not supply it.

The forest actor runner and MM Saria resource runner now compile their standalone C++ fixtures with `FMT_HEADER_ONLY`, matching existing standalone tests. This retains the actual resource classes and logging implementation. It changes no application compiler settings, game source, resource data, actor behavior or audio integration. The native monkey suite remains the first forest-runner check.

The missing implementation was reproduced with genuine spdlog 1.12.0 and fmt 9.1.0 headers in external-fmt configuration. Verification covers the complete forest runner with both external and bundled fmt configurations, and the MM Saria snapshot/resource suite under ASan/UBSan. Exact-head hosted continuation and full executable/archive builds retain their separately reported status.

## Editor grant fixture dependencies

The full local NEI continuation reached both native editor grant fixtures and reproduced an unresolved `TravelTools_Grant` dependency from the real inline `ExtInv_GiveItem` path. Both editor runners already copy the Grace/hourglass grant functions; they now also copy the actual `TravelTools_Heal` and `TravelTools_Grant` bodies from their respective engine's production inventory implementation. No replacement grant stub or ownership rule is introduced.

The MM suite exercises real randomizer grant arms, grant-all/individual/repeat behavior, ordinary inventory preservation, pool bindings under ASan/UBSan, and complete SaveEditor/GiveItem header compilation. The OoT suite retains its native inventory, selector, mode and repeated-grant checks. These are fixture dependency repairs only; the source merge still does not establish configured gameplay acceptance.
