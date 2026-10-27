# Lake Hylia tunnel sand — cumulative source packaging

The correction is geometry coverage, not a recolored grass-strip texture.
`combo/entrances/LakeHyliaSurface.h` partitions the existing platform into
stone and dedicated sand regions. Sand follows the stream bends and covers
49.9% of the dry floor. It does not cover the stream or change collision.

## Provenance and bounded change

The full floor partition, clean native rock selection, and backdrop overlap
were already present in cumulative source commit
`fa81f7926038f2aa85c7f3e73c96fec043168b66`. This work preserves that geometry
and makes the dedicated selected sand texture a normal packaged source asset.
It does not claim to identify which earlier executable produced the reported
narrow sand stripe.

The dedicated source sand resource is preserved byte-for-byte.

- Dedicated resource: `textures/comboship/regional/lake_hylia/sand`.
- Resource SHA256: `a4bed76544f7d6e397e2a8341f2a849502b34b40127a939901c20d3f266f30eb`.
- Texture format: 256×256 RAW RGBA32 pixels with the renderer's existing
  logical 32×32 RGBA16 sampler and preserved HD scale metadata.

The deterministic source pool contains that resource and an identical
`alt/` alias. It uses ordered lossless zlib/base64 text parts, bounded to
96 KiB compressed bytes and 128 KiB text per part, with encoded/decoded part,
pool, and per-resource SHA256 identities. The preparation target writes only these two files beneath
`soh/assets/custom`; both enter the normal `soh.o2r`. No external Lake scene
archive or corrective addon is required. Existing override/load-order rules
still apply if a user deliberately installs another resource at this path.

No native scene texture path is replaced. In particular, the old grass-band
atlas `spot06_sceneTex_00FEB8` remains untouched. The source renderer's clean
stone fallback remains `spot06_sceneTex_010EB8`, owned by the user's OoT
archive. Geometry, collision, water, scene/setup headers, transitions, actors,
and progression code remain unchanged by this package.

## Verification and preview

```bash
python3 -m unittest discover -s tests/lake_tunnel_sand -v
python3 tests/regional_passages/run_lake_sand_tests.py
python3 tests/regional_passages/run_lake_bridge_tests.py --sanitize
python3 tools/lake_tunnel_sand/preview.py --output coverage_preview.png
```

The preview and original authoring image data are restored from bounded source
parts by `python3 -B scripts/assets/prepare_cumulative_assets.py`. Repacking
the sand pool is authoring-only and accepts only the pinned input archive:
`python3 -B scripts/assets/prepare_lake_tunnel_sand.py --pack-from-archive PATH`.

The new package tests failed before preparation existed, then passed after
implementation. They verify native/Alt parity, the exact sampler contract,
bounded output membership, grass-atlas preservation, and rejection of corrupt
resources before changing an installed texture.

The active native render fixture executes production `CaveDraw`, GBI wrappers,
combiner setup, and vertex dispatch. It measures sand area from the actual
queued quantized vertices, under the currently bound texture, across all
native/replacement material combinations and full/drained/refilled draws.
This guards against a correct isolated mesh that is not the submitted draw.

`coverage_preview.png` evaluates the current C++ surface builder and uses its
rounded floor vertices and selected sand pixels. The checked-in preview also
uses the stone texture from the source scene archive; the stream is a
schematic blue region. It shows 27,355 sand units² out of 54,850 dry-floor
units² (49.872%). It is a material/geometry preview, not an in-game capture or
proof of lighting, transitions, or user acceptance.

Remaining runtime check: inspect the rebuilt cumulative candidate at the
tunnel in native and Alt Assets modes, including re-entry and pause/unpause.
Confirm broad irregular sand coverage, the right seam, and unchanged
collision/water/travel. No master promotion is implied by static tests.
