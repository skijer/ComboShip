# Playable MM Lost Woods: shared weather

| Field | Record |
| --- | --- |
| Baseline | Published isolated forest/shop candidate `b975551f8d87c69a31b028d313f63086564dc63b`, branch `fix/shop-forest-entrances-20261016`. |
| Candidate | `fix/mm-lost-woods-seasons-20261016`, directly based on that candidate. |
| User report | Seasonal foliage tints already work. Weather should respond to every existing shared control, including independent weather without Rod ownership. |
| Cause | The native opening forest uses covered `SKYBOX_2`; shared weather eligibility accepted only normal/alternate outdoor skyboxes. The earlier foliage-only exception did not enable weather. |
| Change | Enroll ordinary ComboShip exploration visits in rooms 0 and 1 in the existing shared weather eligibility check. Both seasonal selection and the independent weather override use that check. |
| Preservation | Existing foliage selection/palettes, scene geometry, textures, collision, purple light, stump return, arrival camera, title card, onward route, inventory and cycle state remain unchanged. The current POC3 scene archive is reused unchanged. |
| Boundaries | Existing playable-slot, spawn-zero, layer-zero and saved-cutscene predicate applies. Title room 2, real intro/flashbacks, other special-sky interiors, story lighting/filters, underwater, disabled sky and game-over retain their existing exclusions. |
| Sky | Keep the authored covered sky and room lighting. Aurora and sunbeams retain their existing sky/sun visibility requirements. Weather integration does not convert the forest to an open-sky scene. |
| Status | Implemented and verified with production-code fixtures; installed-game validation and full platform builds remain untested. No master promotion. |

The added opening-forest regression executes the real paused season-wheel
confirmation, native environment update, supplemental particle lifecycle and
native rain draw gate. In both rooms it checks spring rain/audio and 96 petals,
96 autumn leaves, 64 winter snow particles, summer/Off cleanup, and independent
rain with no Rod owned. Native precipitation targets, sky, inventory/progression,
day and time are preserved. The native seasonal fixture now compiles with
`COMBO_BUILD` so it exercises ComboShip's scene policy.

Verification commands:

```sh
python3 -B tests/lost_woods_entrance/opening_forest_test.py --sanitize
python3 -B tests/seasons/run_native_weather_tests.py --sanitize
python3 -B scripts/diagnostics/run_mm_weather_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_mm_winter_atmosphere_tests.py --sanitize
ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_mm_summer_tests.py --sanitize
MM_FOLIAGE_TEST_CXXFLAGS='-O1 -g -fsanitize=address,undefined,bounds -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py
clang-format-14 --dry-run --Werror mm/2s2h/Enhancements/Audio/MMWeather.cpp mm/tests/winter_atmosphere_render_test.cpp
git diff --check
```

The weather selector regression failed on the baseline before the eligibility
change. Runtime acceptance remains a separate check: use the current forest
pack, enter both rooms, switch seasons/manual weather, enable independent rain
without a Rod, and verify precipitation/audio, mist, Off, pause/re-entry and
exit to an interior. Confirm the established tints and return presentation.
