# Reward GI Energy 4K POC1 assets

Candidate parent: `56c83a878562052ca873b7d414f7d0e1175b6325`.

The six element images are analytic, periodic white-RGB alpha masks. Their
curves suggest flowing currents, flame filaments, caustics, spiral wisps,
smoke, and light ripples. No reward symbol, geometry, or native texture is
regenerated. `metal` adds restrained neutral directional brushing.

| Private resource | Candidate elemental tint | Shared stone | Candidate stone tint |
| --- | --- | --- | --- |
| `objects/nei_reward_gi/forest` | `#54D85B` | Kokiri Emerald | `#46E879` |
| `objects/nei_reward_gi/fire` | `#FF593C` | Goron Ruby | `#FF4564` |
| `objects/nei_reward_gi/water` | `#459DFF` | Zora Sapphire | `#459DFF` |
| `objects/nei_reward_gi/spirit` | `#FFA64D` | — | — |
| `objects/nei_reward_gi/shadow` | `#A675EB` | — | — |
| `objects/nei_reward_gi/light` | `#FFE58A` | — | — |
| `objects/nei_reward_gi/metal` | Neutral | — | — |

Stones reuse the Forest, Fire, and Water masks with their own candidate draw
tints. These color values are a candidate palette for visual review.

Each extensionless resource contains a 64-byte OTR header, V1 `<4I2fI>`
texture metadata, and 4096×4096 row-major RGBA bytes. The texture type is
RGBA32 (`1`), flags are RAW | IMG (`3`), and both scales are `1.0`.
IMG uploads the complete physical image; unit scales retain the renderer's
32×32 logical tile accounting, as in the accepted elemental-arrow assets.
A quarter logical texel equals 32 physical texels. Each payload is 64 MiB.

Rebuild with the dependency versions recorded in `material_manifest.json`:

```sh
python tools/reward_gi/build_textures.py --output /path/to/output
```

The generator writes resources only under
`soh/assets/custom/objects/nei_reward_gi/`, then makes a deterministic ZIP
archive named `Reward_GI_Energy_4K_POC1_Assets.o2r` with exactly those seven
private keys. The JSON manifest is a sidecar and is not a game resource.
Rows are streamed in strips to avoid retaining seven full images in memory.
The repository stores those exact resources compressed in `assets.zip`.
`PrepareNeiRewardGiAssets` verifies the bundle and restores its seven payloads
before the collision check and normal `soh.o2r`/`2ship.o2r` export targets.
Restoring requires only Python's standard library and preserves each resource
hash. The private keys are checked against the other game's live asset tree.
After regenerating artwork, replace `assets.zip` with the new standalone pack
alongside the updated material manifest. Generated raw payloads are ignored.
The fields use periodic sine/cosine waves and nonlinear periodic warps,
without random noise, pixel speckle, border fades, or patched seams.

Validation parses every header and length, checks the white-RGB/alpha or
neutral-metal rules, compares pixel and resource hashes, checks the archive
name set and CRCs, and hashes decompressed entries against source resources.
Translation by one whole period and first derivatives agree analytically in
both axes; sampled wrap differences stay within interior neighboring-pixel
differences. Preview images are decoded from the written 4K payloads.

The contact sheet shows the candidate tints. The wrap sheet repeats each tile
2×2. The looping GIF and cyclic MP4 sample one complete 128-quarter-step
scroll cycle every two steps. Their timing is illustrative; they contain
texture art only. Renderer/build checks, actual receipt-size readability,
Alt Assets parity, runtime memory, and in-game acceptance belong to the
separate integration POC and are not proven by these previews.
