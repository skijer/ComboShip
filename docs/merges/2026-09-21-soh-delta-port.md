# Current SoH delta port — 2026-09-21

## Candidate and baseline

- Host: ComboShip candidate `integration/shared-items-soh-20260921`, based on `cac526a8b9dc08c83f4c2ad7fece0113d284513d` with the upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439` merge under integration.
- Exact previous donor: `fcc86528612ecabb192fe3642acaacf7c2766869`.
- Current donor: `6f03c439ab83cbf149c4a368b529e53724f6fc47` (`poc/pedestal-stump-clearance`).
- Port scope: 208 donor-changed paths: 85 production/build, 55 tests, 33 scripts, 35 documents. OTRGlobals.cpp is reconciled separately by the integration owner from the exact donor patch.
- Baseline branches remain unchanged. No archive, ROM, model pack, migration, or save format changes were made by this port.

## Method and ownership decisions

Each existing file was merged from its exact old donor, current merged ComboShip host, and new donor blobs. New donor files were added individually. Missing Git blobs were retrieved through the connected GitHub API and SHA-1 verified; no Git network fetch was used. No whole-tree replacement was performed. Twelve paths required 23 manual conflict-hunk decisions.

- `soh/CMakeLists.txt`: add donor test registrations; preserve ComboShip SHARED target, dynamic Windows CRT, hidden ELF visibility, NEI symbol alias, launcher/packaging paths, ASAN setup and source discovery.
- `ResourceManagerHelpers.cpp`, `OTRGlobals.h`, `z_play_otr.cpp`, and `graph.c`: retain OoT-pinned audio resource lookup, host context ownership, headless constructor/API, resource-load exception diagnostics, launcher exit and MM-to-OoT loop reset. Add Prelude load-probe lifecycle calls without restoring the standalone donor's deleted unconditional PlayState skybox startup.
- MM adapters inside OoT: use donor per-archive graph owners, complete optional-model fallback, retained immutable texture aliases and vertex byte offsets. This supersedes the earlier raw-ImageData bridge while retaining private-archive selection and texture lifetime. The new aliases preserve dimensions/format and only opt the documented Skull Kid namespace into global replacements. Native MM game code is outside this port.
- Static actors: apply newer dialogue, Anju/Kafei/Lulu/Skull Kid/Shop Gal, Kokiri pose, Ruto face and culling work while retaining host actor-spawn diagnostics. Host-only differences in conflicted viewer/catalogue hunks were formatting; unrelated host diagnostics remain in z_actor.c and their original actor sources.
- Weather: combine authored-scene rain and overcast ownership release with ComboShip's weather location diagnostics; retain GlobalOutdoorRain_Reset and host game-transition reset ownership.
- PAK UI: donor missing-selection preview handling subsumes the host map initialization guard; retain host empty-vector and empty-array guards.
- Audio cache: consolidate duplicate permanent-cache bounds checks from the automatic merge, retaining donor negative/upper-bound validation and all host capacity-dependent behavior. The host audio test continues using the actual permanent-cache capacity and sample-table fixture entries.
- PAK equipment/cosmetics, local time pedestal, Time Gate chest, native material factory/cache, and per-frame probes are ported together with their donor tests.

## Fixture repairs found during validation

These changes affect test harnesses only:

1. `static_story_mm_pose_behavior_test.c`: the donor fixture extracted the updated ordinary-MM draw function but lacked its Anju-only dependencies. The runner failed to compile with an undeclared Anju callback. Added fail-fast boundary stubs for Anju's callback, graph selection and strict texture publication; the existing Skull Kid/Lulu production behavior test then passed.
2. `time_gate_chest_test.c` and its runner: added real time-pedestal and kaleido headers, the production Item_GetSlot helper now used by ComboShip's obtainability routine, and the cheat CVar prefix. The existing test first failed to compile on the missing pedestal constant/slot helper/age macro, then passed both chest and message cases.

Neither repair changes the assertions or substitutes success behavior for the production logic under test.

## Verification

The following checks were observed passing locally (GCC/G++, no game/ROM):

| Check | Result |
|---|---|
| Time pedestal production runner | PASS: both ages, offers/actions/guards, selected geometry, equipment/save ownership, local return, cameras/teardown, bounded scripts |
| Pedestal sword selection runner | PASS: actual PAK layers, per-age maps, selected slot, disabled body, remote/stub guards |
| MM viewer `--viewer-only` | PASS: production lifecycle/render fixture and complete viewer C syntax; existing engine-wrapper pointer warnings remain |
| Static story culling runner | PASS: default, configured and legacy cases |
| MM pose behavior runner | PASS after fixture repair: body turn/return, angle wrap, Lulu local facing, grounding witnesses |
| PAK menu runner | PASS: selection, production config/runtime, stale/disabled/empty/valid map selection; ImGui v1.91.9b-docking |
| Native material probe runner, without archives | PASS: profile/command safety, load probe attribution, actual generated scroll commands, independent buffers/lifetime, real binary factory, metadata reuse/reload/retry/concurrent imports |
| Direct catalogue/MM actor/display-list patch tests | PASS |
| Owned-path `git diff --check` and conflict scan | PASS before final whole-integration formatting |
| Stabilization runner | PASS: complete runner exit 0; 14 initial binaries, 10 archive-audit and 3 progression-probe tests, streamed audio, combat/weather bridges, Time Gate chest/messages, Ruto face and custom-item color/runtime/syntax checks |

Dependency versions used for focused tests: nlohmann/json v3.11.3, ImGui v1.91.9b-docking, and spdlog v1.15.1. Downloaded dependency files are temporary validation inputs, not repository additions. Missing unchanged host files needed by the fixtures were materialized from verified current host blobs without overwriting existing files.

The stabilization syntax checks also emit existing engine-wrapper pointer conversion warnings; their compiler processes and the complete runner returned zero. The full Windows/Linux build/link gates are owned by the integration workflow. In-game behavior, real-archive checks, Alt-assets parity, pause/unpause/re-entry stability, cross-game transitions and the latest pedestal framing/clearance remain unproven here. The donor's pause-crash report remains outstanding. No runtime acceptance or master promotion is implied.

## Changed path inventory

### Production and build

- `soh/CMakeLists.txt`
- `soh/include/functions.h`
- `soh/mods/extended_equipment.c`
- `soh/mods/extended_equipment.h`
- `soh/mods/nei_save.cpp`
- `soh/mods/nei_save.h`
- `soh/mods/pak_loader/pak_loader.cpp`
- `soh/mods/pak_loader/pak_loader.h`
- `soh/mods/pak_loader/pak_selection.h`
- `soh/mods/transformation_masks/assets/mm_asset_loader.cpp`
- `soh/mods/transformation_masks/assets/mm_asset_loader.h`
- `soh/mods/transformation_masks/assets/mm_display_list_patch.cpp`
- `soh/mods/transformation_masks/assets/mm_display_list_patch.h`
- `soh/mods/transformation_masks/assets/mm_kafei_resource.h`
- `soh/mods/transformation_masks/assets/mm_normal_actor_resource.h`
- `soh/mods/transformation_masks/assets/mm_strict_texture_binding.cpp`
- `soh/mods/transformation_masks/assets/mm_strict_texture_binding.h`
- `soh/mods/transformation_masks/mm_player_form.cpp`
- `soh/soh/Enhancements/Graphics/NativeMaterialProfile.cpp`
- `soh/soh/Enhancements/Graphics/NativeMaterialProfile.h`
- `soh/soh/Enhancements/Graphics/PreludeLoadProbe.h`
- `soh/soh/Enhancements/Graphics/PreludeNativeMaterialScroll.cpp`
- `soh/soh/Enhancements/Graphics/PreludeNativeMaterialScroll.h`
- `soh/soh/Enhancements/StaticStoryDialogue.cpp`
- `soh/soh/Enhancements/SwitchAge.cpp`
- `soh/soh/Enhancements/SwitchAge.h`
- `soh/soh/Enhancements/audio/AudioEditor.cpp`
- `soh/soh/Enhancements/audio/GlobalOutdoorRain.cpp`
- `soh/soh/Enhancements/audio/GlobalOutdoorRain.h`
- `soh/soh/Enhancements/audio/GlobalOutdoorRainBridge.h`
- `soh/soh/Enhancements/audio/HyruleFieldNightMusic.cpp`
- `soh/soh/Enhancements/audio/HyruleFieldNightMusicInternal.h`
- `soh/soh/Enhancements/audio/SceneRainPolicy.cpp`
- `soh/soh/Enhancements/audio/SceneRainPolicy.h`
- `soh/soh/Enhancements/audio/WeatherSamplePlayer.cpp`
- `soh/soh/Enhancements/audio/WeatherSamplePlayer.h`
- `soh/soh/Enhancements/customequipment.cpp`
- `soh/soh/Enhancements/customequipment.h`
- `soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp`
- `soh/soh/Enhancements/randomizer/draw.cpp`
- `soh/soh/OTRGlobals.h`
- `soh/soh/ResourceManagerHelpers.cpp`
- `soh/soh/SohGui/SohMenuEnhancements.cpp`
- `soh/soh/SohGui/SohMenuNEI.cpp`
- `soh/soh/SohGui/UIWidgets.hpp`
- `soh/soh/resource/importer/SkeletonFactory.cpp`
- `soh/soh/resource/type/Skeleton.h`
- `soh/soh/z_play_otr.cpp`
- `soh/src/code/audio_heap.c`
- `soh/src/code/code_800EC960.c`
- `soh/src/code/code_800F9280.c`
- `soh/src/code/graph.c`
- `soh/src/code/z_actor.c`
- `soh/src/code/z_demo.c`
- `soh/src/code/z_draw.c`
- `soh/src/code/z_en_item00.c`
- `soh/src/code/z_inventory.c`
- `soh/src/code/z_kankyo.c`
- `soh/src/code/z_parameter.c`
- `soh/src/code/z_play.c`
- `soh/src/code/z_player_lib.c`
- `soh/src/code/z_sram.c`
- `soh/src/overlays/actors/ovl_Bg_Toki_Swd/time_pedestal_cutscene.c`
- `soh/src/overlays/actors/ovl_Bg_Toki_Swd/z_bg_toki_swd.c`
- `soh/src/overlays/actors/ovl_Bg_Toki_Swd/z_bg_toki_swd.h`
- `soh/src/overlays/actors/ovl_Bg_Toki_Swd/z_bg_toki_swd_cutscene_data_1.c`
- `soh/src/overlays/actors/ovl_Bg_Toki_Swd/z_bg_toki_swd_cutscene_data_2.c`
- `soh/src/overlays/actors/ovl_En_Box/z_en_box.c`
- `soh/src/overlays/actors/ovl_En_Box/z_en_box.h`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_actor.c`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_actor.h`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_dialogue.c`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_dialogue.h`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_kokiri.c`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_kokiri.h`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_mm_actor.c`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_mm_actor.h`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_talk.c`
- `soh/src/overlays/actors/ovl_En_Viewer/static_story_talk.h`
- `soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c`
- `soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.h`
- `soh/src/overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.c`
- `soh/src/overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.h`
- `soh/src/overlays/actors/ovl_Item_B_Heart/z_item_b_heart.c`
- `soh/src/overlays/actors/ovl_player_actor/z_player.c`
- `soh/tests/audio_engine_types.h`
- `soh/tests/audio_font_id_test.c`
- `soh/tests/audio_stream_runtime_test.c`
- `soh/tests/custom_item_color_test.c`
- `soh/tests/global_outdoor_rain_audio_test.cpp`
- `soh/tests/hyrule_field_night_music_test.cpp`
- `soh/tests/hyrule_field_night_runtime_test.cpp`
- `soh/tests/mm_display_list_patch_bridge.cpp`
- `soh/tests/mm_display_list_patch_test.cpp`
- `soh/tests/mm_skull_kid_model_test.cpp`
- `soh/tests/mm_strict_texture_binding_test.cpp`
- `soh/tests/native_material_export_probe.cpp`
- `soh/tests/native_material_factory_test.cpp`
- `soh/tests/native_material_runtime_test.cpp`
- `soh/tests/native_material_scroll_test.cpp`
- `soh/tests/night_combat_fixture.h`
- `soh/tests/night_runtime_stubs/z64.h`
- `soh/tests/pak_menu_combobox_test.cpp`
- `soh/tests/pak_selection_runtime_test.cpp`
- `soh/tests/pak_selection_test.cpp`
- `soh/tests/pedestal_sword_selection_test.cpp`
- `soh/tests/prelude_load_probe_test.cpp`
- `soh/tests/rain_engine_boundary.cpp`
- `soh/tests/rain_engine_stubs/libultraship/bridge/consolevariablebridge.h`
- `soh/tests/rain_engine_stubs/soh/Enhancements/game-interactor/GameInteractor_Hooks.h`
- `soh/tests/rain_engine_stubs/soh/ShipInit.hpp`
- `soh/tests/rain_engine_test.c`
- `soh/tests/scene_rain_binding_test.cpp`
- `soh/tests/scene_rain_policy_test.cpp`
- `soh/tests/scene_rain_stubs/ship/resource/archive/Archive.h`
- `soh/tests/scene_rain_stubs/ship/resource/archive/ArchiveManager.h`
- `soh/tests/scene_rain_stubs/soh/resource/type/Scene.h`
- `soh/tests/scene_rain_stubs/spdlog/spdlog.h`
- `soh/tests/static_story_actor_test.c`
- `soh/tests/static_story_child_ruto_face_test.c`
- `soh/tests/static_story_culling_test.c`
- `soh/tests/static_story_dialogue_test.c`
- `soh/tests/static_story_ganon_test.c`
- `soh/tests/static_story_kokiri_pose_test.c`
- `soh/tests/static_story_mm_actor_test.c`
- `soh/tests/static_story_mm_pose_behavior_test.c`
- `soh/tests/static_story_mm_resource_test.cpp`
- `soh/tests/static_story_mm_viewer_test.c`
- `soh/tests/static_story_ruto_water_test.c`
- `soh/tests/static_story_tael_palette_test.c`
- `soh/tests/static_story_talk_test.c`
- `soh/tests/static_story_tatl_matrix_test.c`
- `soh/tests/static_story_viewer_talk_test.c`
- `soh/tests/test_water_temple_caustics.py`
- `soh/tests/time_gate_chest_test.c`
- `soh/tests/time_gate_message_test.cpp`
- `soh/tests/time_pedestal_fixture.h`
- `soh/tests/time_pedestal_test.cpp`
- `soh/tests/weather_audio_stubs/weather_runtime.h`
- `soh/tests/weather_sfx_engine_fixture.c`

