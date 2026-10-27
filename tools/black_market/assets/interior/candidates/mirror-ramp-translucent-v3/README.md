# Translucent Magic Mirror ramp candidate v3

This replaces the rejected opaque ramp candidate. The ramp now belongs to each native room's translucent draw stream. Its native XLU blender tests depth and blends with existing pixels without writing depth; uniform primitive alpha112/255 makes the surface visibly see-through. Back-face culling prevents the closed mesh from stacking its rear faces over the front surface. Purple RGB,32x64 I8 mask framing, tile1/segment08 scrolling, exact mesh/UVs, collision and Mirror access stay intact.

The default source correction is `task-13.patch`, delivered in the coordinator's one combined code patch. It modifies the forest presentation recipe, its derived output hashes and a focused test. It uses the existing public baseline mask and requires no additional texture pack. All immutable forest split parts and original resource hashes remain unchanged.

`Black-Market-Mirror-Ramp-Translucent-v3-Candidate.o2r` additionally supplies the smoother private mask already authored for task9. It is standalone against commit4fff75754ef1578e586312fbf8c68e3adb99b4df and includes exactly6 resources: the private I8 mask, ramp material, ramp root and combined/MM/OoT room0 headers. Those headers differ from baseline only by moving the existing ramp root pointer from the opaque field to the translucent field. No scene header, collision, vertices, other room entry, bounds, wall/bench mask, portal or Mirror behavior is replaced.

Do not mount the old opaque task9 ramp archive with this one: its material would undo the requested transparency. Mount this single revised ramp candidate through the active host's normal mod convention and restart before comparing. Remove it and restart to revert the optional overlay. Baseline forest resources remain required; competing room/material overrides may shadow this candidate. No archive was installed or game executed here.

Rebuild with standard Python only:

```
python3 -S build_overlay.py --output Black-Market-Mirror-Ramp-Translucent-v3-Candidate.o2r
python3 -S test_overlay.py
```

`overlay.json` pins all resource/pixel/native-header/source-part/archive identities. The builder checks bounded exact decompression, a single part below96KiB binary/128KiB base64, exact6resource inventory, native XLU/alpha/depth-state tokens, the unchanged geometry command prefix, both native room pointer fields, scroll binding and deterministic ZIP metadata. It rejects OPA/full-alpha/wrong-scroll/restore/owner/geometry/shared-mask regressions. Its additive source patch changes no existing file or default asset parts; it only creates this optional standalone builder directory.

The comparison PNG is an offline CPU target preview, not gameplay. It uses the exact source ramp geometry/UVs and the unchanged private purple mask, demonstrating the112/255 source-over target over an explicitly diagnostic grid. The grid is not forest terrain. Native filtering, sorted XLU interactions with the forest, scroll motion, both-host GPU appearance, camera transitions and walking/Mirror interaction remain runtime checks. The old task9 archive remains preserved separately as a rejected visual candidate, not a new default.
