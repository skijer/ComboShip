# Authored Giant's Knife and broken held model

| Field | Record |
|---|---|
| Baseline | `25e99033`, current recovered regional/source checkpoint; existing held-sword tests passed before edits |
| Candidate | `poc/broken-giants-knife-20261024`, isolated from the regional and menu worktrees |
| Scope | OoT held Knife identity, derived broken model, exact elemental casing fit and focused regressions |
| Preservation | Source GI/catalog, existing held/aura IDs, Biggoron/Four/Great Fairy behavior, custom-body/equipment ownership, vanilla-held toggle, damage/reach/durability/save/progression and unrelated integrations |
| Configuration | OoT adult and child; `ITEM_SWORD_BGS` and `ITEM_SWORD_KNIFE`; depleted/nondepleted durability; Alt off/on selection; Fire/Ice/Light; standalone and Combo compilation |
| Verdict | Implemented, held/serialized geometry and native rendering-fixture verified; configured GPU/gameplay appearance and user acceptance pending |
| Recovery | Focused binary patch from `25e99033` plus cumulative patch from public `196ac9b7`, exact refs/trees and changed-file snapshot; accepted masters untouched |

## Cause and change

The old held selector returned to native broken presentation only for
`ITEM_SWORD_KNIFE`, while the elemental selector also used `!bgsFlag`. An
equipped `ITEM_SWORD_BGS` with depleted durability therefore received the full
authored Biggoron blade and a registered full-blade casing. The regression
fixture reproduced the exact wrong resource path before the fix.

The intact Knife now uses its existing authored iron crossbar/brown grip GI,
rather than the authored Biggoron model. The new broken representation derives
from those exact serialized resources, clips a chipped fracture and caps its
exposed metal. It shares the intact Knife's uniform scale, grip and sockets;
its maximum held X is approximately 2498.93 versus 5500 intact. It does not
stretch the stump or change native attack reach.

The new source mesh is private to held rendering under
`objects/nei_held_swords/broken_giants_knife/model`. The intact GI catalog and
the separate Broken Goron's Sword trade item remain unchanged. New held and
aura identities are appended without renumbering prior ones. Fire/Ice/Light
fits use each actual serialized Knife blade, including the fracture, instead
of a clipped Biggoron surrogate. All 2,283 preexisting generated aura/texture
declarations remain byte-identical.

An active Great Fairy appearance and the equipped Four Sword retain priority.
Selected body, native combined hand and generic/breakable/broken longsword
overrides retain ownership. Missing authored dependencies and the live vanilla
held-swords option return to existing native presentation. This does not add a
breakable Knife or new durability semantics to MM.

## Evidence and reproducibility

- The real held-selector fixture failed first with the full Biggoron path for
  a broken `ITEM_SWORD_BGS`. It now covers both IDs, ages, Alt selection, exact
  hand/aura registration, missing dependencies, selected equipment/body,
  Great Fairy/Four priority, vanilla option and unchanged player/save state.
- The actual selected-hand/renderer/GBI fixture failed first on missing new
  profile geometry, then passed for all three elements, both ages and repeated
  break/repair transitions, including captured hand matrices and final compound
  display-list identity.
- Serialized source checks verify uniform socket/grip scale, short tip,
  unchanged protected hilt vertices, balanced matrix pushes/pops and paired
  fracture edges. The only open blade root edges are the original hidden
  seating boundary at author Y=0.
- Independent review found signed normal bytes were interpolated as unsigned
  values when clipping new vertices. A literal source-edge regression failed
  before correction and now passes. Lit normals are decoded before clipping
  and encoded afterward; unlit colors retain unsigned interpolation. Original
  protected vertex bytes remain unchanged. Review found no other concrete
  defect in selection, ownership, fallback, schema, closure or preservation.
- Existing authored source assets and all prior generated aura declarations
  are unchanged. Regeneration is deterministic and needs no original game
  archives: `tools/nei_held/build_broken_knife.py`, then
  `tools/nei_held/build_swords.py`, then
  `scripts/assets/build_elemental_sword_aura.py`.
- Verification commands: `tests/nei_held/run_held_sword_tests.py`,
  `scripts/diagnostics/run_biggoron_aura_tests.py --sanitize`,
  `scripts/diagnostics/run_elemental_sword_aura_tests.py --sanitize`, and
  `scripts/diagnostics/run_nei_gi_tests.py --sword-regressions-only`.
  ASan/UBSan runs use `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot
  enumerate processes in this environment. Native-header warnings predate this
  change. The GI suite uses the existing JSON/spdlog dependency include paths.

All four native Biggoron configurations pass (OoT/MM, solo/Combo), including
the new actual held Knife/casing cases in both OoT configurations. The elemental
aura suite passes its real selection/lifetime/state tests and full renderer
compilation in all four configurations. The GI sword suite passes all 63
serialized pickup/shop frames and the common/overhead/shop native translation
units. The held suite passes both hosts plus four serialized geometry tests.
Clean regeneration of the broken mesh, all 33 attachments and manifests,
unchanged prior aura declarations, clang-format 14 on changed C/C++ regions,
and `git diff --check` also pass. No remaining source-test failure is known.

## Remaining runtime check

Rebuild the executable **and matching `soh.o2r`**. With Alt off, equip an intact
Giant's Knife, break it and replace it. Check Fire/Ice/Light along the full or
short blade for adult and child; repeat using the intended selected equipment
pack and Alt configuration. Toggle vanilla held swords and confirm the native
fallback. Check Great Fairy/Four supersession. No configured game, GPU result,
full executable package, publication or accepted-master promotion is claimed.
