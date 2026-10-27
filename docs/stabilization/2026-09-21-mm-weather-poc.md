# MM outdoor weather candidate

This candidate adds opt-in persistent/intermittent rain to the MM side of ComboShip. It preserves the existing SoH stack and the streamed music repair at `cc1346ce`. It does not require a placed weather actor or a scene policy. The September 21 follow-up makes enabled outdoor rain override native weather and story presentation restrictions, as requested by cor.

## Controls

Open MM's **Enhancements → Audio Editor → Audio Options → MM Outdoor Weather**. The controls are also registered with menu search. Settings are independent of SoH.

| Setting under `gAudioEditor.MMWeather.` | Default | Behavior |
| --- | --- | --- |
| `Enabled` | Off | Add rain in visible normal/regional-gloom outdoor skies, regardless of native storm policy or story lighting. |
| `Mode` | 0 / Persistent | 1 / Intermittent alternates showers and dry intervals. |
| `RainColor` | RGB 150, 255, 255 | Tint added rain without recoloring native precipitation. |
| `Overcast` | On | Compose cloudy textures and darker output colors with MM's day-specific palette. |
| `Thunder` | On | Added lightning and concurrent thunder; an active flash finishes when switched off. |
| `RainVolume` | 100% | Rain loop gain, also affected by master and SFX volumes. |
| `ThunderVolume` | 100% | Thunder gain, including shots already playing. |
| `ThunderFrequency` | 100% | 25–200%; higher values shorten intervals. |

Rain fades over one second of simulation time. Intermittent rain starts after a 2–4 second dry interval, sustains a shower for 8–14 seconds, then waits 20–25 seconds between showers. Pause freezes this progression; rendering FPS does not advance it. Scene/room re-entry starts a new cycle. Native precipitation, Song of Storms, outdoor cutscenes, room storm policy, regional gloom and fog/lighting effects no longer suppress added rain. Held charges, quick spins and their release effects therefore cannot reset its density or audio. Indoor/disabled skies, underwater cameras, game-over and non-gameplay contexts still clear it. Intermittent mode retains its chosen dry intervals.

## Implementation boundaries

- Added density never writes `EnvironmentContext.precipitation` or `gWeatherMode`. Native day-two scheduling and soft-soil checks keep their existing inputs.
- Overcast composes final rendered sky/light colors without replacing native sky/light configurations. Added flashes have their own timer and bolt storage; they do not take ownership of native `lightningState`, `gLightningStrike` or bolt slots. Added rain/bolts use a separate presentation PRNG.
- MM's native ambience sequence identifies soundfont 2 instrument 18 at C4 for rain, 19 at A2 for low thunder, and 20 at C4 for lightning. This was verified in [the upstream MM ambience sequence](https://github.com/zeldaret/mm/blob/0365728815ec72317d9234e496fc5faa5089b7d0/assets/audio/sequences/seq_1.prg.seq). The adapter uses MM's own font map and resource loader, selects the appropriate instrument note range, and applies native tuning and pitch.
- PCM is decoded once on demand on the game thread. The mixer owns its samples and has a rain loop plus two thunder voices. It adds saturated stereo output before ComboShip's inactive-game mute and does not start/stop native sequence players.
- Play/environment initialization, room changes and teardown clear voices. Audio shutdown joins the worker, clears decoded samples and forces MM to resolve its own resources on re-entry. Missing or unsupported nature samples produce one diagnostic per load attempt; other audio continues.

## Verification and acceptance

Local diagnostics compile the actual environment/play C translation units and the new C++ controller/audio adapter against MM headers, then exercise the production state machine, decoder and mixer. They cover native-state preservation, timing, controls, ownership, scene/pause reset, ADPCM bounds, sample tuning, looping/resampling, clipping, master/SFX mute, live thunder mute, missing samples and the production final-output mute ordering. Existing streamed-ID and scene-randomization diagnostics remain required. The artifact gate runs these checks before Windows/Linux builds. A fresh code review found two rendering integration gaps: the native-only Play draw gate and shared bolt storage. Regressions reproduce those failures on the earlier revision; the candidate includes the caller fix and private bolt ownership with view/reset cleanup.

Full build results and in-game acceptance are separate. This document does not claim a rendered or audible runtime pass.

| Runtime check | Expected | Status |
| --- | --- | --- |
| South Clock Town, Termina Field and Woodfall | Rain despite storm-disallowed rooms and story weather, with continuing BGM/SFX | Pending cor's updated-build test |
| Held spin charge, quick spin and release fade | Rain particles and loop continue without restarting | Pending |
| Overcast in regional gloom, dawn/night and live toggle | Cloudy sky, fading filters/stars, original sky when disabled | Pending |
| Indoor entry/exit, room reload and underwater camera | No added weather in incompatible views; fresh fade on re-entry | Pending |
| Day-two scheduled rain; winter/spring mountain variants | Enabled rain continues; native quest/weather state remains intact | Pending |
| Song of Storms, outdoor cutscenes and soft soil | Added rain continues without changing native progression inputs | Pending |
| Final Hours, fanfares and streamed music | Music/SFX continue alongside added weather | Pending |
| Pause, save/reset and repeated OoT↔MM switches | No stale weather or voices; settings remain MM-specific | Pending |


