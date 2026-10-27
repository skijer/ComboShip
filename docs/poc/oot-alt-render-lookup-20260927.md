# OoT Alt resource lookup POC1

Baseline: `3ccad569d6c9ceac0cfac57ef0e22e19d6771b96`, the build recorded in the supplied runtime log. Din ClothingCommands POC2 remains the test pack. This is an isolated source candidate, not an accepted or promoted master.

## Hypothesis and change

Warm filepath vertex/display-list lookups currently allocate a ready promise/future through ResourceManager::LoadResourceAsync. Reusing ResourceManager::GetCachedResource directly can remove this overhead. A miss or dirty resource still uses the original LoadResource. There is no added cache, retained resource, invalidation policy, altered display-list command, or graphics-state change.

The shortcut is enabled only during OoT RunCommands with a valid play state in any OoT scene, Alt enabled in the resource manager, and an unpaused game. The flag is cleared before RunCommands returns. The shared renderer defaults to disabled, and MM never enables it. Routed display lists still select their original resource manager.

`gDeveloperTools.AltRenderLookup` defaults to 1 in this candidate. Set it to 0 for the original loader path in the same build. Existing FrameTimingProbe and RendererCostProbe settings still control diagnostic collection.

Each sampled RenderCostProbe record now includes `alt_render_lookup`: enabled, vertex_hits, vertex_fallbacks, display_list_hits, display_list_fallbacks. These are per sampled draw, not cumulative; disabled reports have zero counters. A hit may select native or Alt resources according to the existing manager policy.

## Preservation

Scope expanded at the user’s request from Hyrule Field only to every OoT scene; Hyrule Field remains the primary benchmark.

No changes to Din, actors/count/placement/behavior, scene assets, geometry, caustic bindings, textures, weather, collision, progression, audio, or MM game code. Both resource managers retain their current owner/archive/Alt selection and dirty-resource behavior. The existing build-time texture-scroll patch remains part of the baseline and is not duplicated in this candidate's source diff.

## Evidence and limits

- Production resource-cache fixture passes for OoT and MM under undefined-behavior sanitizer: Alt on/off, tagged paths, dirty reload, unload/reload, missing resource, foreign manager, owner switch and externally replaced cache entries.
- Frame timing accounting, render-cost accounting, C bridge, and real asynchronous JSON logger tests pass, including the new lookup fields.
- Complete interpreter.cpp translation unit passes g++ C++20 syntax checking with the baseline build-time texture-scroll patch applied and required CVar definitions.
- Full application build is not locally verified: CMake configuration stops at the missing SDL2 development package. Header-only dependencies were sufficient for the checks above. Windows CI/build and in-game appearance/FPS remain unverified.
- Supplied POC2 child Hyrule Field data: approximately 28.05 weighted FPS, 26.07 ms graphics commands per presented frame, 1.215 ms play update per game tick. Vertex filepath commands averaged about 4.70 ms and display-list filepath commands about 1.61 ms per renderer sample; these include work beyond resource lookup and are not predicted savings. Profiling measures CPU elapsed time, including driver waits, not GPU timestamps.

## Decisive runtime comparison

Use the same save/child age, Hyrule Field entry route, camera, weather, actor state, mod order, Din POC2, resolution and MSAA. Keep TP NPC packs and Meadow actor trim disabled as in the supplied POC2 run. Compare warmed intervals with the lookup setting 0 and 1; confirm the enabled field and hit counts in the log. Compare graphics-command time and weighted FPS, and check appearance. Repeat a matched comparison in Sacred Forest Meadow and Temple of Time. The shortcut should also be enabled there with Alt on. Also pause/unpause, exit/re-enter, toggle Alt, and switch to MM to check scope and resource stability. No expected FPS target is asserted in advance.

Recovery: set the lookup setting to 0, or use the unchanged baseline build. Do not promote until runtime comparison and visual checks are accepted.
