# Native stock forest and Black Market

The user authorizes implementation now: the scene layout and shop must work without a supplemental forest/market mod archive. The final clarification excludes all retextures, MM3D forest geometry, texture pixels, masks, poster and scrolling treatments from the native bake. Use stock MM Lost Woods geometry, the authored building mesh and moved Mirror stump. Saria belongs beside the OoT/MM portal, not in front. Preserve the private OoT solo forest and MM solo behavior.

## Baseline and evidence

Source baseline is PR45 head `613802447cf8a4fbc3e058ba084f9e3dd7b9f7ae`. Work is isolated on `poc/native-forest-market-20261020`; no accepted master is promoted.

- Retained original stock-scene edit: `tools/magic_mirror/scene/LostWoods-MirrorStump.prelude.o2r`, SHA256 `7fde769411d21792f6bc6cde62ee934fb17b8e24d7a01b8d74f30069cce49a83`.
- POC9 placement reference: SHA256 `72d750b00583379f3619dac52dce59e2957a351fdfabd3a3a8da11f9bc19daf0`. Its stump predates the Mirror edit. The authored moved cap is `(-377,80,416)`; supported recall landing is `(-377,0,296)`. Translate visual and collision together; do not append a second stump.
- FINAL POC19: SHA256 `9097583c896a11c204e4950bd9a80aa8e69fb2a42307f55f50e75d4195f30e42`. Use only its current building geometry, collision and authored door `(-680,0,72)`, packed MM yaw13695. Its MM3D scenery and textures are excluded.
- Market POC14: SHA256 `3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`. Preserve room geometry, collision, spawn and offer frame; optional pack retains its presentation.
- Original stock MM archive needed for complete native DL ancestry and room2: SHA256 `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`. The user supplied a newer stock donor on 2026-10-20, SHA256 `2f44bafe1a00f44ddaf7eb5e5c99e73a3d23771038529d6d1748819ad575dc80`. Its native stump records retain the exact required translation with all other bytes unchanged, and its three-room graph was validated. Both exact donor identities are supported; the real bundle uses the supplied newer archive.

## Implementation

Generate private exploration scene/room and changed mesh/Vtx identities. Keep the combined MM scene ID and native prologue/flashback framing unchanged; only exploration loads the built-in scene. Preserve stock type2 culling and update bounds for the moved stump. MM retains native material animation; OoT receives corresponding per-frame segment bindings when the native marker is present. Legacy solo scenes retain their existing separate FINAL six-track path. Native scenes use the new material identities and native animation data.

Both normal port archives receive identical common geometry through a deterministic, hash-checked preparation step. Native textures remain ordinary ROM resources: do not copy texture payloads. OoT's existing MM base-asset loader supplies native MM dependencies. Missing genuine base resources must fail closed rather than attempt an incomplete scene.

Each generated forest mode carries a complete dependency Blob listing all private display-list ancestors, its own three-room framing and every external stock graph resource. Runtime checks the actual inventory with separate private and base owners; the fixed core list alone cannot enable entry. OoT queries base archive membership without loading/logging every stock resource on each update. Native portal rims/triggers follow the 44-unit stock opening; wider legacy geometry retains its existing 58-unit trigger and material treatment. The active room mesh selects native data-driven segments even when an optional legacy scrolling marker is mounted.

The Black Market has a built-in room/mesh/collision fallback using ordinary stock material references and the vanilla native keeper. Readiness requires the active host's native core, not optional texture/cosmetic/title files or the other engine's scene. Complete custom keeper overrides retain precedence; partial replacements must not be selected. All prices, inventory, grants, dialogue, locks, browsing and return handling remain unchanged.

Use the same collision-checked portal-local side placement for both fallback Sarias. Authored actor placements and the separate Termina song giver retain precedence. Epona remains on the opposite side.

## Verification and status

Test source routing, per-host readiness and missing-resource recovery using actual production bodies. Verify no texture resources in generated bundles, stock geometry/UV preservation, stump/collision/culling parity, native door/return, prologue exclusion, correct segment lifetime and rates, optional override isolation, and deterministic clean preparation. Run the existing native forest, market, Mirror and actor suites and compile changed native translation units.

Build/static verification is separate from configured game rendering, traversal, Mirror pickup/recall and user acceptance. Keep source checkpoints recoverable after each verified milestone. Do not merge or promote the candidate without demonstrated runtime acceptance.

## Supplied-donor integration

The real bundle contains 39 resources and no texture payloads; its 88 unchanged stock dependencies stay in the normal MM base archive. The native TexAnim is a zero-segment Empty sentinel, so native scenery needs no scroll bindings. MM ambient particles retain Demo_Kankyo; OoT uses its equivalent Object_Kankyo params0, with stock segment8 binding inside the actor draw. Original cutscene actor IDs remain excluded from private exploration. Both normal port asset trees are prepared from this same bundle. Complete game linking and configured runtime remain unproven; local root CMake configuration is blocked by SDL2 development files.

## Optional presentation compatibility

The retained FINAL POC19 archive replaces the legacy combined scene and MM3D mesh graph. It is not a texture-only pack: none of its normalized resource identities targets the generated native graph or its stock dependencies. Once combined exploration selects the private native scene, that archive cannot supply its old presentation. Re-enabling its scene headers would also re-enable the excluded MM3D geometry and bypass native stump protection.

Native material targets are documented in `tools/native_forest/README.md` and the generated manifest. A visual-only archive must target those material lists, stock texture identities and, when needed, both native MM TexAnim and OoT material parameters. Converting the old FINAL art/UV graph into such a pack is separate visual work and is not implemented or verified by this geometry-only change. Existing FINAL compatibility must not be claimed for combined native exploration.
