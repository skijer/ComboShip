# Sword held meshes, GI presentation, and informative receipts candidate

Baseline: `a82852a3a02c8cabc541cce7c124da71c5755efb` on `develop` (daily polish PR #38).
Candidate branch: `poc/sword-held-upright-polish-20261008`. Keep this candidate separate from the accepted baseline until runtime acceptance.

The cumulative candidate also carries the later Summer atmosphere, autumn visibility/Alt foliage, Moon's Tear ownership, and cross-game Farore's Wind integrations from `bridge/combined-recovery-20261008`, including its final diagnostic dependency fixes at `3cee3121d2dd0f468938a6a39fd12eef11a5a899`. Its earlier sword snapshot is superseded by the final held-model priority, native MM carrier, and Four Sword shelf fixes here. The later integration delta was applied on top of those fixes; the recovery branch is retained as a merge parent. Feature-specific evidence and runtime limitations remain in the corresponding POC documents.

## Behavior

- Four Sword and the progressive sword families reuse the exact redesigned GI mesh/material graph as held weapons in OoT and MM. Resource-only grip matrices attach them to the native adult, child, or MM human fist. Held weapons do not add GI particles or shimmer. All nine source meshes and their materials remain byte-for-byte unchanged.
- Vanilla ComboShip held swords use the existing shipped-only `@oot-gi-base` owner, including every mesh, matrix, vertex, and texture dependency. Alt selection stays live. Selected body/hand/equipment models, progressive piece overrides, legacy Four Sword pairs, and the final Din draw stage retain priority. Missing authored graphs retain the existing fallback. Biggoron remains eligible at zero knife health; broken Giant Knife keeps its native model.
- Selected standalone custom sword GIs rotate +X to exactly +Y with a 90-degree Z rotation. Model coordinate conversion remains separate from award effects.
- Custom sword pickups and world displays grow 15% about their existing fitted frame center. The blade, Din core/flame layers, particles, and shimmer receive the same outer scale. Authored vanilla GI sizing remains at its accepted fit. Shop shelf scale, placement, pose, and effect footprint stay unchanged; the accepted 1.8-radian shelf recipe is preserved separately from the straight-up pickup pose. MM Goron receipts keep their current fitted size because enlarging them worsens an existing Kokiri shimmer edge cut-off.
- Ikana's native OoT no-mod GI fallback now queues the same live MM-owned shield graph that its adaptive orientation query measures. This removes the old permanently cached host-pointer mismatch. The existing flat-mesh correction, local mod priority, MM native/foreign recipes, authored shield fallback, and MM-owned icon remain intact. Missing MM data skips the GI rather than borrowing a host graph.
- Room Key is included in the MM native import catalog, so foreign rewards resolve its real identity instead of the blue-rupee sentinel. Both hosts use the supplied MM3D textured Room Key as an authored GI. The importer retains all 262 nondegenerate source triangles and the original diffuse texture pixels, with a fitted upright frame. Exact Room Key identity distinguishes it from other MM trade items sharing the same OoT callback. Existing legacy GI replacements retain priority, and unavailable authored resources retain Room Key's original GI callback. Its shop frame fits the existing shelf envelope.
- MM shares SoH's unchanged English, German, and French random rupee name pools and the default-on `gRandoEnhancements.RandomizeRupeeNames` setting. Queued native/foreign rupee receipts and the seven native award textboxes use the enhancement, including the 10-rupee award. Original rupee value, color, icon, grant, and native textbox headers are preserved. Nickname selection uses an independent cosmetic random generator; it does not consume seed/gameplay randomness. Turning the setting off takes effect without reloading. Native price/bank dialogue and dungeon silver-rupee puzzle names are excluded. MM currently forces English in its native message dispatch; the other locale builders are verified but do not establish additional runtime language support.
- Dungeon maps and compasses keep the existing middle pickup tier. MM's Junk Only setting preserves both native and imported information receipts; Everything But Major and Always may skip them. SoH's Skip Junk setting preserves native and imported maps and compasses, while Skip All may bypass them. Exact dungeon names cover older foreign seed metadata without changing item ownership, fill logic, grants, or disguised-trap handling.

The sword/GI/receipt changes do not change animation, melee reach, collision, save, inventory, equipment ownership, or transformations. Per-draw compound copies prevent later players/clones from changing an earlier deferred sword or body-color submission. The carried integrations have their own documented save and gameplay behavior.

## Verification

Focused gates cover production selectors and real OoT/MM graphics/resource types, shipped-owner isolation and repeated Alt toggles, all held attachment matrices, native fist/hand classification, deferred clone lifetime, Din priority, original fallback callbacks, and matrix/owner restoration. Native C/C++ syntax checks include the changed player seams, equipment/item unity, GI renderers, MM draw helpers, and the existing 19 receipt translation units.

The supplied-pack gate uses these unchanged authorized inputs; their geometry is not redistributed with the source candidate:

| Archive | SHA-256 |
| --- | --- |
| `Din_Swords_Fire_POC3.o2r` | `5f8ee79131ce25ed1bb87cff524cbf26187758d503a1710bc6c9cbfe05a60bbd` |
| `zz_Din_Sword_GI_Icon_ComboShip_MM_POC1.o2r` | `c18b9676cce48c0de9b3ea5be49ab08ea9bb8df4b631c943074ae34c5e10397c` |
| `zz_Din_Sword_GI_Icon_ComboShip_OOT_POC1.o2r` | `5c07946a5cbc786333dfd93307568209e32b5cf9eb3335de171493be294d5fae` |

Camera checks use the actual serialized vertices and resource matrices for all ten pack roots, complete Din layers, both host array factories, and all five MM Item0 receipt cameras. Separate checks project packed procedural vertices through the same cameras, rejecting new or worsened clipping. Existing Goron Kokiri/MM Kokiri shimmer clipping is retained, not marked fixed. Shelf geometry stays within the original `[-22,52]` Y envelope and 76-unit spinning width.

Final focused checks pass, including exact shelf baseline equality, all nine held graphs and three grip frames, the two native player unity units, both MM held seams, and OoT's native GI callback. The supplied meshes' worst projected coordinate is 0.840958 against a 0.99 limit. Independent review found no remaining important source defect within these scopes.

Added receipt/GI gates cover the actual Room Key import alias and authored binding in both hosts, legacy override priority, missing-resource callback fallback, all 62 authored GI frames, and the converted texture/mesh resources. Native MM imports cover 30 identities. Production receipt and native rupee-hook tests cover the original locale pools, all seven award IDs, amount/color/header retention, live settings, save guards, and exclusion of unrelated dialogue. Production MM/SoH pickup-queue tests verify that maps and compasses preserve their receipts at Skip Junk without changing skipped rupee behavior. Real-header syntax checks cover the new native hook, both GI renderers, the receipt builder, both pickup queues, the shared SoH rupee source, and the MM menu.

Across the recorded combined-regression run and continuations, 77 of 79 commands pass. Two unchanged historical comparisons remain blocked: `nei_used_fx` requires unavailable revision `c77c18587a976f6d6cb5c8f91f27593286469218`, and the icon-route comparison requires `122dd5f68cb37fcf515c3726f8a77043db5dcdf4`. Checkpoint 12 records the same blockers. The full combined suite is not claimed green. Fixture boundaries were updated for the new presentation helper/callback; missing local CMake dependencies were supplied, and those checks then passed. ASan/UBSan remain enabled where requested; leak detection is disabled because this container cannot inspect the required process data.

The recovery checkpoint records exact command results, source hashes, the source commit/tree, forward and reverse patches, and the incremental git bundle. Controlled compiler/sanitizer/graphics-boundary checks do not establish an executable build or running-game/GPU behavior. No runtime visual acceptance is claimed.

## Runtime acceptance

- Receive every sword tier and Four Sword with independent OoT/MM Alt settings; toggle after caches warm. Confirm upright blades and proportional pickup effects, including Din and the long Great Fairy/Biggoron variants.
- Inspect native child/adult, MM human, and MM adult held grips, both LODs, Four Sword clones, and sword swings/grass interactions. Confirm authored vanilla held models and existing mod/Din priority.
- Check shelf previews and purchases in both games for the unchanged scale and shelf clearance.
- Receive Ikana through native OoT/MM and foreign rewards, with the installed shield packs and Alt on/off. Confirm it is upright, the authored fallback stays upright, and dialogue/equipment use the MM Mirror Shield icon. The supplied screenshot confirms a horizontal shield but does not identify its selected resource graph; unsupported pack pose graphs still need installed-stack runtime proof.
- Receive Room Key natively and as a foreign reward in each host; inspect the authored MM3D key and installed legacy GI override, then test the original Room Key GI fallback with the authored graph unavailable.
- Receive ordinary currency and repeat/minigame native rupee awards with random names on/off. Check values and colors, nickname/player-name decoding, unchanged native headers, and unaffected bank/price/puzzle messages.
- With Skip Junk/Junk Only enabled, receive native and imported maps and compasses and confirm the information hint is visible. Confirm stronger skip settings retain their existing behavior.

Do not promote this branch to `develop` or master based solely on these source checks.
