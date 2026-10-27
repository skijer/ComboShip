# Retail shop entrance selection

| Field | Record |
| --- | --- |
| Baseline | `1bbb94e692629c0d910f97b57dd7cd02240d3775`, the merged door-only/forest source. Existing runtime acceptance is not inferred from that merge. |
| Candidate | Shop correction on `fix/shop-forest-entrances-20261016`; source-only candidate, separate from the protected master. |
| Scope | All retail shop openings, exclusive shop-pool ownership, native generation exclusion, versioned catalogue replay and focused paired-return investigation. |
| Preservation | Original Mask Shop/Clock Tower portal; ordinary native interiors; dungeon/boss/grotto pool isolation; existing recorded layouts and progress; distinct Kakariko potion front/back passage and backyard access. |
| Configuration | Production native descriptors/enums and pre/post override functions in compiled fixtures. Child/adult and native-off/coupled native permutations are named below. No ROM, game window, asset-pack order or Alt configuration was exercised. |
| Verdict | Implemented and compiler/fixture verified. Exact reported-seed traversal, linked packaging and in-game acceptance remain untested. No master promotion. |
| Recovery | Revert the isolated shop correction; existing saves and seed files need no migration or reset. |

## Confirmed selection defects and correction

The previous classifier selected broad Interiors before checking retail identity.
It also only recognized shop openings with physical doors. Consequently, broad
Interiors included shops while the shop toggle was off, broad Interiors overrode
the shop pool while it was on, and open OoT Kokiri/Goron/Zora shops plus MM Goron
Shop were absent from the narrow shop choice. The native OoT Interior pool and
one-way Spawn/WarpSong targets independently admitted shop endpoints as well.

New generation has one owner for retail entrances: `Cross-Game Shops`, Pool 4.
It includes ten OoT doorways and seven MM storefronts, regardless of whether an
opening door exists. The OoT set is Kokiri, Goron and Zora shops, both Bazaars,
Market potion shop, Bombchu Shop, Granny's shop and both Kakariko potion doors.
The MM set is Zora Hall, Magic Hags, Milk Bar, Curiosity storefront, Trading Post,
Bomb Shop and Goron Shop. The existing Mask Shop portal stays outside the pool.
Minigames, Lottery, smithy services and Curiosity's separate back hideout retain
their previous nonretail classifications.

Broad Interiors now selects nonretail interiors only. Door Interiors selects its
nonretail door subset when broad Interiors is off. Shops can be toggled alongside
either choice, and their permutation remains separate. New native OoT generation
excludes all retail forward/reverse endpoints before native decoupling/mixing and
from one-way targets. This prevents native entrance settings from independently
moving shops while the shop toggle is off. When an older native shop-to-house map
is supplied, v4 still identifies only the actual retail destinations as Shop; a
house cannot inherit retail classification from that recorded physical source. Recorded native overrides are replayed
through the existing separate `ParseJson`/`ApplyEntranceOverrides` path.

## Seed compatibility

New maps containing any extended entrance category use schema v4. Dungeons-only
maps retain schema v1. Versions 1–3 remain accepted with their exact recorded
pool identities, catalogue order, native-resolved returns and permutations.
`OptionsFromSeed` derives the old membership policy from the recorded version,
and carries it through host catalogue transport. It restores the old 40 OoT /
42 MM broad catalogue and seven OoT / six MM door-shop catalogue for old records.
A v4 option field cannot override its version's new membership policy.
There is no game-save schema change, progress reset or automatic reroll.

The headless CLI determines exact replay before installing its selected pool
options. If `--cross-*` flags change an old settings file's choices, that becomes
fresh generation and clears the legacy membership policy before building the new
catalogue. Otherwise the old catalogue could be written as v4 and rejected on its
next reload. The regression executes the actual CLI selection block, native
catalogue and codec: unchanged/identical choices preserve v2/v3 exact replay;
changed Grotto/Shop/Interior flags produce reloadable v4 maps with the current
retail policy; disabling the last pool cleanly disables the cross map.

