# MM persistent/intermittent weather implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Add independently controlled outdoor rain, overcast and thunder to MM without changing native weather or interrupting music.

**Architecture:** An MM-owned simulation produces cosmetic rain and lightning independently of native precipitation. A small C bridge composes its presentation with the environment renderer; an owned PCM mixer reads MM's nature instruments on the game thread and mixes before inactive-game muting. Native weather, cutscenes and incompatible rooms preempt the enhancement.

**Tech Stack:** C11, C++20, existing MM/Ship resource and CVar APIs, Python diagnostic runners, GitHub Windows/Linux builds.

**Spec:** `docs/superpowers/specs/2026-09-21-mm-weather-port-design.md`

## Global Constraints

- Actor-placed rain and authored scene policies are outside this port's scope.
- MM gets independent weather settings, the main enable checkbox defaults off, and native MM weather takes priority.
- Use MM-prefixed internal symbols and `gAudioEditor.MMWeather.*` settings.
- Keep enhanced rain's target/current density separate from the native precipitation targets and global weather mode.
- Preserve the complete SoH weather implementation, MM native snow/fog/day-two rain/Song of Storms, day/night and Final Hours music, ordinary SFX, shared items, saves, randomizer logic, scene placements and assets.
- Compile actual production translation units alongside behavior fixtures.
- Promotion requires the relevant native-weather regressions and cor's runtime acceptance.

## Review Focus

- Native rain or snow starting during an added shower must immediately regain presentation and audio priority (Task 1).
- Pause, faster rendering and authentic update-rate changes must not accelerate rain/thunder cycles (Task 1).
- Reusing a PlayState address or switching OoT→MM must not retain voices or stale resource pointers (Tasks 1 and 2).
- Muted master/SFX sliders and malformed or missing sample data must not leak weather sound or corrupt playback (Task 2).
- A user changing modes, thunder or enable during a flash must get predictable fades and a completed flash without a new strike (Tasks 1 and 3).

### Task 1: Independent weather simulation and environment composition

**Files:** Create `mm/2s2h/Enhancements/Audio/MMWeatherState.{h,cpp}`, `MMWeather.{h,cpp}`, `mm/tests/weather_state_test.cpp`, `scripts/diagnostics/run_mm_weather_tests.py`; modify `mm/src/code/z_kankyo.c`, `mm/src/code/z_play.c`.

**Interfaces:**
- Produces `MMWeather::State::Step(const Settings&, bool eligible, bool nativeWeather, int ticks)`; ticks are 60 Hz simulation units; zero ticks freezes the state.
- Produces C bridge `MMWeather_Update(struct PlayState*)`, `MMWeather_Reset(void)`, `MMWeather_RainDensity(void)`, `MMWeather_Overcast(void)`, `MMWeather_DrawLightning(struct PlayState*)`, `MMWeather_RainColor(uint8_t*, uint8_t*, uint8_t*)`.
- Task 2 supplies `MMWeatherAudio_SetRain(float)`, `MMWeatherAudio_Thunder(float)`, `MMWeatherAudio_Reset(void)`; Task 1 tests use explicit no-audio link stubs until Task 2 exists.

- [x] Write behavioral tests first, including these assertions and 60 Hz versus 20 Hz duration comparisons:
  ```cpp
  MMWeather::State state;
  MMWeather::Settings settings;
  state.Step(settings, true, false, 60);
  assert(state.Density() == 0);
  settings.enabled = true;
  state.Step(settings, true, false, 60);
  assert(state.Density() == 25);
  state.Step(settings, true, true, 3);
  assert(state.Density() == 0);
  ```
- [x] Run `python3 scripts/diagnostics/run_mm_weather_tests.py`; expect compilation failure for absent production state files.
- [x] Implement integer simulation ticks: persistent fades over 60 ticks; intermittent initial dry 120–240 ticks, subsequent dry 1200–1500 ticks and sustain 480–840 ticks. Use a private deterministic PRNG, never the game's random stream. Thunder uses its own timer scaled by the live frequency setting; let an active flash finish when disabled, and preempt it for native weather.
- [x] Implement the bridge using real MM headers. Eligibility requires ordinary gameplay, a rain-compatible outdoor sky/room, no underwater camera or cutscene, and no native precipitation/lightning/weather override. Derive ticks from clamped `R_UPDATE_RATE`. Reset explicitly at environment initialization and Play teardown, regardless of pointer identity.
- [x] Compose density and tint only in rain drawing. Compose overcast with local skybox indices/colors and final light output; do not replace day-specific sky/light configurations. Draw enhancement-owned flashes separately from native lightning state. Example draw boundary:
  ```c
  if (MMWeather_RainDensity() > 0) {
      precip = MMWeather_RainDensity();
  } else if (play->envCtx.precipitation[PRECIP_SOS_MAX] != 0) {
      precip = play->envCtx.precipitation[PRECIP_RAIN_CUR];
  }
  ```
