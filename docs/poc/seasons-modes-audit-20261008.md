# Rod of Seasons mode recovery

This implements the approved Audit Merge settings follow-up. It is source and fixture verification; live gameplay acceptance and a linked platform build remain separate checks.

The seed stores one mode and a separate Start with Rod choice. Both native option enums append the new keys, preserving existing numeric keys. Mode zero remains Individual seasons for old native save arrays and old consolidated seed snapshots. Invalid mode values also use Individual seasons.

| Mode | Shuffled pool when NEI items are enabled | Rod behavior |
| --- | --- | --- |
| Individual seasons (0, default) | Four independent season items | The first season grants the Rod. Each pickup grants only its own season. |
| Rod (1) | One Rod item | The Rod grants all four seasons. |
| Gated (2) | One Rod item | The Rod is usable with Off selected; completed checks unlock the season choices. |

Start with Rod grants the Rod at native save creation regardless of the NEI pool switch. It removes the base Rod pickup in Rod/Gated modes and preserves the four shuffled pickups in Individual mode. The existing OoT plentiful-pool duplicate policy remains unchanged. Combo uses the MM-backed menu choices for both generators, even when general settings sync is off. Gameplay uses saved seed options, not live menu values.

Only Gated mode uses the following completion alternatives. Any one alternative in a row suffices. These are native event/check-completion flags, independent of randomized song, stone, medallion, or boss-remains possession.

| Season | OoT completion alternative | MM completion alternative |
| --- | --- | --- |
| Spring | Windmill Song of Storms check **or** Jabu-Jabu's blue warp **or** Water Temple's blue warp | Great Bay Temple clear |
| Summer | Fire Temple's blue warp | Stone Tower Temple clear |
| Autumn | Forest Temple's blue warp | Woodfall Temple clear |
| Winter | Ice Cavern's final Sheik check (vanilla/MQ) | Snowhead Temple clear |

Spring's any-one rule and the four MM mappings were recovered from the accepted user request. The OoT Fire/Summer, Forest/Autumn, and Ice/Winter mappings are **recovery assumptions**, chosen with the integration owner's approval because their exact historical chat mappings were unavailable. They are not quoted user approval. Ice Cavern uses the native collection event for its final Sheik check; OoT has no separate Ice Cavern boss-clear flag.

Rod ownership, earned season pickups, and completion gates are distinct save fields. Old season masks or the full-width Rod inventory cell recover Rod ownership. Gates do not grant the Rod. Gates latch before Rod acquisition, survive Song of Time and save/reload, and merge by union during either cross-game handoff. Receiving an empty gated Rod repairs its full-width inventory cell without fabricating a season pickup. Off remains available whenever the Rod is owned. Debug editor cell repair retains the selected Off choice and does not recount an earned empty Rod.

Seasons remain optional to both reachability solvers. This follow-up does not add seasonal traversal dependencies or change weather/sky visuals.

The canonical `scripts/diagnostics/run_combo_nei_regressions.sh` gate includes the new mode, native OoT gates, complete MM generation/starting-items, and native MM save fixtures. Existing native settings, both-host persistence/sync, editor, Rod input, and weather fixtures also cover the new fields. The native settings fixture executes real OoT option registration, MM-first finalization, save-array reload, native NEI pool/count logic at all pool densities, and the starting-Rod inventory prefix. Standalone fixtures execute production functions and pickup/sync branches with native headers; they do not run the fill worker, launcher, or game loops.
