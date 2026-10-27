# SoH weather port to ComboShip's MM side

Status: approved by cor on 2026-09-21, narrowed to persistent/intermittent global rain. Implementation and runtime acceptance are separate gates.

## Intended result

cor wants the recent SoH weather system available while playing the 2Ship/MM side of ComboShip. The first candidate should provide opt-in outdoor rain, persistent/intermittent modes, rain color, overcast, thunder and live volume/frequency controls, while retaining music and ordinary sound effects. Actor-placed rain and authored scene policies are outside this port's scope.

Approved defaults: MM gets independent weather settings, the main enable checkbox defaults off, and native MM weather takes priority. cor expressed interest in disabling native conditions eventually, but explicitly agreed to leave them alone in the first implementation.

## Pinned inputs and preservation

| Input | Revision and role |
| --- | --- |
| ComboShip candidate | `cc1346ceec9060e574cae4c57b89635a4bc2b3d9`, `poc/2ship-scene-randomization-stream-capacity`; retains Shared Items, current SoH stack, MM scene-reload randomization and expanded streamed IDs. Its replacement Windows/Linux build passed in run `35647288150`. |
| Prior integrated SoH donor | `marsh6487/Shipwright:poc/pedestal-stump-clearance`, `6f03c439ab83cbf149c4a368b529e53724f6fc47`, recorded in the existing shared-items integration spec and fetched for comparison. |
| Later weather ownership work | `poc/lost-woods-rain-small-fixes`, `c433b6c7e0839d0bd8dddfd9b0de0bb228de76bd`; per-owner requests, scene-owned policy, script precedence and lifecycle cleanup are present in the integrated SoH source. |
| Older integration reference | `integration/nei-weather-static-actors`, `9a76eb637a4090279ccbcca14e5c043412ea766b`; lacks the later scene-rain policy and is insufficient as the sole weather donor. |

Use ComboShip's current SoH weather files as the immediate reference so its integration adjustments survive. Comparison against `6f03c439` found identical sample-player, concurrency helpers and environment source; other weather files include formatting and host diagnostic differences. Do not replace either game subtree.

Preserve the complete SoH weather implementation, MM native snow/fog/day-two rain/Song of Storms, day/night and Final Hours music, ordinary SFX, shared items, saves, randomizer logic, scene placements and assets. Existing SoH acceptance does not establish MM runtime acceptance.

## Approach

| Approach | Assessment |
| --- | --- |
| MM-owned controller and sample mixer, adapted from SoH | Recommended. Reuses the weather behavior while adapting MM environment fields, sound formats and game lifecycle. |
| Move both games onto a new shared weather subsystem | More reuse, but changes the established SoH path and DLL interfaces before MM parity is demonstrated. Defer. |
| Copy SoH weather writes and resource names directly | Unsuitable. MM precipitation affects gameplay, its sample layouts/names differ, and several SoH APIs are absent. |

## First POC contract

Add an MM-specific weather controller with clear ownership of its rain presentation, sky adjustment, lightning requests and private audio voices. Use MM-prefixed internal symbols and `gAudioEditor.MMWeather.*` settings so SoH settings do not silently enable the new candidate.

Provide the main enable checkbox, persistent/intermittent mode, rain color, overcast toggle, thunder toggle, rain/thunder volume and thunder frequency. Advance cycles once per unpaused simulation update, with durations expressed in simulation time; renderer FPS must not change cadence. Initially preserve the donor's real-time cycle durations. MM's longer day cycle is a separate tuning consideration, not a reason to copy raw frame counts without checking the update rate.

Native snow, scripted weather, native rain and Song of Storms take precedence over added global weather. Preserve the native weather tags' parameter meanings. The first POC does not repurpose an existing MM tag or interpret an OoT Lost Woods policy as an MM scene policy.

Keep enhanced rain's target/current density separate from the native precipitation targets and global weather mode. Combine it only where drawing needs an effective density. Native day-two scheduling and soft-soil checks must continue to observe their original state. Respect underwater rendering gates and native snow suppression. Restore only presentation state owned by this controller, and allow an active lightning flash to finish before release.

Adapt the private sample mixer to MM's native sample/loop layout and its own resource manager. Resolve and verify the actual rain/thunder sample mappings during implementation; the SoH `Rainfall_META`/`Lightning_META` strings are not verified MM paths. MM's apparent `ResourceMgr_LoadAudioSample` helper is currently inside disabled code, so it cannot be used as an existing API. Decode with the correct source tuning/rate, retain owned PCM, and avoid resource lookups on the audio thread. Missing/unsupported audio resources should produce one useful diagnostic and leave native audio operational.

Mix owned weather audio into MM's final PCM buffer before ComboShip's inactive-game mute. Apply MM master and SFX volumes to private voices, as well as the weather sliders. Preserve its separate ambience player, BGM, fanfare and streamed-track behavior. Release voices and controller state on play teardown, reset and `MM_PrepareForTransition`; reacquire resources from MM's manager after resume.

## Concrete integration points

| Area | Existing source and required adaptation |
| --- | --- |
| Controller/reference | `soh/soh/Enhancements/audio/GlobalOutdoorRain.*`, `WeatherSamplePlayer.*`, `concurrent_weather_audio.*`; add MM-owned counterparts under `mm/2s2h/Enhancements/Audio/`. |
| Rain, sky and lightning | `mm/src/code/z_kankyo.c`; integrate controller updates with the unpaused environment update, effective draw density and owned lightning audio. Adapt sky blending to MM's day-specific configurations and restore ownership safely. |
| Native schedule/gameplay guards | `ovl_En_Test4/z_en_test4.c`, `ovl_En_Weather_Tag/z_en_weather_tag.c`, `z_demo.c`, `func_800FE9B4`; use these as regression targets, not a replacement schedule. |
| Audio and game switches | `mm/2s2h/BenPort.cpp`: `OTRAudio_Init`, `OTRAudio_Thread`, `OTRAudio_Exit`, `MM_PrepareForTransition`, `MM_ResumeGame`. |
| Settings | MM `AudioEditor.cpp` and the existing ComboShip menu bridge; register MM-specific controls and search entries. Shared master/SFX sliders already mirror into MM's `gSettings.Audio.*` floats. |
| Build and regression gate | MM CMake source registration, focused MM weather tests and the existing build-artifacts gate. Compile actual production translation units alongside behavior fixtures. |

## Decisive verification

1. Default-off behavior: native state and output unchanged, including day-two rain, snow, Song of Storms, soft-soil conditions and cutscenes.
2. Controller behavior: fade transitions, mode changes, live controls, ownership/preemption, scene/room reloads, pause and reused PlayState addresses. Prove stable timing at different rendering/update configurations.
3. Audio: valid sample decoding/loop boundaries, missing resources, zero master/SFX/weather volume, transient completion, simultaneous BGM/fanfare/SFX, reset and game-switch silence. Keep the expanded streamed-ID regressions running.
4. Build: real source/header compilation, formatting and full Windows/Linux CI on a separate candidate. A fixture-only pass is insufficient.
5. Runtime: start in day-one South Clock Town and Termina Field; test persistent/intermittent modes, all controls, indoor exit/re-entry and OoT-to-MM round trips. Then exercise day-two rain, mountain winter/spring, underwater views, Song of Storms, Final Hours and pause/save/reload.

The first visible success is added MM outdoor rain with uninterrupted music/SFX and clean game switching. Promotion requires the relevant native-weather regressions and cor's runtime acceptance. Keep the source candidate and original masters available for rollback. There is no actor authoring work in this port.
