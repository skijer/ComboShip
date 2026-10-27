# Skiejer Black Market asset candidate

## Current default source preparation

The source candidate `poc/market-forest-followup-20261022` prepares its default
market through `scripts/assets/prepare_native_market.py` and the normal
`PrepareNativeMarketAssets` CMake dependency. Both port archives receive the
retained room, original architecture/prop/shelf materials, original IA8 title
with a black lettering edge, and a native interior handle door. Native pot
vapor and pale violet torch effects are supplied by the controllers. This path
does not require a supplemental market pack or any Djipi texture. An authored
keeper remains optional; the existing native keeper is the fallback.

The prepared graph has 141 resources; its immutable 125-resource geometry input
is preserved separately. Old private scene/room/material/vertex/title overrides
can replace the new defaults and should be excluded during default review.
The optional Granny appearance is separate. Current scope, exact preservation
and source/native-versus-runtime evidence are recorded in
`docs/poc/market-forest-followup-20261022.md`.

## Legacy optional pack authoring

The following instructions describe the older standalone builder and its
reproducible donor packs. They are not dependencies of current default source
preparation.

This builder creates private native MM and OoT resources for the isolated market source branch. It does not modify input donors or the accepted POC9 Lost Woods archive. The editable output directory sits beside the output O2R; it contains native resource/XML sources, not a Prelude project file.

The earlier standalone presentation candidate was **Black Market POC12 KeeperLean**, paired
with the cumulative shop/forest source. It selects the retained native Sway
crossed-arm lean and positions her at `(0,1,130)` behind the counter using
`combo/BlackMarketKeeper.h`. `keeper_lean.py` reproduces this bounded update from
the exact unchanged POC11 pack: only the MM room carrier and manifest payloads
change. All 190 other payloads remain identical. Install POC12 as the selected
market version; retain POC11 only for reproduction/rollback. Details and current
evidence are in `docs/poc/market-final-presentation-20261017.md`.

Both native controllers now use locked-only translucent wrapping X ribbons.
Their native model-frame capture includes actual GI fitting, selected scale,
rotation, bob and asymmetric offsets. The offline preview calls the same C++
X geometry/IA8 texture around its sample models' posed bounds, retaining item
visibility. It is an asset preview, not a game capture. The preview verifies
that source and pack identities stay unchanged during its complete render.

Required donors are the retained Djipi scene map/textures packs and `MMShopGal_GOTH_EyeContrast_POC1.o2r`. Granny's Potion Shop supplies complete architecture; the Zora Shop reference is incomplete. Mesh collision is derived from donor triangles and still needs gameplay validation. The entrance shell is a private crop of the donor entrance geometry. Credits remain with the Djipi and Goth donor creators.

Use `python3 tools/black_market/build.py --help` for exact input flags. Build outside the repository, then run `python3 tests/black_market/check_assets.py PATH.o2r` and `python3 tools/black_market/preview.py PATH.o2r PREVIEW_DIRECTORY`. The builder refuses an existing editable output directory.

The source candidate requires its matching O2R in the active engine's mods directory: MM for combined/MM-only, and OoT for OoT-only. Combined mode also requires the accepted POC9 Lost Woods pack. Install only one market candidate; earlier internal packs are superseded. Keep existing overrides/backups isolated. Native scene registration is gated on complete resource availability. Combined seeds without market metadata remain disabled; solo files generate and freeze native stock on first load using the current market options, then retain that stock with the native save.

`assets/arcane_stone.png` is a new original texture made with the built-in image generation tool. The builder resamples and encodes it for the engine, replacing every interior material with a coherent, slowly scrolling magical surface. Generation brief: a seamless square diffuse/albedo texture; flat orthographic midnight-indigo enchanted stone; soft violet and muted turquoise mineral wisps; faint fine arcane line fragments; low contrast; no objects, bricks, medallions, text, frame or central emblem; edges tile. Runtime material coverage is complete in the asset contract, but the preview is not an engine capture.

Lost Woods and the MM market use the shared centered MM title-card path. The forest uses the existing OoT Lost Woods card resource; the shop has a private IA8 title. The OoT market initializes its native title context directly because the vanilla place-name initializer ignores unknown scenes. Review the source ledger under `docs/poc/` for verification boundaries. This is an isolated checkpoint, not a complete linked or runtime-accepted build.

The large roof void and thin shelf-side crack in the earlier preview came from its incorrect `Triangles1` alias: the native command is `Triangle1` with `V00/V01/V02/Flag0`. Correct decoding restores the existing donor faces. Seven added triangles close the two genuinely missing shelf end faces and thin floor band. Collision now follows the actual root draw graph: it restores 130 reachable donor faces omitted by the old parser and excludes 208 previously included faces from an unused donor list. It matches all 7,819 drawn, nondegenerate room faces. All original visual architecture, keeper and texture resources remain intact. `tests/black_market/check_room_polish.py` checks the formerly empty preview rays, collision equality and native scene framing; the asset contract rejects display-list commands unsupported by the native factory.

The cosmetic editor discovers a **Skiejer's Black Market** category from the pack's `CosmeticEntries` manifest. Violet Stone, Teal Stone, Rose Stone and Lavender Stone each control seven materials, preserving the four-color defaults on reset. The existing editor supplies color, rainbow, lock and reset controls. These are texture tints; the source texture and scroll directions remain part of the asset pack.

The native OoT scene headers use `scenes/shared/skiejer_market_oot/` and share the same private room, collision, keeper and title resources. Their actor and object lists are empty: the dedicated controller owns the keeper rather than importing MM actor IDs or starting a vanilla shop. The OoT spawn uses native idle mode, and its room behavior follows SoH's binary factory layout. Native routing and configured gameplay require their own validation; a compatible resource pack does not establish those behaviors.