## Potion-shop return investigation

No same-door return bug has been reproduced in the available source fixtures.
The user reported Market Bazaar entering adult Kakariko potion shop and leaving
outside MM Bomb Shop; their exact seed and selected potion exit were not supplied
in this workspace. The two potion entrances share a room but own different
physical exits. Entering the back and leaving the front can legitimately follow
the front's independently shuffled source, including Bomb Shop. Collapsing both
exits onto the last arrival would break passage/backyard access, so no such runtime
change is included.

The new test obtains OoT portals from the production native table and
`CatalogueJson`, then executes production `Entrance_OverrideNextIndex`,
`Entrance_OverrideDynamicExit`, runtime route state and scene/checkpoint bodies.
An explicit Market Bazaar → potion back / Bomb Shop → potion front layout returns
to Bazaar through the back exit and to Bomb Shop through the front. A sweep of
32 cross permutations × child/adult × native-off/v3 coupled native shop cycles
checks 128 layouts, with every same-physical-exit inverse returning to its actual
source after exactly one native/cross composition. The actual MM transition and
handoff bodies also check Bomb Shop's paired potion-back return.
These are compiled behavioral fixtures, not observed in-game traversal.

The next decisive check is the reported seed's two recorded mappings plus the
actual potion entry spawn and physical exit. A same-door mismatch in that exact
configuration remains an open investigation rather than a claimed repair.

## Verification

Failing controls were observed before production changes: broad Interiors
incorrectly admitted retail shops with Shops off; the actual OoT and MM
catalogues omitted open shops; the native simple/all Interior pools still admitted
retail endpoints; and the production menu still exposed/disabled the old door-only
shop control. The corrected cases pass.

- `tests/cross_dungeon/run_tests.py`: complete suite passed for the main shop
  correction, including selection, native exclusion and production returns. The
  CLI review correction has focused normal/sanitizer and full headless translation
  unit proof; its complete-suite rerun was still running at that checkpoint.
- `shop_pool_test.cpp`: exclusive selection, v4 mixed-pool codec and v3 replay;
  version-owned membership and legacy host option transport.
- Production OoT/MM catalogue fixtures: exact membership, every broad/door/shop
  toggle combination, legacy v2/v3 catalogue preservation and original portal exclusion.
  A direct reference probe using the baseline OoT classifier/catalogue bodies also
  produced identical legacy JSON for 768 pool-option/Ganon/native-layout combinations.
- `shop_cli_options_behavior_test.py`: actual headless option selection, legacy
  exact replay, changed settings → production catalogue → v4 encode → reload validation;
  normal and ASan/UBSan runs pass. The complete headless translation unit also
  passes `g++ -std=c++20 -fsyntax-only`.
- Production menu fixture: independent Shops with broad Interiors, retained Door
  Interior preference, updated labels/help.
- `soh/soh/Enhancements/randomizer/entrance.cpp`: complete translation unit passes
  `g++ -std=c++20 -fsyntax-only` with `COMBO_BUILD` and actual engine headers.

- New shop codec, production OoT shop catalogue, native pool exclusion and 128-layout
  return fixtures pass ASan/UBSan; MM catalogue and actual transition/handoff fixtures
  also pass with `ASAN_OPTIONS=detect_leaks=0`. The first two MM attempts hit this
  sandbox's LeakSanitizer `/proc` task-inspection failure; disabling leak inspection
  allowed address/undefined-behavior checks to complete.
- `scripts/diagnostics/run_cross_dungeon_reload_test.py`: actual launcher reload
  preflight, rollback and legacy disabled path pass.
- Complete native `entrance.cpp` translation unit also compiles without `COMBO_BUILD`;
  the standalone native behavior is guarded from the shop-generation policy.
- clang-format 14 and `git diff --check` pass for the changed production sources.

No full platform link, package or runtime result is implied by these checks.
