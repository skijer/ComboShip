# ComboShip cumulative merge audit — October 19, 2026

This is the current inclusion checklist for the merge into `develop`. It supersedes the October 18 inventory where newer checkpoints are identified below. **Source integration, external asset installation, successful builds, and gameplay acceptance are separate statuses.** The merge is being prepared as a draft candidate so cor can add any forgotten items.

## Exact starting points

| Role | Checkpoint |
| --- | --- |
| Protected develop baseline | `a82852a3a02c8cabc541cce7c124da71c5755efb` |
| Existing cumulative PR | [PR #44](https://github.com/marsh6487/ComboShip/pull/44), initially `31ea4e711f13f1e5b2b1b9ac9b36ea43bc13b18b` |
| Existing cumulative tree | `bbc04bc7603296c9e8a5b69d5c0a2f81127ba45e` |
| Later published forest/Mirror source | `5ec0aeda5f77d2f1645fc4d63072f4b8e5802e94`, tree `fc0fe09166e1f444f3a17e2ed2b6178fe8e53926` |
| Local equivalent forest checkpoint | `b9f8c3be20a9d7d95f2715c004cc139e8330d2ea`; exactly the same tree as the published source |
| Recovery / integration branch | `integration/dinner-audit-20261019`, created on GitHub at the published forest checkpoint |
| Woods of Mystery donor | `dbd2f6ec54777b72e8f38a5c30642b7053e48b90`; only its Woods delta after `de044b300` is applied |
| Current publication | The candidate combines the later forest source with the bounded Woods/preflight delta. Final source/tree and publication receipts are recorded separately. |

GitHub confirms that the later forest/Mirror checkpoint is six commits ahead of PR #44, with no divergence. The older weekend bundle, elemental swords, Chateau POC1, blush grass and thawed Domain changes are already represented in PR #44 and are retained without applying their old full snapshots again.

## Included source — complete workstream checklist

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
| 27 | Saria forest behavior | Scenery/waiting/seated variants, one-time song gift, Prelude placement anchors, green shimmer and local MM resource snapshots, with all three mode guards. |
| 28 | Epona and Skull Kid | Portal-side Epona and second-clearing crossed-leg/reclining Skull Kid placement, actor hosts and authored override guards. Placement still needs installed review. |
| 29 | MM monkey in OoT | Prelude-placeable native MM monkey: idle, high-jump and scratch poses, blink and stationary dialogue. |
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
| 42 | Woods of Mystery native support | Pack/room/Alt/mode guards, legacy passage suppression and bounded animated green hollow hexes for the selected visual bridge. POC4 asset recipes/tests are retained separately from gameplay acceptance. |
| 43 | Chateau Romani bundled source | MM3D model in both ports, native/foreign item routing, gold/burgundy shimmer and preserved tutorial receipt behavior. |
| 44 | Integration/build maintenance | Existing canonical test registration and fixture repairs, clang-format 14.0.6 and collision checks; this pass repairs shared Granny header compilation and clean-runner dependency order. |

## Matching external assets and source-only boundaries

A source merge does not install whole-scene/NPC texture archives. These are part of the release/configuration checklist and must be selected deliberately; obsolete whole-scene copies must not shadow newer ones.

| Asset workstream | Current checkpoint / selection | Status or installation boundary |
| --- | --- | --- |
| Combined final Lost Woods | `zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r` | Supersedes earlier POC9/14/17/18 whole forests. Contains final authored shop/bench/stump work, fixed native scroll address flags and avatar poster. Requires matching source, existing Market interior pack and MM Alt Assets. |
| Both native solo forests | `zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r` | Supersedes solo POC2. Use in the selected game's mods with matching source and native Market pack. |
| Woods of Mystery | `zzzzz_Woods_of_Mystery_OoT_Makeover_POC4.prelude.o2r` | Latest recovered checkpoint: recovered OoT wood tunnels, fitted bridge/walls, Henriko foliage, opaque backing and preservation recipes. POC3 sky coverage and POC2 bridge shape were accepted; POC4 final appearance is not yet accepted. Latest reported black backdrops/gaps stay open. |
| Black Market interior | Retained existing native/combined Market interior archive | FINAL replaces exterior scene data; it does not remove the separate interior dependency. |
| Chateau 4K glass upgrade | `Chateau-Romani-MM3D-4K-POC2.o2r` inside `ComboShip-Chateau-Romani-4K-Glass-20261019.zip` | **Separate HD addon/source checkpoint, not baked into this candidate's stock port archives.** 4096 atlas, cork detail, 512 glass reflection and reduced glass opacity; same UVs/geometry, shimmer and tutorial. Can accompany the POC1-capable executable with Alt Assets on. Baking it into stock `soh.o2r`/`2ship.o2r` is outstanding. |
| Elemental grotto | `OoT-Grotto-Water-Texture-Scroll-POC9.prelude.o2r` | Includes the elemental material work, ice room 6 and actual water textures scrolling in rooms 0/10. Archive work; no extra executable feature is implied. |
| Luminous skies | Universal POC1 | Day, night, overcast, dawn and dusk. MM requires Use OoT Sky Textures. Overcast acceptance remains provisional/configuration-specific. |
| Djipi red ice | Red Ice Cosmetics POC1 | Load after Djipi07/08, Alt Assets on. Existing source already supplies cosmetic RGB/alpha. |
| Native NPCs | Saria Anime POC12 or Traditional POC11; twins POC9; young Malon POC12; adult Malon POC5; Fado POC2 | One version/alternative per actor. Accepted twins are retained; young Malon final iris still awaits acceptance. |
| Other MM3D scenes | Deku Palace, Clock Tower interior, Honey & Darling, Moon interior | Source support and retained authoring checkpoints included; repaired texture-width packs and scene-specific holds remain below. |
| Regional Great Fairy fountains | Regional 3DS POC1 | Existing separate archive/checkpoint retained. |
| BossSoul flame control | `BossSoul-Flame-GI-Control-20261019.zip` | Optional A/B diagnostic only. Not enabled by default and not a confirmed stray-particle fix. |
| Grotto texture concepts | Room 02/09/12 concepts | Retained authoring options, not automatically installed replacements. |

### Exact newest scene/addon hashes

| File | SHA-256 |
| --- | --- |
| Combined FINAL avatar POC19 | `9097583c896a11c204e4950bd9a80aa8e69fb2a42307f55f50e75d4195f30e42` |
| Solo FINAL avatar POC4 | `65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e` |
| Woods of Mystery POC4 | `2fdafaae09435525f135936adab670c82758bea7596b0fac81c6df9161b48cfa` |
| Chateau HD POC2 addon | `81174120f18e888f52659e21f0bb5bf8f21a66daa1097886d7a663bd3c7076eb` |
| Immutable final authored Lost Woods input | `89bdb0ce3ce2e22ca7e4aed438251a66a167ee8b1d68a7b1b019a05c35ac34f9` |

The final forest source archive is pinned at SHA-256 `f8880c6ed580215487c5b06dd01a887afe21e4e952dff4c5c6ef6fe32a2a73f3`; Woods POC4 source recovery at `6a5eee1ea765753598c8ca080ccb99468ccb17a9c4a15ef9b9c281e1fd2dd66e`; Chateau 4K source recovery at `18547dd53de7796cf53969a03e97f94eb6a1a8a7dd9f744ebcb62ccd59948556`. The recovery package records all collected checkpoint hashes and restoration instructions.

## Outstanding work and explicit exclusions

| Item | What remains |
| --- | --- |
| Matching full application builds | PR44's `31ea4e7` hosted runs have now completed with failures: Windows UI found unavailable `gsSP2Triangles`; the Linux gate compiled the renderer before installing `nlohmann/json.hpp`. Local reproductions identify both. Both narrow fixes pass their local reproductions in this integration pass; fresh hosted Windows/Linux binaries and both generated port archives remain required. |
| Prelude freeze | The reported first-frame Lost Woods freeze has not been established as Prelude-only or declared solved. The native segment repair and correct bindings are source/static evidence; actual load/render testing is still needed. |
| Woods of Mystery tunnel finish | Latest clips report remaining gaps/black backdrop at the swamp exit and OoT/2Ship bridge. POC4 has static seam/ray coverage evidence; latest configured appearance and traversal remain open. |
| Chateau HD packaging | HD addon/source is preserved and can be used separately. Stock port-archive baking and actual translucent glass/label/cork appearance, drinking/purchase, tutorial and foreign receipt need verification. |
| Boss-soul stray particles | The optional control isolates the symptom; no confirmed production repair is merged. Preserve the approved flame artwork. |
| Young Malon | Final POC12 iris needs cor's acceptance; source recipes/art are retained. |
| Older MM3D scene holds | Exact Clock/Honey/Moon `_HByteScaleFix1` bytes need recovery or reproducible rebuilding. Palace omitted object-local components/collision gaps, Honey wall/floor cycling, Moon bind-pose instances/tree UV polish and other fitting are not newly fixed here. |
| Latest Demise revision | Exact unavailable `cfb9508f` remains excluded; older audio-bearing revision is not substituted. |
| Seasonal/Zora presentation | Existing installed Spring-night lighting and Zora material reports lack fresh acceptance. |
| Emotes | Replacement shortcut design is unimplemented; two-client visibility/transitions remain unproven. |
| Existing isolated item fixture | The final forest handoff records `tests/black_market/run_oot_items_tests.py` missing its `TravelTools_Grant` link dependency. This is a fixture hold, not proof of an in-game item failure; it is not counted as passing. |
| Final gameplay acceptance | All three modes; both forest rooms/shop/local and cross-game exits; save/reload/re-entry/void/death/Farore; absent-addon recovery; selected native/Alt packs; pickups, soul colors, transformations/SFX, shield, NPC poses/blink/tracking/gift/shimmer, weather/thunder/music, grass/Domain and Mirror pickup/cast/recall. |

## Verification evidence and limits

The prior frozen PR44 audit records successful coverage of 149 canonical command entries across resumed segments/workers; it was not an uninterrupted serial replay. NPC discovery recorded 103 passes and 15 external-archive skips, plus a separate optional Fados archive skip. These historical results are retained as historical evidence.

This integration pass completed **23 fresh focused commands, all exit 0**, covering the actual shared Granny header in UI/default and engine GBI configurations, the real renderer object's C interpolation linkage, Woods effect/active-pack ownership, final forest bindings and native shop/door paths, and Mirror runtime/inventory/save/sync/visual fixtures. Exact commands, exit codes and logs are retained with the integration evidence. Workflow parsing/order, shell syntax, diff whitespace, exact clang-format 14.0.6 for all four changed C/C++ files and the custom asset collision gate also pass (1,914 shared paths, no new collision).

The Granny header repair replaces each two-triangle cap macro with the same two single-triangle commands. Vertex inputs, triangle winding and materials stay the same. Gate dependency installation moves ahead of the renderer compile. Neither correction establishes a complete Windows/Linux build.

No new configured gameplay, linked executable, GPU appearance, audible output or accepted-master promotion is claimed. `develop` remains unchanged. Before release, use a build and generated assets from the exact final source.

Independent source review found no Critical or Important issue and accepted draft publication. One minor test gap is deferred: the selected Woods pack's legacy-arch suppression is inspected in source but is not directly exercised by the passage fixture. This limits that coverage claim; it is not a reported runtime failure. GPU appearance, traversal, audio, Prelude load, external-pack compatibility, full application linkage and user acceptance were not judged by that review.

## Source records

- Current baseline inclusion: `docs/poc/final-cumulative-audit-20261019.md`.
- Earlier detailed asset/setup evidence: `docs/poc/weekend-source-audit-20261018.md` and its saved weekend merge kit.
- Upstream/retained merge details: `docs/poc/big-merge-audit-20261016.md` and `docs/poc/soh93-merge-20261012.md`.
- Later forest/door record: `docs/poc/final-forest-solo-scroll-20261019.md` and `docs/poc/lost-woods-final-door-20261019.md`.
- Mirror safety/ownership/arrival details: `docs/poc-magic-mirror-20261019.md`.
- Woods preservation/rebuild: `tools/woods_mystery/README.md` and saved POC4 source recovery.