### Diagnostics

- `scripts/bind_mm_fountain_animation.py`
- `scripts/build_water_temple_caustics.py`
- `scripts/diagnostics/anju_hd_contact.py`
- `scripts/diagnostics/anju_hd_finish.py`
- `scripts/diagnostics/anju_hd_fit.py`
- `scripts/diagnostics/build_anju_hd.py`
- `scripts/diagnostics/build_anju_hd_goth.py`
- `scripts/diagnostics/build_stump_clearance_poc.py`
- `scripts/diagnostics/check_time_pedestal_syntax.py`
- `scripts/diagnostics/fit_anju_seated.py`
- `scripts/diagnostics/render_anju_hd_fit_review.py`
- `scripts/diagnostics/render_anju_hd_review.py`
- `scripts/diagnostics/render_anju_preview.py`
- `scripts/diagnostics/render_anju_seated_review.py`
- `scripts/diagnostics/run_child_ruto_face_test.py`
- `scripts/diagnostics/run_custom_item_color_tests.py`
- `scripts/diagnostics/run_kokiri_pose_tests.py`
- `scripts/diagnostics/run_mm_pose_behavior.py`
- `scripts/diagnostics/run_mm_rendering_regression.py`
- `scripts/diagnostics/run_native_material_probe.py`
- `scripts/diagnostics/run_pak_menu_tests.py`
- `scripts/diagnostics/run_pedestal_sword_selection_tests.py`
- `scripts/diagnostics/run_rain_runtime_test.py`
- `scripts/diagnostics/run_stabilization_tests.sh`
- `scripts/diagnostics/run_static_story_culling.py`
- `scripts/diagnostics/run_time_gate_chest_tests.py`
- `scripts/diagnostics/run_time_pedestal_tests.py`
- `scripts/diagnostics/test_mm_fountain_binding.py`
- `scripts/diagnostics/test_skull_cull_archive.py`
- `scripts/diagnostics/verify_anju_hd_contact.py`
- `scripts/diagnostics/verify_anju_hd_finish.py`
- `scripts/diagnostics/verify_anju_pose_selection.py`
- `scripts/diagnostics/verify_anju_seated_fit.py`

