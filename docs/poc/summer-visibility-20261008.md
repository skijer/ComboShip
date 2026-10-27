# Summer visibility follow-up — 2026-10-08

| Field | Record |
|---|---|
| Source baseline | Published combined-recovery commit `35ce5f22f11c845a4ef0b28228880547e14ee92d`. The five edited source/test/preview files are byte-identical in the later equipment recovery `0e9aeb05cc67771ce6a62c31abff6968b7a63e12`. |
| Runtime evidence | Both supplied October 8 clips were sampled at full resolution and as contact sheets. User accepts the summer atmosphere and firefly appearance, requests clearer cottonwood and more fireflies or a gentle luminosity increase. Clips are not independently tied to a build hash. |
| Candidate | `poc/summer-visibility-20261008`; isolated from the shared recovery and equipment work. |
| Scope | Cottonwood size/opacity, firefly count/halo size, count-aware firefly spawn rows, matching preview and existing fixture expectations. |
| Preservation | Same authored IA8 sprites, colors, firefly pulse and peak opacity, motion speeds/orbits, depth tiers, dusk/dawn fades, pause/reset/recycle handling, depth comparison, weather/audio/season policy and default-off sunbeams. No scene, archive or pack changes. |
| Verification | Baseline summer checks pass with the existing JSON include supplied. The revised night renderer expectation fails on the baseline's 40 particles, then passes with the candidate's 56. Production motion/lifecycle, 241-frame native/JS parity and real-header renderer checks pass normally and under ASan/UBSan (leak detection disabled). Existing MM weather state/mixer/bridge/sky/rain/audio and syntax checks pass. Source formatting and patch whitespace checks pass. |
| Status | Implemented and focused build/static verified. Complete linked Windows build and candidate gameplay/Alt Assets appearance remain untested; no master promotion. |

| Tuning | Before | Candidate |
|---|---:|---:|
| Cottonwood slots | 32 | 32 |
| Cottonwood radius/depth | 0.0032 | 0.0045 (about 41% larger) |
| Cottonwood maximum radius | 7 | 10 |
| Cottonwood unfaded opacity | 0.40–0.64 | 0.58–0.80 |
| Firefly slots | 40 | 56 (+40%) |
| Firefly radius/depth | 0.0025 | 0.0030 (+20%) |
| Firefly maximum radius | 4 | 4.8 |
| Firefly unfaded opacity | 0.12–0.94 | 0.12–0.94 |

The increased firefly count is distributed over seven rows within the same lower/middle view band, retaining the prior near/middle/far depth distribution. The shader, texture, RGB color and blend mode are unchanged. A wider existing halo increases its visible footprint without another rendering pass or a brighter core. At dusk, the complete particle draw remains below the existing 400-command regression bound.

## Reproduce focused checks

Run these from the repository root, pointing the include flag at an installed nlohmann JSON include directory when it is not already on the compiler search path:

```bash
MM_SUMMER_TEST_CXXFLAGS='-I/path/to/json/include' python3 scripts/diagnostics/run_mm_summer_tests.py
ASAN_OPTIONS=detect_leaks=0 MM_SUMMER_TEST_CXXFLAGS='-I/path/to/json/include' python3 scripts/diagnostics/run_mm_summer_tests.py --sanitize
MM_WEATHER_TEST_CXXFLAGS='-I/path/to/json/include' python3 scripts/diagnostics/run_mm_weather_tests.py
```

## Recovery and next check

Apply `summer-visibility.patch` with `git apply --check` followed by `git apply`; it is a narrow follow-up to the combined recovery and also applies over the equipment recovery. Reverse with `git apply -R summer-visibility.patch`. The ZIP includes before/after copies of every changed file and verification logs.

In the same save, pack order and scene as the supplied clips, compare Summer daytime cottonwood against the tree backdrop and nighttime firefly density. Keep sunbeams off for that comparison. Check camera movement, dusk, pause/unpause and a room transition. Candidate gameplay appearance is for user review; fixture results do not prove scene occlusion or performance with real packs.
