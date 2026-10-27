# Slate rings that turn with the model

The previous clearance experiment kept the circles facing the camera. That made them look like a stationary overlay while the GI rotated.

The revised rings are front-mounted sigils in the Slate's local XY plane. Their native plane is Z=5.79, just ahead of the raised eye at approximately Z=5.04. Camera-facing ribbon skirts retain enough clearance after vertex packing. The GI matrix now rotates the ring paths with the tablet.

At side angles, the circles narrow into ellipses and a thin edge profile. From the back, the solid casing hides these front-face sigils. This occlusion is expected for their placement.

[Camera-facing versus model-attached animation](slate_rotation_attached_comparison.gif)

[Front/back comparison](slate_rotation_attached_comparison_front_back.png)

[Eight-angle diagnostic](slate_rotation_attached_contact.png)

These use the exact exported GI mesh and shared production C++ effect sampler, including 1/16-unit effect vertex packing, opaque depth testing and alpha blending. Shimmer is disabled to isolate the ring motion. This is an offline Mesa render; in-game appearance still needs verification.

`python3 scripts/diagnostics/run_nei_gi_tests.py --combo` passes: all six variants have camera-independent model-local ring centerlines, and every packed effect vertex clears the actual exported front relief across rotation and tilt. Existing owner descriptors, MM host draws, toggle states, and all 64 authored bindings also pass. The attachment test failed on the previous camera-facing implementation before this change.

The original preview branch published media and notes. The implementation and regression changes are now included on `slate-element-preview`.
