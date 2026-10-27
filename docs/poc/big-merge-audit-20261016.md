# October 16 integration audit and recovery

The big merge is assembled on `integration/big-merge-20261016`. This is an
isolated source candidate. Accepted develop remains
`a82852a3a02c8cabc541cce7c124da71c5755efb`; cumulative source
`1bbb94e692629c0d910f97b57dd7cd02240d3775` is retained as the integration parent.
The interrupted audit and unfinished working changes were recovered in the
same checkout, rather than reconstructed from older exports.

## Included source

| Component | Included behavior and source ownership |
| --- | --- |
| Cumulative bridge | Existing SoH 9.3, NEI, seasonal weather, Zora fin/audio and earlier accepted-source contributions remain in the cumulative parent. |
| Hyrule Field repair | Retains the cold Alt-resource probe correction from `e48132064b`; no replacement scene geometry or texture data is invented. |
| Goron and Somaria | Retains audio/collider/grotto repairs and native/foreign Somaria GI ownership from `d5a8024b3`. |
| Four Sword and Byrna | Exact 35-file recovery import retained; 27 authored wrappers use the XML loader's `CallDisplayList`; Byrna retains its native action alias and separate stance hook. |
| Cross-game entrances | Cumulative dungeon, ordinary boss, expanded dungeon, interior, grotto/grave, native composition, clock/time admission, Farore, boss completion and recovery routes remain included. |
| Generation/reset | Cancellation joins the generator before reset/file-select reuse; original entrance identity and checked return metadata remain retained. |
| Shops | Latest v4 retail pool includes open storefronts. Retail belongs to Shops independently of broad/nonretail Door Interiors. Old recorded seed membership replays unchanged. |
| Native transitions | Outgoing animation/fade completes before engine handoff; return and recovery ownership remain separate. |
| Vanilla and Master Quest | Both quest assets, selected/random layouts, recorded quest capabilities and deterministic seed/save replay are retained. |
| Solo games | Combined, OoT-only and MM-only launcher modes; native vanilla/randomizer paths, selected-game ROM setup and separate save ownership. OoT retains MQ; solo MM retains its native Harpoon path. |
| Forest returns | Latest stump return, both forest arrival cameras, title card, safe playable opening Lost Woods and separate Woods of Mystery route from `b975551f8`. |
| Lost Woods weather | Weather child `fb84a0827` enables native/shared weather for the playable forest; latest POC3 mesh and private seasonal tint targets remain preserved. |
| OoT save editor | Updated NEI item entries inside the existing 9.3 editor layout. |
| Bundled emotes | All eight supplied animations, manifests/credits, controls, playback/audio cleanup and multiplayer pose groundwork. Both solo menus expose controls; MM PCM honors native float master/SFX volume. Two-client acceptance remains pending. |
| Config import | Separate exact OoT/MM mod-order strings, explicit empty orders and MM-only import ownership. Source JSON is preserved. |
| Shared seed settings | Native Stat Upgrades/RPG widgets and Map & Compass Information appear under Shared Seed Settings with original callbacks/seed guards. The relocated information control appears once. |
| Solo MM information rule | Append-only native seed option, default Off, saved/reloaded by native MM. Combined mode retains the authoritative saved OoT rule; solo MM does not query a dormant OoT context. |
| Map/compass hint ownership | Solo SoH keeps its native randomizer hints. Solo 2Ship reads its own saved rule and physical boss placements for collection messages and owned pause-menu descriptions. Dormant combined seed data cannot replace solo MM information. |

RPG pickups and configuration remain authored by the OoT randomizer in combined
mode. Relocating their controls does not implement a native solo-MM RPG item
pool. Native MM already exposes its Rod of Seasons, starting Rod, Hylia's Grace
and reward-gate settings; those existing ownership rules are preserved.

The requested hint integration is for the solo SoH and solo 2Ship randomizer
modes. Solo 2Ship exposes Map & Compass Information in its native hints menu;
the saved option applies to all four temples, including items owned from the
start. Off retains the ordinary collection text and suppresses the extra pause
information. Existing short option arrays load with the new rule Off.

The same source also serves combined mode. Its descriptors now read the recorded
cross-entrance permutation, compose with native OoT overrides, and read rewards
from the assigned physical boss check. This protects existing combined-mode
receipts while adding solo ownership. It does not change gameplay routing.

## Matching external MM scene packs

The separate scene-candidate package contains these six archives. They must be
installed with the matching source and effective MM Alt selection/load order.

