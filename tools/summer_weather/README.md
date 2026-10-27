# Summer day/night atmosphere POC

| Field | Record |
|---|---|
| Baseline | ComboShip `a82852a3a02c8cabc541cce7c124da71c5755efb`; existing accepted seasonal weather/rain work retained. No new baseline gameplay run was performed here. |
| Candidate | `bridge/summer-20261008`, isolated recovery of the approved `poc/mm-summer-day-night-20261008` work. The original checkout was preserved because another conversation could still be writing it. |
| Scope | 32 drifting dandelion seeds by day, 40 hovering/pulsing fireflies at night; smooth 05:00–07:00 and 17:00–19:00 crossfades. Three optional soft sunbeams are an independent review option, **off by default**. |
| Preservation | Existing outdoor/season eligibility, rain settings, autumn/winter/spring behavior, native weather/audio and sky restoration retained. No scene geometry, materials, collision, actors, archives, Alt namespaces, pack routing or load order modified. Private persistent IA8 sprites avoid resource lookups. |
| Lifecycle | Pause freezes motion; scene/room/play changes and leaving Summer clear state; particles fade on entry/recycle. Recycle generations delimit frame interpolation. Translucent particles/beams use depth comparison without depth writes. |
| Evidence | Production state, native/JS motion and sprite-radius parity, actual native guLookAtF projection seam, correct billboard UV handedness, real-header renderer commands/lifetimes, existing weather regressions, source syntax, ASan/UBSan, actual preview control-script execution and offscreen GLES shader/draw rendering. |
| Verdict | Implemented and focused/static verified. Full game build, gameplay appearance/occlusion, vanilla/Alt runtime parity, and user acceptance remain **untested**. No master promotion or push. |
| Preview | Self-contained HTML with Day/Dusk/Night, pause and default-off beam comparison; exact production IA8 bytes and parity-tested motion. MP4 software-projects native exported frames. Lighting/background tint is simulated; the still includes existing HUD/rain and has no reconstructed scene depth/occlusion. Neither artifact is gameplay capture. Browser rendering/CSS layout QA could not run because Chromium was unavailable. The actual preview JS controls were exercised in a Node VM, and their captured shaders/geometry rendered through a real offscreen EGL/GLES context. |
| Recovery | Source is an isolated local commit; apply its patch to the exact baseline or revert the commit. Keep the original recovered checkpoint unchanged. |

The original recovered renderer shared a radial glow for both effects. A failing test demonstrated that daytime seeds lacked an asymmetric tuft/stem. The finished renderer selects a dedicated seed texture and sizes seeds for several pixels at a 720p/60-degree view without changing their flight paths. A second failing test compared real native guLookAtF screen coordinates with the spawn grid and caught a mirrored camera basis. Camera vectors and browser billboard corners now match the native screen/UV orientation.

CMake already registers all `2s2h/*.cpp` and headers through `GLOB_RECURSE ... CONFIGURE_DEPENDS`; no manual build-list additions are needed.

## Focused checks

From the checkout, provide the installed nlohmann JSON include directory through the runner flags when it is not on the compiler's normal include path. In the recovery environment:

```bash
MM_SUMMER_TEST_CXXFLAGS='-I/workspace/scratch/ace0bf23d2bc/native-deps/root/usr/include' python3 scripts/diagnostics/run_mm_summer_tests.py
ASAN_OPTIONS=detect_leaks=0 MM_SUMMER_TEST_CXXFLAGS='-I/workspace/scratch/ace0bf23d2bc/native-deps/root/usr/include' python3 scripts/diagnostics/run_mm_summer_tests.py --sanitize
MM_WEATHER_TEST_CXXFLAGS='-I/workspace/scratch/ace0bf23d2bc/native-deps/root/usr/include' python3 scripts/diagnostics/run_mm_weather_tests.py
```

LeakSanitizer cannot inspect this host's process/task namespace under ptrace; `detect_leaks=0` disables only leak detection. AddressSanitizer and UndefinedBehaviorSanitizer completed. C++ syntax used the baseline build's compilation flags with checkout paths remapped; memory/vector/string PCH includes and the Debian SDL architecture include directory were supplied explicitly. Existing header warnings were retained in logs.

Native/JS parity samples cover day, dusk, night, pause, rain/day visibility, beam gates, disable/reset and re-entry. The native state test separately covers long flight, edge recycling and camera travel. The renderer fixture checks private texture selection, native screen projection, balanced matrix/interpolation/display scopes, bounded commands, beam buffer lifetime, and season/rain/disable/reset gates. Tests are asset-free; native weather-audio fallback is expected when nature samples are absent.

## Reproduce the review artifacts

```bash
python3 tools/summer_weather/build_preview.py --background scene-still.jpg --output summer-preview.html --fragment-output summer-fragment.html
c++ -std=c++20 -Imm tools/summer_weather/export_motion.cpp mm/2s2h/Enhancements/Graphics/MMSummerAtmosphereState.cpp -o summer-export
./summer-export clip > summer-clip.json
python3 tools/summer_weather/render_motion.py --samples summer-clip.json --background scene-still.jpg --output summer-motion.mp4 --stills summer-stills
```

The MP4 uses Pillow, NumPy and ffmpeg. When a Playwright Chromium installation is available, run `node tools/summer_weather/verify_browser.cjs summer-preview.html preview-checks` for offline WebGL, rendered controls, pause/play and 360px layout checks. Add `record` as the third argument for browser video capture. This browser check was unavailable in the recovery environment; JavaScript syntax, native parity, real control-script execution and offscreen GLES rendering were checked instead.

The offline fallback is reproducible with:

```bash
node tools/summer_weather/validate_controls.cjs summer-fragment.html preview-draws.json
python3 tools/summer_weather/render_preview_gles.py preview-draws.json scene-still.jpg preview-stills --gles-library /workspace/scratch/ace0bf23d2bc/native-deps/root/usr/lib/x86_64-linux-gnu/libGLESv2.so.2
```

The preview script also has a finite-vertex regression check: beam vertices carry a fourth alpha component, which must not enter the 3D camera projection.

## Next decisive gameplay check

Use the existing required packs/load order and save, select Summer outdoors (e.g. Clock Town), then compare noon, dusk and night with beams off. Inspect visibility against geometry, camera turns/travel, pause/unpause, room entry/re-entry, and changing away from Summer. Compare vanilla/Alt Assets settings with the same pack order. Review the optional beam toggle only after the default atmosphere; it is not accepted for inclusion by this candidate.
