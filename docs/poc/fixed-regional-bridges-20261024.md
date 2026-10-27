# Fixed regional bridges candidate — 2026-10-24

Baseline: marsh6487/ComboShip develop `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`, tree `32e1dcb6a49e936bdab838c820052e206400ba9f`. Candidate only; no develop/master promotion. The prior regional endpoints depended on opportunistic wall/grave discovery and were reported absent in game. This candidate uses authored fixed endpoints and injected native geometry.

## Placement and behavior

| Route | OoT side | MM counterpart |
|---|---|---|
| Lake Hylia ↔ Great Bay | Deep cave below laboratory bank; mouth(-2185,-1305,3717),yaw16384; dry strips and low shoreline return plinth, native bank stone, no bank switchback ramps; inward inlet water follows native full/drained/refilled state | Dry wall beside Great Bay Coast's Termina approach; mouth(3404.66382,80,4431.17334),yaw25177; native entrance0 arrival |
| Graveyard ↔ Road to Ikana | Existing final grave nearest Royal Tomb in five-grave row; home(22,120,-336),room1,yaw0,params0; native pull and hole, gold-orange shimmer above and around the closed stone, native grotto return/pop-out | Wall beside native Road to Ikana approach; mouth(-3132.08398,0,-165.22842),yaw25730; native entrance0 arrival |
| Zora's Fountain ↔ Snowhead | Glacial shelf and dark recessed passage on the Ice Cavern approach; mouth(1840,400,0),yaw-16384; ice-blue shimmer across the black face and nearby snow/chill; native Ice Cavern entrance retained | Rock wall beside Mountain Village owl platform; shared winter/spring mouth(-396.76031,8,-322.01703),yaw-1362; native entrance8 arrival |

No additional scene archive is required. Cave/path meshes and dynamic collision are process-owned native actors. Native/Alt scene archives, waterboxes, lab doorway, Ice Cavern exit, other graves and their checks are unchanged. The marked grave retains native child/day pull restrictions; it is pullable under the normal adult/night conditions. MM season selection follows the existing Snowhead-cleared flag. Native small snowballs/owl remain; a measured walking bypass reaches the Mountain mouth without an item requirement.

Each bridge's geometry, trigger/effects and return facing use one fixed pose. Native camera data and inward wall/ceiling winding are tested with the engine's actual dynamic collision/camera functions. Source return staging survives fade/reload and waits until native Player_Init consumes the authored respawn. Explicit MM arrivals clear only the persisted pause-save marker that otherwise overrides native start-mode selection. File/seed ownership, solo barriers and dungeon/boss/Farore arrival priority remain protected.

## Verification and recovery

Required portable commands (the existing entrance/render CI groups invoke these):

```sh
python3 -B tests/regional_passages/run_tests.py --sanitize
python3 -B tests/regional_passages/run_presentation_tests.py --syntax
python3 -B -m unittest discover -s tests/ci -v
```

On a local SDK without system nlohmann headers, COMBO_NEI_FIXTURE_CPATH/CPATH may point to an installed single_include directory. All default tests are repository-relative and require no private game archive. Optional additional evidence:

```sh
python3 tests/regional_passages/run_lake_bridge_tests.py --sanitize --scene-dir /path/to/extracted/scenes
python3 tests/regional_passages/run_snowhead_bridge_tests.py --scene-dir /path/to/extracted/scenes
python3 tests/regional_passages/mm_collision_test.py /path/to/mm.o2r
```

Real evidence uses the supplied overworld export and unmodified native archives.
Whole-input identities remain in private recovery evidence and are excluded
from publication. The portable Lake fixture contains33 measured collision
triangles relevant to the approach; optional checks use the full1050-polygons.
The latest Lake refinement also ran that full native collision independently;
its inherited Alt control is separate from an actual custom Alt runtime test.
See the regional entrance visual-refinement record for the later full-native
Fountain render and adult ice-platform checks. Custom Alt render replacements
remain outside those native proofs.

Independent scoped reviews required corrections to deferred actor readiness, respawn staging, persisted pause-save start-mode override, dynamic camera data and inward collision winding. Their native failing cases were reproduced before correction. Source checks and native fixtures are separate from hosted compilation and gameplay acceptance.

Current state: combined local sanitized/native fixtures, real-asset collision controls, graphics syntax and21 CI unit checks pass. All four component reviews and their scoped corrections are approved. Whole-branch review is approved with no findings; hosted Windows/Linux CI is pending. No configured GPU/gameplay session has been run in this environment. Before runtime acceptance, check all three visible entrances, child/adult water climb-out and return, native child/day grave rules, repeated grotto pop-outs, Fountain ascent without crossing the Ice Cavern exit, winter/spring MM camera/start modes, every form, live ice-platform interactions, native/Alt rendering, shimmer/particles and nearby wave audio. Do not promote this candidate as runtime-proven on compilation alone.

Recover from the exact public baseline using the provided source checkpoint/restorer or patch, retaining normal oot/mm base archives and existing accepted packs. All generated runtime bridge geometry is source-contained; no private donor export is needed for a fresh build.
