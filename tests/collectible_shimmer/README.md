# Collectible shimmer source POC

Baseline: ComboShip `5e679b0f2e913e79e0d52daf350744a6f8ed03b1`, preserved in the sibling `shimmer_inputs`/`source_inputs` directories. Candidate overlays the partial source checkpoint `bf98489`. No accepted master, models, textures, pickup logic, sprite hearts, or preexisting effects are replaced.

One **Rupee & Heart Shimmer** checkbox appears in both cosmetic editors under **Link & Items → Appearance**, immediately after Bottle shimmer. The shared CVar is `gEnhancements.CollectibleShimmer`; absent/zero means off. No other effect toggle is required. The existing NewDrops toggle selects OoT drop models independently.

The palette belongs to the item's game. OoT uses Consumable rupee/Hearts settings; MM uses Collectibles rupees and HUD.Hearts. The shared effect reuses the authored NEI SampleShimmer geometry, timing, mesh submission, white cores, arena/resource-owner handling and GPU-state restoration. Explicit tint mode 2 in the existing integer itemShimmer ABI field avoids the legacy mask neutral-RGB sentinel without changing the ABI layout. Foreign descriptors stay appearance-dependent even while the checkbox is off, and owner rainbow sampling uses the existing foreign-frame clock. No alpha flags, textures, particles, or new per-item colors were added.

Coverage includes OoT's five ordinary rupee types and special randomized silver drawer, all six MM rupee GI types, 3D recovery hearts, heart pieces and heart containers. Native direct-mesh paths add one bounded overlay; GI-based drop paths already use the GI wrapper. MM's native container object guard remains in force. The MM silver rupee has no EnItem00 drop variant; its GI path is covered.

Run from the source root:

```sh
python3 tests/collectible_shimmer/native_regressions.py
python3 tests/collectible_shimmer/renderer_regressions.py
python3 tests/collectible_shimmer/draw_regressions.py
git diff --check
```

These compile production eligibility/sampler/native drop functions and foreign descriptor/cache boundary excerpts with small engine stand-ins. Renderer tests use the unchanged authored effect policy from the checkout or preserved original. Red evidence: the native fixture failed on the first enabled green rupee before implementation; `renderer_regressions.py --baseline` fails on explicit RGB 220/225/240 against the original neutral sentinel. All current fixtures pass. They are source fixture compilation, not complete game compilation or runtime proof.

Runtime proof pending: in each game compare checkbox off/on with vanilla and Alt assets; show every rupee color, native and GI drop paths, 3D recovery heart versus an unchanged 2D sprite, heart piece and container. Change/reset/rainbow the current owning-game editor color while a native and foreign item remain visible; repeat off/on after the foreign recipe was cached. Verify a foreign item keeps its owner's palette, even while that owner is dormant. Check pickup/re-entry and a room with several collectibles alongside existing magic, mask, bottle and sword shimmers. The native overlay conversion scales (25 small rupees, 17.5 large, 21 native pieces, 20 native containers, with MM's existing container conversion retained) are source-verified but their visible size and placement remain unproven in game.
