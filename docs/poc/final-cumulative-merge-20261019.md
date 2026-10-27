# Final cumulative source merge — October 19, 2026

This is the reconciled source inclusion list for cor's authorized cumulative merge into `develop`. It retains the complete existing PR44 candidate and adds the newer Woods POC6 checkpoint that the earlier dinner audit did not include. Source integration and successful compilation remain distinct from installed gameplay/visual acceptance.

## Exact inputs and recovery

| Role | Identity |
| --- | --- |
| Develop before merge | `a82852a3a02c8cabc541cce7c124da71c5755efb` |
| PR44 before final Woods integration | `0864a312f9f03e4d192427d8627d4d3ae9bb905b`; tree `0e4a9020838013baae61f36cfa98a236de284bca` |
| Protected prior PR ref | `checkpoint/pr44-before-final-woods-20261019` |
| Woods input | `ef72039e4d01879a24ea58b955ff5bde6572f145`; tree `c1cb7b5d21d16f712037a484b3c16ff453e47f9d` |
| Already integrated Woods boundary | `dbd2f6ec54777b72e8f38a5c30642b7053e48b90` |
| Current Woods recovery bytes | `Woods-of-Mystery-POC6-Source-Recovery.zip`, SHA-256 `8a6bb10c9b2ed6a74a7080a4852dc4afa35d34b872f97c0161fd96344f41911f` |
| Optional final Woods scene | `zzzzz_Woods_of_Mystery_OoT_Makeover_POC6.prelude.o2r`, SHA-256 `0a0c98c32ef8dc1df3b47e7920b6e3853c6b58d353e654a84124a907e6096bd4` |

The 42-path Woods delta applied without conflicts. Before formatting, all 42 owned files exactly matched the donor. Four C++ files receive only clang-format 14.0.6 whitespace changes; lexical token sequences are identical. All unrelated source and assets retain the current PR44 versions. The shared weather preflight gains the missing `combo/menu` include path; its real player translation-unit check reproduced the hosted failure before this correction.

## Complete source inclusion

