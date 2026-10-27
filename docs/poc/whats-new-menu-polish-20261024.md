# What's New and menu organization — October 24 source candidate

Baseline: `25e990332ef9d9d5147c2df4ef5413c61af7aa5b`, tree
`9c5b1d273f90498405fb77dc5b42febfb159ed52`, the independently preserved
Snowhead/regional-actors/native-shop candidate. This pass lives separately on
`poc/whats-new-menu-polish-20261024`; develop/master and accepted archives are
unchanged. The downloadable checkpoint records the exact final commit/tree.

## Result

The existing Settings > General > What's New preference now covers 180 exact
control identities and ten tab identities. Gold NEW and cyan UPDATED retain the
existing meanings. Disabling this preference restores ordinary control colors,
markers and tab appearance; no gameplay option is written by presentation.

Coverage includes both Grace/season setting authorities and their gates/starting
options, child-dungeon masks, OoT Goron/Zora mask behavior, HD Wolf Link, MM
positional audio, MQ selection/count/per-dungeon controls, all seven Generate
entrance controls, all four Generate Black Market entries, MM unrestricted items,
spin styles/elemental spin effects, OoT sky textures, back equipment/scabbard,
night music, both weather editors and the fixed integrated cosmetic color rows.
The pre-existing release controls remain covered. Arbitrary third-party
pack-discovered Mods entries do not inherit markers through substring matching.

The obsolete Instant Transform marker is removed following the supplied
screenshot. Disabled new controls still display their release marker so that
Child Dungeon Mask Rewards remains discoverable; their actual text, input,
progression gate and disabled explanation retain native behavior.

| Location | Organization |
| --- | --- |
| Both Cosmetics > Link & Items | Appearance comes first: Bottle shimmer, then Use Vanilla Held Swords, before color lists. |
| OoT Cosmetics > Keys | Shared Dungeon item colours switch sits beside the actual key colors. |
| MM Cosmetics > World | Dungeons, Dungeon item colours and Dungeon Items colors/emblems are together. The latter group's entries are exclusively dungeon items. |
| Both Cosmetics > Effects | Dividers separate visible effect families, preserving primary/secondary, prim/sec, core/outer, charge/burst and the Zora shield trio. |
| Both Audio Editor > Weather & Seasons | Four highlighted season tabs and the existing outdoor-weather controls. MM's widgets move here from Audio Options, including their search destinations. |
| OoT Audio Editor > Audio Options | Night music retains its native context and live selection/save behavior; the raw track selector is now highlighted. |

Tab captions, widths and IDs remain native. They use accented text and a change
note on hover; styles are restored before rendering contents. The Black Market
custom container delegates markers to each actual child control, avoiding a
duplicate badge before the first checkbox. All cosmetic definitions, default
colors, picker arguments and existing callbacks are preserved.

## Evidence

The existing `tests/whats_new/run_tests.py` CI command now runs the focused audio,
Generate and cosmetic probes as part of its five-test suite. The actual pinned
Dear ImGui revision is `4806a1924ff6181180bf5e4b8b79ab4394118875` with the
repository's libultraship patch applied. Native drawing/input and extracted
production menu functions run against real ImGui; config, resource availability
and save/audio-engine boundaries are fixtures.

| Check | Result |
| --- | --- |
| Combined What's New runner | PASS, 5 tests: exact inventory/identity, native OoT/MM checkboxes, narrow controls, disabled input, notes and style restoration. |
| Audio menu probe | PASS in both hosts: all four season tabs at 320/420/600 pixels; unchanged selected weather, search routing and IDs; actual night-track selection with exactly one refresh/save. |
| Generate/appearance probe | PASS: 11 single markers; stable IDs and off geometry; no draw writes; original three input gates; boss setting click, one seals save and unavailable-but-saved Granny reset. |
| Cosmetic group probe | PASS: native visible-row filtering/suppression, family boundaries, sorted pairs, Zora trio and trails. Baseline negative control fails the five missing-boundary cases. |
| Native cosmetic picker geometry | PASS for both full production pickers at 450/650/850 pixels; longest integrated label retains 19 pixels before Random with highlights enabled; zero CVar/save/apply writes. Reproducible script and source/header hashes are included as checkpoint evidence. |
| Held-sword native menu probe | PASS: unset default, saved values, shared live switching, save/apply calls, inactive tabs and unrelated CVars. Fixture changes add presentation APIs only. |
| Shared seed/menu and Granny header probes | PASS, including native MM solo/combined information authority and actual shared/native header compilation. |
| Full MM weather runner | PASS, including native state/mixer, ten real-header syntax checks, bridge/lifecycle, sky and native audio adapter. JSON/spdlog/SDL headers were provisioned without modifying test runners. |
| Sanitizers | PASS for the instrumented real helper/Generate probes. The combined runner also passes with SANITIZE=1; child audio/family probes remain normal native compilation. Leak detection uses the existing detect_leaks=0 harness convention; no leak-coverage claim. |
| CI policy suite | PASS, 21 tests. The existing manifest command remains unchanged. |
| Whitespace/format | PASS: git diff --check; changed C++ lines formatted with clang-format 14.0.6. |

The missing inventory was reproduced before registration changes. The raw
Generate negative control fails all eleven missing accents and its marker
count. A first attempt to append tab markers changed native tab IDs; the actual
audio probe caught it, and unchanged native captions now pass off/on identity
checks. That abandoned suffix approach is absent from the final source.

Independent review of `25e99033..d6230f92` found no product blocker. Its runner
compatibility finding was treated as material because the existing CI entrypoint
also supports CPATH-only ImGui discovery. With IMGUI_DIR unset, the actual parent
runner reproduced the audio child's KeyError. The parent now passes its resolved
ImGui path to children; all five tests pass with only CPATH configured. No product
source changed after the reviewed snapshot.

## Limits and recovery

This is implemented and verified at the scoped native menu/fixture level. A
complete shared-UI translation-unit attempt stopped before compilation because
the environment lacks libzip's `zip.h`; no complete changed-TU or newly linked
executable claim is made. The earlier baseline's native build evidence remains
separate. Hosted Windows/Linux CI, configured Combo/OoT-solo/MM-solo menus and
in-game visual acceptance are still unrun for this UI candidate.

The checkpoint contains the exact changed files, UI-only binary patch, evidence,
hash manifest and a source bundle restoring the whole inherited candidate from
public prerequisite `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`. A fresh bare
repository seeded only with that depth-one public baseline independently fetched
the bundle, matched the final commit and complete recursive tree, and passed full
git fsck without alternates. Original game archives and accepted appearance packs
remain separate inputs and are not distributed or changed here.

Decisions: execute the user's detailed UI request without another approval pause;
preserve native tab captions/IDs; retain disabled input while showing markers;
use exact release-owned keys for color rows; keep this unpublished source
candidate separate from runtime acceptance and master promotion. Each decision
is reversible within the isolated candidate. No review finding is deferred.
