# Weekend development audit — October 16–18, 2026

Historical checkpoint record. See `final-cumulative-audit-20261019.md` for the
current cumulative source inventory, selected NPC revisions and verification.

The weekend source is assembled into the isolated branch `integration/weekend-audit-20261018`, based on GitHub PR #44’s exact head `43131342882972eeb4a257599ec8b061c7dc6d43`. It is a prepared source candidate for a later GitHub integration. The full regression gate, linked platform build, configured gameplay acceptance and master promotion remain pending.

The accepted `develop` baseline is still `a82852a3a02c8cabc541cce7c124da71c5755efb`. PR #44 and the earlier PR #41 remain separate remote drafts. This task has made no remote source, PR, deployment or master changes. Final source/tree identities and verification results are recorded in `source-checkpoint.json` in the merge kit.

## Source inventory

| Workstream | Source disposition | Evidence and remaining work |
| --- | --- | --- |
| Friday cumulative integration | Retained PR #44 as the parent | Existing SoH 9.3, NEI, seasonal weather, equipment, receipts, resource/cache and earlier fixes are retained. Do not merge the older PR #41 tree over this candidate. |
| Combo / OoT-only / MM-only launch modes | Retained Friday integration | Separate mode selection, native saves, vanilla/randomizer and imported settings remain. Fresh solo-mode gate and selected-game boundaries are checked; installed saves and transitions still need gameplay review. |
| Vanilla / Master Quest setup | Retained in PR #44 | Existing layout and ROM setup work stays included. No replacement with the earlier isolated MQ checkout. |
| Map / compass information | Retained in PR #44 | Full receipt checks exercise combined ownership and solo rules. The earlier 2Ship WIP is superseded. |
| Eight emotes, audio, credits and networking groundwork | Retained in PR #44 | Core, native-adapter and shared-menu checks pass. Two-client visibility remains untested. M1/M2 still mean configurable game modifier buttons; a replacement shortcut design was not implemented this weekend. |
| OoT NEI save editor and per-game mod-order import | Retained in PR #44 | Existing source and Friday evidence remain; no fresh complete editor/import UI acceptance is claimed. |
| Windows picker/build preflight corrections | Retained `4313134` parent | NOMINMAX and earlier build/preflight scheduling changes are already present. A new Windows build is still required for this cumulative candidate. |
| MM grotto return jump | Integrated published `5beac6b61` / `87736b239` | Preserves native jump arrival after explicit composed returns. Includes native return fixtures and recovery record. |
| Queued OoT dungeon/boss arrivals | Integrated `c519fffe3` | Composed arrivals retain precedence over ordinary Market/forest fallback and Farore recovery. Full forest arrival checks pass. |
| Foreign Master Sword receipt freeze and pickup audit | Applied cumulative pickup patch once | Includes replacement-loop repair, MM 120-glyph page budget, Chateau/save-text ID separation, supported glyphs, icon operands and extended IDs. Catalogue, sanitizers and complete receipt runner pass. Actual shop purchase remains untested. |
| Boss soul dialogue colors | Applied `c8f3218a` checkpoint delta | Explicit receipts for all 14 souls in both hosts; only the name span receives the dungeon color. Full receipts and both renderer-color checks pass; configured pickups remain untested. |
| Lost Woods portal, stump and mushroom seams | Retained cumulative source and final POC9 identity | Latest approved portal alignment is preserved. Earlier POC3–8 forest overlays are retired. Archive visual approval does not prove collision/traversal or save behavior. |
| Black Market, title cards, keeper, counters and item presentation | Integrated reconstructed source through `7a9b96d3` | Includes the native shop controllers, raised counter, continuous view, flat offer layout, fitted X locks and corrected orientation. Actual native transaction/controller fixtures pass. Required external pack is market POC14. |
| Optional Granny appearance and native browsing | Included with market source | External Djipi/native resource adapter; appearance defaults off and falls back safely. No converted Granny archive was introduced. Native browsing tests pass; installed visual parity is pending. |
| Physical shelves and detailed ethereal material | Integrated `532e2450` over later source | Both appearances retain the source-built shelves, detailed magical finish and cosmetic binding. No new shelf archive is required. |
| Native forest approach in both solo games | Included in reconstructed market/forest source | Private native scene copies, local returns, weather/seasons and save lifecycle are retained. Native spawn, return and complete solo/forest source gates pass. Requires the separate Native Solo Forest POC2 addon. |
| Forest / Black Market music entries | Integrated `df56c2c1` | Independent entries and native sequence/scene ownership in both hosts. Production collection and runtime fixtures pass. |
| Native arrival fade-ins | Integrated `8d9500ee` | Black Market, Lost Woods and Mystery routes use native arrival fades without overriding composed returns. Arrival-fade and forest checks pass. |
| Rain thunder and scene/generation music shuffle | Integrated `78e44582` | Fixes thunder decoder end offset, adds the Field night row, and preserves replay/preview ownership. Native mixer/decoder, rain, night, scene and shuffle checks pass. Audible rain/BGM comparison is pending; seasonal-only rain intentionally does not request added thunder. |
| Prelude-placeable MM monkey in OoT | Integrated `810c51fb` and `1251d4ef` | Native high-jump, idle and scratch poses, blink and stationary dialogue host. Uses the existing native MM donor resources; no new monkey pack. |
| Saria placements, seated song gift and green shimmer | Integrated through `1e650f7f`, plus captured solo changes | Includes scenery/waiting and seated variants, fixed one-time song gift, placement anchors, shimmer and local MM resource snapshots. Combined and both solo route guards are covered. Placement, gift, sound and appearance still need installed review. |
| Portal-side Epona and second-clearing Skull Kid | Integrated `1e650f7f`, `4b26143b`, plus captured solo changes | Includes reserved actor hosts and automatic/authored override guards in Combo and both solo modes. Skull Kid uses the requested reclining/crossed-leg cutscene pose. Native callbacks and mode guards pass; placement is provisional until seen in game. |
| Native MM transformation sequences in OoT / solo SoH | Integrated `19bff493` and `8dc496de` | Deku/Goron/Zora/Fierce Deity animation and native SFX sequence/lifecycle handling. Combo and solo controller cases and native asset closure are checked. GPU appearance, timing and audible output remain untested. |
| Din’s Fire shield ground clearance | Integrated published `100aaa715` | Both hosts retain the stance-dependent clearance change. Shield and MM fire/damage checks pass; slopes, crouching and installed appearance remain untested. |
| Native NPC adapter, Saria and twins source | Integrated all 12 commits through `ae025b3d` | Preserves pose sampling, weighted joints, blink/expression routes, corrected Saria grip/standing/leg color and HD ocarina authoring. Latest Saria is POC11. Twins POC4’s neutral face remains rejected and must not be treated as visually approved. |
| Young/adult Malon poses, scarf/brooch and tracking | Reconciled latest frozen POC7 compiler with `eb9ac7d1` | This work was on separate branches/snapshots. Both approved Malon packs rebuild resource-identically; tracking, blink wrappers, sampler and real actor checks pass. Saria/twins compiler output is preserved exactly. |
| Young Fado pose repair | Existing adapter retained; POC2 source/pack identified | Current NPC source already contains the matching donor header. Exact POC2 asset rebuild and four archive checks passed; the original pack is separately available. Do not apply the old full Shipwright donor patch wholesale. |

