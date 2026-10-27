# Summer firefly sustained glow candidate — 2026-10-09

| Field | Record |
|---|---|
| Baseline | `56c83a878562052ca873b7d414f7d0e1175b6325`, confirmed identical to remote `bridge/audit-merge-20261008` during this investigation. |
| Evidence | Supplied `MedalTVScreenRecording20261009005634403.mp4`, frames inspected at 3, 14 and 27 seconds. Windows, Day 1, 10:15 p.m., time speed 0; wooded view with a raised wooden platform. Clip does not identify its executable commit or exact scene/setup. |
| Finding | The merge contains the October 8 count increase from 40 to 56 and 20% wider halos. That patch deliberately retained the original 12–94% opacity pulse. Full night weight is already active at the clip's time. Tiny sprites, low pulse opacity and depth occlusion can make the visible population much smaller than the allocated count. |
| Candidate | `poc/summer-firefly-glow-20261009`, isolated from the cumulative merge. |
| Scope | Change only the firefly opacity pulse and its matching browser motion preview; add an emitted-opacity regression to the existing renderer fixture. |
| Preservation | Keep 56 fireflies, existing 94% peak opacity, IA8 sprite/color, halo size, spawn rows/depth tiers, motion, interpolation identities, dawn/dusk and entry/recycle/edge fades, depth testing, cottonwood, sunbeams, weather/audio and season policy. No O2R or pack changes. |
| Status | Implemented; focused native state, real-header renderer and preview parity checks pass. Complete Windows package build, exact-scene gameplay visibility, Alt Assets configuration and user acceptance remain untested. Master is unchanged. |

The old pulse was `0.12 + 0.82 * pulse²`. The candidate is `0.30 + 0.64 * pulse`: it raises the dim phase and sustains the glow longer while retaining the same maximum. No additional particles, draw passes or graphics commands are added. Geometry still properly occludes the effect; this candidate does not guarantee that all 56 particles are visible from every camera position.

The existing renderer test previously checked that all 56 particles issued draws, but did not test readability. The added check samples actual emitted primitive-color alpha over 30 simulated seconds at 10:15 p.m. It excludes deliberate entry/recycle fades and the screen edges. The test fails on the baseline's dim opacity, then passes on the candidate:

| Emitted opacity, 33,436 eligible samples | Baseline | Candidate |
|---|---:|---:|
| Minimum | 0.122 | 0.302 |
| Mean | 0.432 | 0.623 |
| Peak | 0.941 | 0.941 |

These are sprite opacity measurements, not framebuffer luminance or scene visibility measurements. The private texture also multiplies that alpha. Native state/lifecycle checks, 241-frame native/JS preview parity, the renderer's depth-test and command-budget checks, and the MM weather regression suite pass. Independent code review found no issues; ASan/UBSan checks pass with leak detection disabled because LSan is incompatible with this environment's ptrace.

Run the focused checks from the repository root:

```bash
MM_SUMMER_TEST_CXXFLAGS='-I/path/to/nlohmann/single_include' python3 scripts/diagnostics/run_mm_summer_tests.py
ASAN_OPTIONS=detect_leaks=0 MM_SUMMER_TEST_CXXFLAGS='-I/path/to/nlohmann/single_include' python3 scripts/diagnostics/run_mm_summer_tests.py --sanitize
MM_WEATHER_TEST_CXXFLAGS='-I/path/to/nlohmann/single_include' python3 scripts/diagnostics/run_mm_weather_tests.py
```

Next decisive check: compare the same nighttime wooded view and camera in the supplied clip, with the same save and packs, on the next cumulative build. Check stationary visibility, a camera turn, pause/unpause and re-entry. Review whether the sustained glow is readable without becoming distracting. This is a source candidate for the next integration; no remote push or Actions build was performed here.