| Scene | Selected candidate |
| --- | --- |
| Lost Woods | `zzzz_MM3D_Lost_Woods_Stump_Return_POC3.o2r` |
| Deku Palace | `zzzz_MM3D_Deku_Palace_Henriko_POC1.o2r` |
| Clock Tower interior | `zzzz_MM3D_Clock_Tower_Henriko_POC1_HByteScaleFix1.o2r` |
| Honey & Darling | `zzzz_MM3D_Honey_Darling_POC1_HByteScaleFix1.o2r` |
| Moon interior | `zzzz_MM3D_Moon_Interior_Henriko_POC2_HByteScaleFix1.o2r` |
| Regional Great Fairy fountains | `zz_Regional_Fairy_Fountains_3DS_POC1.o2r` |

The recovered scene audit records 733 distinct resource paths and no mutual
collisions among those packs. It reproduced half-width raw-texture uploads in
the three original final-scene packs: 168 original cases fail and all 168
metadata-repaired cases pass the production CPU renderer boundary with
ASan/UBSan. Only horizontal byte-scale metadata in 39 texture resources changes;
pixel data, geometry, collision, UVs and other resources remain preserved.
This is not GPU/gameplay acceptance. Older Lost Woods POC1/POC2 and uncorrected
final-scene archives must not shadow these candidates. The fountain output was
reproduced from its supplied source and matches its recorded original hash.

## Verification and recovery

The recovered canonical run completed 126/128 commands. Both failing harnesses
were reproduced here:

- Forest camera lacked the selected-game global after solo integration. The
  fixture now provides it and the complete forest runner passes.
- Used-effects comparisons required historical Shipwright revisions absent from
  the checkout. Twelve exact original files were recovered from the original
  repository, pinned under `tests/nei_used_fx/baseline`, and verified by their
  original Git blob hashes. The complete used-effects runner now passes without
  historical Git objects. No current output is substituted for the baseline.

The new shared menu and native MM option/save-migration fixtures pass under
ASan/UBSan. All four affected production translation units pass the actual
configured compiler checks. The real-header receipt syntax gate also passes.
Existing seed/receipt regression results and final application-build status are
recorded in the accompanying audit evidence. Recovered earlier results and
fresh focused reruns are identified separately; they are not one new full run.

Fresh solo verification passes three compiled mode/options probes and eighteen
production-boundary suites covering launcher selection, native menus, seed
loading, saves, logic and stale cross-route isolation. The native MM information
option/menu/save fixtures pass with ASan/UBSan in both combined-build and
standalone-build configurations. The actual MM receipt and owned pause composer
also pass all four solo temples with saved Off/On/Off, physical reward icons,
unchanged save data and stale combined seed data ignored.

Receipt verification passes the native catalogue, actual OoT donor/MM receiver,
information/save and tracker stages under ASan/UBSan. After the final solo-Off
fallback repair, the affected catalogue and receiver were rebuilt and rerun;
the remaining information/tracker stages then completed successfully. Earlier
unchanged driver stages passed before that repair. These are focused resumed
checks, not a claim of another complete 128-command canonical run.

The native Linux `ComboShip`, `soh` and `2ship` targets complete successfully,
including the shared UI. A final incremental invocation returns success with
no work remaining. This verifies the linked application targets; Windows,
custom-port-archive packaging and configured gameplay are separate checks.

Recovery01 retains the initial assembled source. Recovery02 captures its later
commits plus all unfinished shared-settings changes before this chat resumed.
Final Git source/tree identities and publication are recorded in the handoff.

## Merge blockers and remaining acceptance

1. **Latest Demise source is absent.** Exact final voiceless/dissipation revision
   `cfb9508f` has not been recovered. Older `c7987256` preserves its old audio and
   is not applied as a substitute. The requested final update remains excluded.
2. **Configured runtime acceptance remains pending.** Test vanilla/Alt paths,
   both solo games and existing combined saves, MQ replay, repeated forest
   handoffs/save/reload, installed scene priority, weather, all eight emotes and
   two-client transitions. Rebuild and deploy matching binaries and generated
   custom port archives together.

The earlier map/compass source finding is addressed. Both item owners use the
recorded routes in combined mode, including native-before-cross boss traversal,
physical reward names/icons and both Stone Tower orientations. Solo SoH keeps
its existing native entrance resolver. Solo MM bypasses the donor for ordinary
map/compass receipts while its own saved rule is Off; a dormant SoH On setting
therefore cannot enable those hints. The actual donor/receiver regression now
covers that Off/On/Off sequence across all four temples.

This checkpoint starts a reviewable merge workflow. It does not promote the
candidate to accepted master or claim every audited feature is proven in game.
