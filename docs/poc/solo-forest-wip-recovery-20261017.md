# Native solo forest WIP recovery checkpoint

This is an intentionally incomplete recovery snapshot, reconstructed from the
agent's tool-call history after the execution host lost its scratch workspace.
It is based on `8ccbd47d`, in the isolated
`poc/black-market-solo-forest-recovery-20261017` worktree. No fresh build or test
result is claimed for this reconstruction. Preserve this checkpoint before
addressing the known defects below. It is not a release or a runtime acceptance.

## Intended local routes

| Mode | Approach | Forest | Shop and onward travel |
| --- | --- | --- | --- |
| OoT only | Existing pink Fairy tunnel in native Lost Woods room 1 | Native OoT exploration copy, scene `0x6F`, entry `0x618`–`0x61B` | Same physical shop doorway at `(410, floor, -300)`; stump and native Clock-exit block `0x61C`–`0x61F` return to captured native OoT Lost Woods origin |
| MM only | Established Woods of Mystery room-1 passage at `(160, floor, -160)` | Native MM exploration copy, unused scene `0x03` and entrance-scene `0x0B`, entrance `0x1600` | Same physical shop doorway; stump returns to captured Mystery origin; onward floor exit remains native Clock Tower interior `0xC000` |
| Combined | Existing combined portal routes and playable original opening forest | Existing original scene and guards | Unchanged |

The copies deliberately separate exploration from the original intro/flashback
scene. They do not weaken `ComboOpeningForest_IsExploration` or cross-game mode
guards. The work is for native solo modes inside ComboShip, not standalone
non-COMBO binaries.

## Immutable inputs and last pre-outage artifact

- Custom forest donor POC9 SHA256:
  `72d750b00583379f3619dac52dce59e2957a351fdfabd3a3a8da11f9bc19daf0`.
  Its old local path was
  `/workspace/scratch/508a50b2b151/asset-baselines/lostwoods-poc9/ComboShip-MM-LostWoods-Right-Nudge-POC9/mods/2ship/zzzz_MM3D_Lost_Woods_Right_Nudge_POC9.o2r`.
  That local copy was lost. Do not substitute or recreate the scenery. The
  converter intentionally rejects any donor with another hash.