The originally uncommitted forest routing was captured before integration in `forest-solo-uncommitted.patch` plus its new source files. The combined Malon patch is based on `ae025b3d`; it already contains the tracking delta, so that delta must not be applied a second time. Recovered standalone snapshot roots were converted to bounded deltas rather than merged as unrelated histories.

## Archive and authoring inventory

| Workstream | Selected candidate | Merge / acceptance boundary |
| --- | --- | --- |
| Grotto editing recovery | POC7 retained historically | Repairs the earlier export’s editable material bindings. POC8 used the later user authoring export, so POC7 is not its byte parent. |
| Elemental grotto scrolling, including ice room 6 | POC8 changes retained inside POC9 | 44 material-motion routes, nine shared small patterns. Original authoring entries and saved edits are preserved; no game-source change is required. |
| Actual water texture scrolling in rooms 0 and 10 | `OoT-Grotto-Water-Texture-Scroll-POC9.prelude.o2r` | Fresh 14-room draw comparison covers 201 calls / 10,154 triangles, preserving geometry/UV/color/indices and unrelated draws. The actual water images scroll. Native rendering, pools and re-entry remain untested. |
| Shadow / replacement grotto texture concepts | Room 02/09/12 concept ZIP retained | Authoring options, not automatically installed or accepted room replacements. |
| Djipi red-ice cosmetics | Red Ice POC1 addon | Five checks and default-red pixel comparison pass. Existing actor RGB/alpha source already supplies the cosmetic; no executable patch. Keep Djipi07/08, addon last, Alt on. |
| Day, night, overcast, dawn and dusk skies | Luminous Skies POC1 Universal | Eight zenith checks and current production-renderer fixture pass. One replacement pack; MM also needs Use OoT Sky Textures. Overcast was accepted provisionally for packaging, not proven in game. |
| Friday MM3D scene ports and regional fountains | Exact six-pack manifest retained separately | Supporting runtime source is already cumulative. These are external O2Rs, not included by the Git bundle. Friday Lost Woods POC3 is superseded by POC9. |
| Clock / Honey / Moon texture metadata repair | `_HByteScaleFix1` identities and recipes recovered | Repairs 39 texture-width metadata fields while preserving pixels/geometry/collision; historical CPU checks record 168 failing originals /168 passing repairs. Corrected pack bytes and original Final-Scenes POC2 ZIP remain unrecovered in this audit. |
| Model reference art and Tripo authoring | Existing reference GLBs/art retained | Model-generation inputs and visual review are separate from source compilation and live actor behavior. |
| Storage cleanup / retained work history | Earlier cleanup CSV and archived histories retained | No new cleanup or deletion of original work was performed for this merge. The handoff reuses existing large packs instead of duplicating them. |

