# Autumn Scene Materials Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for inline implementation. Preserve the isolated candidate and verify each step.

**Goal:** Apply the existing autumn grass and forest palettes across eligible MM scenes, including the proven Termina Field ground grass.

**Architecture:** Follow the primary render tile through texture loads rather than treating the last image load as the material. Resolve scene namespaces from the canonical MM scene table. Keep a reviewed registry of exact grass and forest texture identities, including native duplicates and explicit Alt aliases.

**Tech Stack:** C++20, MM F3DEX2 GBI, libultraship resource manager, Python diagnostic runner.

**Spec:** The requirements and design below record the user's request in this conversation.

## Global Constraints

- Target any eligible MM scene containing grass or the targeted forest wallpaper/canopy; North Clock Town and Laundry Pool are examples, not a scene limit.
- Preserve the chosen texture pixels, geometry, UVs, collision, alpha and original material commands.
- Grass uses existing amber `0xD99C45FF`; forest foliage uses existing copper `0xB96848FF`.
- Preserve the forest renderer writeback correction from candidate `785909825b0305d02e85e547fefc40c368a8b96c`.
- Retain `MMWeather_SeasonForPlay` eligibility and Off restoration. Do not promote a sacred baseline or claim game runtime acceptance from diagnostic tests.
- `02B050` and `02C850` are water in both native MM and the supplied Prelude export; exclude them.
- The recovered accepted grass mapping identifies Termina ground keys `01BE50`, `01C650`, `01DE50`, `02C050`; their image source remains unchanged.

## Review Focus

- A selected primary grass texture followed by an unrelated detail image must still tint grass.
- A selected secondary image must not recolor an unrelated primary material.
- Scene changes and Alt toggles must restore original resources and reuse safe variants.
- Mixed grass/architecture lists must only tint selected triangle batches.
- Water, snow, masonry, roof artwork and existing custom grayscale lists must retain their treatments.

### Task 1: Material selection and scene coverage

**Files:** Modify `mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp` and `mm/tests/autumn_scene_foliage_test.cpp`; create `mm/2s2h/Enhancements/Graphics/AutumnSceneMaterials.inc`; extend `scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py`.

**Interfaces:** Consume `MMWeather_SeasonForPlay`, canonical `tables/scene_table.h`, serialized scene display lists and texture CRC64 identities. Preserve `BuildVariant(const std::vector<Gfx>&)` and the public update/reset API.

- [x] Add a failing test for two texture tiles: grass in tile 0 and unselected detail in tile 1, then the converse and a reused primary TMEM slot.
- [x] Run `python3 -B scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py`. Expected: assertion failure because the last texture hides the primary grass.
- [x] Track image identity, tile TMEM bindings, texture loads and the active render tile in `BuildVariant`; preserve conservative child-call resets and original commands.
- [x] Add a scene lifecycle test using a selected material in North Clock Town and Laundry Pool, testing native/Alt resources and unknown scene IDs.
- [x] Run the scene suite. Expected: failure at the current two-scene restriction after the tile test passes.
- [x] Resolve scene names using the canonical scene table. Register visually reviewed native grass/forest identities and exact decoded duplicates, including shared scene texture paths. Exclude mixed masonry and water/snow/artwork.
- [x] Extend the diagnostic runner to generate actual native material coverage from a supplied MM archive. Independently decode primary texture bindings and retain original source commands.
- [x] Verify positive grass/forest coverage and negative material controls, Off transitions, native/Alt restoration and exact command preservation. Run actor grass/forest regression suites and changed-source syntax/format checks. Expected: all requested checks pass.
- [x] Prepare the isolated candidate, obtain independent review, and address important findings with failing regressions before publishing.

The reviewed registry contains 169 exact identities. The frozen native reference contains 538 draw-list cases: 267 selected and 271 negative controls. Review removed five cracked-masonry/moss identities; all 11 lists loading that family remain as negative or mixed controls. These counts precede runtime season eligibility and do not establish game visual acceptance.

### Task 2: Reviewable handoff

**Files:** Candidate branch and a compact evidence report.

**Interfaces:** Consume the verified Task 1 commit and input/archive hashes. Publish a new POC branch without changing the prior forest branch or opening a PR.

- [ ] Upload the exact tested source tree to `poc/autumn-ground-grass-20261010` under the user's existing authorization.
- [ ] Verify the remote parent and source tree. Expected: parent `78590982`, source tree identical to the local tested tree.
- [ ] Save the scope/evidence report with input hashes, check results, and game runtime limitations.
