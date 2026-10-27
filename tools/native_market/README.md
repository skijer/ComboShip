# Native Black Market geometry

This source bundle makes the existing market scene available through the normal
`soh.o2r` and `2ship.o2r` custom-asset inputs. No supplemental market archive is
required. Include `CMake/NativeMarketAssets.cmake` and make the ordinary archive
generation targets depend on `PrepareNativeMarketAssets`.

The immutable donor is
`zzzz_Skiejer_Black_Market_POC14_EntranceOrientation.o2r`, SHA-256
`3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`.
Its 124 retained resource identities cover the room, entrance, geometry, vertices,
collision and both games' scene/room headers. All non-material payloads remain
byte-identical, including vertex UVs/colors, collision and player/keeper spawns.
The additional marker is `custom/skiejer_market/native_geometry_v1`.

Within the immutable source bundle, the 28 material XML resources keep their public identities but use the existing
stock MM 32×32 RGBA16 bark resource
`scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0`, neutral white primitive
color, ordinary opaque rendering and static wrapping. Tile masks/line sizes and
load dimensions match that stock texture. No texture pixels are copied. The
source bundle excludes optional textures, magical masks, scrolling, cosmetics,
custom keeper assets, and the title image. This is the preserved geometry
input, not the final prepared interior appearance.

Normal preparation applies the original architecture and prop materials,
audited prop UV corrections and relocated torch stands, then adds the original
IA8 scene-title sprite and each host's native handle-door transition record.
It produces 141 resources for each port archive. Collision, room headers,
spawn payloads and topology remain unchanged; only the two scene headers gain
the native transition actor command before player initialization. The immutable
125-resource bundle and its manifest remain untouched by these passes.
See `docs/poc/market-forest-followup-20261022.md` for the scoped preservation
contracts and native verification. No Djipi texture or supplemental market
archive is needed for this default prepared appearance.

Runtime integration must select the existing native keeper fallback when custom
keeper assets are absent: `objects/object_bg/gTreasureChestShopGalSkel`, native
`object_bg_Anim_009890`, open/half/closed up-looking eyes and its native TLUT.
OoT must mount its normal base MM asset archive before checking those resources.
The builder's `core_contract_header(resources)` returns separate
`kNativeBlackMarketMmResources` and `kNativeBlackMarketOotResources` arrays for
the runtime owner's generated header. Each requires its own host's scene/room,
shared geometry, native marker, original default surfaces/title and native
keeper essentials. Neither requires the other host's scene headers or optional
cosmetics. Optional packs keep their ordinary override behavior; old private
scene/room/material/vertex/title overrides must be excluded to review these
defaults and the native exit door.

Rebuild the checked-in deterministic source from the immutable donor:

```sh
python3 -B tools/black_market/native_market.py /path/to/zzzz_Skiejer_Black_Market_POC14_EntranceOrientation.o2r --bundle-dir tools/native_market/bundled
```

The source is a sorted, uncompressed ZIP serialization compressed once with
zlib and encoded as base64 for source control; it is not an installable O2R.
The adjacent manifest records compressed/decoded and per-resource sizes/hashes,
plus preservation receipts against the donor. Preparation checks every resource
and the XML dependency closure before writing either game's asset tree, uses
atomic file replacement, skips identical files and repairs changed outputs.

```sh
python3 -B scripts/assets/prepare_native_market.py --output-root /path/to/source-tree
python3 -B -m unittest discover -s tests/native_market -p 'test_*.py' -v
```

The resource tests compare the real donor, rebuild the checked-in bundle, check
geometry/UV/collision/header preservation and static material dependencies, and
exercise corruption rejection and idempotent preparation. They do not establish
configured game rendering, room traversal, keeper presentation, optional pack
priority, or player/camera behavior. Native/runtime integration is owned by the
matching source candidate and requires its own checks.