The archive appendix gives exact hashes, paths, required packs/settings and restoration recipes. Latest active choices are combined forest POC9, market POC14, solo forest POC2, grotto POC9, Saria POC11, young Malon POC7, adult Malon POC5, and Fado NPC POC2. Keep one version per actor/scene; use one Saria alternative. Full POC11 already contains the HD ocarina; the separate HD overlay belongs only above its matching POC10 base.

## Integration repairs made during this audit

- Combined the native-NPC regression hook with the existing solo/emote/pickup CI hooks; retained both sides of the only content conflict.
- Reconciled Malon’s frozen compiler and tracking branch with current Saria authoring, instead of replacing the newer shared scripts with older copies. All five output comparisons preserve the intended resource sets.
- Repaired fixture include paths for current combined headers. The original failures and successful reruns are recorded; no production behavior was changed for those fixes.
- Added the forest, market, scene-music and shield suites to the source regression gate. The archive-backed rotation check stays a separate local check because hosted source CI does not supply native game archives.
- Independent review found 87 imported files failing the mandatory format gate. Exact clang-format 14.0.6 now passes all 169 changed files selected by that gate. Formatting outside the repository gate was reverted.

## Verification and limits

The final focused pass completed **32 distinct commands**, **24 forest/native actor object builds**, **12 audio object builds**, and the **169-file clang-format 14 gate**. `verification-summary.json` lists the exact latest commands, source heads, logs and results. Checks cover the forest/solo/arrival boundaries, both shop hosts, shelves/browsing/Granny, music/shuffle/rain/night, transformations, shield/damage, complete receipts, NPC callbacks and emotes. Address/undefined-behavior sanitizers are used where the existing runners support them; local LeakSanitizer is disabled. Native objects are compiled against real engine and retained third-party headers, separately from game linkage.

The reconciled NPC compiler passed 89 Python tests with no skips, 32,768 routing cases and 1,442 tracking checks. Resource comparisons match adult POC5 (286), young POC7 (337), and the current Saria Anime (351), Saria Render (361) and twins (286) compiler outputs. Those Saria comparisons preserve the current compiler behavior; final exported POC11 refinements still require their recorded refinement scripts. MM Saria’s rigid/private-pose draw path does not yet establish parity with SoH’s new live weighted skin path.

Independent review found no additional critical/important defect in source registration, actor dispatch, explicit resource ownership, solo guards, or arrival precedence. That is a scoped source review, not complete runtime validation.

The complete canonical regression gate and a linked Windows/Linux application build were not rerun in this preparation task. The new candidate has not run through hosted CI. Earlier builds and package links belong to earlier commits and do not contain this whole weekend candidate. Runtime acceptance remains pending for the actual installed packs, settings, modes, save state and GPU/audio path.

## Holds and next decisive checks

