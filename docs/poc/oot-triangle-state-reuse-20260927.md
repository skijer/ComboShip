# OoT triangle state reuse POC

Base: PR #19 head `3d72b13ad6fae22369a5738777f7e5010efecbad`.

Hyrule Field's capture identifies substantial CPU triangle-submission work at roughly 118,000 triangles per rendered frame. This candidate removes repeated shader selection, texture-size calculation, sampler checks and shader-layout queries within consecutive pure triangle commands. It does not cache transformed vertices, change geometry, remove actors, alter assets, or change interpolation/scheduling.

## Scope and invalidation

- Enabled by default for every OoT scene via `gDeveloperTools.TriangleStateReuse`; no scene-ID filter.
- The engine defaults to disabled, and OoT clears its opt-in after rendering. MM uses the disabled specialization.
- Only the seven native/OTR triangle/quad handlers can reuse preparation. Every other dispatched command invalidates before and after execution. Vertex loads, rectangles, framebuffer changes, texture changes, nested display lists, unknown/rejected commands, microcode loads and frame resets end reuse.
- Rectangles and S2DEX retain their original temporary-state path. Clipping, culling and vertex emission still execute for every triangle.
- Shared texture-cache nodes bypass reuse to preserve sampler transitions and flushes.
- The combiner lookup key is zero initialized, including its previously uninitialized unused `shader_id` member.
- `FrameTimingProbe` now defaults to **0**. Explicit saved settings still apply. Disabling diagnostics is not included in the reported optimization savings.

## Verification

`python3 scripts/diagnostics/run_gfx_triangle_run_tests.py` compiles the production triangle/combiner functions, header and dispatch class against a recording graphics backend. It checks:

- One shader-layout query for an unchanged 32-triangle run.
- 16,384 material, texture, fog, grayscale, wrap, clamp, clipping and culling variants: bitwise identical vertex output and identical backend state/draw events with reuse on and off.
- Buffer rollover, temporary rectangle state, explicit frame resets and shared sampler nodes.
- Hand-derived position/depth/color output and triangle-dependent LOD values with reordered shared vertex indices.

All 16 digest blocks of vertex output also match the untouched parent commit, compiled separately. The shader/texture import boundary is simulated; these are not screenshots or a real GPU replay.

AddressSanitizer and UndefinedBehaviorSanitizer pass. Local LeakSanitizer cannot inspect processes in this environment, so local ASan runs use `ASAN_OPTIONS=detect_leaks=0`; CI uses its normal sanitizer environment. Existing texture-address, display-list-address, and both OoT/MM Alt-segment binding tests pass.

Independent review found no critical or important production defect. The skipped-command invalidation and LOD coverage points raised during review were addressed. Full Windows/Linux builds and user runtime acceptance remain separate gates.

## CPU benchmark

Run `python3 scripts/diagnostics/run_gfx_triangle_run_tests.py --benchmark` and repeat with `--baseline-ref 3d72b13ad6fae22369a5738777f7e5010efecbad`.

The synthetic workload submits 122,880 triangles in 7,680 runs of 16 triangles for each of five materials. It uses GCC 13.3, `-O3`, one warm-up and ten measured iterations per mode, alternating on/off order. Recording and render profiling are disabled. The reported time is the median. It excludes resource loading, actor work, opcode dispatch, real driver calls and GPU work, and is not a captured Hyrule Field command replay or FPS prediction.

Final timing values and the disabled-path sensitivity are recorded in the PR description. The five cases were approximately 20–34% faster than the untouched parent in the final sequential run. Compare against the untouched parent as well as the candidate's disabled path; compiler layout and shared-host timing can affect the measurements.

## Runtime acceptance

Primary benchmark: **Hyrule Field**. Meadow is the comparison scene. Keep repaired Din, mod archives/order, save, camera, resolution and settings fixed. Diagnostics remain off. The optimization defaults on; an in-place comparison can use:

```
set gDeveloperTools.TriangleStateReuse 0
set gDeveloperTools.TriangleStateReuse 1
```

Acceptance requires improved steady Hyrule Field performance without rendering regressions. No SoH parity, Windows FPS gain, or master promotion is claimed by the local checks. This is a draft performance candidate stacked on the existing diagnostic PR, not a promotion of that baseline.
