# Black Market original prop materials — 2026-10-22

The prepared default interior now has explicit material roles for all 28 retained groups. It contains 140 resources: 125 retained resources and 15 original texture uploads. The immutable native-market bundle remains unchanged. The eight new bitmap originals supplement the existing timber, charcoal slate and sage plaster; three deterministic packed atlases place their material patches on the retained mixed-role UV islands.

## Audit evidence

The audit follows the actual `custom/skiejer_market/room` calls, material selection, vertex cache, triangle connectivity and S/T ranges. Material numbers are not the suffixes of their triangle/vertex owners. Each group was rendered in isolation (`all-materials.png`) and compared with the supplied room reference `image(20261022-040928).png`; the wide entry, right bay and counter views then checked the assignments on the assembled room. `tools/black_market/assets/interior/material-audit.json` records exact owners, triangle counts, bounds, UV ranges, texture paths and the source PNG hashes. The retained POC14 donor SHA-256 is `3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`. Its replacement image payloads are not used.

All existing render, combine, alpha, fog and cull commands remain unchanged. Native opaque and texture-edge groups retain their original routes. Every added texture pixel is opaque; the wicker recesses are dark painted color. Steam and the two native pale-violet flames are separate runtime effect draws.

## Exact material map

All paths below are private resources shared by the MM and OoT prepared graphs. Uploads are 512×512 RGBA8; the final column gives the logical native UV period.

| Material | Actual retained owner | Triangles | Audited role | Texture path | UV period |
|---:|---|---:|---|---|---:|
| 00 | tri0 | 36 | front-right cauldron liquid | `custom/skiejer_market/texture/baseline_liquid` | 64 |
| 01 | tri23 | 62 | entry floor and threshold | `custom/skiejer_market/texture/baseline_slate` | 32 |
| 02 | tri15 | 42 | overhead hanging supports | `custom/skiejer_market/texture/baseline_bronze` | 64 |
| 03 | tri22 | 175 | ceiling and structural panels | `custom/skiejer_market/texture/baseline_timber` | 32 |
| 04 | tri1 | 612 | hanging bulb/root cluster | `custom/skiejer_market/texture/baseline_herbs` | 64 |
| 05 | tri14 | 370 | small bottles, basins and cabinet face | `custom/skiejer_market/texture/baseline_fittings_atlas` | 64 |
| 06 | tri9 | 16 | left wall hanging panel | `custom/skiejer_market/texture/baseline_cloth` | 64 |
| 07 | tri19 | 78 | rear draped curtain | `custom/skiejer_market/texture/baseline_cloth` | 64 |
| 08 | tri25 | 22 | right wall ledges | `custom/skiejer_market/texture/baseline_timber` | 32 |
| 09 | tri21 | 131 | wall infill panels | `custom/skiejer_market/texture/baseline_plaster` | 32 |
| 10 | tri17 | 1488 | seven hanging herb baskets | `custom/skiejer_market/texture/baseline_wicker` | 64 |
| 11 | tri11 | 1554 | wall panels, upper bundles/trim and small inserts | `custom/skiejer_market/texture/baseline_walltrim_atlas` | 64 |
| 12 | tri8 | 510 | small jars, bone curio and paper/label islands | `custom/skiejer_market/texture/baseline_curios` | 64 |
| 13 | tri7 | 24 | three right-side lattice screens | `custom/skiejer_market/texture/baseline_wicker` | 64 |
| 14 | tri3 | 72 | long hanging herb leaves | `custom/skiejer_market/texture/baseline_herbs` | 64 |
| 15 | tri26 | 218 | three faceted celestial star ornaments | `custom/skiejer_market/texture/baseline_celestial` | 64 |
| 16 | tri18 | 72 | middle-right and left cauldron liquids | `custom/skiejer_market/texture/baseline_liquid` | 64 |
| 17 | inactive | 0 | inactive retained material; no default draw owner | `custom/skiejer_market/texture/baseline_celestial` | 64 |
| 18 | tri5 | 122 | open book pages and standing leather books | `custom/skiejer_market/texture/baseline_curios` | 64 |
| 19 | tri6, room_boundary_repairs | 940 | room framing, cabinet wood and boundary repairs | `custom/skiejer_market/texture/baseline_timber` | 32 |
| 20 | tri4 | 36 | short hanging herb leaves | `custom/skiejer_market/texture/baseline_herbs` | 64 |
| 21 | tri2 | 1001 | four cauldron bodies, bronze torch stands and counter fittings | `custom/skiejer_market/texture/baseline_counter_atlas` | 64 |
| 22 | tri13 | 32 | floor runner | `custom/skiejer_market/texture/baseline_cloth` | 64 |
| 23 | tri24 | 174 | three capped ceramic canisters | `custom/skiejer_market/texture/baseline_ceramic` | 64 |
| 24 | tri20, room_boundary_repairs | 41 | main floor and boundary floor repair | `custom/skiejer_market/texture/baseline_slate` | 32 |
| 25 | tri10 | 13 | right-side hanging leaves and small retained insert | `custom/skiejer_market/texture/baseline_herbs` | 64 |
| 26 | tri12 | 42 | paper panels and bone/curio display islands | `custom/skiejer_market/texture/baseline_curios` | 64 |
| 27 | tri16 | 36 | entry-right cauldron liquid | `custom/skiejer_market/texture/baseline_liquid` | 64 |

