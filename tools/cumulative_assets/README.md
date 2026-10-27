# Exact cumulative asset preparation

New runtime resources and their reproducible authoring inputs use the same
lossless source format as the established spin-resource publication workflow.
The source tree contains ordered base64 text parts, each at most 131,072 text
bytes representing at most 98,304 bytes of one zlib stream. The original
baseline asset parts are inherited unchanged.

The pool deduplicates identical byte sequences. The manifest pins each encoded
and decoded part, the complete compressed and decoded pools, and every output
path, size, offset, and SHA256. `source_files.json` independently pins the exact
source resource identities accepted for this cumulative checkpoint. Restoration
preserves PNG bytes, native texture headers, pixel data, dimensions, UV-related
metadata, and resource names exactly.

Normal builds run:

```bash
python3 -B scripts/assets/prepare_cumulative_assets.py
```

`PrepareCumulativeAssets` runs before custom-asset collision checking and both
port archive targets. Regression/preflight runners also prime the resources
before directly executing asset scripts. Normal restoration needs only the
Python standard library and the committed source parts.

Authoring-only repacking refuses any source file whose bytes differ from the
pinned inventory:

```bash
python3 -B scripts/assets/prepare_cumulative_assets.py --pack
python3 -B -m unittest discover -s tests/cumulative_assets -v
```

All inventory and size bounds are validated before destination writes. A
missing, corrupt, reordered, oversized, or malformed part fails closed.
Existing matching files are retained; damaged files are replaced atomically.
Generated full-size resources and original PNG inputs are ignored by Git and
are reconstructed at their existing runtime/authoring paths.

The dedicated Lake tunnel sand uses a separate small source pool under
`tools/lake_tunnel_sand/bundled`, with the same decoder and bounds. The chamber's
native scene package has its own independently verified source inventory.
