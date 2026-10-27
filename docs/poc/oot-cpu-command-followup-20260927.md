# CPU command follow-up to PR #20

Baseline: `d9cdb052c1e6d4692ef40aadf3f6c8947c802e96`, the PR #20 build cor tested with repaired Din, the Twilight Princess enemy pack enabled, and RenderFlight off. Reported FPS: Hyrule Field 20–30; Lost Woods Room 9 tunnel 30–35, Room 10 20–25; Meadow 20–35; Kakariko about 20. This is a useful runtime baseline, not accepted final performance or SoH parity.

The new capture identifies substantial CPU triangle/vertex-command work, including roughly 135,000 commands in Hyrule Field and 195,000 in Lost Woods Room 9. This candidate reduces repeated CPU work without changing scene content.

## Changes and boundaries

- Pass the executing interpreter directly to all graphics opcode handlers. The dispatcher already owns this context; reacquiring its global weak pointer for every command is unnecessary. External helpers retain their existing instance lookup.
- Reuse a loaded vertex's packed attributes within a pure triangle run when the shader has multiple inputs or two textures. Repeated indices append the exact bytes already emitted to the current submission buffer.
- Store only an offset and length for each of 64 indices. Every flush invalidates those offsets before the buffer is reused. The existing non-triangle, microcode and frame barriers invalidate prepared state; rebuilding that state resets the vertex mask.
- Preserve per-triangle clipping, culling and draw order. Bypass packed reuse for rectangles, shared sampler nodes and `G_TL_LOD`, whose attributes can depend on the first vertex of the triangle.
- Cheap shaders use a separate direct-emission specialization. This avoids the regression found when packed reuse was applied indiscriminately.
- Packed reuse follows the existing OoT `gDeveloperTools.TriangleStateReuse` opt-in, throughout OoT rather than by scene ID. MM retains direct vertex emission. Direct interpreter dispatch applies to both games with equivalent handler operations.

Protected: geometry, collision, actors and their behavior, repaired Din, textures/UVs, Alt ownership, animated material/segment bindings, weather, audio, interpolation and frame scheduling. Resource-manager cache policy and resource lifetimes are unchanged. A vertex-transform specialization trial was discarded after its small measured benefit did not justify the complexity; `GfxSpVertex` remains identical to the baseline.

## Verification

The production-code fixture `scripts/diagnostics/run_gfx_triangle_run_tests.py` checks:

- Commands affect the executing interpreter even when another interpreter is globally selected. The baseline failed this assertion before the dispatcher change.
- Packed indices are retained within an eligible run. This assertion failed before packed reuse was implemented.
- Shared/reordered indices produce identical attributes; changed vertices are repacked after a barrier.
- Index 63 and repeated indices, temporary rectangle indices 64–67, and explicit flush followed by reordered triangles without a state reset.
- Existing 16,384 combinations of material, texture, fog, grayscale, clipping, culling and backend clip conventions produce bitwise-identical vertices and identical backend events with reuse on/off.
- All 16 output digest blocks also match the untouched PR #20 build.
- Buffer rollover, frame changes, temporary rectangle state, shared samplers, and triangle-dependent LOD retain their prior regression checks.

The fixture passes with ASan and UBSan. Local LeakSanitizer is disabled because this execution environment cannot inspect processes; CI keeps its normal sanitizer setup. Texture-address and display-list-address regressions pass with ASan/UBSan. Both games' Alt-segment binding/dirty-reload/manager-owner switching regressions pass with UBSan. These checks run in the existing CI gate; full Windows/Linux compilation is a separate gate.

Independent read-only review identified a missed external shader helper during the mechanical dispatch conversion; its original lookup was restored. Review also prompted the index-63 and explicit-flush tests.

CI exposed remaining Epona fixture calls and the configure-time custom texture-scroll patch that still used the old handler signature. The Epona and standalone strict-texture fixtures now pass the executing interpreter explicitly. The scroll patch and its typed runtime fixture use the same interface; clean patch application, independent clocks, interpolation, signed wrapping and long-running clock checks pass. Patch applicability and the scroll runtime fixture now run in the shared-renderer CI gate. The Epona cosmetics suite also passes locally.

## Synthetic CPU measurements

GCC 13.3, `-O3`, diagnostics off, recording backend with output recording off. Compare against the untouched PR #20 source using `--baseline-ref d9cdb052c1e6d4692ef40aadf3f6c8947c802e96`; run parent and candidate sequentially. These are production-function fixtures, not captured scene replays, driver/GPU timings or predictions of game FPS.

The triangle fixture emits 122,880 triangles in 7,680 runs of 16. It slides adjacent indices so successive triangles share two vertices. Medians of ten measured iterations after warm-up:

| Material case | PR #20 reuse on, ms | Candidate reuse on, ms |
|---|---:|---:|
| 0 | 2.65 | 2.75 |
| 1 | 3.62 | 3.71 |
| 3 | 8.09 | 5.94 |
| 15 | 3.94 | 3.82 |
| 127 | 8.20 | 6.09 |

The complex cases improve approximately 26–27%. Cheap cases range from about 3% faster to 4% slower, showing compiler/layout sensitivity; do not present only the favorable cases.

The command fixture alternates real depth/key state handlers with triangle commands. Its repeated-four-index case is favorable to packed reuse; its disjoint-index case controls for that. Medians of fifteen measured iterations after warm-up:

| Commands / triangles | Index pattern | PR #20, ms | Candidate, ms |
|---|---|---:|---:|
| 138,240 / 122,880 | Disjoint | 8.83 | 9.06 |
| 138,240 / 122,880 | Repeated four indices | 9.18 | 4.79 |
| 193,750 / 75,000 | Disjoint | 6.27 | 6.00 |
| 193,750 / 75,000 | Repeated four indices | 6.31 | 3.40 |

The disjoint control ranges from roughly 3% slower to 4% faster. The repeated-index cases improve 46–48%, but use cheap state-setting commands rather than actual resource loading or vertex transformation. These percentages cannot be applied to whole game frames.

## Runtime handoff and recovery

Hyrule Field remains the primary benchmark; Lost Woods Rooms 9/10, Meadow and Kakariko are the reported problem areas. Keep the same repaired model, full mod stack and settings, with RenderFlight off. The candidate is enabled through the existing default-on triangle reuse setting. User testing must establish actual FPS and visual stability, especially animated/scrolling materials, transitions and re-entry. No additional diagnostics-only build is required.

The prior PR #20 build remains available as recovery. Publication/build success does not imply final runtime acceptance, merging, master promotion, or SoH performance parity.
