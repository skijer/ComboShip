# What's New color-only follow-up — October 24, 2026

Baseline: `32a20c26af19ff40c03fd1bef299c053b1e4a2c5`, including reviewed UI
checkpoint `87e6cf7c`. Candidate: `fix/whats-new-color-only-20261024`.

The user requested color-only release accents, a sweep for missed integrations,
the MM Graphics settings (OoT location cards, Midna, chest sizing, persistent
drops), MM item-pool Grace/Seasons, and OoT Seasons immediately below Shuffle
Masks. Gold/cyan retain their existing meaning. Native captions, IDs, geometry,
tooltips, callbacks, defaults, disabled gates and seed policies are protected.
No assets or gameplay behavior change.

The shared scope currently submits a separate marker item and changes spacing.
MM's direct randomizer controls bypass that scope. OoT's Seasons controls are
registered later by `SohMenuNEI.cpp`, so append at the bottom of Shuffles.

Implement the existing scope as text styling only, cover exact missing keys and
direct MM controls, and insert OoT's two existing Seasons registrations after
the existing Shuffle Masks widget without changing their storage ownership.

Verification: existing real ImGui menu fixtures, including unchanged on/off
geometry and disabled input; focused native registration/insertion probes; diff
and formatting checks. Full executable and configured in-game menus remain
unproven until separately built and tested. Keep the candidate unpublished and
preserve an independently restorable source checkpoint.

Progress and final sweep/verification results will be recorded below.

## Completed sweep and evidence

Added 28 exact control keys: MM location titles, both native Midna companions,
MM chest style and size, persistent drops, Din sword appearance, and the native
OoT/MM custom-item, equipment, mask, quest, crossover, Bomb Arrow and Elemental
Wand shuffle policies, plus MM map/compass information. The Midna sound-assignment
section also gains an exact section identity. No prefix matching or blanket
upstream highlighting was introduced. Direct MM drawing now scopes fourteen
existing controls; the existing Grace/reward/Seasons/starting-Rod keys resolve
on the actual Item Pool screen. Source-only features do not gain invented UI.

The five existing real Dear ImGui suites pass: audio tabs and seasonal controls,
cosmetic families, Generate/Black Market, native OoT/MM/fallback widgets, and
native toggle registrations. Updated assertions first failed on marker geometry,
then passed with identical complete on/off rectangles, unchanged IDs, native
help, disabled input and balanced style stacks. The gated-shop fixture compares
the same pack/configuration on both sides, since its missing-pack message is
legitimately part of native layout.

Focused extracted-production probes pass for exact OoT row order, retained
widget/options objects, generation gates, missing-anchor fallback, and Solo and
Combo storage ownership. The actual MM custom Item Pool block passes against
real ImGui for preference on/off, both NEI visibility states, generation gates,
exact colors and unchanged rectangles. Boundary option/storage/widget adapters
are fixtures; they are not a complete game runtime. The existing shared-seed
suite also passes for both native MM mode guards and saved map/compass rules.
Every existing MM CVar checkbox/combobox/slider call retains identical argument
tokens compared with baseline.

Real-header C++ syntax checks pass for SohMenu.cpp and SohMenuNEI.cpp in Combo
and standalone OoT (zero warnings), and MM Rando/Menu.cpp with and without
COMBO_BUILD (30 header/unity warnings each). These are translation-unit checks,
not a link or executable build. Pinned project ImGui, existing SDL/JSON/spdlog,
BS thread-pool and libzip headers were used without adding product dependencies.

Independent read-only review found no Critical, Important or Minor issue. Its
unassessed areas are the full executable/configured in-game menus and historical
release completeness beyond the current source inventory. Those remain outside
the proven evidence; there is no blanket claim that every possible older
integration is highlighted. No reviewer finding is deferred.

This is an implemented, native-source/headless-verified candidate. No public
push, CI run, merge or master promotion occurred. It requires rebuilding before
in-game inspection. Required appearance packs and load order are inherited and
unchanged. The handoff archive records exact refs/trees, a bounded UI patch,
complete changed sources, verification evidence and a restorable source bundle.
