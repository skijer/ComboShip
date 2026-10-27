# Optional Black Market room materials

Baseline: `2d4e08a72eb8f9ddbc8d87ba871b05deefdf2add`, the saved close OoT-camera candidate. Camera, browsing and item sizing were accepted from the preview; configured gameplay remains untested. The prior source recovery archive remains unchanged.

User request: restore the first iteration's magical room materials as a separate checkbox with cosmetic controls independent of shelves; use the proven compressed source upload workflow. Keep publication and the combined build on hold for other sources.

Implementation choices: off by default; one shared appearance checkbox, four per-host World cosmetic colors (violet, teal, rose, lavender) using the original palette. On uses a private material variant of the current room; off restores the current static materials. This is a material mode, not a second overlapping room draw. Current vertices, corrected UVs, collision, camera, item sizes and shelf materials stay intact. Original Granny mode takes precedence while active. No extra scroll-speed or opacity settings are needed.

Original texture: recover only `custom/skiejer_market/texture/arcane_stone` from the retained POC14 archive SHA256 `3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`. Give the room its own texture path. Preserve the existing shelf texture bytes. The recovered pixel SHA256 exactly matches the existing original shelf-art source stream (`3e2bac601d1a9c3e0317d159aa6b9f5354abaac982a24848edfec5e97dbb3231`). Reuse those verified zlib/base64 parts (at most 128 KiB text each) without duplicating uploads; reconstruct an independent room texture through normal source preparation for both hosts. The source manifest retains part, aggregate and pixel hashes.

Alternative considered: an additive magical overlay would preserve visible static texture art while on, but would be a new effect rather than the requested original material appearance and risks duplicate draw/depth behavior. Private material variants give exact restoration when off and no geometry changes.

Validation: decoded asset identity and malformed-part rejection; original resource hashes unchanged; all material substitutions stay private; MM variants retain its approved room-fit vertices; native dual-host enable/disable, tint separation, Granny precedence, teardown/fallback behavior; relevant preparation and camera/Granny regression checks. Native compilation and preview evidence do not establish configured game acceptance.

Implementation commits: `3ad3be88` (private asset variants), `8e2506a3` (build dependencies), `8ac9f363` (runtime and UI), `ad1b9f4b` (native draw-exit cleanup and registered renderer ownership). Independent asset review and final scoped correction review both pass, with no remaining findings. Source and bounded native checks are verified; configured gameplay and visual acceptance remain untested. No build publication or promotion while the source hold is active.

## Controls and behavior

- `Magical Room Materials` in each host's existing Black Market appearance controls under Graphics; off by default, applies immediately, persists across sessions.
- Cosmetic editor > World: Black Market Room Violet, Teal, Rose and Lavender. These use the standard per-host color/rainbow/reset controls; shelf color remains separate. Alpha stays opaque.
- Current material/UV appearance is restored when off. Active Regular Granny Interior takes precedence. Missing or incompatible optional resources retain the current room.
- The selected graph is attached only during the native room draw, and its original shape pointer is restored afterward. No extra overlapping room draw, actor update, camera change or collision change is introduced.

## Evidence and limits

- Six asset tests pass, including exact source pixels, unchanged existing resources and rejection of corrupt/reordered compressed source parts. Six existing MM room-presentation tests and five MM scene preparation tests pass.
- Native ASan/UBSan checks pass for graph ownership, palette independence, reset/rainbow reads, both host attachments/restoration, scene transitions, Granny precedence, resource failures and teardown. Normal/Alt and registered solo-host resource ownership are covered through native boundary fixtures; absent named renderer routes fail closed.
- Both generated graphs load through the actual native DisplayListFactory and VertexFactory: 7919 unchanged triangles, 342 vertex loads, 30 original scroll invocations. All source instructions and vertex references remain intact.
- Both complete host controllers, both cosmetic editor translation units and the shared appearance UI compile to objects. Existing native Granny/shelf regressions pass in both hosts. Eleven protected camera/layout/geometry/shelf/receipt files remain byte-identical to the camera parent.
- Review found that native pause/transition/capture branches can bypass general draw-end hooks. Private cleanup now runs at each host's common draw-exit label before tail camera updates, without moving or repeating the general hooks. Tests project the actual native draw control flow and exhaust 64 predicate combinations per host, including 4 MM and 26 OoT end-hook bypass paths. Both complete native `z_play.c` units and controllers compile after this correction. The new control-flow test requires pycparser; ordinary asset preparation remains standard-library-only.
- An approximate offline comparison uses the accepted native camera trace and current source assets. It is not a gameplay capture or visual acceptance.
- Python asset preparation succeeds through the normal entrypoint. The CMake executable is unavailable locally; full linking, configured gameplay/GPU rendering and installed-pack/Alt parity are untested. Combined build/publication remains on hold.

Later configured acceptance: toggle within the entered shop, change/reset/rainbow room colors while confirming shelf independence, check Granny precedence, leave/re-enter, and check the user's installed packs and Alt setting. Preserve the prior camera checkpoint and this optional-material candidate independently until that acceptance.
