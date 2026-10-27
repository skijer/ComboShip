# Native OoT Title Parity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship native OoT title lettering in the implementation and an optional smooth 4x replacement across every card in the existing pass.

**Architecture:** Build native preoutlined IA8 cards into both port archives through a bounded asset decoder and existing archive preparation dependency. Prefer existing full-card overrides, then built-in native cards, retaining safe legacy fallbacks for unknown content. Custom/forest cards share this curated asset source and existing native presenters.

**Tech Stack:** C/C++, Python standard library for preparation/packing, native IA8 texture records; Pillow/NumPy/potracer only for offline curation/previews. Normal builds use only the standard library.

**Spec:** docs/superpowers/specs/2026-10-23-native-title-parity-design.md

## Global Constraints

- Baseline marsh6487/ComboShip 3c3cd4fe0c37ccfb009c2d138773e185d977c174; isolated candidate poc/title-card-parity-20261023.
- Native appearance requires no optional pack; optional replaces the same resource names with smooth restored curves and unchanged logical dimensions.
- Preserve the existing opt-in checkbox, native fade/eligibility/language behavior and cross-game requests.
- No unrelated scene, actor, shop behavior, weather or audio edits; retain original native inputs and earlier artwork unchanged.
- No full-game archives or external font files in candidate delivery.

## Review Focus

- All six former English fallbacks must have built-in art; unknown/localized messages retain safe fallback.
- Existing user whole-card overrides retain their 192x24 contract; new native cards use 240x24 without squeezing.
- Optional old font packs cannot suppress the new built-in cards; native/HD resource names and logical geometry agree.
- Forest and Black Market use the correct owner's resources and existing title timing; a missing pack cannot strand gameplay.
- Default CMake preparation must actually reach both archive packers, including stale-generated-file repair.

### Task 1: Curate native cards and smooth optional HD art

**Files:** Create tools/location_titles/{native_assets.py,curate_native_titles.py,hd_assets.py,curate_hd_font.py,preview_native_hd.py}, tools/location_titles/assets/hd-v1/{manifest.json,glyphs.zlib.{001,002}.b64}, tools/location_titles/assets/native-v3/{manifest.json,cards.zlib.b64}, scripts/assets/prepare_native_titles.py, CMake/NativeTitleAssets.cmake, tests/location_titles/test_native_assets.py. Modify CMakeLists.txt.

**Interfaces:** `native_assets.resources(directory=ASSETS) -> dict[str, bytes]` returns exactly 69 native records under custom/oot_location_titles/v1. `native_assets.hd_resources(directory=ASSETS) -> dict[str, bytes]` returns the same names, 4x RGBA replacement records. `prepare_native_titles.prepare(output_root=ROOT) -> int` writes both hosts atomically and skips matching bytes.

- [x] Add tests for 67 native IDs + market/forest, malformed input bounds, native/HD coverage, smooth alpha, native contour agreement and geometry parity, and stale output repair; watch the missing-built-in-assets assertion fail.
- [x] Curate actual source glyphs/halo and native-only additional characters; generate deterministic native bundle and provenance without redistributing input archives. Provide source/control contact sheet.
- [x] Implement strict stdlib decoding, native/HD framing and atomic default preparation. Add the CMake target to CheckAssetCollisions.
- [x] Run `python3 -B tests/location_titles/test_native_assets.py`; expected all tests pass. Verify the real CMake preparation target in an isolated minimal CMake fixture. Commit bounded source/art milestone and save a recoverable checkpoint.

### Task 2: Route every existing title presenter through the native cards

**Files:** Modify mm/2s2h/Enhancements/Graphics/OotLocationTitleCard.c, ComboOpeningForestTitleCard.cpp, mm/2s2h/Enhancements/Gameplay/{BlackMarket.cpp,ComboSoloForest.cpp}, soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp, tests/location_titles/run_tests.py, tests/native_market/{run_title_draw_tests.py,title_draw_test.cpp,run_title_tests.py}, and narrowly affected forest fixtures.

**Interfaces:** Native ordinary path `__OTR__custom/oot_location_titles/v1/<language>/<id>` uses 240x24; market uses `.../market` at 240x24; forest uses `.../lost_woods` at 144x24. Native resource availability remains owner-aware. Existing 192x24 custom/mm_location_titles overrides retain priority.

- [x] Add recorded production draw assertions for built-in width/centering and override precedence, all 67 names without optional font probes, and no additional outlines; watch old code fail.
- [x] Add native dispatch and switch the custom card resources/dimensions; keep native request/fade code intact. Update only expected resource/rectangle values in existing production fixtures.
- [x] Run native asset, location-title, native-market title draw/presentation and forest/cross-game return suites; expected green. Run applicable source-format checks once changes settle.
- [x] Produce source patch, native/HD comparison/contact sheets and optional smooth HD pack with restoration instructions. Commit; obtain one fresh whole-branch review; fix important findings with focused failing controls. Save final independently restorable checkpoint. Record configured gameplay/GPU verification as untested.

## Accepted refinement

cor rejected repeated-pixel HD and accepted the smooth South Clock Town reference's spacing and border. Independent review then found that a generated alphabet did not preserve native letter shapes. The final HD source traces cubic contours directly from native face coverage, retains native advances/baselines and fractional halo, and traces the exact original Lost Woods wordmark as a whole. The new source-contour comparison rejects the former generated alphabet. No generated alphabet is used in the final pack.

Final review refinement: preserve the native composed-card transparent rows before HD interpolation; verify packed HD perimeters. Original Lost Woods donor framing remains its documented exception.