## Spin, story-weather and sky follow-up

Baseline: combined draft candidate `626d0e325f0f57eb4fd1cbb5b4b8aa47c6974086`. The previously conservative native-weather veto was explicitly superseded by the user's request. The controller leaves native precipitation, weather mode, lightning ownership and quest flags intact while its independent rain stays active. Snow and other native effects can still coexist; this is not a gameplay weather rewrite.

The real `EnMThunder_AdjustLights` / `Environment_AdjustLights` path changes fog during both charged and quick spins. The earlier generic fog veto reset rain and audio. The expanded bridge regression failed on that actual path before repair, then passed held-charge, quick-spin and release profiles after removing the veto. It also covers Termina Field, Woodfall and Clock Town with 14 native/story conditions, preserving native environment bytes, plus view/lifecycle exclusions. Parsing the supplied native scene commands confirms normal sky type 1 for Woodfall and both alternates, and Termina Field plus all nine alternates; some Field story headers use fixed lighting, now admitted by the override.

The sky regression executes production sky configuration tables, texture binding, filter commands and star setup. It reproduced full regional gloom skipping texture updates. The repair admits overcast sky updates in that state, binds both synchronous texture slots before rebuilding, fades the fog/custom sky filters, and hides stars through the same overcast fade. It checks all native day configurations at 12 times, simultaneous slot changes, emitted filter alpha, live toggles and restoration. These are source-level rendering checks, not a GPU or in-game visual pass.

The supplied `zzReloaded Sky.o2r` (SHA-256 `e349be49b332f8d85a49ae78477154468c1aa6ab0cd469ba3701f57ebdd6d8fd`) contains OoT `vr_fine*` / `vr_cloud*` sky faces, not MM `misc/d2_fine_static/gSkyboxClear*` / `misc/d2_cloud_static/gSkyboxCloudy*`. MM still uses its own native/Alt resource lookup and palette. The user's log also mounts a separate MM Reloaded HD pack, which was not available for byte inspection. The uploaded OoT pack alone does not establish MM sky-art compatibility; the combined mod stack needs a visual check.

## Square ground splashes

The supplied 9.77-second clip `283856c0-0e3c-49ea-8fad-c28d66d02514.mp4` shows solid white splash quads in South Clock Town; inspected frames at roughly 2, 5 and 8 seconds confirm the appearance. It does not identify the exact executable or supplying texture pack.

The native MM `gEffShockwaveDL` uses texture alpha multiplied by primitive alpha. Environment alpha is not consumed, so changing the caller's environment alpha is not a supported repair. In the supplied `mm.o2r` (SHA-256 `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`), `gEffShockwaveTex` is I8 64×64 with all 252 border texels zero. Native I8 conversion supplies matching transparent alpha. The inspected TP miscellaneous texture and Djipi effects archives contain no replacement at that path; the exact offending override remains unidentified.

The asset candidate SHA-256 is `b311e547bc2212ac2e2ab4750cdc8b7e1a3c756a7da946088c8ac355733bcbce` (1,595 bytes; one entry). It copies only the original MM texture into `alt/objects/gameplay_keep/gEffShockwaveTex`, byte for byte, with a filename that sorts after every 2Ship pack in the supplied log. It changes no code, display list, vertices or original archive. This shared texture also serves other shockwave effects; the candidate restores their native texture as well. Keep Alt Assets enabled and place it in `mods/2ship`, after any later-added pack that replaces the same path. Verify circular/transparent splashes with the same pack stack; if squares remain, inspect the resolved display list before broadening the override. Pack contents and transparent borders are verified; in-game acceptance remains pending.

## Bundled item compatibility

At cor's request, the build candidate also includes the MM custom magic-pot and heart-model cosmetic compatibility patch from `poc/mm-custom-item-colors-20260921`, based on the same audio-fixed `cc1346ce` baseline. It applies the current `HUD.Magic` or `HUD.Hearts` color to custom item bodies during held-item, pickup, boss-heart and Double Defense drawing. It resolves the first Alt display list before checking custom ownership, preserves heart borders and native models, and clears the added tint after the body draw. No model or texture archive is changed by this code bundle.

`python3 scripts/diagnostics/run_mm_custom_item_color_tests.py` covers 62 emitted-draw-state cases, the affected production C translation units and the Double Defense C++ bridge. It joins the existing weather/audio/shared-item checks in the build gate. Custom pack appearance, live color edits and reset behavior remain pending in-game acceptance alongside the weather matrix above. The separately requested Djipi font O2R is not a code dependency.

Keep this as a combined build POC. Do not promote to the sacred baseline until the build gate and relevant runtime checks are accepted.
