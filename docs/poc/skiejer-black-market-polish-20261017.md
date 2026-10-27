# Skiejer Black Market polish and native solo access

Baseline source: `9a4c1aabe63e54d8d3874611ac57aaf79d89e76c`.
Candidate branch: `poc/skiejer-black-market-polish-20261017`.
The baseline recovery checkout remains separate. No shared branch, full application build, or master promotion is part of this candidate.

## User scope

Repair the visible upper-room void, add cosmetic-editor controls for the room textures, and show the actual goth Treasure Chest Shop Girl together with the merchandise. Preserve the complete Granny Potion Shop architecture and the accepted magical material treatment. Make the location accessible within ComboShip's combined, MM-only, and OoT-only modes with native actor and local-return routing. Standalone binaries outside ComboShip are not established by these changes.

## Room and cosmetics

The large roof void and thin shelf-side crack in the earlier preview were decoder errors. The existing native `Triangle1` commands already contain those faces, including the complete rear riser at Z398/Y189–204. The preview now decodes them correctly. Wide-angle inspection separately confirmed two genuinely missing shelf end faces at Z60 and a thin floor band. Seven added triangles close only those boundaries with their neighboring materials and UVs; they have no coplanar overlap with the original mesh.

The native factory accepts `Triangle1` with `V00/V01/V02/Flag0`; the old extractor's unsupported `Triangles1` alias also omitted original donor faces from collision. It visited an unused donor list, adding invisible interior obstacles. The candidate follows the actual root graph, restores 130 reachable donor faces, excludes 208 previously included unused faces and matches collision to all 7,819 drawn, nondegenerate room triangles. It preserves the original visual geometry and bounds, adding only the seven audited repair triangles. The asset contract checks every display-list command against the actual native factory's accepted tags. Independent tests reject missing/invisible collision faces and duplicate donor repairs.

The pack's native `CosmeticEntries` manifest adds **Cosmetic Editor → Mods → Skiejer's Black Market** in both engines. Violet Stone, Teal Stone, Rose Stone and Lavender Stone each control seven existing materials. Their original palette values remain the reset defaults. Existing color/rainbow/lock/reset behavior is used; these controls tint the texture rather than replace its image or change its scroll.

All original keeper resources, original mesh vertices/triangles and the source texture remain protected. Input donor archives and the accepted POC9 forest pack are unchanged.

## Keeper and merchandise

The private keeper is the exact Goth Treasure Chest Shop Girl donor: 23 limbs, 16 flex matrices, 24 joint entries and the packed 32-frame idle with blink display lists. MM replaces the carrier actor's init before native dispatch; OoT registers a dedicated ActorDB NPC profile. Neither starts a vanilla minigame/shop controller. Native skeleton drawing, attention/talk and message services remain engine-specific.

The eight displays are drawn by the special shop controller. They are floating magical offers, not spawned vanilla shelf actors. MM's synthetic shop identity and OoT's native `NeiGi_DrawShop` helper select the existing shelf-fitting paths. The controller owns Buy / Next offer / Leave, seed-fixed identities, price/lock checks, rupee deductions, exact grants and purchased bits. OoT latches native Shovel/Dominion ownership before replacing their shared slot, preserving consecutive purchases before pause-menu migration.

The display plane moves from Z82 to Z48 to clear hanging donor clutter; X/Y spacing and item scales remain unchanged. Seals use radii12/10 with a softer locked alpha105 and selected alpha150. Preview bounds cover a complete spin and bob, including the selected scale. Procedural GI effects and exact game lighting remain outside the offline geometry preview.

## Native access

| Mode | Route | State ownership |
| --- | --- | --- |
| Combined | Existing hidden MM opening-Lost-Woods doorway | Consolidated seed/save and existing local MM shop return |
| MM-only | Established Woods of Mystery room1 passage anchor, X160/Z-160 | Native MM save stock/clear bits and captured local entrance/room/position |
| OoT-only | Native OoT Lost Woods gateway into private scene0x6E, entrance block0x614–0x617 | Native OoT save section and local entrance/position |

Solo routes do not invoke a disabled engine or enable combined-only cross-game callbacks. The MM prologue exploration guards remain intact. Shop scene/entrance registrations have no entrance-shuffle pool records. Dedicated local RETURN handling must preserve unrelated respawn slots, including TOP; exact routing and persistence evidence is recorded with the candidate.

Solo stock is generated from the native seed and market options once, then saved with the native file. Native save sections retain stock identities, purchased bits, unique clear bits and the local return point. Combined seed ownership and opt-out behavior remain in the existing launcher path.

Saved-inside recovery is deferred until native Play initialization, after all load hooks, so entrance-randomizer and Remember Save Location hooks cannot overwrite the chosen local return. RETURN is staged with the latest native state preserved; TOP remains untouched. OoT publishes the last grounded return candidate only after waiting for its save worker, and its custom serializer uses the native snapshot rather than live play state. MM's corresponding native writer is synchronous.

The OoT scene uses private `scenes/shared/skiejer_market_oot/` headers and compatible common room/collision/model resources. It has no MM actor IDs, no vanilla shop actor list, no MM actor-cutscene command and a native idle spawn outside the shop exit trigger.

## Verification boundaries

Asset checks cover dependency closure, all28 scrolling materials, cosmetic bindings/defaults, visible-gap ray intersections, collision additions and actual native OoT scene-factory framing. The exact keeper skeleton, pose and item-fitting paths are used in the offline preview. Production-body fixtures and native object compilation are separate evidence from a complete linked executable.

The matching asset is `zzzz_Skiejer_Black_Market_POC11.o2r`, SHA-256 `d439e88073d6f8d5230d4ccff1aa744824be6b60975ac8469e1ba9fde9c346c2`. Both asset checks pass. The corrected preview reads 7,915 triangles including degenerate donor commands; 7,819 are nondegenerate and form the collision. The unannotated 114-degree view has no exposed background regions. All eight sample models' complete spin/bob envelopes clear the packed room, including the selected scale; this is not a full-catalog or normal/Alt runtime claim.

Six focused market suites pass: stock, price parity, cane, MM native, OoT native and OoT item adapters. ASan/UBSan fixtures exercise actual production bodies for local routes, slot/seed isolation, save/reload, void recovery, load-hook ordering, exact grants and worker ordering. A separate production scene-name fixture passes in both ComboShip and standalone SoH, protecting the appended scene from stats-array overflow while preserving the standalone table. The broad solo gate passes three compiled mode/options probes and all 18 production-boundary suites. Exact commands, results, source hashes and native-object checks accompany the recovery package.

Configured playthrough testing remains pending: both native keeper controllers and blink/talk behavior, purchases and special grants, mode-specific entries/exits, void and saved-inside recovery, normal/Alt parity, and actual game lighting/title display. No preview or source check is reported as runtime acceptance.