| # | Workstream | Included behavior |
| --- | --- | --- |
| 1 | SoH 9.3 cumulative integration | Retains the merged 9.3 engine/randomizer, foreign-item/file-select/trap/tracker repairs and matching source ownership. Existing NEI and MM behavior remain cumulative. |
| 2 | NEI equipment and grants | Retains custom item identity, cross-game grants, equipment ownership, held/get-item models, native fallbacks, pause names/icons and prior receipt fixes. |
| 3 | Earlier equipment polish | Retains Four Sword/Byrna recovery, Somaria native/foreign draw ownership, Zora fin/audio repairs and the earlier sword/Room Key/receipt work already carried by the cumulative bridge. |
| 4 | Rendering/resource preservation | Retains resource/cache, Alt lookup and texture-failure handling; cold Hyrule Field correction; custom texture-scroll interpreter support; existing authored assets and ownership fixes. |
| 5 | Combo / OoT solo / MM solo | Launcher selection, selected-game ROM setup, native vanilla/randomizer paths, separate saves/settings and dormant-engine isolation. |
| 6 | Vanilla and Master Quest | Both quest assets, selected/random layouts, saved capabilities and deterministic seed/save replay. |
| 7 | Cross-game entrances | Dungeon, boss, expanded dungeon, interior, grotto/grave, clock/time admission and native entrance composition; completion and recovery ownership. |
| 8 | Seed generation and retail pool | Generator cancellation/join before reset; checked return metadata; v4 retail/open-storefront membership and existing-seed replay. Shops retain their separate pool setting. |
| 9 | Shared settings | Existing Stat Upgrades/RPG controls and Map & Compass Information under Shared Seed Settings, with seed guards and original callbacks. This does not create a new solo-MM RPG item pool. |
| 10 | Map and compass information | Recorded combined entrance/reward ownership, physical boss names/icons, native SoH hints and independently saved solo-MM Off/On behavior across all four temples. |
| 11 | Editors and config import | Updated OoT NEI save editor; independent exact OoT/MM mod-order strings, explicit empty orders and native MM import ownership. |
| 12 | Eight emotes | All eight animations, controls, native audio/volume behavior, cleanup, manifests/credits and multiplayer pose groundwork. New shortcut design is still outstanding. |
| 13 | Pickup text and Master Sword | Foreign Master Sword replacement-loop freeze repair; MM page budget, glyph/icon operands, extended IDs and Chateau text-ID separation. |
| 14 | Boss soul dialogue | Explicit receipts for all 14 souls in both hosts, with dungeon color applied only to the name span. |
| 15 | Din's Fire shield | Retains the shield orientation/stance and ground-clearance work in both hosts; fire/damage behavior is preserved. |
| 16 | Native transformations in OoT | Deku, Goron, Zora and Fierce Deity animation/SFX sequencing and lifecycle handling, including solo SoH. |
| 17 | Elemental swords | Selection and receipts, selected-resource handling and built-in elemental auras. |
| 18 | MM ice arrows in OoT | Retains the October 19 elemental-swords/MM ice-arrow integration and its native ownership guards. |
| 19 | Seasons and weather | Retains cumulative native/shared seasonal controls, forest weather, seasonal foliage/surface tinting and winter/spring source fixes. |
| 20 | Blush spring grass | Blush-pink spring grass in both hosts, distinct from summer; the newer grass checkpoint is retained. |
| 21 | Always Thawed Zora's Domain | Immersion-only checkbox, off by default; adult presentation can use child water/textures while progression, story and logic flags remain untouched. |
| 22 | Audio regressions | Thunder decoder end-offset repair; independent rain/lightning presentation ownership, Field-night entry and scene/generation music shuffle. Seasonal-only rain does not itself request added thunder. |
| 23 | Forest and Market music | Independent entries and native sequence/scene ownership in both hosts. |
| 24 | Native transitions and returns | Outgoing fade/animation before handoff; composed arrivals; MM grotto native jump return; queued dungeon/boss arrivals; save/void/death/Farore recovery precedence. |
| 25 | Forest routes and cameras | Playable Lost Woods and separate Woods of Mystery routes, stump/Clock returns, arrival cameras/title support and native arrival fades. |
| 26 | Private native solo forests | Private scene copies, local returns, weather/seasons, addon availability checks and save lifecycle in OoT solo and MM solo. |
| 27 | Saria forest behavior | Prelude-placeable waiting, visible-ocarina playing and seated scenery/song-giver poses; one-time shared song claim, actor-relative green hollow hexes, native/Alt replacement resources and all three mode guards. |
| 28 | Epona and Skull Kid | Portal-side Epona and second-clearing crossed-leg/reclining Skull Kid placement, actor hosts and authored override guards. Placement still needs installed review. |
| 29 | MM monkey in OoT | Prelude-placeable native MM monkey: idle, high-jump and scratch poses, blink, stationary dialogue and repeating native MM calls for all three entries, with per-emitter room/lifetime cleanup. See `monkey-room-audio-20261019.md`. |
| 30 | Native NPC runtime | Weighted skin/pose sampling, expression/blink/tracking, prop behavior and native actor registration; MM/SoH rendering parity is limited to demonstrated configurations. |
| 31 | Saria model authoring | Final Anime Saria POC12 and Traditional POC11 source reconciliation. Select one matching Saria asset alternative. |
| 32 | Kokiri twins | Accepted POC9 posture/model tooling and credits support. The earlier rejected POC4 face is superseded. |
| 33 | Young and adult Malon | Adult POC5 and pose/hand/basket/stance/tracking work retained; young Malon POC10/11/12 face, hair and iris recipes/art included. Final young POC12 visual acceptance remains open. |
| 34 | Young Fado | Retained native adapter and exact POC2 source/pack identity; no wholesale old Shipwright donor patch. |
| 35 | Black Market gameplay | Native shop controllers, transactions, keeper, raised counter, continuous browsing view, flat offers, fitted X locks/orientation and title-card support. |
| 36 | Granny and shelves | Optional Granny/native resource adapter and safe fallback; source-built physical shelves, detailed magical material and cosmetic binding. |
| 37 | Authored shop door | The placed plain door opens the native shop, returns relative to its loaded position/yaw and suppresses the retired coordinate trigger when authored markers exist. |
| 38 | Magic Mirror item | Permanent fixed stump reward outside the shuffled pool; native pickup/get-item, independent ownership, shared page-two selector with Time Gate, full-width button routing, save/load and sync. |
| 39 | Magic Mirror travel/presentation | Approved ALTTP-style blue/gold model; outdoor safety prompt/cancel; temporary Farore-style casting/overhead presentation; loaded-stump-relative pickup and recall in Combo and both solo modes. Farore/grotto ownership is preserved. |
| 40 | Final forest scroll bindings | All six authored tracks supplied natively to OoT opaque/translucent lists; MM private scene uses native animated materials. Building/bench layers, purple return portal and stump effects come from the matching FINAL scene. |
| 41 | Wanted poster | User-selected GitHub-avatar poster, source PNG and provenance. It replaces the gold-haired portrait in the matching combined and solo scene candidates. No origin/reuse-license clearance claim is made. |
| 42 | Woods of Mystery native support | POC6 source through ef72039e4: eight baked bridge resources, reversible native wall/canopy patches, curated green hexes, floor/shoulder/canopy authoring corrections and seasonal targeting of private grass, wall trim, foliage and grass actors. Optional full-scene archive remains separate from source/runtime acceptance. |
| 43 | Chateau Romani bundled source | Baked 4K MM3D atlas, detailed cork and translucent glass resources for both stock port archives, existing Alt selector, native/foreign routing, gold/burgundy shimmer and preserved tutorial receipts. No separate Chateau addon is required after regeneration. |
| 44 | Integration/build maintenance | Existing canonical test registration and fixture repairs, clang-format 14.0.6 and collision checks; this pass repairs shared Granny header compilation and clean-runner dependency order. |
| 45 | What's New UI | Saved default-on checkbox in Settings > General; gold NEW/cyan UPDATED accents and hover explanations across native OoT/MM and shared menus, with responsive narrow-column behavior and original IDs/callbacks/guards retained. |

