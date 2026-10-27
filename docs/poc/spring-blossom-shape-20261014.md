# Spring blossom shape correction

Cor reported that the falling spring particles look like hearts. The supplied
reference shows pale, rounded five-petal blossoms with crimson cores and golden
anthers. Inspection confirmed that both game modules were drawing the same
two-lobed, pointed, untextured mesh; no external texture pack caused the shape.

| Field | Record |
| --- | --- |
| Baseline | ComboShip PR #41, `1818beabd38e3e083ecfb3adf73d2bb8018d5299`, branch `bridge/audit-merge-20261008` |
| Baseline evidence | Hosted Build Artifacts run `38488833325` and Windows scroll probe `38488833320` completed successfully; cor reported the heart appearance in game |
| Candidate | `fix/spring-blossoms-20261014`, parent `1818beab` |
| Scope | Spring particle silhouettes and colors in both OoT and MM |
| Preservation | Existing 96-slot owner, falling trajectories, flutter, rotation, fades, camera/player clear pocket, pause behavior, recycling, interpolation and private RNG; all other seasons, foliage, audio and scene/archive data |
| Configuration | Existing Spring / Cherry Blossoms control, outdoors; geometry is independent of Alt Assets and scene packs |
| Evidence | Production renderer shape failure before correction, native OoT/MM suites and real engine header checks, ASan/UBSan, static geometry preview and independent code review |
| Verdict | Implemented and local/static verified; revised shape/palette render accepted by cor; installed-pack/GPU appearance and runtime acceptance pending |
| Recovery | Parent commit retains the original mesh; no accepted master is changed |

## Corrected appearance

`combo/NeiSpringBlossoms.h` now owns the mesh and blush palette shared by both
renderers. Each existing layer contains 24 intact five-petal blossoms and 72
smaller, curved loose petals. Rounded lobes remove the heart cleft. Intact
flowers have broad blush petals, a small crimson core and gold anthers. Loose
petals retain a single rounded end and a narrower attachment end.

The choice and palette are stable for each particle slot. Existing fall,
wind drift and flutter remain unchanged; this correction does not introduce
new emitters attached to tree actors. The submitted geometry remains untextured
and two-sided, with smooth vertex colors. MM restores the surrounding textured
XLU material through its existing cleanup path.

Each intact flower uses 50 triangles and vertex batches of 31, 11 and 30, within
the native 32-vertex limit. A loose petal uses 12 triangles. The maximum layer
is 2,064 triangles, compared with 1,152 previously, with the same 96 matrices
and no new runtime allocations. No replacement scene or mod archive is needed.

## Verification

- The revised OoT renderer test failed on the original mesh at `lobes == 5`.
  It passes on the corrected production mesh and checks the accompanying loose
  petal, curvature and existing command/matrix/material/lifecycle constraints.
- `ASAN_OPTIONS=detect_leaks=0 python -B scripts/diagnostics/run_soh_season_particle_tests.py --sanitize` passed, including actual production renderer and C++20 engine-header checks.
- `ASAN_OPTIONS=detect_leaks=0 CPLUS_INCLUDE_PATH=<local dependency include> python -B tests/seasons/run_native_weather_tests.py --sanitize` passed, including actual MM actor C checks, 24 blossoms/72 loose petals across Termina Field and five outdoor Clock Town districts, movement, pause, toggles, sole ownership and textured XLU restoration.
- Clang-format 14 checks passed for the shared header and affected game/renderer
  test files; `git diff --check` passed. Code review found no concrete issues.
- The preview rasterizes the actual candidate vertex colors and triangle
  lists at enlarged scale; it is not an in-game screenshot.

Leak detection was disabled because this execution host blocks LeakSanitizer's
process inspection. Address and undefined-behavior checks remained enabled.
This correction adds no dynamic allocations.

Cor approved the five-petal shape, requested a slightly pinker appearance and
corrected the core interpretation to red/crimson. The revised shared blush
palette and small crimson core follow that direction. The red is confined to
the core so it does not spread across the entire petal fan. On October 13,
2026 (America/Chicago), cor accepted the revised render and authorized
publication to PR #41: "that revision lands with me - proceed". This accepts
the demonstrated shape and palette; installed in-game appearance remains
a separate check.

The next decisive test is Spring / Cherry Blossoms in the same runtime
configuration where cor saw hearts, in both OoT and MM: confirm the rounded
flowers and loose petals remain readable while drifting and fluttering at
normal gameplay distances. The existing spring nighttime-lighting report
remains a separate unresolved issue.