1. **Asset holds:** twins POC4’s rejected face; Palace’s omitted 11 object-local components and collision gaps; Honey’s missing native wall/floor cycling; Moon bind-pose instances; remaining scene fitting. The exact repaired Clock/Honey/Moon packs must be recovered or rebuilt from their pinned inputs before installation. These holds do not erase the prepared source work.
2. **Inherited exclusions:** the unavailable final Demise revision `cfb9508f` stays excluded. Earlier unresolved Spring-night lighting and Zora material reports have no fresh installed acceptance in this audit.
3. **Source/build gate:** restore the bundle onto the pinned PR44 parent, run the full canonical regression gate, then build matching binaries and generated `soh.o2r` / `2ship.o2r` together. The local preparation checkout is sparse to limit storage; an ordinary restored clone has the full tracked tree.
4. **Runtime route matrix:** Combo, solo SoH and solo MM → both forest rooms → shop → stump/Clock exits, then save/reload, re-entry, void/death and Farore recovery. Test the required native/Alt configurations and absent-addon recovery.
5. **Runtime polish:** buy the foreign Master Sword and collect native/foreign souls; test each native transformation with audio; listen to thunder and Field-night shuffle; review shield clearance and NPC poses/blink/tracking/shimmer. Check emotes across two clients separately.

The source candidate is prepared for review and the next integration build. Accepted masters and external scene archives were not promoted by implication.

## Existing asset downloads

These are the existing separately saved files; the source kit does not duplicate their large archive bytes. Native and broad-pack dependencies/settings remain as documented in the asset appendix.

| File | Size |
| --- | --- |
| [zzzz_MM3D_Lost_Woods_Right_Nudge_POC9.o2r](https://chatgpt.com/api/library/files/libfile_b30c146060f88191b559aa07132f3765/download) | 116.57 MB |
| [zzzz_Skiejer_Native_Solo_Forest_POC2.o2r](https://chatgpt.com/api/library/files/libfile_d38c8cb30a008191804860127369960b/download) | 116.60 MB |
| [zzzz_Skiejer_Black_Market_POC14_EntranceOrientation.o2r](https://chatgpt.com/api/library/files/libfile_1d3053804d1081918189648843b8a585/download) | 9.44 MB |
| [OoT-Grotto-Water-Texture-Scroll-POC9.prelude.o2r](https://chatgpt.com/api/library/files/libfile_9c0f729f578c81919d7fa211ccd3620f/download) | 359.01 MB |
| [zzzLuminous-Skies-POC1-Universal.o2r](https://chatgpt.com/api/library/files/libfile_6934654c9d2c8191bc992ec04e2bf9eb/download) | 49.17 MB |
| [zzzz_Djipi_RedIce_Cosmetics_POC1.o2r](https://chatgpt.com/api/library/files/libfile_7a6ed7dbc9e081918638655145d06fda/download) | 0.17 MB |
| [Saria_Native_NPC_POC11_Review.zip](https://chatgpt.com/api/library/files/libfile_af05821e37a08191a2d04bd71e551c3e/download) | 61.22 MB |
| [Young_Malon_POC7_Review.zip](https://chatgpt.com/api/library/files/libfile_8f6d95dbcb7081918153e55c1c4b5662/download) | 42.68 MB |
| [Malon_POC5_Review.zip](https://chatgpt.com/api/library/files/libfile_a2e8487a01c08191a849368df9374605/download) | 69.14 MB |
| [Young_Fado_SoH_NPC_POC2_Pose_Fit.o2r](https://chatgpt.com/api/library/files/libfile_d39bee9fc8148191a71c0317e00e9d89/download) | 22.11 MB |
| [Luminous-Skies-POC1-Recovery.zip](https://chatgpt.com/api/library/files/libfile_75e365e5d8488191bfa298cc473a503c/download) | 70.61 MB |
| [Regional_Fairy_Fountains_POC1_Source_Checkpoint.zip](https://chatgpt.com/api/library/files/libfile_057fd07f4c6481919a9282d9282e2e3d/download) | 19.04 MB |

## Work observed after the main snapshot

The original forest workspace advanced to `fe129929` during this audit. Its final exact MM resource-loader provider and mode documentation are now included; the provider compiles against the real API and matches the snapshot consumer’s C++ symbol. The resource test also exercises the actual native donor and both POC11 Saria packs.

A newer Anime Saria POC12 smile/waiting refinement is still evolving in the original workspace. `Newer-Saria-Refinement-WIP-20261018.zip` captures one timestamped source state separately. Later receipts refer to different pack hashes and still report right-hand/left-arm contact intersections. This WIP requires reconciliation with the combined Malon compiler before merging; it is not part of the prepared source branch or an accepted asset replacement. See the NPC follow-up appendix.