- [x] Run state tests, actual C/C++ header syntax checks and native-state bridge tests; expect all passing and byte-identical native precipitation, weather mode and lightning fields.
- [x] Record the verified change in the candidate commit, using the connected user's git identity.

### Task 2: MM sample decoder, concurrent mixer and lifecycle

**Files:** Create `mm/2s2h/Enhancements/Audio/MMWeatherAudio.{h,cpp}`, `MMWeatherMixer.{h,cpp}`, `mm/tests/weather_mixer_test.cpp`; modify `MMWeather.cpp`, `mm/2s2h/BenPort.cpp`, and the diagnostic runner.

**Interfaces:**
- Consumes Task 1's `MMWeatherAudio_SetRain(float)`, `MMWeatherAudio_Thunder(float)`, `MMWeatherAudio_Reset(void)` calls.
- Produces C bridge `MMWeatherAudio_Mix(int16_t*, size_t)`, `MMWeatherAudio_Shutdown(void)`; shutdown clears decoded PCM and voices after the audio worker joins.
- Mixer accepts owned mono PCM, validated loop bounds and a positive source/output rate ratio. It mixes an independent rain loop and two thunder voices with saturating stereo addition.

- [x] Add failing tests for a loop crossing its endpoint, fractional resampling, two transients plus loop, clipping, invalid ADPCM predictor/order/loop, empty sample, reset, and zero master/SFX volume:
  ```cpp
  int16_t stereo[] = { 1000, -1000, 1000, -1000 };
  mixer.Mix(stereo, 2, 0.0f);
  assert(stereo[0] == 1000 && stereo[1] == -1000);
  mixer.Reset();
  mixer.Mix(stereo, 2, 1.0f);
  assert(stereo[2] == 1000 && stereo[3] == -1000);
  ```
- [x] Run the runner; expect compilation failure for absent mixer files.
- [x] Adapt the donor ADPCM decoder with strict bounds, signed multiplication instead of negative left shifts, owned PCM and rate-aware linear interpolation. Validate codec 0/3, order 2, predictor index, complete frames and loop bounds. Reject unsupported resources with one diagnostic per initialization.
- [x] Resolve native nature soundfont through MM's `gFontMap[2]` and explicit MM resource manager, after the audio maps exist. Upstream `zeldaret/mm/assets/audio/sequences/seq_1.prg.seq` identifies instrument 18 for rain at C4, instrument 19 for low thunder at A2, and instrument 20 for lightning at C4. Select the instrument's note range and apply tuning times the corresponding MM pitch frequency. Retain decoded PCM, never resource pointers, in audio voices.
- [x] Protect mixer state with a mutex. Resource loading/decoding runs only on the game thread, never inside Mix. Apply weather gain plus `gSettings.Audio.MasterVolume` and `gSettings.Audio.SoundEffectsVolume` to final PCM. Mix before `gFscAudioMuted`; clear voices at every scene/reset and decoded resources after audio shutdown.
- [x] Run the runner, including production adapter compilation; expect all passing. Run existing streamed audio and scene randomization diagnostics; expect no regressions.
- [x] Record the verified change in the candidate commit, using the connected user's git identity.

### Task 3: User controls, build gate and reviewable candidate

**Files:** Modify `mm/2s2h/Enhancements/Audio/AudioEditor.cpp`, `.github/workflows/build-artifacts.yml`; create `docs/stabilization/2026-09-21-mm-weather-poc.md`.

**Interfaces:** Controls write `gAudioEditor.MMWeather.Enabled` (0), `.Mode` (0 persistent, 1 intermittent), `.RainColor` (150,255,255), `.Overcast` (1), `.Thunder` (1), `.RainVolume` (100), `.ThunderVolume` (100), `.ThunderFrequency` (100 percent, 25–200).

- [x] Add configuration/bridge tests proving all values use MM-only keys and default-off produces no added rain or audio. Run the runner; expect absent registration coverage to fail before editing the UI.
- [x] Register searchable checkbox, mode, color and slider widgets in MM Audio Editor. Show a short explanation that native weather takes priority; expose no native override or actor options. Add all widgets to the existing Audio Options pane using `MenuDrawItem` and the existing CVar color picker.
- [x] Add `python3 scripts/diagnostics/run_mm_weather_tests.py` to the artifact gate. MM's `CONFIGURE_DEPENDS` source glob includes the new `.cpp` files; no manual source-list addition is necessary.
- [x] Format changed game sources with clang-format-14; run `git diff --check`, weather diagnostics, scene randomization and streamed audio diagnostics. Expect all passing.
- [x] Document source mappings, exact settings, tested evidence and the runtime matrix: day-one Clock Town/Termina Field; persistent/intermittent and live controls; indoor/underwater; native day-two rain and winter snow; Song of Storms/cutscene/Final Hours; pause/reload and OoT↔MM.
- [x] Obtain one fresh whole-branch code review. Reproduce substantive findings with failing tests, fix, and rerun the relevant suite.
- [ ] Publish a separate draft PR based on the existing audio candidate, then inspect its Windows/Linux gate and builds. Report build evidence independently from runtime acceptance; do not merge or promote.
