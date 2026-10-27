# Native Lost Woods presentation polish — 2026-10-21

| Field | Record |
|---|---|
| Baseline | PR53 source `724e0476b1dd2555f40faf7cc01b8d1cf0329ecd`; source ZIP SHA256 `b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5` |
| Candidate | `fix/forest-presentation-20261021`; bounded code and four deterministic derived scene resources |
| Evidence | cor's three marked screenshots and 30.366-second clip; frames across the clip show the green/purple split, detached purple left-bark bloom, and camera-dependent ramp appearance |
| Status | Implemented candidate; source/native checks recorded below; configured gameplay and user acceptance pending |
| Recovery | All six immutable ZIP parts retained; restore checkpoint against the pinned parent, or omit this branch to retain the baseline |

## Requested result

- Match the 2Ship/private-forest portal face and actual NEI twinkles to the building's purple `#581880`. Preserve white star cores, timing, alpha, geometry and full black-face coverage. Saria and Woods of Mystery retain their existing green; the Mirror remains pale blue/white.
- Remove only the active call to the old `ReturnTunnelPinkLight` rim bloom. Independent geometry inspection located its left fan at `(-713,18,-239)`, matching the faint left-bark spot. The original resource remains preserved in the source ZIP.
- Swap only S coordinates for the 48 front/rear hanging-sign vertices: front Black Market text, rear crescent/key. All positions, T coordinates, colors, textures and the separate four-vertex Skiejer avatar poster remain exact.
- Keep the ramp's complete shape, footprint, collision and opaque ownership. Give it a self-lit lavender/purple material using the existing I8 magic mask, with explicit one-cycle/tile-LOD, geometry, texture, combiner and depth/blend state. The old material inherited two-cycle state and could sample a previous draw's second tile when culling changed draw order. No natural bark/stone texture remains bound to the active ramp.
- Move Skull Kid to the selected raised left mushroom/tree ledge at X=2302,Z=-185. Native floor there is Y=290; the existing reclining float remains 35 units above it, facing the clearing. Preserve parameter, animation, collision and authored-actor precedence.
- Follow the Saria/Epona flank marks with lateral offsets -230/+230 and forward depth 120. They remain about 254/253 units from the protected stock-tree center; full native footprint and headroom checks pass (Saria radius12, Epona radius35, twelve perimeter samples). Preserve existing actor/dialogue/progression and prologue/flashback guards.
- Prefer the center of the actual loaded stump cap for Mirror recall, facing down the native ramp. Native position is `(-377,80,416)`. Keep complete footprint/headroom checks, ground fallbacks for obstructed/custom caps, one-shot context and native yaw encoding; do not overwrite Farore/grotto respawns or cached scene data.

## Source ownership

`verified_resources` continues to return the original, fully verified source for supported authored rebakes. `prepared_resources` applies the cosmetic recipe and checks all four output sizes/hashes before either host writes assets. Archive preparation uses the latter. This avoids changing the immutable 49 MB ZIP or depending on a mounted scene mod. Both ports consume the same recipe and receipt. Dependency names, scene/room headers, all collision/water/camera/actor records, original texture payloads, six animation tracks and ramp geometry are byte-identical.

The two independent review findings (raw/effective rebake ownership and I8 decoding) were verified against production source and corrected. No remaining production blockers were identified. Existing later/parallel Mirror ritual or native Fairy changes must be merged deliberately with this branch; this source candidate does not replace those branches.

## Verification

The source checkpoint includes exact commands and output logs. The full native-forest suite passed 52 checks with two optional original-donor checks skipped (54 discovered); the available immutable authored donor was exercised. Seven affected actor/Mirror objects, both native Mirror hosts, four stump-access tests, three ordering checks, production particle/renderer checks and both actor suites passed. The new checks first reproduced the previous ground arrival, mixed portal palette, sign ordering and inherited ramp pipeline, then passed after the bounded changes. Native Mirror tests cover solo OoT, solo MM and combined MM arrival, moved anchors, ground fallback, scene/file/seed guards, resource and respawn preservation, pickup, prompt and cast lifetime. Particle tests exercise actual production draw bodies and native GI renderer, retaining unrelated green and pale-blue palettes. Native actor suites preserve dialogue, rewards, authored overrides and exploration guards. Changed actor and both Mirror adapter objects compile with real headers. Asset checks cover deterministic source reconstruction, all four transform hashes, both port trees, rejected/corrupt receipts and the supported donor rebake.

These checks do not prove configured gameplay rendering, Alt Assets parity, pause/re-entry or acceptance. No executable is published and no runtime master is promoted. The next decisive in-game pass is the marked placements, consistent purple portal with no left bloom, both sign faces, ramp under camera rotation, and Mirror return on the cap after each game handoff.