## Separate asset installation and remaining runtime checks

- Rebuild the executable and normal `soh.o2r`/`2ship.o2r` from this source. Chateau's 4K stock resources and the eight Woods bridge resources are build inputs; old port archives do not acquire them automatically.
- Optional whole-scene Woods POC6 requires Alt Assets and must replace earlier Woods room overrides. The baked diagonal bridge is available without the full makeover archive; the complete nine-room makeover remains an external scene installation.
- The final combined Lost Woods avatar POC19 and native-solo forest avatar POC4 archives remain separate, along with the native/combined Black Market interior dependency. Use one selected replacement pack per NPC.
- Retain the existing grotto, sky, red-ice and regional Great Fairy archive dependencies from the dinner audit. No old full-source or whole-scene snapshot is reapplied over newer work.
- Configured in-game bridge/floor/canopy/seasonal/Alt switching and traversal remain unverified here, as do the Prelude first-frame freeze, boss-soul stray particles, young Malon final iris acceptance, earlier MM3D scene holds, final unavailable Demise revision, emote shortcuts/two-client behavior and the wider three-mode gameplay acceptance list.
- This source merge does not declare those reports fixed or promote an external scene/NPC candidate to an accepted runtime master.

## Verification record

All payload hashes in the recovered POC6 package and its source manifest match the exact donor. The independent cold asset rebuild reproduces the final visual archive byte-for-byte and passes its preserved Prelude/geometry/texture/coverage suite. Native asset regeneration preserves the 194 accepted bridge triangles and exact texture bytes; only the two authorized native canopy vertex Y fields change. Seasonal tests execute real XML FILEPATH handling for six images and nine rooms, preserving alpha/scale/protected colors, plus native/carried/replacement grass and canopy material ownership.

Local production fixtures, sanitizer probes, formatting/collision gates and hosted build receipts are recorded with `final-cumulative-checkpoint-20261019.json` and the final publication receipt. The previously recorded isolated Black Market item-fixture link hold is repaired by including the actual production TravelTools heal/grant bodies in that fixture; gameplay source is unchanged. Full application build status and runtime acceptance are not inferred from the focused checks. The original dinner/Saria/Chateau/UI evidence remains historical evidence for its exact checkpoint.
