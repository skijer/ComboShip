# Native solo forest weather and foliage checkpoint

This continues the saved forest candidate rooted at `8ccbd47d`; it does not
change the accepted forest archive, the original intro/flashback scene, or the
combined-game entrance guards.

The private MM exploration scene now joins the existing covered-sky forest
weather and foliage path only in MM solo, rooms 0/1, ordinary file slots and
normal layer/cutscene state. Manual controls and Rod selection use the existing
controller. Existing game-over, underwater, disabled-sky and story-lighting
restrictions still apply. No inventory, story, time or native precipitation
fields are changed by eligibility.

Eight private forest texture identities use the retained donor palettes in
both base and Alt caches. OoT uses its existing reversible texture-copy path
with an optional autumn color per material; every older material keeps its
previous default. The leaf/grass atlas uses canopy blush/frost, matching its MM
donor treatment. Ground remains native in spring. Mushroom, cut-wood, plain
bark, return bark, alpha and non-green pixels remain protected.

Fresh evidence:

- Native MM weather suite passes with ASan/UBSan, including both forest rooms,
  combined and solo copies, real Rod confirmation, rain/audio, petals/leaves,
  winter snow, independent weather without a Rod, Off/re-entry, native save
  preservation and negative private-scene guards.
- Complete MM seasonal scene-material suite passes with ASan/UBSan, including
  the private eight-target base/Alt cases and the existing original forest,
  mixed materials, protected images, reload and teardown controls.
- Both OoT foliage pixel and binding suites pass with ASan/UBSan, including
  exact private-path autumn palettes, spring/winter selection, base/Alt,
  immutable originals and cleanup. The original code fails the new private
  target controls. Leak detection is disabled because this executor's process
  tracing prevents LeakSanitizer from reading `/proc`; address/UB checks remain
  enabled. This is not a leak-check claim.
- All three changed native weather/material translation units compile against
  real engine and recovered SDL2/GLM headers. The reproducible helper is
  `scripts/diagnostics/compile_solo_forest_weather.py`.
- The existing complete OoT seasonal-weather controller suite passes, including
  native particle slots, weather-tag entry/exit, sky handoffs, native lighting,
  flash/release and save ownership. The private OoT forest retains outdoor
  eligibility (`indoors=false`, no skybox) through this unchanged controller.

Commands (run from the forest checkout; dependency paths are recorded in the
saved native dependency recovery notes):

```sh
CPATH="$JSON_INCLUDE:$AUDIT_INCLUDE" python3 tests/seasons/run_native_weather_tests.py --sanitize
ASAN_OPTIONS=detect_leaks=0 MM_FOLIAGE_TEST_CXXFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' python3 scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 scripts/diagnostics/run_soh_season_foliage_tests.py "$TEST_OUTPUT" --spdlog-include "$AUDIT_INCLUDE" --sanitize
```

Raw RED/GREEN logs are included in the recovery package. Full native linking,
configured in-game weather/lighting and visual parity remain unverified.
