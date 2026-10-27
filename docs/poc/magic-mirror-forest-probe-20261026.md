# Magic Mirror forest reflection / Farore sound / Time Gate camera probe

User approved this bounded probe on 2026-10-26, including the Time Gate camera,
Farore's arrival sound correctly routed in 2Ship, and a forest reflected in the
glass. Baseline: `fa81f7926038f2aa85c7f3e73c96fec043168b66` (PR 65).

Preserve the round ALTTP blue/gold frame, Farore casting motion, green cast
seal/wisp motion and green palette, pickup/ownership/save, single button assignment, prompt cancellation,
outdoor checks, destination/arrival geometry, solo/combined modes, and all other
cumulative fixes. No Time Gate behavior changes and no master/develop promotion.

Implementation choices: one forest texture on the recessed glass only; native
32px RGBA16 plus an optional 512px resource/icon companion for both asset modes.
Use a fixed room-zero view of the actual archived forest geometry and textures,
as clarified by the user. The generated concept image is not used. Retain the
source image/provenance and deterministic render/conversion/build recipes.
The later screenshot reference fixes the shop/trunk/purple-portal composition;
include a small Saria silhouette by the portal as requested. Only that silhouette
sprite is generated from the reference; the forest/building remain actual scene
assets. This fixed reflection adds no scene actors or live forest render pass.
Audio follows the user's clarified timing: a one-shot magical activation shimmer
(`NA_SE_EV_TRE_BOX_FLASH`) when an eligible Mirror opens its prompt; native Farore
teleport (`NA_SE_PL_MAGIC_WIND_WARP`) only when confirmed travel commits and at
the actual destination arrival. Animation start/holstering/cancellation remain
silent. Rejected travel or handoff has no departure cue; departure and arrival
are each consumed once. Never play `NA_SE_PL_MAGIC_WIND_NORMAL`, the spell's wind
effect. Both sounds already exist in both native sound banks; use each engine's
own audio API. Audible preference still needs an in-game listen.

The 2026-10-27 follow-up explicitly includes the casting texture. Give the Mirror
private botanical seal and leaf-vein wisp resources in both native archives and
base/Alt companion entries. Keep the existing green ritual geometry, Farore
motion, growth/fade timing and MM resource-owner bridge. Add only a six-vertex
floor quad beneath the existing ritual; replace its shared wisp material with
the private forest material. Missing textures retain the green geometry-only
ritual. Time Gate and other items keep their original resources and dispatch.
Original generated RGBA artwork, normalized prompts, hashes and deterministic
packing/preview recipes live in `tools/magic_mirror/cast`.

Reuse OoT's Time Gate TURN_AROUND/KEEP4 type 10 framing on a privately owned
subcamera. MM's compatibility alias maps TURN_AROUND to NONE; provide a private
real KEEPON4 setting with the donor type-10 values for all native forms. Never
overwrite shared item-camera settings. Preserve the main camera and release only
the exact owned camera generation on cancel, timeout, reset or departure.
Before MM allocates its Mirror camera, enable the existing native cutscene
manager ownership guard for the current scene. Keep it enabled after release:
an interrupted native cutscene can still retain the Mirror as its return target.
This also prevents the manager's idle snapshot from overwriting an occupied
final camera slot. Scene initialization resets the guard normally.

Status: implemented on an isolated probe branch; eleven targeted check commands
passed, including ASan/UBSan cast/equip/camera/recall and both native guarded
draws. The full MM camera C translation unit passes syntax compilation. The
scene-anchor check and five asset/geometry checks also pass. The shared USED VFX
suite passes both native Mirror dispatches, green-only palette, missing-texture
fallback, matrix restoration, Time Gate isolation and accepted rod regressions.
The reflection
adds 15 XLU commands (244 total) and no vertex/triangle/arena-size increase.
Independent review identified the MM manager lifetime interaction above. The
actual native manager reproduces the failure without the guard; its idle
snapshot and interrupted return-camera paths pass with the fix under ASan/UBSan.
Source restore verification is recorded at handoff.
No full executable/Windows CI or configured gameplay acceptance is claimed.

Decisive runtime check: in each game cast from open ground, cancel the choice,
cast with a held item, recall and re-enter a scene; inspect glass/icon/cast with Alt
on/off and listen for one magical prompt chime plus Farore departure/arrival,
with no wind effect or cancelled departure. Check child/adult
and MM form framing, near-wall angles, and ordinary Time Gate/Farore use.

Rebuild the frozen reflection with:

```sh
python3 -B tools/magic_mirror/render_forest_reflection.py \
  --source /path/to/zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r \
  --output tools/magic_mirror/reflection/forest-scene.png
python3 -B tools/magic_mirror/generate.py --preview-dir /path/to/previews
python3 -B tools/magic_mirror/build_cast_textures.py
python3 -B tools/magic_mirror/render_cast.py --output /path/to/previews/cast.png
python3 -B tools/magic_mirror/build_forest_addon.py \
  --output /path/to/zzzzz_Magic_Mirror_Actual_Forest_POC1_OOT_MM.o2r \
  --preview-dir /path/to/previews
```

The render uses the original archived room, current native-v2 sign atlas UVs,
the actual building/portal meshes and a frozen Saria silhouette. Preview lighting
and layered magical materials are an offline approximation. It is a static
reflection asset, not live reflection rendering. Install the same optional HD
addon after the older Mirror/Gerudo NEI icon addon in each applicable game host.
The model glass/cast/camera/audio require the matching source build; no Windows
binary is included in this source probe.
