# BossSoul flame alpha repair source

This is the faithful editable-source import of the existing POC5 archive-only
repair. The three Python files, shader metadata/snapshots, two POC4 inputs, four
native resources, original README and original hash inventory are unchanged.
This import changes no runtime code, materials, models, artwork or progression.

Source archive: `BossSoul-Flame-Alpha-Repair-POC5-20261020.zip`, 581,395 bytes.
SHA-256: `8444ff614a508679bf0f3703cd52481a41de590df6321ca20a735ffab1876ded`.
Its ZIP CRC and all 21 entries in its `SHA256.json` inventory were verified before
import. The 15 retained files total 339,863 bytes and match their original bytes.

`README.txt` and `SHA256.json` describe the original complete delivery archive.
The inventory also identifies its generated packs and verification reports,
which are deliberately not committed here. `shader-sources.json` pins the three
unchanged production renderer snapshots. `inputs/` and `native/` are recovery
and checker inputs, not installable mods.

The shader snapshots retain their original line endings and whitespace so their
recorded hashes stay reproducible; the local attributes preserve those bytes.

## Repair and protected behavior

The native MM combiner can wrap intermediate alpha below `-0.51` into opaque
flecks outside its fixed flame envelope. The POC4 moving image reaches alpha
66/255. POC5 raises only MM physical alpha bytes below 86/255 to 86/255; the worst
zero-envelope intermediate is then about `-0.499008`, including safe filter
interpolation. Exactly 4,271 alpha bytes change (3.26% of pixels).

All MM RGB bytes, texture headers and the fixed envelope remain exact. Native
fixed/moving resources and display lists remain exact; the retained native
moving image has minimum intensity 107/255. The native display-list combiner is
`FC272C60 35FCE378`; the OoT list retains LOD 128, while the MM draw caller supplies
LOD 192. The complete OoT replacement resources are unchanged because its LOD
does not cross the wrap threshold. The recipe preserves both packs' two-entry
Alt-texture scope, colors, scrolling, meshes, boss models/jaw, animation,
accepted soul text and progression.

| Pinned input | SHA-256 |
|---|---|
| `inputs/POC4_MM.o2r` | `cf1b71ef263f59a4eda10f6b0ca04a9825b5db91c28d2335251799452ef3ed80` |
| `inputs/POC4_OoT.o2r` | `066f877c737ef9113b9971ec687921d25afbc16446c99dbb20499d886176e79a` |

| Deterministically rebuilt output | SHA-256 |
|---|---|
| `mods/2ship/zz_BossSoul_Realistic_Flames_MM_POC5_AlphaSafe.o2r` | `d3df70cf45eb1add95f2e9a7e4f282268ef6241206913a2801926866fb1f818d` |
| `mods/soh/zz_BossSoul_Realistic_Flames_OoT_POC5_AlphaSafe.o2r` | `1dd1814ab9942254df97a5df57431043d652f18597b53fb88e82ac3f5be6589d` |

## Reproduce outside the checkout

Dependencies: Python 3, NumPy, Linux desktop OpenGL and surfaceless EGL
(Mesa/llvmpipe works). The original scripts write relative to their own location,
so copy this source into a temporary directory before running them:

```bash
boss_flame_work="$(mktemp -d /tmp/boss-soul-flames.XXXXXX)"
cp -R tools/boss_soul_flames/. "$boss_flame_work/"
python3 -B "$boss_flame_work/repair_pack.py"
python3 -B "$boss_flame_work/verify_flame.py" \
  --mm "$boss_flame_work/mods/2ship/zz_BossSoul_Realistic_Flames_MM_POC5_AlphaSafe.o2r" \
  --oot "$boss_flame_work/mods/soh/zz_BossSoul_Realistic_Flames_OoT_POC5_AlphaSafe.o2r" \
  --quarter-phases --out "$boss_flame_work/verification.json"
```

The original negative control is expected to exit nonzero with escaped pixels:

```bash
python3 -B "$boss_flame_work/verify_flame.py" \
  --mm "$boss_flame_work/inputs/POC4_MM.o2r" \
  --out "$boss_flame_work/baseline-gpu-failure.json"
```

## Source-import checks — 2026-10-21

External regeneration reproduced both pack hashes above and the original full
POC5 manifest exactly. ZIP CRC, resource hashes, unchanged fixed envelopes,
unchanged RGB/header bytes, the 4,271-byte MM alpha delta and complete OoT
resource equality passed. Native resource hashes and all shader snapshots
matched the source inventory.

The existing checker ran on `llvmpipe (LLVM 20.1.2, 256 bits)` at 256×512 and
reproduced the original verification report exactly. Nearest, bilinear and
three-point filtering each produced zero escaped pixels over 256 quarter-frame
MM phases and 1,024 quarter-frame OoT phases. The original MM control reproduced
its expected failure across 64 phases: 2,978 nearest, 2,723 bilinear and 2,789
three-point escaped pixels. Only OpenGL was executed; HLSL and Metal are retained
source evidence. Generated packs, manifests and checker output stayed outside
the repository.

## Installation and runtime boundary

This remains an archive-only repair; importing or compiling this source does not
activate it in a game build. Close the game, disable older flame POC/control
archives, and install only the generated archive matching each owner. Keep boss
model packs enabled. The owning game's Alt Assets must be enabled, and Pak
Loader must select POC5 as the winner for the same flame paths; filenames alone
do not establish priority. Restart after changing selection.

The two texture paths also serve MM EnLight effects and OoT blue-fire GI.
Configured game pickup, the installed pack stack, other GPU backends and master
promotion remain unverified by this source import. The historical clip described
in `README.txt` showed the original failure and accepted text presentation,
not the repaired candidate. To roll back, disable POC5 and restore the prior
pack selection; the unchanged POC4 inputs remain retained here.
