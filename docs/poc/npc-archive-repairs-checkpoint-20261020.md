# Rejected NPC model archive repair checkpoint

All model candidates remain unaccepted. No master promotion or configured game runtime proof is implied.

The game-source rebuild checkpoint is `ee83e2613ebcd0cb029869fc6dce90f7a09fdd75` on `poc/npc-runtime-review-20261020`, tree `f1a5ab6db1c8fcc2dbeff596fc5e37c72e92583f`. That branch is frozen for the user's bug-fix rebuild. It adds optional uniform DrawScale and size-checked V2 placed-Saria ABI while preserving legacy ABI sizes; rebuild both OoT and MM modules. The requested 1.10 values live in the replacement young-Malon and anime-Saria archives. These archive-tool changes do not require further game-source updates.

## Verified archive repairs

- Twins: six 256x128 shoulder bakes did not cover their native 512x512 render tile. Exact 2x4 pixel duplication fixes the repeated bands; every mesh/UV/normal/rig/pose byte stays exact. One active shoulder bake increases from 128 KiB to 1 MiB, a memory tradeoff documented for runtime review.
- Young Fado: the preceding checkpoint contains its connected shoulder-cap refit and existing-contract skin-resource recipe. Original binary resources remain exact; all static clips and laughter were checked offline. Residual underarm intersections remain in some poses. No acceptance claim.
- Adult Malon and anime Saria: bounded common skin normal fields reduce polygon-scale forehead/cheek lighting variation. Only normal XYZ bytes in 17 head/expression buffers per model change. Saria's authored eye cleanup uses a connected aperture mask; rejected POC2 had a cheek seam, so only POC3 is eligible for integration.
- Anime Saria: native-reference blue hue changes iris texels in Diffuse and EyesHalf only. Pupils, catchlights, shapes, alpha, non-iris skin/hair and closed blink remain exact. Traditional Saria's existing blue eyes are audited and preserved.
- Anime Saria hands: bounded topology-local normal smoothing preserves geometry, grip, UVs, weight ownership and wrist-rim normals. Broad shape and baked crease artwork remain; a restrained authored paint pass softens crease shadows while preserving nails/wrist/non-hand sampling in both native filter modes.
- Both Saria ocarinas: rejected packed RGBA16/RAW pixels were interpreted by the raw RGBA32 importer. Repack the same decoded colors as raw RGBA32, horizontal byte scale16 and vertical pixel scale8. The original 32x16 RGBA16 tile then uploads the intended 256x128 image. Geometry, UVs, grip and display lists stay exact. Do not stack an older prop overlay over the repaired full model.
- Lossless rebatching from the prior checkpoint reduces redundant matrix/vertex commands; triangle corners retain matrix ownership and every 16-byte vertex record. No NPCs or triangles are removed. Actual FPS and TP-young-Epona cost are unmeasured.

## Evidence and remaining work

The production raw-upload fixture reproduces the old 128x128 color mismatch and passes the repaired 256x128 decoded colors under ASan/UBSan. Python unit and exported-resource tests cover masks, palette preservation, shoulder sampling, hand normal boundaries and batching. An independent review found file-alias overwrite hazards; shared path/inode guards now reject protected destinations before parsing or writing. Actual synthetic hardlink corruption and report aliases were demonstrated red, then green after correction.

Final bounded blouse and hand bitmap passes are included as archive-only recipes. Blouse POC5 changes 66,033 texels, and hand POC2 changes 46,432 texels. A native filtering review caught small arm/nail color leaks in earlier trials; actual 0xffff ST truncation and the linear half-texel shift are now included in both protection masks. Real failing arm/nail witnesses are regression tests. Earlier POC4 blouse / POC1 hands must not be integrated. Offline inherited peach triangles and shoulder tears on Malon remain runtime-review items; two-sided preview overlap/culling may contribute. No source UV or game-hole diagnosis is claimed. Root owns candidate assembly, protected-resource readback, literal-reference/load-order checks, durable recovery publication and the final handoff. Full executable build and configured in-game evaluation remain pending. Existing parent archives and native `oot.o2r` remain read-only.

The final asset-source checkpoint also extends the path/inode and report protections to the Fado builder. The game-source rebuild branch stays frozen at ee83e261; subsequent changes are repair tooling, evidence and model archives.

## Final source verification

The full NPC Python suite ran 195 tests, 16 skipped, exit0. Five destination-preservation methods also passed after adding the readback report guard. Exact native-mode audits found zero protected sampling changes in final blouse POC5 (42 non-chest readers /29 UV profiles) and hand POC2 (53 non-hand readers,20 wrist routes,two distal/nail sets /39 UV profiles). Both native point and linear transforms are covered. Both complete Saria style combinations had zero unresolved literal references or cross-pack collisions.