The shared shelf fixture also receives `custom/skiejer_market/texture/arcane_stone`, an original slate upload with logical period 64. Material 17 has no default draw owner. Celestial material 15 owns `tri_26`; the retained `tri_27` is an unused duplicate mesh. Material 27 owns `tri_16`, a cauldron liquid surface.

## Mixed-role UV corrections

The cauldron/torch/counter group 21 uses `counter_atlas`: four glaze islands, a separate bronze stand strip and leather inserts. Group 05 uses `fittings_atlas` for the bottle row, basins and timber cabinet. Group 11 uses `walltrim_atlas` for plaster panels, woven upper details and small paper inserts. Their packed rectangles are recorded in `props-encoded.json`; their native S/T values are preserved.

The native curio UV ranges overlap distinct roles. Directly applying four material quadrants made the bone assembly sample glass, and one large paper panel crossed paper/leather while another sampled bone. Three bounded S/T-only adjustments fix this: vertex group 5 maps open pages into paper and standing books into leather; group 8 maps the bone curio into bone, the flat label/paper inserts into paper, and small vessels into celadon glass; group 12 maps the three large flat panels into paper and the associated curio details into bone. Each connected native component is affine-mapped into its padded quadrant. Positions, vertex colors, face indices and draw ownership stay unchanged. No source bundle is rewritten.

## Complete visible torch fixtures

The two disjoint 68-triangle native stand components are translated only in the prepared graph, together with their runtime flame anchors:

| Fixture | Native bounds | Prepared translation | Prepared bounds | Flame anchor |
|---|---|---|---|---|
| Right, 78 vertices | (111,0,-19)–(145,83,13) | (-55,0,+213) | (56,0,194)–(90,83,226) | (75,76,205) |
| Left, 73 vertices | (-156,0,-19)–(-122,83,13) | (+29,0,+213) | (-127,0,194)–(-93,83,226) | (-110,76,205) |

Stand UVs, colors, topology and source collision are preserved. Native small decorative flame scale 0.004 gives a full radius of 16 and height of 64: right bounds (59,76,189)–(91,140,221), left (-126,76,189)–(-94,140,221). The effects audit tested these complete stand/flame boxes against every prepared room triangle with 13-axis triangle/box SAT, excluding the stands themselves and supporting floor triangles at Y≤1; it found zero intersections. The existing close keeper camera may crop side decoration; entry/right-bay and the corresponding bank view show the fixtures.

## Original bitmap provenance

No reference image was passed to image generation. These are original flat albedo surfaces, not copied Djipi art or pictures of whole pots. Full exact prompts, dimensions, SHA-256 values and creation mode are retained in `props-provenance.json`. Existing architecture originals and their encoded stream remain unchanged.

| Original | Intended surface | SHA-256 |
|---|---|---|
| `ceramic.png` | Jade/teal glazed pottery | `ddd8ff5f0820253dfda4e74b7a8bb21419c01f7c8c4e53d2317a9288e1aff492` |
| `bronze.png` | Muted aged bronze fittings | `88b328e79785411d4774186427db01c84a520aa5e670b511bd093ee38d493d0a` |
| `wicker.png` | Opaque willow weave | `1d011ff044338bc4e8aa8d1b0dc9cf06a638201afad5df422fa20b3827f59a5e` |
| `celestial.png` | Deep blue, fine celestial pattern | `3d7180a9d4be05b4e32c04ae714a9c4d1faedbec04e250cc8226e0aa295ec235` |
| `herbs.png` | Dried herb surface | `c369e794d2baa820ae6ec728a0647c3a9c0b8f4cf671e9607fc04b48e2d5046d` |
| `curios.png` | Paper / burgundy leather / ivory bone / celadon glass quadrants | `a41a95e9e25ce409578eb0f021ebac14459c767b0aaa4b2b570d26245836716a` |
| `liquid.png` | Teal potion surface | `ff268814d6c3661ad4ee7b61caa822ab65df47977746a351480a5d3e4de269f7` |
| `cloth.png` | Burgundy woven fabric | `e1131499422a20291753a6a127c26dbec2bdb83184a2a07f6d78e183d14451da` |

The optional Pillow authoring step resamples originals to 512px and packs flat material patches. It writes 11 encoded prop/atlas uploads as one zlib stream split into two base64 source parts below the per-file publication limit. Ordinary asset preparation reads those verified streams using only Python’s standard library.

## Reproduction and checks

```sh
python3 -B tools/black_market/market_interior.py --rebake
python3 -B tools/black_market/market_interior.py --audit-output tools/black_market/assets/interior/material-audit.json
python3 -B -m unittest discover -s tests/native_market -p 'test_*.py' -v
python3 -B tools/black_market/preview_interior_materials.py --native-mm ../inputs/mm.o2r --output-dir ../review/prop-materials
```

The optional preview uses the actual prepared graph, Mesa preview lighting and production effect poses sampled with the user’s native MM cloud/fire textures. It emits entry, right-bay, counter and both bank views with explicit offline labels. It is asset inspection, not gameplay or configuration-specific runtime acceptance. Native MM/OoT visual acceptance remains unproven in this environment.