### Donor documents

- `docs/PAK_SELECTION_AND_BACK_EQUIPMENT.md`
- `docs/poc/2026-09-19-hyrule-field-time-gate-chest.md`
- `docs/poc/2026-09-20-custom-heart-magic-cosmetics.md`
- `docs/poc/lost-woods-loading-cache-poc1.md`
- `docs/stabilization/2026-09-19-child-ruto-face-binding.md`
- `docs/stabilization/2026-09-19-rain-small-fixes.md`
- `docs/stabilization/lost-woods-materials-time-pedestal.md`
- `docs/stabilization/prelude-native-material-probe.md`
- `docs/stabilization/water-temple-caustics.md`
- `docs/static_story_actor_poc6_params.md`
- `docs/superpowers/plans/2026-09-13-prelude-native-materials.md`
- `docs/superpowers/plans/2026-09-15-mm-recovery.md`
- `docs/superpowers/plans/2026-09-16-skull-kid-3ds-tael.md`
- `docs/superpowers/plans/2026-09-18-water-temple-caustics.md`
- `docs/superpowers/plans/2026-09-18-zoras-domain-native-caustics.md`
- `docs/superpowers/plans/2026-09-19-lost-woods-scene-rain.md`
- `docs/superpowers/reports/2026-09-15-mm-dialogue.md`
- `docs/superpowers/reports/2026-09-15-mm-fountain.md`
- `docs/superpowers/reports/2026-09-15-mm-kafei.md`
- `docs/superpowers/reports/2026-09-15-mm-ordinary.md`
- `docs/superpowers/reports/2026-09-15-mm-rendering-root-causes.md`
- `docs/superpowers/reports/2026-09-15-mm-skull.md`
- `docs/superpowers/reports/2026-09-16-anju-hd-r2.md`
- `docs/superpowers/reports/2026-09-16-anju-hd-r3.md`
- `docs/superpowers/reports/2026-09-16-anju-hd.md`
- `docs/superpowers/reports/2026-09-16-anju-kafei.md`
- `docs/superpowers/reports/2026-09-17-anju-goth-build.md`
- `docs/superpowers/reports/2026-09-17-anju-hd-r4.md`
- `docs/superpowers/reports/2026-09-17-anju-hd-r5.md`
- `docs/superpowers/reports/2026-09-17-anju-seated-r9.md`
- `docs/superpowers/reports/2026-09-17-anju-standing-recovery.md`
- `docs/superpowers/reports/appendices/mm_ordinary_verify.py`
- `docs/superpowers/reports/appendices/mm_skull_kid_verify.py`
- `docs/superpowers/reports/appendices/mm_texture_verify.py`
- `docs/superpowers/reports/appendices/tael_palette_verify.py`

The inventory includes the donor test files under `soh/tests/`. The two current integration plan/spec documents are owned by the integration workflow and were not overwritten.
