# Cross-scene material-binding reuse source probe

Real upstream: 5e679b0f2e913e79e0d52daf350744a6f8ed03b1.
Enable gEnhancements.PreludeMaterialBindingReuse=1; default0, sampled once per Interpreter::Run. This replaces the earlier Field-only caustics probe and has no scene or filename gate.

Compatible resource-backed single- or two-texture draws can reuse an unchanged live binding after segment/UV animation dirties it. Formats: RGBA16/32, IA4/8/16, I4/8. Water, light sheets and caustics can benefit when their actual draw meets the guard. Native animation timing, interpolation, UV packing, shader selection and sampler updates stay on the existing path.

First tile must be0 and each used tile must map to its expected texture slot. Reuse requires matching live cache key, trusted current-Run resource identity, unchanged full-image address and valid extents. Cache clear/delete/eviction, frame reset, framebuffer binds, palette textures, replacements/masks, shared nodes, rectangles, primitive depth and unsupported layouts retain or force ordinary imports. No new segment-table or resource-lifetime architecture.

Tests compile actual production cache/import/invalidation methods, texture/UV loops, tile setters, framebuffer paths and Run sampling. Engine resource/GPU API boundaries are controlled fixtures; this is not a full game build.
Run: python3 tests/caustic_binding/native_regressions.py --sanitize

Native tests went red on the original loop and on the Field-only guard before broadening. Final native and ASan/UBSan checks passed, including arbitrary-scene two-texture water and single-slot I/IA light-sheet cases. Root independently reran the sanitizer fixture.

Runtime FPS, visual fidelity and full Windows compilation remain unverified. Compare the same scene archive/build/settings with the option0/1 in Field, Lost Woods and other water/light-sheet scenes; check vanilla/Alt toggling, re-entry and pause/unpause. This is a performance hypothesis, not a proven explanation for every caustics slowdown.
