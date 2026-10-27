# Mirror stump scene input

`LostWoods-MirrorStump.prelude.o2r` is cor's unchanged original editable
`1 - lost woods 2ship granny shop and stump moved.prelude.o2r`, retained under a
portable filename. Its Prelude project includes the stump's visual vertex edits
and the matching collision, alongside the Granny shop work from that export.
The complete original bytes and their SHA-256 are preserved.

The current complete candidate is
`zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r`, identified
in `manifest.json`. It is derived from cor's final export with the bench, shop
door and scrolling. The original final archive remains unchanged. The bounded
recipe in `tools/lost_woods_entrance/final_scene_segments.py` fixes exactly twelve
native scroll-call address flags. `tools/black_market/poster_texture.py` then
replaces only the wanted-poster texture with the user-selected GitHub-avatar
design. Every other resource remains byte-identical to the segment-fixed input.

Use this complete candidate for combined-mode checks. Do not layer the older
editable source or POC14 over it. Their whole scene/collision would replace later
work. The manifest retains both older identities for recovery and distinguishes
the final input's file identity from the newly derived candidate. See
`docs/poc/final-forest-solo-scroll-20261019.md` for both native solo adapters,
poster replacement, the door route and remaining runtime proof.

The source edit is for recovery/editing against its original forest donor. Future
Prelude edits should begin with the latest complete scene and export visual and
collision changes together. Keep one winning forest scene in the MM mod load
order. Solo modes now use the FINAL-avatar POC4 private forest addon in `manifest.json`; recall resolves the
stump in whichever forest collision is actually loaded in that mode.

The current cap is centered at `(-377, 80, 416)`; its clear base landing is
`(-377, 0, 296)`, facing the stump. These are verification snapshots, not hardcoded
runtime destinations. Both the pickup and recall use the same loaded 12-triangle
stump cap. Translation/rotation preserve that identity; resizing or duplicating
the stump requires revisiting the locator. Missing/ambiguous geometry or no clear
supported landing retains the scene's ordinary entrance spawn.

```sh
python3 -B tests/magic_mirror/check_scene.py
python3 -B tests/magic_mirror/check_scene.py --compiled /absolute/path/to/zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r
```

These checks compare visual and collision placement, execute the production cap
locator on all loaded horizontal faces, and inspect static collision support and
clearance at the current landing. They do not establish configured game rendering,
archive priority, player/camera behavior, or user acceptance.