- Native MM donor SHA256:
  `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
- Native OoT title donor SHA256:
  `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b`.
  Title: `textures/g_pn_36/gLostWoodsTitleCardENGTex`, 3536 bytes, 144×24.
- Last pre-outage addon:
  `zzzz_Skiejer_Native_Solo_Forest_POC1.o2r`, 88 resources, SHA256
  `900cc0008d5bdff816d070f486562cdaa98d2e99f3c09bc196b834de2c950698`.
  That artifact was not recovered with this source snapshot and still had the
  native rotation defects listed below.

`SoloForestResources.h` restores the previously generated 88 logical resource
names from recorded archive contents; it does not recreate physical assets.
The converter copies POC9 geometry, UVs, vertex colors, textures and material
values with private resource-path remapping only. The native collision retains
all 612 vertices, 810 polygons and surface indices; camera settings map to
native normal camera 1. The private namespace is `custom/skiejer_forest/`.
MM resources are `scenes/nonmq/skiejer_forest/skiejer_forest...`; OoT resources
are `scenes/shared/skiejer_forest_oot/skiejer_forest_oot...`.

The private exploration headers omit Epona, Skull Kid, Tatl/Tael, cutscene and
alternate intro headers. MM retains native dust, grass and rock actors. OoT
maps native Obj_Mure2 and omits unsupported MM dust/Keaton grass actors rather
than passing MM IDs to different OoT actor implementations. MM covered skybox
2 maps to OoT skybox NONE 0 (OoT numeric 2 is Bazaar), with original light
settings retained. OoT uses native Lost Woods music and Kokiri nature sounds.

## Known unfinished defects — fix after preserving this checkpoint

1. **Packed rotations (resolved by the rotation milestone below):** the recovered `forest.py` copied the MM transition yaw
   `0x877F` directly into OoT. MM decodes it as 270 degrees; OoT needs direct
   binang `-16384`. Private MM player spawn likewise needs packed degrees and
   cutscene fields, for example `(68 << 7) | 127`, instead of OoT binang
   `12288`. The existing resource test still expects the incorrect raw fields.
   Add meaningful tests using actual native actor-spawn production bodies.
2. **Invisible MM approach (resolved by the display milestone below):** existing `ComboLostWoodsPassage.cpp::DrawPassage`
   still rejects solo MM. Add a narrowly gated native forest availability API
   and enable only the existing Mystery arch with the addon available. Keep
   combined/opening/prologue conditions intact. Its production fixtures need
   the additional dependency and positive/negative solo draw coverage.
3. **Portal glints/state (resolved by the display milestone below):** both new glint loops draw only two triangles. Copy
   the accepted four-triangle diamond fan and final PipeSync/setup restoration.
   Verify native OoT setup binding `Gfx_SetupDL_25Xlu` against the full headers.
4. **OoT init deadlock (resolved by the lifecycle milestone below):** the recovered `AddInitFunction` invoked `LoadForestState`, which
   calls `ThreadPoolWait` while native `InitFile` runs under `saveMtx`; a queued
   worker needs that mutex. Init must clear only pending lifecycle markers.
   Loaded origin publication should wait in `OnLoadGame` outside the mutex.
   Add an actual-file-load marker so repeated age/warp OnLoadGame notifications
   do not reset an active origin from stale pending JSON.
5. **OoT include (resolved by the native compilation milestone below):** the confirmed pre-outage object failed on nonexistent
   `randomizer/context.h`; use the actual `SeedContext.h`. An attempted patch
   was terminated without a result before the outage. This reconstruction
   intentionally restores the confirmed failing include for the WIP snapshot.
6. **Remaining validation (native units and save fixture checked below):** compile both new controllers, both changed shop
   controllers, both Play units, MM SaveManager, and scene-name units with real
   headers. Update MM native save behavior fixture for the new forest save
   callback. Run existing combined forest and broad solo guards. Test forest
   load/reset/copy/import and later-hook ordering, worker-state immutability,
   legitimate shop RETURN, void/death, missing addon, all ages/forms and slots.
7. **Documentation and presentation integration (updated below):** the earlier native-shop
   document still describes direct Mystery/native-LW shop routes and must be
   corrected. Root separately owned weather/foliage reconstruction; those lost
   edits are not included here. They need narrow private-scene predicates and
   eight private texture targets, including base/Alt parity. Preserve original
   portal finish, local music and authored covered lighting.

## Historical evidence, not fresh reconstruction verification

Before the outage, both new extracted production forest fixtures and the updated
shop fixtures passed ASan/UBSan. Resource comparison passed the exact scenery,
closed dependencies, native scene framing and collision topology checks, but
missed packed-angle semantics. The complete MM forest controller compiled to a
native object; the OoT one failed at the include above. Review then found the
init deadlock, invisible approach and incomplete glint fan. No full executable
was built and no traversal was observed in game. Historical scratch logs were
lost, so do not label them current evidence.

## Recovery commands after the checkpoint is saved

From this repository, with recovered JSON/native include dependencies available:

```sh
python tests/black_market/run_solo_forest_tests.py
python tests/black_market/run_native_tests.py
python tests/black_market/run_oot_native_tests.py
python tests/black_market/run_scene_names_tests.py
c++ -std=c++20 -Icombo tests/black_market/solo_forest_test.cpp -o /tmp/solo-forest-predicates
/tmp/solo-forest-predicates
```

Once the exact POC9 donor is recovered:

```sh
python tests/black_market/check_solo_forest.py "$POC9_FOREST" "$NATIVE_MM"
python tools/black_market/forest.py "$POC9_FOREST" "$NATIVE_MM" "$NATIVE_OOT" "$NEW_ADDON_PATH" --contract combo/entrances/SoloForestResources.h
```

Use a new candidate output, preserve all donors, and put new logs under
`lock-visual-verification/solo-forest*.log`. The old dependency paths and native
object helper were also scratch files; recover/recreate that environment
explicitly rather than claiming the source checkpoint is already tested.

## Native rotation milestone after durable recovery

The converter now decodes the MM hallway's packed degree field before writing
the direct OoT binary angle. Both native actor dispatchers consequently receive
the same hallway yaw `-16384`. MM's private player row now encodes 68 degrees
(the nearest native integer degree to 67.5), actor cutscene 127 (none), and all
half-days; OoT keeps direct yaw 12288 and its idle start parameters. The safe
arrival position and donor hash requirements are unchanged.

`run_solo_forest_rotation_tests.py` reads only the exact native MM archive's
transition row, executes the production converter statements, and compiles the
actual MM and OoT `Actor_SpawnTransitionActors` and `Actor_SpawnEntry` bodies
against their real engine types under ASan/UBSan. Separate failing controls
reproduced the OoT hallway and MM player errors before the change. The fresh
passing fixture checks both room directions, once-only transition spawning,
native actor IDs/parameters, player pose and MM day/cutscene flags.

Evidence: `lock-visual-verification/solo-forest-rotation-red.log` and
`solo-forest-rotation-green.log` outside the worktree. This validates source
conversion and native actor dispatch, not a rebuilt POC9 addon or rendered
traversal. The exact custom POC9 donor is still required for the full asset
comparison and a new addon. The other review defects remain outstanding.

## Native save lifecycle milestone

The OoT registered InitFile callback now clears only pending/lifecycle markers;
it does not wait or mutate the worker-facing slot, seed or saved origin under
native `saveMtx`. A guarded OnLoadGame binder publishes loaded state after
waiting outside the native file-load mutex. Only native init/load callbacks
mark a pending file bind; repeated age/warp notifications retain the active
origin and cannot re-arm a stale saved-inside marker.

The production callback fixture reproduced both prior failures separately:
waiting under the simulated native save mutex, then replacing an active origin
from old pending JSON on an ordinary notification. It also exposed and fixed a
legitimate shop RETURN being discarded for the other three private OoT entrance
variants. All four variants now preserve native RETURN data.

Fresh ASan/UBSan fixture evidence checks real registered callbacks, initialization
without waiting or origin/slot/seed mutation, genuine same-slot reloads, later
entrance-rando/Remember Location hook writes, stale seed/slot rejection,
grounded candidate publication after the save-worker wait, airborne entry,
safe forest reload/void/missing-pack recovery and TOP preservation in both
native engines. Logs: `lock-visual-verification/solo-forest-lifecycle-red.log`
and `solo-forest-lifecycle-green.log`. Native object compilation and configured
game traversal remain separate outstanding checks.

## Deterministic addon packaging milestone

The exact POC9 donor has now been recovered by replaying its authoritative asset
pipeline, and its original SHA256 matches the immutable contract above. The CLI
keeps sorted resource order and now writes fixed ZIP metadata: 1980-01-01
00:00:00, Unix regular files with mode 0644, and deflate level 6. No resource
payload or route conversion changed in this packaging milestone.

`run_solo_forest_pack_tests.py` runs the actual CLI twice on the real POC9/MM/OoT
archives while mocking ZIP's wall clock to 2021 and 2033. Before the fix, all
resource payloads and generated contracts matched but the full archive hashes
differed. After the fix, exact archive bytes, all 88 resource payloads and the
generated contracts match. The test also checks metadata, sorted unique paths,
ZIP integrity, donor immutability and refusal to overwrite a candidate.
Both fresh builds produced SHA256
`3d45015fa43997e2f3a96440e018a6668a2b37d654bc3e32843c0f9100093cfb`.
The full exact-donor scenery/collision/schema checker also passed freshly.

Evidence: `lock-visual-verification/solo-forest-pack-red.log` and
`solo-forest-pack-green.log`. Use a newly named POC2 output for the resulting
candidate; the historical POC1 is not promoted by this check. Scene traversal
and configured visual/runtime acceptance remain untested.

## Native approach and portal submission milestone

The existing Mystery bark arch now draws in MM-only mode when the current
playable slot and seed have bound the native forest state and every required
addon resource exists. Its scene/room check remains the original Mystery room
1 check. Original opening-forest/prologue predicates are unchanged; unavailable
addons and other solo modes do not advertise the passage. Pausing does not
remove an otherwise available arch.

Both private forest controllers now submit all four triangles of each animated
glint diamond, followed by the accepted PipeSync and native XLU setup restoration.
OoT uses its actual native `Gfx_SetupDL_25Xlu` binding. Existing explicit XLU
render modes, fog clearing, dynamic vertex allocation and purple geometry are
preserved.

Fresh failing controls reproduced the invisible MM solo arch, incomplete fans
in both hosts, and missing end-state restoration in both hosts. Passing
ASan/UBSan fixtures execute the production passage selection/availability and
both complete forest draw bodies against native types, replacing only graphics
submission/allocation boundaries. They check the exact six four-triangle fans,
26 rim quads, animated vertex values, state restoration, title dimensions and
scene/safety/allocation guards. Existing combined finish/pack selection also
passes. Evidence: `solo-forest-passage-{red,green}.log`,
`solo-forest-draw-{fan,state}-red.log`, and `solo-forest-draw-green.log` in
`lock-visual-verification/`. Full native object compilation and configured
rendered traversal remain separate outstanding checks.

## Full native integration compilation milestone

The native OoT compilation first reproduced the recorded missing `context.h`
include. The controller now includes the actual `SeedContext.h`. Both complete
forest controllers, both native shop controllers, the Mystery passage renderer,
MM SaveManager, both native Play units, gameplay statistics and scene-name
utility compile freshly to native objects with real engine/dependency headers.
No link, executable or gameplay claim is implied by these ten objects.

`scripts/diagnostics/compile_solo_forest_core.py` records the exact compiler
command, source/shared-contract hashes, exit status and object hash for each
unit. Successful compiles also retain the compiler's native dependency list
with a SHA256 for every included file. It uses each engine's CMake language
standard and MM's native C global-header precompile environment; no test shims
are inserted. Supply the restored JSON, audit, SDL2/GLM and libzip include
directories explicitly through repeated `--include` arguments.

The existing MM native-save fixture first failed on the missing forest save
callback dependency. It now checks that native primary and backup writes call
the forest snapshot with slot 0, native copy calls slot 1, and combined writes
do not call the private forest snapshot. Existing read/write/copy/erase/global
and solo-guard assertions are retained and pass freshly. Native market docs now
describe the shared forest approach instead of the superseded direct routes.
Root separately restored and checked the private weather/foliage integration;
its evidence is in `solo-forest-weather-recovery-20261017.md`.

Evidence: `lock-visual-verification/solo-forest-core-*-compile.log`, companion
`*-dependencies.json`, the preserved `solo-forest-native-red/` include failure,
and `solo-forest-mm-save-fixture-{red,green}.log`. Root owns the fresh broad
combined/solo regression gates and final independent source review. Native
file IO in a linked game, configured traversal/rendering, child/adult/MM forms,
collision/camera acceptance and user acceptance remain pending.
