# Renderer Cost Probe Implementation Plan

**Goal:** Identify the work responsible for OoT Meadow's 43–51 ms graphics-command frames without altering assets or rendering semantics.

**Architecture:** Extend the existing isolated diagnostic build. Sample one of every 31 presented OoT frames, measuring command handlers, named display lists, texture imports/uploads, shader creation, and driver draw submissions. The prime cadence rotates across common interpolation phases; each report includes game tick, interpolation index and fraction. Keep normal stage timings and emit structured results automatically through the existing independent logger.

**Tech stack:** C++20, the current shared libultraship, the existing JSON/spdlog diagnostic adapter, deterministic C++ tests and Windows Release CI.

## Constraints and evidence

- Parent: `6505ac900bff85d9bf44f1a63e154bdd4ebecbd4`; separate `poc/oot-render-cost-20260926` branch in the existing isolated worktree.
- The OpenGL/Direct3D backends and core vertex/triangle/matrix functions match SoH's pinned `c57da1b4afa775b24b58b2adf93d63d3b561bb65` engine. The first asset compaction did not resolve Meadow.
- Preserve game/asset behavior, accepted scenes, texture identities, interpolation, resource lifetimes and cache policy. No speculative cache fix, geometry reduction or master promotion.
- CPU elapsed scopes include driver waits and nested work. They are not GPU timestamps. Label overlaps and sampled-frame overhead explicitly.
- Restrict detailed profiling to OoT requests; disable it after the OoT render loop so the shared MM renderer does not inherit profiling.

## Implementation and verification

1. Add a bounded recorder in `libultraship/include/fast/RenderCostProbe.h` and standalone tests. Test disabled/unsampled frames, sampling cadence, nested owner restoration and branches, bounded attribution, counters, reset and deterministic elapsed accounting.
2. Add optional instrumentation in `interpreter.h/.cpp`: command time/count keyed by microcode/opcode, marker-based display-list attribution, texture import/cache/upload data, shader creation, draw calls/triangles and framebuffer setup/finish. Unmarked lists remain explicitly unattributed; timings exclude attribution lookup where possible.
3. Extend `FrameTimingProbe`'s C++ adapter and `OTRGlobals.cpp` to request profiling, log backend/render dimensions/MSAA and the sampled results under current scene/room/age/Alt tags. Verify actual async output with ordinary logging disabled, and add the new tests to the existing CI diagnostic gate.
4. Run deterministic tests, compile touched renderer code with available headers, review the diff for unintended rendering changes, and build Windows Release on the POC branch. Keep runtime status untested until the user supplies the resulting capture.

## Review focus

- Shared-engine game transitions must not leave profiling enabled for MM.
- Branches and returns must restore the correct named-list owner; missing markers must not inherit a false name.
- Driver/upload timings overlap command-handler timings and must not be summed as independent phases.
- Resource/command attribution is bounded and must not retain live game resources or change lookup/cache results.
- The log must report enough configuration and counters to distinguish geometry work, repeated texture rebuilds and expensive driver calls in one focused capture.

## Decisive runtime capture

Use the existing accepted stack and textured Din model. Spend about 30 seconds in the bad Meadow view, then 15 seconds in Temple of Time. The build logs automatically regardless of the normal logging level. Compare named lists, handler time, upload bytes/cache misses and driver elapsed time before selecting a fix.

## Verification and delivery status

- Six existing timing regressions, C bridge compilation, recorder attribution/sampling/bounds tests, and the production Context async output fixture pass locally with normal logging Off/Warn, teardown and restart.
- The complete modified renderer source passes a C++20 syntax compilation with COMBO_BUILD, dependency headers and the repository's CVAR/ImTextureID definitions. Modified game files pass clang-format-14; the diff has no whitespace errors.
- Independent read-only review found a fixed 30-frame cadence could miss two of three interpolation phases. A regression reproduced that failure; a prime 31-frame cadence now passes it, and the output identifies each sampled phase. No rendering-order, cache-policy, ownership or shared-MM enablement blocker remained in the review.
- The parent includes the proven Windows Context logger correction at 6505ac90. The existing separate-DLL CI output check now also validates renderer report fields and exports the new entry point.
- Windows compilation and runtime performance are not yet verified. The user explicitly approved publishing this isolated diagnostic branch and opening a draft PR on 2026-09-26. Windows CI must pass, including the separate-DLL output check, before delivering a package.
- This is a diagnostic, not a demonstrated performance improvement. Driver/GPU behavior, actual sampling overhead, visual acceptance and the Meadow root cause require the runtime capture above.
