# Final cumulative source audit — 2026-10-19

This record supersedes the October 18 weekend audit and the interrupted-upload
record for the current source inventory. Publication, linked builds, configured
runtime proof and user model acceptance are separate statuses.

The source preserves PR #44 parent `43131342882972eeb4a257599ec8b061c7dc6d43`,
the reconstructed weekend source, and the bounded October 19 donors through
`6af31f1a`. Checkpoints `a9862b3e` and `e4c8988e` add the final NPC reconciliation
and isolated test-fixture repairs. No older standalone snapshot was substituted
for the cumulative tree. Accepted source/asset baselines remain recoverable.

## Included source

| Workstream | Included behavior |
| --- | --- |
| Friday integration / SoH 9.3 | Existing NEI items/equipment, seasonal weather, receipts, resource/cache fixes and earlier accepted source |
| Combo and both solo games | Game selection, native saves/settings, vanilla/randomizer paths, standalone boundaries and per-game mod-order import |
| Vanilla / Master Quest | Existing layout selection and ROM setup; no replacement with the earlier isolated MQ tree |
| Map / compass, editors, emotes | Ownership/receipt rules, OoT NEI save editor, eight emotes/audio/credits and networking groundwork; new shortcut design remains excluded |
| Entrances and return ownership | MM grotto native jump return; composed dungeon/boss arrivals; portal/stump/Clock Tower routes and Farore/save recovery precedence |
| Foreign pickup text | Master Sword replacement-loop freeze repair, MM page budget/glyph/icon rules, extended IDs and Chateau text-ID separation |
| Boss soul receipts | Explicit colored name spans for all 14 souls in both hosts |
| Black Market | Native controllers/transactions, keeper, raised counter, continuous view, flat offers, fitted locks/orientation and title-card support |
| Shop appearance | Optional Granny/native resource adapter, source-built physical shelves, detailed magical material and cosmetic binding |
| Native solo forest | Private scene copies, local returns, weather/seasons and save lifecycle in both solo hosts |
| Forest actors | Saria scenery/waiting/seated variants, one-time song gift, placement anchors, green shimmer and MM snapshots; portal-side Epona; second-clearing crossed-leg Skull Kid; Combo/solo guards |
| Monkey in OoT | Prelude-placeable native MM monkey idle/high-jump/scratch poses, blink and stationary dialogue |
| Transitions | Native arrival fades for Market/Lost Woods/Mystery routes, preserving composed entrance ownership |
| Audio | Independent forest and Market music entries; Field-night row and scene/generation shuffle; thunder decoder end-offset repair with native/added-weather ownership retained |
| Native transformations in OoT | Deku/Goron/Zora/Fierce Deity animation, native SFX sequencing and lifecycle handling, including solo SoH |
| Din's Fire shield | Stance-dependent ground clearance in both hosts |
| Native NPC runtime | Weighted skin/pose sampler, expression/blink/tracking, Malon hand/basket/stance rules and twins credits/posture guards |
| Final NPC authoring | Anime Saria POC12, Traditional POC11 and twins POC9 tooling; complete young Malon POC10/11/12 asset recipes/art; adult Malon POC5 and Fado POC2 retained |
| Elemental swords / MM ice arrows | Sword selection/receipts, built-in auras, selected-resource handling, MM ice arrows in OoT |
| Chateau Romani | Bundled MM3D model, gold/burgundy shimmer and preserved tutorial receipt paths |
| Spring grass | Blush-pink spring tint in both hosts, kept distinct from summer |
| Always Thawed Zora's Domain | Immersion checkbox using child water/texture presentation during adult time; progression/story/logic flags remain untouched |
| Integration maintenance | Canonical regression entries, extracted observer boundaries and repository-selected clang-format 14.0.6 |

## Separate model and scene archives

These remain external assets; a Git source commit does not install them.
The exact final NPC hashes and selected alternatives are recorded in
`final-npc-assets-20261019.json` and `npc-final-source-reconciliation-20261019.md`.
Anime Saria POC12 and twins POC9 supersede the older WIP/rejected-face holds.
Young Malon POC12 iris remains a candidate awaiting final acceptance.

Retain the existing forest POC9, Market POC14, Native Solo Forest POC2,
grotto POC9 (including ice room 6 and actual water textures scrolling in rooms
0/10), Luminous Skies Universal POC1 (day/night/overcast/dawn/dusk), Djipi Red
Ice POC1, regional fountains and Friday MM3D scene inputs. Settings/load-order
requirements remain in the preserved weekend asset appendix: one scene/actor
version at a time; the matching forest/solo addons; Red Ice above Djipi07/08
with Alt Assets on; MM sky replacement uses Use OoT Sky Textures.

The BossSoul Flame GI Control archive is an optional A/B diagnostic. It is not a
confirmed stray-particle fix and is not installed by default. The approved flame
artwork remains protected.

## Verification

- Native NPC production slices and all eight real actor objects plus static
  Kokiri passed, including routing, sampler, credits and four Malon tracking modes.
- Final NPC Python discovery: 118 tests, 103 passed and 15 external-archive checks
  explicitly skipped. Exact POC12/POC11/POC9 pack hashes were checked locally.
- Seven inherited render/menu/save/export fixture failures were reproduced, repaired only
  at external observer boundaries, and passed focused reruns. Production behavior
  was unchanged for those repairs.
- clang-format 14.0.6 processed all 1,995 repository-selected code files. Asset
  collision checking passed with no new collisions. Conflict-marker scan is clean.
- All **149 canonical command entries** are covered by successful runs across
  the ordered serial segments and two independent workers using unique temporary
  directories. This is recorded command coverage, not a clean uninterrupted serial
  replay. The first 148 passed on the unchanged production tree; the final Ikana
  export fixture needed the real native item-enum and GI policy includes after
  Chateau's producer branch was retained. Its full normal and ASan/UBSan runners
  then passed, and both real headers compiled independently. The extracted producer
  body, assertions and all production files remain unchanged by that repair.
- The private Fados mesh probe inside the Ikana runner remains explicitly skipped
  because that optional external archive was not supplied. This is separate from
  the 15 Python NPC skips and does not imply an installed-asset test.

No matching full Windows/Linux application link or regenerated `soh.o2r` and
`2ship.o2r` is claimed by the native/source checks. Hosted build results must
identify this exact final source rather than reuse an earlier build's success.

## Remaining evidence and asset holds

- Configured runtime: all three modes, forest rooms/shop/Clock exits, save/reload,
  re-entry, void/death/Farore recovery, selected Alt/native asset combinations and
  absent-addon recovery. Listen to thunder with its actual weather settings,
  BGM/fanfare ownership and Field-night/Market shuffle. Seasonal-only rain does
  not request added thunder by design.
- Runtime polish: native/foreign pickups and soul names, all transformation
  sequences/SFX, shield clearance, Chateau model/tutorial/shimmer, blush grass,
  Domain presentation, NPC poses/blink/tracking/gift/shimmer and two-client emotes.
- Existing asset holds: exact repaired Clock/Honey/Moon `_HByteScaleFix1` packs
  still require their pinned bytes or reproducible rebuild; Palace omitted
  object-local components/collision gaps, Honey wall/floor cycling, Moon bind-pose
  instances and remaining scene fitting are not newly fixed here.
- Inherited exclusions: unavailable final Demise revision `cfb9508f`, unresolved
  installed Spring-night lighting/Zora material reports and unimplemented emote
  shortcut replacement remain outside this patch's claims.

Publication permission does not establish gameplay acceptance. These holds are
recorded without discarding the retained source and accepted model progress.
