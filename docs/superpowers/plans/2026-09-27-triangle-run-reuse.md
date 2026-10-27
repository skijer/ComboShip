# Triangle run reuse implementation plan

> **For agentic workers:** Use superpowers:executing-plans for inline implementation.

**Goal:** Reduce OoT CPU triangle submission work while preserving the submitted graphics output.

**Architecture:** Cache prepared shader/texture state only during consecutive pure triangle commands. Every other command invalidates the cache, including vertex loads, texture changes, rectangles, display-list transitions, and microcode changes. OoT opts in; MM remains on the existing path. Each frame starts fresh.

**Tech Stack:** C++20 interpreter, Python standalone test runner, existing GitHub build gates.

**Spec:** The design in this document implements cor's request for a concrete performance PR following diagnostic PR #19.

## Global constraints

- Baseline: `3d72b13ad6fae22369a5738777f7e5010efecbad`, PR #19 head; isolated candidate branch.
- Hyrule Field is the primary runtime benchmark; Meadow is a comparison scene.
- Preserve repaired Din, mods, geometry, actors, textures, weather, animation and frame scheduling.
- Apply to all OoT scenes; no scene-ID filter. No new diagnostic-only build.
- Local synthetic CPU results do not prove Windows game FPS or visual acceptance.

## Review focus

- Invalidation on all non-triangle commands and frame boundaries.
- Rectangles/S2DEX retain their existing path and temporary-state semantics.
- Shader/texture switches retain draw ordering and flush boundaries.
- Cull/clip rejection and buffer rollover preserve triangle counts and vertex bytes.
- Both graphics API clip conventions, dual textures, fog, grayscale, clamp/mirror and LOD-dependent inputs.

## Task 1: Triangle state reuse

**Files:** `libultraship/include/fast/interpreter.h`, `libultraship/src/fast/interpreter.cpp`, `soh/soh/OTRGlobals.cpp`, `libultraship/tests/gfx_triangle_run_test.cpp`, `scripts/diagnostics/run_gfx_triangle_run_tests.py`.

- [x] Compile the real production triangle and combiner functions in a recording-backend fixture. Establish bitwise output equivalence and draw-event equivalence; assert one shader-info query per unchanged run rather than one per triangle. Observe the baseline fail the reuse assertion.
- [x] Add prepared triangle state and conservative command-scoped reuse. Retain the original calculation order and per-triangle clipping/culling.
- [x] Wire the OoT opt-in flag, reset at the end of OoT rendering, and preserve an off switch for matched runtime comparison.
- [x] Run deterministic and varied-state fixtures, sanitizers, and the existing renderer address/Alt tests.
- [x] Benchmark optimized production functions with tracing disabled, with and without reuse; document synthetic workload and limits.

## Task 2: Review and publish

- [x] Add the focused regression to the existing CI gate.
- [x] Review the full diff, invalidation and lifetimes; fix findings.
- [ ] Publish a draft performance PR stacked on PR #19, with measured local results and pending full-build/runtime checks stated explicitly.

## Execution record

- Baseline address regressions passed before edits; the unchanged-run layout-query assertion failed on the baseline and passed after reuse.
- Shared sampler invalidation was verified by removing the guard in the generated test source: backend event equivalence failed. Production source was retained.
- Separate template specializations keep cached-state storage out of the disabled path. Explicit local working variables preserve the original computation structure.
- Independent review completed; its skipped-command barrier and LOD coverage points were addressed.
- Runtime FPS and full application builds remain unproven until their respective gates run. Publication was already authorized by the user.
