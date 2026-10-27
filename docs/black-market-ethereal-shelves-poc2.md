# Black Market ethereal shelf material POC2

Baseline: original shelf-material source `3af965932120c31ab0d1024deb35ddba5d29f52a`,
restored standalone as `7a9b96d30423c0f4041502d3e4d8e5612ca56266`, tree
`80b359223ca77569d920b168f32dd59c6e5f9b4f`.

cor rejected the simple wavy shelf texture and requested detail comparable to
the ethereal shop, while keeping the offers readable. POC2 reuses the existing
packed 512x512 `custom/skiejer_market/texture/arcane_stone` resource. The brighter
frame uses consistent world-space texture density and diagonal scroll; the
darker backing uses a larger scale and horizontal scroll in the other direction.
The existing Black Market Shelves cosmetic row remains active in both engines;
its shared default is now RGB `(218,210,255)`. This is a separate shelf material
and animation using the room's existing image, rather than a newly painted image.

The approved shelf geometry, collision, stock positions, browsing cameras,
keeper, routes and transaction logic are unchanged. Granny's complete vertex
positions, colors, UVs, material assignments and external resource paths compare
exactly with the baseline. POC14 is unchanged; no new asset archive is required.
The magical pack must still be installed in the active native engine, as before.

The first procedural colored-cloud attempt was rejected during offline visual
review because its veins still looked too simple. It is not the delivered source.

Verification: both actual native GBI/Dyna/camera fixtures pass under ASan/UBSan,
including RGBA texture binding, both scroll directions, live/reset tint,
stationary Granny materials, missing Granny resource rejection and re-entry.
The source mesh is compared against the complete selected item/seal envelopes
and every keeper pose. Preview stills and motion use the packed image, shared
source UVs/colors and native browsing-camera fixture. No new screenshot collage
or replacement textbox is used. Lighting remains approximate.

This candidate is for appearance review. Full linking, configured gameplay,
live cosmetics/Alt parity and all mode combinations remain untested. No remote
push or master promotion. The later forest/market music checkpoint changes no
shelf files; apply this bounded patch to the cumulative source using the recorded
baseline, rather than replacing that newer checkout with the older snapshot.
