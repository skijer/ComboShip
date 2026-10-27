# Resource and vertex-stage follow-up

Baseline: accepted PR #21 commit `9c084d74a7f7d0ae8cf88b18c0ed74abde03a950`, Windows Build Artifacts #51. Cor reported steady Hyrule Field around 30 FPS, Meadow/other Lost Woods areas 35–45, Kakariko 40–45 with location-dependent variation, Room 10 around 30 and Room 9 tunnel 30–35. This is the performance baseline for this candidate; preserve its branch and artifacts for recovery.

The user requested one combined resource/segment and vertex/lighting pass before the later SoH integration port. Hyrule Field remains the primary runtime benchmark. The SoH port is a separate integration step after evaluating this candidate.

## Changes and preservation

- A warm synchronous `ResourceManager::LoadResource` returns its existing cached resource directly instead of allocating an already-completed promise/future. Both games' segment-binding helpers use this path, including exact Alt preloads. Cold/dirty requests retain the existing asynchronous loader.
- Cached resource references move from owned temporary variants instead of undergoing redundant atomic retain/release operations. This also reduces overhead in direct `LoadResourceProcess` calls.
- Within one vertex command, adjacent identical normals reuse the preceding vertex's directional-lighting RGB and, when enabled, generated UVs. Positional lighting and ambient-only plain lighting bypass reuse. Every new command starts fresh.

No additional resource or segment-pointer cache is introduced. Cache selection, cold Alt preloading, owner/archive-parent scope, dirty/unload/external-replacement behavior and cross-game routing retain their existing policies. The existing prefix/initData behavior is preserved, including the legacy omission of initData after stripping an OTR prefix.

Position/matrix transforms, per-position lighting, fog, alpha, clipping, geometry and animations keep their original calculations. Vertex-stage savings come from repeated directional lighting/texgen, not a new transform algorithm. This preserves the prior packed-attribute/triangle reuse. Scene assets, repaired Din, actors, weather, audio and frame scheduling are outside this pass.

## Synthetic CPU measurements

GCC 13.3, `-O3`, diagnostics off. These measurements use production function bodies with fixture dependencies, not game replays or GPU timings. They cannot be applied as percentages to whole frames.

Resource medians from five paired baseline/candidate runs, nanoseconds per call. A thread is created/joined before timing to exercise the runtime's atomic shared-pointer path. Warm cases use 500,000 calls; mixed cases use 100,000 and alternate owners with periodic cache clearing, dirty replacement and missing paths. Archive/import/scheduling and the fixture map are substitutes.

| Resource path | Baseline ns | Candidate ns |
|---|---:|---:|
| Synchronous, Alt off | 275.79 | 97.82 |
| Synchronous, Alt on | 318.21 | 136.98 |
| Mixed, Alt off | 273.52 | 109.54 |
| Mixed, Alt on | 323.02 | 166.03 |
| Direct process, Alt off | 70.25 | 65.44 |
| Direct process, Alt on | 107.27 | 102.39 |

The existing render lookup shortcut is already efficient: 81.02→77.96 ns with Alt off and 118.83→122.38 ns with Alt on show no reliable additional improvement. The principal resource gain is the synchronous game/segment path.

Vertex medians use eleven post-warmup measurements with alternating baseline/candidate order, 200,000 vertices in batches of 32, four total lights (including ambient). Reuse cases have 0, 15 or 31 adjacent repeated normals per batch. Different normal patterns also change the original calculation cost; compare each pair within its column.

| Vertex path | Random normals, ms | Moderate reuse, ms | High reuse, ms |
|---|---:|---:|---:|
| Directional lighting | 3.533→3.344 | 3.015→2.538 | 2.457→1.513 |
| Texgen | 4.478→4.379 | 4.162→3.139 | 3.589→1.548 |
| Linear texgen | 10.189→10.042 | 7.243→5.016 | 6.347→1.762 |
| Positional control | 7.890→7.604 | 7.237→7.109 | 6.377→6.476 |

The positional high-reuse control is about 1.5% slower; controls overall are approximately level. Actual normal-reuse frequency in the user's models has not been measured, so favorable synthetic reuse is not a scene-FPS forecast.

## Verification and limits

- Resource baseline and candidate fixtures execute the real cache/loader decisions and both games' segment helpers. Cold direct/.meta Alt overrides, toggles, dirty reload, missing/wrong/empty resources, external replacement, owner/manager changes, archive-parent scope, prefix/initData behavior and null/error cache entries are covered.
- A warm synchronous hit allocates no promise/future in the candidate. The accepted source fails that assertion under explicit `--expect-allocation-regression`; normal baseline comparison skips this candidate-only performance requirement.
- 10,240 candidate/historical-baseline vertex batches compare position/UV bits, RGB/alpha/fog, clipping, light coefficients and dirty state. Coverage includes all lighting/texgen/fog combinations, 1–33 lights, framebuffer aspect paths, zero count, changing matrices/lights/lookat between commands and varying vertex alpha. A deliberately mutated candidate position fails the comparison.
- Sanitizer vertex runs exclude only the null-input scenario because the unchanged historical function already forms a member address from null before checking it. Normal comparisons retain that scenario. UBSan and float-cast-overflow pass for the remaining 8,192 batches; the combined candidate also runs ASan in CI.
- Existing 16,384 triangle-output variants and packed-attribute boundaries remain part of verification. The custom scrolling patch must still apply cleanly and pass its runtime fixture.
- The CI gate fetches the exact historical vertex baseline because checkout is otherwise shallow. Full Windows/Linux compilation and packaging remain distinct from these CPU fixtures.

Diagnostic comparison caveat: warm synchronous hits no longer emit `resource.wait` scopes; cold prefixed wait labels are normalized, and cold/dirty synchronous requests perform an extra cache check before fallback. Cache counters and wait-span counts are therefore not directly comparable to the previous implementation.

## Runtime handoff

Use the same repaired model, full mod stack (including TP enemies), settings and RenderFlight-off configuration as the accepted build. Compare Hyrule Field first, then the remaining Room 9/10 trouble spots; check visual correctness of lighting/reflective materials and animated segment materials during ordinary re-entry. One combined candidate is intended, with no separate diagnostic-only test build.

This record establishes implementation and synthetic evidence. Actual FPS, visual acceptance and promotion of this new candidate require runtime evidence. Keep PR #21 and its packaged build as recovery. Do not transfer final acceptance automatically to the later SoH port.
