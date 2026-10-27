# Slate ring clearance review

Historical camera-facing clearance experiment. The current effect behavior is described in [Slate rings that turn with the model](slate-rings-attached.md).

The user raised a concern about a half-circle visible during an earlier rotating preview. The original animation was unavailable, but a new depth-tested render reproduced partial and disappearing rings in the current GI implementation.

The two effect circles are complete meshes. Their camera-facing plane was anchored at a fixed model-local depth of 4 native units. The raised eye extends to approximately 5.04, and rotation moves the casing through that plane. Opaque depth therefore hid parts of the circles.

The fix places the ring plane ahead of the projected bounds of the approved Slate, with a 0.75 native-unit clearance. Its position in the camera plane, two native-unit radii, colors, and animation are retained. Depth testing remains enabled. Perspective can change apparent size when depth changes; final appearance still needs an in-game check.

[Before and after rotation](slate_rotation_comparison.gif)

[Selected front/back views](slate_rotation_comparison.png)

These previews render the exact exported GLB triangles plus the shared production C++ halo sampler, including 1/16-unit effect vertex packing, camera basis updated per turn, opaque depth, and alpha blending without effect depth writes. Optional shimmer is disabled to show the rings clearly. Mesa lighting/camera are offline approximations; these are not captured gameplay.

Verification: `python3 scripts/diagnostics/run_nei_gi_tests.py --combo` passes. A new test failed before the fix, then passed against all six variants using bounds derived from the serialized GI vertices/resource matrices: 24 yaw angles, five pitches, and three effect phases. Existing common/shop/acquisition, base/Alt, effects on/off, owner descriptors, and the real MM host draw checks also pass.

Regenerate current rotation:

```sh
source /workspace/cloud-setup/activate.sh
/opt/codex/runtimes/codex-primary-runtime/dependencies/python/bin/python3 tools/nei_gi/runtime_preview/render_slate.py tools/nei_gi/PREVIEWS --stem slate_rotation_after
```

The original preview branch published review media and notes. The implementation and regression changes are now included on `slate-element-preview`.
