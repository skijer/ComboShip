# ComboShip bottle and final polish candidate — 2026-10-07

Baseline: verified develop `13901c677d0084e0305eec4672c0557f4434aea7`.
Branch: `poc/bottle-final-polish-20261007`. This is an isolated source/asset
candidate, with no master promotion or application build/runtime acceptance.

## Changes

| Request | Candidate behavior | Evidence and limit |
| --- | --- | --- |
| Reusable Peyton bottle casing | One private `objects/combo_bottle_gi/BottleShell` dependency graph, used by empty and potion recipes in both owners. `compose(contents)` puts any compatible GI contents before this same casing. | POC3 casing geometry, vertices, UVs and texture bytes are preserved. New contents still need their own material, placement and runtime fit. |
| Empty bottle GI | No-op opaque pass; translucent casing-only recipe. | Uses the actual recovered open casing; no new cork or invented mesh. |
| Potion glass, liquid and hex colors | Native and foreign recipes draw the owning live pot palette, fitted liquid, then neutral glass in XLU. Whole-volume white gloss is reduced; fill shape and meniscus stay intact. | Both native drawers and real table exports are executed. The complete marked recipe is required; vanilla and partial/competing overrides retain their original draw order. Visual readability remains untested in-game. |
| MM bottled red potion | Separate native bottle row uses the same liquid/shell with its native Hearts palette. | Native root commands 5/6 retain the editor's patch ABI; three edited RGB samples are observed at entry to the liquid. |
| Trident and Roc's Boots | Existing optional shimmer uses gold `#FFD45A` in native, shared and selected-model/legacy fallback routes. | Actual rendered vertices and both foreign overlay branches checked; effect-off remains off. New enum is appended so existing effect values do not move. Guards and generated bounds agree. |
| Boss souls | Native Goht/Gyorg/Odolwa use the established selected-asset skeletal renderer, as Twinmold already did. | Baseline stale-Alt rigs and normal-as-flex matrix overflow reproduced; sanitizer checks pass. Native texture/TLUT dependencies are intact. Exact installed-pack appearance is not proven; see the separate boss audit. |
| Dungeon key receipt dialogue | MM's eight dungeon key identities and imported OoT dungeon keys use SoH's concise named receipt, preserving catalog articles and dungeon color. | Donor catalog/formatter, receipt bytes, icons, pre-grant queue, immutable counters and cycle recollection checked. Chest Game retains its separate tutorial. No key grant/count changes. |

## Install and reproduce

Apply the recovery patch to the stated baseline and build the matching candidate.
The archives require this renderer patch. Replace earlier potion POC packs in
each owning game's mod set with the matching archive; enable that owner's Alt
Assets. Avoid stacking old potion/empty-shell overrides over the new aliases.
Foreign draws follow the source game's registered resource manager and Alt
selection, independently of the active host. Potion descriptors refresh after
owner Alt changes; progressive acquisition still stays latched.

Archives:

- `ComboShip_Shared_Bottle_GI_POC4_OoT.o2r`
- `ComboShip_Shared_Bottle_GI_POC4_MM.o2r`

Rebuild them from the included recovered source archive:

```sh
python3 -B scripts/mods/build_shared_bottle_gi.py \
  --source-o2r TP_Bottle_Potions_POC3_Liquid_Sheen_OoT.o2r --output bottle-gi
```

For another bottle GI, author a contents root in this casing's coordinates and
use `compose((contents_root, ...))`. Contents must own their material and restore
any local matrix. Install an owner-specific alias for the new item; the private
casing geometry stays shared. The current candidate supplies empty and potion
recipes, rather than converting every existing bottled creature or trade item.

The original shell bounds are X/Z ±21, Y −31..27; fitted liquid bounds are
X/Z ±17, Y −27..7 (meniscus at 7). These are geometry bounds, not a guarantee
that an arbitrary contents model fits or renders correctly on a shop shelf.

## Verification

The recovery ZIP includes exact commands, exit codes and logs for the focused
diagnostics. They cover native C/header compilation, actual owner exports and
foreign dispatch, complete/partial wrapper selection, repeated Alt changes,
palette slot preservation, native/foreign gold vertices and optional-off behavior,
key receipts/queue and boss rig/matrix safety. Full changed native MM boss
translation units also pass syntax checks with their normal PCH prerequisites.
Existing engine controller/const warnings remain.

Useful negative controls:

```sh
python3 -B scripts/diagnostics/run_bottle_gi_tests.py --baseline-state 13901c677d0084e0305eec4672c0557f4434aea7
python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo --baseline-gold-overlays
python3 -B tests/item_receipts/run_key_receipt_tests.py --baseline 13901c677d0084e0305eec4672c0557f4434aea7
python3 -B scripts/diagnostics/run_foreign_soul_skeleton_tests.py --baseline-mm-bosses
python3 -B scripts/diagnostics/run_foreign_soul_skeleton_tests.py --baseline-mm-bosses-type --sanitize
```

These are expected failures. The gold flag applies the former upper-bound checks
in the fixture's extracted foreign dispatcher bodies without changing this tree.
Leak detection is disabled for the sanitizer harness in this environment; no
leak-free claim is made. Source/archive checks are separate from game runtime.

## Runtime acceptance and rollback

Compare both games' empty, red, green and blue potion GIs on pickup and shelves,
including MM's separate bottled red potion. Check unchanged fill/meniscus,
transparent glass/liquid against bright and dark backgrounds, live Hearts/Magic
hex edits, rainbow/reset, and owner Alt off/on/off in native and foreign hosts.
Check selected competing packs keep their original recipe. Test Trident and
Roc's Boots with optional shimmer on/off, authored and selected pack models.
Compare Goht/Odolwa/Gyorg with the installed boss pack through Alt toggles and
re-entry; preserve accepted Twinmold presentation. Read small/boss key receipts
in native and cross-game placements and verify unchanged grant counters.

Remove the two POC4 archives and revert/cherry-pick out the isolated checkpoint
to return to the baseline. Keep the previous accepted source/assets for rollback.
Other polish chats remain separate patches; overlapping files need a normal
merge review before integration. Runtime visuals and full build remain pending.
