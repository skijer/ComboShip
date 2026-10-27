# Accepted bottle polish integration candidate

Parent: `e38dc74978f9387ca4f983656455584b157ed66c` (audited October 8 recovery).
Accepted visual source: `2Ship_Bottle_Shimmer_POC2_20261008`, provenance
`2ce9283f63db977e301a15e9cf38994b73559d93`. Approval covers POC1 fitted
models plus the POC2 shimmer profiles; this port still needs game framebuffer
verification in its final combined configuration.

The source OBJ/MTL/PNG files and mushroom GLB are copied byte for byte. The
mushroom uses its original 512px image. Princess and Seahorse retain the
accepted 66% and 71% static donor poses. Serialization uses the established NEI
writer with finer 1/256 GI-unit positions to retain all 2,688 / 1,001 / 578
triangles. OBJ V coordinates are converted to the native top origin without
altering texture pixels. Princess uses XLU because her donor atlases contain
transparent and partially transparent pixels; the other new solids use OPA.
All contents precede the unchanged neutral TP/Peyron glass.

Both port archives include the same private resources. Explicit content-to-
shimmer mapping keeps `CW_SHIMMER_*` independent of draw IDs and GIDs. Colors
are Princess `#80D846`, Fairy `#FFA0EB`, Seahorse `#FFE66D`, Gold Dust `#FFD45A`,
and Mushroom `#DE64F5`. The existing five-star sampler, white centers,
positions, alpha and clock are preserved. Four additional faint fairy motes
use the accepted policy and active host frame, with no RNG or persistent pool.
The motes follow the bundled fairy's intrinsic VFX; a foreign descriptor's
`itemShimmer` flag controls its outer five-star overlay only.

Bundled fairy shells use the accepted .008 native skeleton scale (2x the prior
.004), preserving wing animation, glow pulse, and motion. Selected fairy/mod
shells retain their existing fit and shell priority. Selected native Seahorse
resources retain their original two-pass recipe. Selected replacements of
bottle marker roots own the complete presentation; selected private resources
follow active-host Alt selection. Caught/loose Seahorse stays unchanged.

The composer checks deferred private mesh dependencies before submission and
suppresses arena-exhausted draws before attempting native fallback. Resource
exceptions stay inside the C ABI. Resource ownership, matrices, and neutral
render state are restored. The casing resources and the production gold
function are covered by preservation hashes in `approved-inputs.json`.

Rebuild from the repository root:

```sh
python3 tools/nei_gi/build_bottle_polish.py
python3 scripts/diagnostics/run_bottle_polish_tests.py
python3 scripts/diagnostics/run_bottle_contents_tests.py
python3 scripts/diagnostics/run_fairy_bottle_tests.py
python3 scripts/diagnostics/run_bottle_gi_tests.py
python3 scripts/diagnostics/run_mask_shimmer_tests.py --sanitize
python3 scripts/diagnostics/run_nei_gi_tests.py --sanitize --fast-math
```

These checks execute production policy/composer/native-export bodies, both
engines' real headers/GBI, arena boundaries, missing resources, selected Alt
priority, exact donor pixels/geometry, and isolated deterministic rebuilding.
They establish source/build/static evidence, not in-game visual acceptance.
3DS animation retargeting remains outside scope. Final runtime checks should
cover native and imported receipts/pickups and shelves in both hosts with Alt
off/on and selected replacement packs, followed by re-entry and pause/unpause.
