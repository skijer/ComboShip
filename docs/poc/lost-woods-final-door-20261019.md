# Final Lost Woods scene and authored Black Market door — bounded candidate

The later solo/poster checkpoint is documented in
`final-forest-solo-scroll-20261019.md` and supersedes the complete scene filename
below. Its native address repair remains identical.

This candidate connects cor's final native wooden door to the Black Market and
repairs native scrolling display-list addresses. It does not promote an accepted
baseline. The reported first-frame Prelude freeze has not been reproduced here,
and neither Prelude preview nor a configured game session is verified.

## Inputs and candidate

The unchanged final user input is
`1 - lost woods 2ship, wood bench added. final for merge.prelude.o2r`,
121116642 bytes, SHA-256
`89bdb0ce3ce2e22ca7e4aed438251a66a167ee8b1d68a7b1b019a05c35ac34f9`.
Its retained identity is `libfile_7242750a38d48191aebd586af45e34bc`, version 0.
Do not overwrite it.

The derived candidate is
`zzzz_MM3D_Lost_Woods_FINAL_native_segment_flags_POC18.prelude.o2r`,
121716901 bytes, SHA-256
`4bef56b0b4e3fbe89308c85e29eefbfce19b4f91d21a7ed79fc5c3c519c4a7ef`.
The deterministic ZIP recipe is:

```sh
python3 -B tools/lost_woods_entrance/final_scene_segments.py INPUT --output OUTPUT --receipt RECEIPT
```

Exactly twelve resource bytes change from `00` to `01`: the low flag bit of
zero-offset raw `G_DL` calls to animated segments 8–13. All 123 resource paths
remain; 111 resources are byte-identical, and the other twelve differ in exactly
one byte each. Scene headers, actor/project edits, scrolling parameters,
collision, vertices, textures, UVs and materials retain their original bytes.
Independent complete rebuilds produced the same archive SHA-256.

## Native rendering finding and limits

The packed display-list factory preserves raw words. `Interpreter::SegAddr`
requires address bit zero to resolve a native segment. The original export's
unflagged calls such as `DE000000 0B000000` remain low raw pointers instead of
calling the bound scrolling list. This is a real native compatibility defect.
The current Combo interpreter guards and skips unresolved low targets; other
interpreter paths may not. It therefore cannot by itself establish the cause of
the reported Prelude freeze. Do not describe the candidate as runtime fixed.

The resource audit found 634 resolved references, 52 display lists (14 packed),
33 vertex resources and 23 textures. It found no missing resources, display-list
cycles, truncation/termination errors, vertex/triangle bounds failures or texture
payload size errors. All six texture-animation bindings terminate correctly.
The native interpreter fixture demonstrates skipped original calls and correct
dispatch after the flag repair, for both pushed calls and terminal branches.

## Door behavior

The scene's native transition index 1 is `ACTOR_EN_DOOR`, both sides in room 0,
position `(-680, 0, 72)`, packed yaw `13695` (106 degrees), parameters `0`.
The authoring identity 1007 is not the compiled native transition index.
The integration identifies the unique same-room door in an eligible forest visit
and matches its live actor by transition index and original position. It accepts
the native entry's positive or spawned negative actor ID. Multiple markers fail
closed; ordinary doors keep their native path.

The native Open action directly requests existing shop entrance `0x1200` with a
black fade, so no entrance collision is required. The default parameters would
otherwise turn this into a closed talking door on MM Days 1–3. A scoped exception
keeps this authored door available throughout the cycle without rewriting actor
parameters or archive data. A missing shop pack, inactive session, dead player,
pause or replaced action cancels safely to native idle with the error sound and
does not restart the global door cutscene. Successful entry retains the native
blocking flag and cutscene startup. The old placeholder exterior mesh and its
coordinate trigger are disabled when the final door marker is loaded.

Shop return derives from the loaded door's position and yaw, 226 units outward,
approximately `(-462.75, 0, 9.72)` at the current placement. The retained collision
supports the 40-unit footprint and 100-unit headroom there. Door translation or
rotation follows automatically. Existing forests without this marker keep their
previous combined/solo behavior. The Magic Mirror's reward and recall still
resolve the placed stump cap; the final collision is unchanged at SHA-256
`c00ee0ac86c0829aa981b284d251442ffd4212b6aa802e57d2f8db317529bb85`.

## Verification and runtime handoff

Focused ASan/UBSan native fixtures pass for Open, day/night offer, all three
ordinary cycle days, entry/return, moved/rotated door, blocked cleanup, native
cutscene lifetime, ordinary-door routing, transactions and respawn preservation.
The actual MM BlackMarket C++ translation unit compiles to an object. Complete
native Player and EnDoor C translation units pass C17 syntax checks. OoT's market
fixture and cross-game boot/handoff/fade/portal regressions pass. Fourteen bounded
segment-repair tests pass. The final archive's Mirror cap, production cap locator,
base landing and authored door return support/clearance pass resource checks.
Read-only door review found no remaining material issues after these fixes.

Configured runtime is still required: use one winning final forest scene,
AltAssets enabled, the existing shop interior pack and a source build containing
this door hook. Keep superseded whole forest packs out of the winning load order.
Check the initial load, scrolling surfaces, native Open on Days 1–3, shop return,
Mirror pickup/recall and existing OoT/MM bridge. If it freezes, retain the first
native stack and log from this exact candidate before further edits. A complete
linked executable, game/Prelude rendering, archive priority and user acceptance
are not established by these source/native checks.

Source checkpoints and the independently verified recovery archive identify exact
local/published commits and their common tree. Publication is a recovery point,
not baseline promotion. The recovery package also retains the repair receipt,
resource audit, input identities and exact binary-safe source patch.
