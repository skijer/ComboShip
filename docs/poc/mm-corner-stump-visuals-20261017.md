# MM opening Lost Woods: corner stump correction

Placement superseded by `mm-room-edge-mushroom-seams-20261017.md` (POC5): the
next annotated screenshot identified the large room-edge trunk rather than the
small cut stump used here. The accepted stump-top UV repair is preserved.

## Contract

The user's three screenshots show a segmented cut-wood surface, mushroom atlas
wedges, and inverted grass cards. Their clarification rejects the previous
tree at (120, 0, 240): the target is the cut stump in the corner, and the
abandoned doorway in the middle of the room must disappear. Preserve the
native onward route, scene progression, weather, seasons, and camera safeguards.

Baseline source is the isolated Windows/integration checkpoint
`43131342882972eeb4a257599ec8b061c7dc6d43`; asset baseline is
`zzzz_MM3D_Lost_Woods_Stump_Return_POC3.o2r`, SHA256
`6f120f52340da1874d5a650235bc57154cc62d5a8a44a5429a056ed8eb3f6284`.
The supplied original Lost Woods model is SHA256
`c60a4470e8201b77ee8e8a8265a83090bcb9f3f4653c2787bd9704ca1b9b3dc1`.

## Findings and implementation scope

The original model's cut stump is Geometry4, centered near (-520, 80, 120),
with footprint X -575..-465 and Z 64..176. The old candidate's doorway was
at a different tree. Put the existing doorway against the stump's east face:
anchor (-390, 0, 120), yaw 16384. Its black mouth is at X -414..-415, ahead of
the stump's protruding root at X -417; its root shell reaches back to X -485.
The first preview at X -430 exposed that root inside the mouth and was rejected.
Move all five doorway vertex
resources together. Change the four active room-zero mesh identities to v3;
keep v1/v2 runtime compatibility tied to their own visible positions.

The imported model encodes negative V in image-row order. The previous port
inverted it again. Correct T on the 22 imported scenery vertex resources;
preserve XYZ, S, vertex colors, textures and all display-list topology. The
authored doorway resources use a separate UV convention and are excluded.

POC3 contains only a static purple gradient. Add a narrow purple rim and
moving glints at the active v3 opening, with frame-owned vertex allocation.
Keep the older packs' draw selection intact. The original effect reference is
not available here; this candidate does not claim exact visual parity.

## Evidence and status

The final archive is `zzzz_MM3D_Lost_Woods_Corner_Visuals_POC4a.o2r`, SHA256
`8acc5368601ab6efdecdc419acf966d66d7fa62dcf2c5a0c9e53bd5b73de9240`.
It changes exactly 31 resources and adds one wrapper. All 12 textures and all
other resources are byte-identical to POC3. Header bytes outside the mesh token
are identical; scenery XYZ/S/RGBA and doorway Y/UV/RGBA are unchanged. Active
opaque/translucent graph traversal reaches each of the five doorway meshes
exactly once and never reaches an abandoned middle-room instance.

`visual_pack_test.py` compares all 22 scenery resources and 14,234 packed
vertices with the original DAE; it also checks all setup bindings and confirms
the native stump roots remain behind the mouth. The unchanged pack failed the
UV/location tests first; the rejected first placement also failed root clearance.

The native return fixture, finish geometry/submission guards, and full portal
camera fixture pass with ASan/UBSan. Camera coverage includes native/v1/v2/v3,
both forest routes, every form/day, projection, collision-service guards,
release, rollback and lifetime. The actual passage renderer compiles and requests
the native C interpolation ABI. Source/candidate/receipt alias and existing-file
checks pass. A Windows receipt-cleanup constraint was reproduced and fixed so a
failed repair closes its stream before deletion and preserves the original error.

Independent review found no critical/important implementation defect and
independently checked UVs, archive hashes/deltas, rigid transforms and draw guards.
Its cleanup and documentation findings were addressed before delivery.

The broader `tests/lost_woods_entrance/run_tests.py` is blocked before execution
by missing `SDL2/SDL.h`, reproduced in the unmodified source baseline. No full
linked build or configured game traversal was performed. Previews show actual
packed geometry/materials and exported native finish geometry under neutral
lighting; they are asset renders, not game captures or collision proof.

Use this archive only with the matching v3 source in the next build; replace the
earlier Lost Woods mod. Keep the established seasonal/weather settings and mod
order. Decisive game checks: identify the cut corner stump, verify no middle
doorway remains, enter/return, inspect grass/stump/mushrooms and purple finish,
then re-enter and pause/unpause. Test the native/Alt paths and relevant forms.
The source and archive remain candidates; no master promotion or runtime
acceptance follows from these checks.
