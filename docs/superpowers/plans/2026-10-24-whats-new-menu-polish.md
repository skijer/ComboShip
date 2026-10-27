# What's New and menu organization implementation plan

> **For agentic workers:** Use bounded tasks with explicit file ownership and preserve the source checkpoint. Steps use checkbox syntax.

**Goal:** Highlight all of cor's integrated controls with the existing What's New preference and make cosmetic/effect/weather controls easy to find.

**Architecture:** Extend the exact-match release inventory, retaining gold NEW and cyan UPDATED. Add a tab renderer that changes presentation without changing IDs. Use the existing native widget paths and explicitly scope raw custom widgets. Group appearance switches before long cosmetic lists, keep dungeon-item colors with dungeon colors, and insert boundaries between effect families while preserving related color pairs.

**Tech stack:** C++20, native Dear ImGui, Python regression runners, existing CMake native build.

**Spec:** cor's October 24 request and supplied Masks screenshot; no new gameplay behavior is requested.

## Baseline and preservation

- Source: `25e990332ef9d9d5147c2df4ef5413c61af7aa5b`, branch `poc/snowhead-regional-actors-20261024`; latest separately preserved source artifact is `ComboShip_Snowhead_Actors_Implementation_20261024.zip`, version 6.
- Candidate: `poc/whats-new-menu-polish-20261024` in a separate worktree.
- Preserve every existing CVar, default, ID, callbacks, save and progression behavior, resource/asset payload and original/combined game guards.
- Keep develop/master and all accepted archives unchanged. Deliver an independently restorable source checkpoint; do not imply gameplay acceptance or publish/merge unrelated ongoing work.
- The user's detailed implementation request supplies the design. Execute without a separate design/permission pause.

## Review focus

- What's New off restores ordinary labels, IDs and colors; no preference changes from rendering.
- Disabled/gated controls retain their existing state and help text.
- Direct custom widgets and native delegated widgets receive one highlight each.
- Cosmetic primary/secondary, core/outer and charge/burst pairs stay adjacent with one divider between families.
- Shared settings and solo menus remain available in both hosts; moved controls appear once at their intended destination.

### Task 1: release inventory, tabs and Generate controls (controller)

**Files:** `combo/gui/WhatsNew.h`, `combo/gui/ComboMenu.cpp`, `combo/gui/BlackMarketUI.h` or actual appearance owner if needed, `tests/whats_new/render_test.cpp`, `tests/whats_new/run_tests.py`.

**Interfaces:** Existing `WhatsNew::Scope(cvar, label, enabled, disabled)` remains compatible. Add `WhatsNew::BeginTabItem(const char* label, bool enabled)`; it preserves the tab's original ID and restores styles before rendering tab contents. Other workers send exact inventory keys, and do not edit this helper or these tests.

- [x] Extend the real ImGui probe with missing exact release keys and tab off/on identity/style/input cases; run to establish missing coverage.
- [x] Extend the inventory for Grace gates, Rod, transformation child gates/Wolf Link, positional audio, MQ, all Generate entrance and Black Market entries, unrestricted items, spin, sky, back equipment, cosmetic additions, night music and both weather editors.
- [x] Highlight raw Generate/Black Market controls without changing CVar/save/callback ownership.
- [x] Run the complete existing What's New runner and applicable native Generate/Black Market menu probes.

### Task 2: cosmetics grouping and effect boundaries (cosmetics owner)

**Files:** Both native `CosmeticsEditor.cpp` / `CosmeticEditor.cpp`; a focused shared cosmetic grouping helper and its tests if needed. Do not edit `WhatsNew.h` or the controller's tests.

**Interfaces:** Use `WhatsNew::Scope` for raw cosmetic controls and `WhatsNew::BeginTabItem(label, enabled)` for relevant tabs. Report every needed exact inventory key to the controller.

- [x] Read actual group iteration and cosmetic IDs; establish the smallest useful behavioral test for pairing/boundaries.
- [x] Put appearance checkboxes including Bottle shimmer and Use Vanilla Held Swords above cosmetic color lists in Link & Items; include existing equivalent OoT controls where supported. Keep defaults/tooltips intact.
- [x] Put Dungeon item colours beside actual dungeon colors/keys, once per host.
- [x] Separate effect families in both hosts while preserving primary/secondary and equivalent pairs; support filtered/advanced entries correctly.
- [x] Verify affected menu/pair grouping with native probes and existing held-sword menu tests.

### Task 3: weather and seasonal tab visibility (audio owner)

**Files:** `soh/soh/Enhancements/audio/AudioEditor.cpp`, `mm/2s2h/Enhancements/Audio/AudioEditor.cpp`; focused audio menu tests only.

**Interfaces:** Use `WhatsNew::BeginTabItem(label, enabled)` for Weather & Seasons and four season tabs. Native `MenuDrawItem` already owns control highlighting. Report all exact backing weather/night CVars and raw controls to the controller.

- [x] Keep OoT's Weather & Seasons structure; add release-aware tab presentation.
- [x] Move MM's existing seasonal/outdoor widgets out of Audio Options into a matching Weather & Seasons tab, retaining all values, native widgets and search routing.
- [x] Keep night music controls in their existing audio context and mark the raw track selector using an explicit release scope.
- [x] Run focused structure/menu probes and relevant existing weather/night audio tests.

### Task 4: combined review, compile and recovery (controller)

- [x] Review every owned diff and confirm game logic and asset paths are unchanged.
- [x] Run combined real ImGui tests, cosmetic/held/menu and affected audio checks; attempt full translation-unit compilation and record its SDK limit (missing libzip zip.h), without claiming a complete build.
- [x] Save the source and known runtime limits before long verification, request one independent combined source review, address material findings, and verify the final tree.
- [x] Produce a deterministic recovery ZIP containing the bounded patch, exact base/candidate/tree identities, SHA256 manifest and restoration instructions. Independently restore and compare the complete tree. Save and provide the finished candidate without merging develop/master.

## Completion record

Core implementation and reviewed source milestone: `d6230f92`. The complete five-test runner, including the existing CPATH-only discovery path, passes; 21 CI policy tests and the affected native menus/weather checks pass. The independent review's CPATH child-runner compatibility finding was reproduced and corrected. Full translation-unit/executable and configured gameplay gates remain open as recorded in `docs/poc/whats-new-menu-polish-20261024.md`. Exact final commit/tree and independently restored source are recorded in the recovery artifact.
