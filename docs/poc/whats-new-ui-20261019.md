# What's New menu accents — October 19, 2026

Baseline: PR44 source `1811fb86108618bf0a40cea13c267bb8c05c10cf`, tree
`9b3a97db056dfe7a14eb8a207e8b5596000422dc`. The protected develop baseline
remains `a82852a3a02c8cabc541cce7c124da71c5755efb`.

## Scope and behavior

`Settings > General > What's New` uses `gSettings.Menu.WhatsNew`, on by default.
It uses the existing native CVar checkbox/save path in both hosts. The combined
menu reads the same preference through its exported native widget. Turning it
off restores normal widget colors and spacing; it changes no gameplay setting.

Gold `NEW` and cyan `UPDATED` text markers explain release changes on hover.
Controls also inherit the corresponding text accent. Markers move above a
control when its label would not fit beside the marker in a narrow column.
Labels, IDs, CVars, callbacks, disabled/hidden checks, and ordinary control
tooltips retain their original identities and ownership. Disabled controls
retain their native disabled presentation without an accent. RAII restores the
previous text color on normal completion, early returns, and exceptions.

`combo/gui/WhatsNew.h` is the release inventory, with exact CVar matches for
18 controls and one exact custom-section identity. It covers the new checkbox,
thawed Domain, MM Ice Arrows, fire shield, elemental sword controls, native
transformation presentation, shared map/compass information, Rod/starting-Rod
modes, Black Market appearance/seals and emotes. This is a curated inventory of
existing UI surfaces from the inclusion audit. Source/asset changes without a
menu control, including Saria placement and baked Chateau textures, do not gain
invented toggles. Replace the inventory deliberately for a later release.

Both native `MenuDrawItem` paths apply the treatment after visibility and disable
evaluation. This also covers their search results and the combined menu's native
delegation. The combined fallback renderer covers raw widgets/custom sections
without adding a second marker to delegated controls. The common emote heading
uses the same helper, including its native solo registrations. No menu ABI,
scene archive, replacement pack, or gameplay source changes are required.

## Verification

- The unchanged baseline fails both native registration probes because the
  What's New checkbox is absent. Both registrations pass with the change and
  resolve to the same default-on preference.
- `tests/whats_new/run_tests.py` compiles the release helper and the exact native
  OoT/MM checkbox, text and styling functions against real Dear ImGui
  `v1.91.9b-docking` (`4806a1924ff6181180bf5e4b8b79ab4394118875`). Native option
  definitions are narrow fixtures; input/layout/style/hover behavior uses real
  ImGui. The probe also exercises the combined renderer's raw ImGui control
  type. It checks exact inventory matching, unchanged IDs, clicks, disabled
  input, on/off layout, narrow columns, change-note hover, and style restoration.
- The existing emote menu/callback suite and shared seed-menu ownership fixture
  pass. The emote fixture adds only the new presentation API surface.
- The new probe is registered beside the existing menu probes in
  `scripts/diagnostics/run_combo_nei_regressions.sh`; it finds the pinned ImGui
  source through `IMGUI_DIR` or the workflow's existing `CPATH`.

The final real-ImGui probe also passes with AddressSanitizer and
UndefinedBehaviorSanitizer, sweeping every window width from 250 through 340
for the OoT, MM and raw combined renderers. Both native option fixtures use the
actual eight-pixel vertical padding. The upstream ImGui configuration and
workflow-style `CPATH` discovery were also exercised successfully. The existing
native emote callbacks and shared seed-menu routing probes pass. All changed
C/C++ matches clang-format 14.0.6; shell syntax and diff-whitespace checks pass.

Independent source review found no Critical or Important issue. Its one Minor
finding identified native checkbox padding at the inline/stacked boundary; the
measurement now accounts for that padding, the width sweep covers the reported
boundary, and the reviewer confirmed the correction. The probe exercises real
native drawing/input; it does not compile or link the complete game menus.

LeakSanitizer cannot inspect this container's process/thread information;
address/undefined-behavior checks use the project's existing
`ASAN_OPTIONS=detect_leaks=0` convention. No leak-coverage claim is made.

The full linked application, installed GPU appearance, and actual menu
preference persistence after restarting each game mode remain unverified here.
This is an implemented/headless-verified source candidate, not gameplay
acceptance or promotion of develop. Check the new preference on/off, hover and
search in Combo, OoT solo, and MM solo with the exact final application build.

## Recovery

Keep the source baseline and this isolated follow-up at
`integration/whats-new-ui-20261019`.
Restoring this candidate requires the baseline tree plus only the paths in its
commit diff; existing source/assets remain inherited from the baseline. Verify
an independent patch application against the candidate tree before publishing.
