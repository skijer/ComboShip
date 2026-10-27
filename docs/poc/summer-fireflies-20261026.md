# Summer nighttime fireflies, 2026-10-26

## Scope and baseline

Request: make Summer nights visibly brighter and more populated in both OoT and MM, including the sparse/tiny fireflies visible in the supplied MM night clip. Source candidate is `poc/bridge-tunnel-sand-winter-mist-20261026`, from `eb3c7cadd6419afd4a68015358880b8d27d0601e`. This is a source POC for the next cumulative combine. No remote publication, full executable build, configured gameplay proof, acceptance, or master promotion is claimed.

## Narrow tuning in both ports

| Runtime quantity | Before | After |
|---|---:|---:|
| Fixed firefly slots | 56 | 96 (+71.4%) |
| Daytime mote slots | 32 | 32 |
| Total fixed state slots | 88 | 128 |
| Full-night pulse alpha before fades | `0.30 + 0.64 * pulse` | `0.48 + 0.52 * pulse` |
| Pulse trough / midpoint / peak | 0.30 / 0.62 / 0.94 | 0.48 / 0.74 / 1.00 |
| Firefly radius before clamp | `depth * 0.0030` | `depth * 0.0036` (+20%) |
| Firefly radius clamp | 0.45–4.8 | 0.45–5.8 |

Brightness is increased through the existing glow's opacity and footprint. Both native renderers already emit the same warm RGB (238, 255, 154) and private soft IA8 texture. Their renderers need no production change. No dynamic lights, extra draw pass, global scene lighting, new actors, assets, archives, or pack/load-order change was introduced. The browser motion counterpart follows the same count/pulse/radius tuning so existing native/preview parity remains useful.

Daytime mote sizing/alpha/flight, sunlight beams, season/exterior and live effect controls remain unchanged. Entry/recycle and screen-edge fades, 05:00–07:00 dawn and 17:00–19:00 dusk smoothing, private RNG, pause behavior, clamped time step, recycle generations, texture-bind reuse, and depth-tested translucent rendering are retained. Extra fireflies still occupy a bounded fixed array. The extra fixed slots increase bounded Summer simulation work during both day and night; additional visible nighttime sprites and larger halos increase night draw/overdraw work. Frame-time impact is unmeasured.

## Regression evidence

Before implementation, the new real-state night-readability test failed in both ports at the expected central mature-particle alpha floor (`p.alpha >= 0.46f`), with the old 0.30 trough. After implementation both state tests passed. The test also exercises at least 88 visible fireflies under a 100-sprite night budget, central mean alpha above 0.70, a peak above 0.98, a readable projected far-tier halo, and a bounded radius. Paused dusk/dawn/day time changes verify fades update while positions, ages, and interpolation generations stay frozen. Existing long-flight, camera/recycle, day-mote, rain, beam, eligibility/reset tests remain in place.

Native renderer fixtures now expect 96 nighttime fireflies and at most 128 dusk sprites plus one optional beam transform, replacing stale 56/88 expectations. MM's actual emitted opacity sample checks are raised to minimum 120/255, mean 0.70, and peak at least 250/255. Its command budget is bounded below 520 commands, covering two sprite textures and optional shafts; scope/matrix and native depth/IA checks remain intact. OoT retains its 2048-command cap and the prior 96-matrix cap for all other seasons, with the Summer cap adjusted to 129.

## Verification commands and status

```bash
ASAN_OPTIONS=detect_leaks=0 \
SOH_SEASON_PARTICLE_CXXFLAGS=-I/workspace/scratch/371eba97e0b8/native-deps/root/usr/include \
python scripts/diagnostics/run_soh_season_particle_tests.py --sanitize
```

**PASS, exit 0:** falling/Summer/Winter state tests, actual native renderer including Summer96/day32/dusk128, rain/sky coexistence and native state ownership, real-header C++20 syntax for both renderer translation units, and all four donor leaf hash checks. ASan/UBSan remained enabled.

```bash
ASAN_OPTIONS=detect_leaks=0 \
MM_SUMMER_TEST_CXXFLAGS=-I/workspace/scratch/371eba97e0b8/native-deps/root/usr/include \
python scripts/diagnostics/run_mm_summer_tests.py --sanitize
```

**PASS, exit 0:** MM state, 241-frame native/preview parity (maximum position difference 0.000326, opacity 0.000001, radius 0.000001, shaft 0.000208), and actual native renderer. Native emitted alpha measured over 56,939 mature central firefly samples: minimum 0.478, mean 0.741, peak 1.000. The private IA texture/depth test, bounded command/matrix budget, balanced lifetimes, pause/live controls, day/night/rain/season gates, disable/reset/reentry checks all passed under ASan/UBSan.

Isolated temporary-tree mutation checks reverted the count, pulse, and halo independently in each port. All six runs failed their intended behavioral assertions: visible swarm count, mature dim-phase alpha floor, and projected halo radius. The production OoT/MM headers and state implementations also match after normalizing only namespaces, include filenames, and the OoT port banner. `git diff --check` passes for the scoped changes.

Initial attempts failed because of missing sparse checkout source/header paths (camera helpers, MM asset/code and Spiritual Stones save headers, and OoT donor leaf textures), the external nlohmann JSON include path, and LeakSanitizer's inability to inspect this host's task namespace under ptrace. Stale native fixture assertions for the former 56-firefly count also failed before being updated to the new requested budget. These were resolved for the final successful commands above. `ASAN_OPTIONS=detect_leaks=0` disables leak detection only; AddressSanitizer and UndefinedBehaviorSanitizer remain enabled. No production workaround was added for these host constraints.

## Remaining runtime proof

No configured OoT or MM executable was launched. Full game compilation, exact installed-pack configurations and load order, vanilla/Alt Assets parity, geometry occlusion, real graphics backend behavior, pause/time travel/room reentry in game, hardware frame times, and visual acceptance remain untested. Review summer outdoors at noon, 18:00, 22:15/23:00, 06:00, and 07:00; toggle fireflies/fluff/beams while paused; travel/turn the camera and change rooms/seasons. Compare the same save and packs against the untouched baseline. Do not promote the POC on host-side checks alone.

## Owned source paths for recovery

- `soh/soh/Enhancements/Graphics/SeasonSummerState.{h,cpp}`
- `mm/2s2h/Enhancements/Graphics/MMSummerAtmosphereState.{h,cpp}`
- `soh/tests/season_particles_summer_test.cpp`
- `soh/tests/season_particles_renderer_test.cpp` (Summer assertions and season-specific matrix budget only)
- `mm/tests/summer_atmosphere_test.cpp`
- `mm/tests/summer_atmosphere_render_test.cpp`
- `tools/summer_weather/preview_motion.js`
- `docs/poc/summer-fireflies-20261026.md`

No shared production renderer edits, staging, commit, or durable save were performed by this worker. The integration owner is responsible for exact-path recovery checkpointing and the cumulative source artifact.
