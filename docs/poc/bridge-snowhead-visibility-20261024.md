# Snowhead readiness and resource ownership probe — 2026-10-24

| Field | Record |
|---|---|
| Baseline | Candidate parent `bb6f927e`, tree `292685496dd311b2b1b97c0847eea537988b6c5a`; equivalent public PR61 snapshot `602e4306` |
| Candidate | `poc/bridge-material-visibility-20261024`; develop/master remain unmerged |
| Scope | OoT ownership of Fountain backdrop/vtx loads; observable readiness rejection; first-cycle material modulation followed by `G_CC_PASS2` |
| Preservation | Same valley/ascent for child/adult, native adult icebergs, Ice Cavern exit clearance, scene/header/collision/water data, manual Prelude regional actor placement |
| Runtime evidence | User reports Fountain mesh and shortcut absent in both ages. Supplied latest log session identifies installed `9c3e41e`; it contains no valley readiness diagnostics. It does not identify the rejected gate or establish this candidate in game. |
| Verdict | Ownership defect reproduced in native fixture; correction and material correction require configured runtime acceptance. No gameplay/GPU proof claimed. |
| Recovery | Bounded source commit on the candidate branch; root handoff includes durable source checkpoint. |

## Confirmed defects and unchanged gate order

`ComboFountainValley_Ready` checks combined game / normal game mode / playable save slot / Fountain scene / room0, then prepares the selected backdrop, then ensures the native valley actor and verifies its scene-collision owner. Both Snowhead actor spawning and the shortcut endpoint require that result. This is age-independent. A rejected backdrop therefore explains how both surfaces and shortcut can disappear together, but the supplied log cannot identify which readiness check failed.

The backdrop provider used the generic `ResourceLoad` bridge, which reads the globally active ResourceManager. Its own scene belongs to registered owner `oot`. A foreign/UI context can select MM globally while the OoT owner still holds the correct Fountain resources. The native fixture keeps those boundaries distinct: global lookup returns no Fountain resource while registered OoT lookup has the verified native backdrop and vertices. The old production `PrepareBackdrop` fails that assertion. Direct registered-owner loading fixes it without changing the global manager. The detailed XML canopy path is also exercised with a foreign active manager. This proves the ownership defect; it does not prove that this was the state in the user's latest Fountain visit.

The companion material regression uses native renderer `GenerateCC` to demonstrate that repeated texture modulation in both cycles under `Gfx_SetupDL_25Opa` consumes inherited texture slot1. Valley and ascent surfaces now modulate in cycle0 and pass that result through cycle1. See the companion material report and native renderer test for the decisive RED/GREEN evidence.

The earlier cached-native-vertex-pointer acceptance remains intact and is not presented as a new fix.

## Active graph evidence and limits

The recovered native archive's actual backdrop has45 commands and the expected12 native vertices. The recovered full XML export calls the named canopy leaf and all378 referenced canopy vertices are at or aboveY1843. Both pass the existing guard; neither establishes the actual selected resource graph in the user's current35-archive configuration. Existing archive fixtures walk the six room setup roots with Alt/fallback resolution and verify complete body clearance; they are distinct from a configured current-pack runtime.

Collision alone cannot justify bypassing the visual-layout gate: the original native tree screen is a render-only obstruction, while the collision route is already clear. A safe non-destructive fallback for an unknown pack would need to traverse that pack's actual active room display-list graph and prove the entire body corridor visually clear. No speculative guard widening or foreign backdrop patch is included.

CMake recursively includes these providers with `CONFIGURE_DEPENDS`; each registers through `RegisterShipInitFunc`. Native `Actor_Spawn` uses `OBJECT_GAMEPLAY_KEEP`, preserves the ordinary deferred-init lifecycle, and has no age gate. The provider checks do not replace scene collision/header or water resources.

## Focused diagnostics

The candidate logs `[SnowheadValley] hooks registered` and `[SnowheadBridge] hooks registered` at initialization. Valley readiness emits only when its status, selected resource/owner, age or scene layer changes, and records the selected metadata path, room, file/game mode, command/vertex counts, cached vertex operands, actor/bg identifiers and scene collision identity. Reasons distinguish missing/foreign native vertices, unrecognized parent list, missing/low detailed canopy, and unavailable collision actor. The ascent reports its first spawn request per scene and each native collision initialization. These are normal log entries, not a separate diagnostics-only build.

## Verification

Run the existing portable/native providers with the normal SDK dependency includes:

```sh
python3 -B tests/regional_passages/run_valley_tests.py
python3 -B tests/regional_passages/run_snowhead_bridge_tests.py --native-archive /path/to/oot.o2r
```

The valley runner compiles the complete production object with strict const-filename checks, verifies unmangled native interpolation hooks, and runs actual production lifecycle/build bodies plus native collision and cached-vertex-handler bodies under AddressSanitizer/UndefinedBehaviorSanitizer. Its new cases verify registered OoT ownership with a foreign active manager, non-mutating rejection, collision-allocation failure reporting, and no readiness log on every stable frame. The Snowhead runner compiles the complete production draw object, checks native interpolation ABI, exercises actual collision/camera/lifecycle functions, all adult iceberg swept poses, and local chill presentation.

Remaining decisive runtime check: identify this candidate build, enter the accepted Ice Cavern-side valley/ascent as child and adult in vanilla and the current Djipi pack/load order, inspect readiness lines if absent, and verify mesh, shortcut round trip, water/icebergs, native Ice Cavern clearance, pause/re-entry and Alt toggling. Source/native checks do not resolve the reported runtime absence by themselves.
