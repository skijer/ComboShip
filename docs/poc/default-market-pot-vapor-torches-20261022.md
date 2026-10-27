# Default market vapor and decorative torches — 2026-10-22

Candidate work follows source checkpoint `ec9ea487`. This is source/native
compilation and offline inspection evidence, not configured gameplay acceptance.

## Cause and bounded repair

The built-in market draws the retained room's opaque display list. Its four
large cauldron mouths and two complete torch stands exist, but the default draw
has no vapor or decorative flame submission. The optional Granny scene has
separate translucent drawing and actor setup; default presentation could not
depend on that variant or its external maps.

`combo/BlackMarketInteriorEffects.h` now draws four narrow, continuously rising
vapor columns and two upright pale-violet native flames. Both host controllers
call it inside their existing interior guard, with `!sGrannyActive`, before the
stock-active early return. Effects therefore continue without loaded offers
and do not double the Granny variant's effects. No actors, lights, sound,
collision, random numbers, save writes or persistent effect slots are added.
Frame-derived poses freeze when the gameplay frame does not advance.

There are twelve overlapping cloud billboards per pot. Their centers rise
0.95 units/frame, with 9.5-unit spacing, slight lateral motion, half-width
6–10.2, half-height 18–27.6 and peak alpha 72/255. The vertical stretch joins the
native mask's soft cores; the earlier rounder trial visibly produced beads.
Billboards use back-to-front translucent cloud drawing with depth testing and
no depth writes. All effect commands go to XLU; native setup 25, neutral
primitive/environment colors and the borrowed segment 8 are restored.

## Native resources and palette

All keys below have the `__OTR__` prefix in production. Texture pixels stay in
the active host's native game archive. No ROM texture pixels or Djipi maps are
included in this source change.

| Purpose | MM native key | OoT native key |
| --- | --- | --- |
| Soft cloud, I8 32×32 | `objects/gameplay_keep/gEffDust1Tex` | `objects/gameplay_keep/gDust1Tex` |
| Flame display list | `objects/gameplay_keep/gEffFire1DL` | `objects/gameplay_keep/gEffFire1DL` |
| Flame textures selected by native DL | `gEffFireTex`, `gEffFireMaskTex` | `gDecorativeFlameTex`, `gDecorativeFlameMaskTex` |

Both `EnLight_Draw` implementations supply primitive/environment colors before
the native fire list. The inspected MM archive's list contains no primitive or
environment color overwrite; the offline helper checks this before decoding it.
The caller supplies primitive `(225,205,255,255)` and environment `(90,45,160,0)`.
The native two-texture scroll uses the same 32×64 / 32×128 tile sizes and −20
quarter-texel/frame mask scroll as EnLight. Scale `.004` is the native small
decorative type 4 scale (`40 * .0001`) in both source tables. Actual MM native
fire vertices span X ±4000 and Y 0–16000, yielding radius 16 and height 64.
OoT native types and source were checked; this session did not have a complete
OoT native base archive for an independent binary-resource decode.

## Physical placement

| Effect | World anchor | Retained owner |
| --- | --- | --- |
| Front-right pot vapor | `(73,37,48)` | material 00 / tri 0 liquid |
| Left pot vapor | `(-91,51,24)` | material 16 / tri 18 liquid |
| Middle-right pot vapor | `(118,45,243)` | material 16 / tri 18 liquid |
| Entry-right pot vapor | `(118,40,333)` | material 27 / tri 16 liquid |
| Right torch | `(75,76,205)` | complete material 21 / tri 2 stand |
| Left torch | `(-110,76,205)` | complete material 21 / tri 2 stand |

The material preparation moves only the two disjoint complete torch components:
right delta `(-55,0,213)`, left delta `(29,0,213)`. Their shape, topology,
UVs and colors stay intact; the original bundled scene remains unchanged.
Prepared full stand bounds are right `(56,0,194)`–`(90,83,226)` and left
`(-127,0,194)`–`(-93,83,226)`. All-orientation full flame bounds are right
`(59,76,189)`–`(91,140,221)` and left `(-126,76,189)`–`(-94,140,221)`.

The asymmetric placement follows the room's actual cabinet and hanging-basket
gaps. Earlier Z145 and Z196 trials failed nearby canister/curio clearance and
were discarded. A thirteen-axis triangle/AABB separating-axis check over all
7,919 prepared room triangles finds zero intersections for either complete
stand or full flame envelope. It excludes only the stand components themselves
and supporting floor planes at Y≤1. Shelf collision boxes, selected-item/seal
envelopes and nine nearby complete prop bounds are also checked in native tests.

Production bank cameras are unchanged. In the corresponding 320×240 native
camera, the actual upright flame projects right X226–274/Y12–101 and left
X21–65/Y21–101, clear of the reserved offer/seal bounds. The native close keeper
camera intentionally crops both decorations; it is not a whole-room view.
Offline wide and corresponding bank views show the complete stands and flames.

## Verification and reproduction

The real native OPA tail allocators do not return null on exhaustion. Before
any allocation or scroll construction, the effect uses the shared
`BlackMarketGraphicsArena::HasRoom` guard to reserve aligned vertices, three
matrices, two twelve-Gfx scroll lists, 256 XLU packets and conservative debug
slack of eight OPA/two overlay packets. Tests reproduce an actual sanitizer
overflow without this guard and check one-byte/packet-short arena rejection
before any mutation, plus successful drawing at the declared full budget.

With the real dependency headers loaded through
`../test-dependencies/include_paths.env`:

```sh
python3 -B tests/black_market/run_interior_effects_native_tests.py
python3 -B tests/black_market/run_lock_draw_tests.py
python3 -B scripts/diagnostics/compile_solo_forest_core.py \
  --unit mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp \
  --unit soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp \
  --out ../interior-effects-verification --jobs 2
```

The effects suite passes ASan/UBSan under both hosts' real GBI/types. It checks
native cloud format/combiner/alpha, explicit two-cycle flame setup, violet
palette, command/matrix/scroll balance, restored state, continuous columns,
frame determinism/wrap, default/Granny/room/lifetime guards and four allocation
failure paths. Both actual controller translation units compile to objects.
The regression entry point now invokes this native effects suite.

`../interior-effects-verification/` contains native logs, object/dependency
manifests, `check_clearance.py`/`clearance.json`, camera projection and offline
preview evidence. `tools/black_market/interior_effects_preview.{cpp,py}` exports
production poses and replays the native MM masks/geometry from a supplied local
archive; it bundles no game pixels. Final room-only captures are
`../review/interior-effects-final-{center,right,left,wide}.png`; the material
audit's `../review/prop-materials-stock/prop-interior-native-{center,right,left}.png`
also includes illustrative offers and seals. These approximate raster filtering
and lighting and are not gameplay captures. Configuration-specific launch,
native archive resolution, pause/transition behavior and user acceptance remain
runtime checks. Purchases, seed-saved offers, browsing cameras/input, collision
and randomizer tiers are protected existing behavior, not newly proven here.
