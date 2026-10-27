# Compact counter display continuation

| Field | Record |
|---|---|
| Baseline | Source `729214db`, preserved counter/keeper candidate POC13; no configured runtime acceptance implied. |
| Candidate | `poc/black-market-shop-layout-20261018`; source-only item placement correction using the unchanged POC13 pack. |
| Request | Present the floating offers like a regular shop around the keeper's counter, rather than spread along the front side walls. |
| Change | Two compact four-offer banks flanking the keeper; lower front row and upper set-back row. Both native controllers use identical coordinates. |
| Preservation | Keeper/lean/raised counter, complete room/collision/material/texture/cosmetic bytes, all stock/price/grant/save/unlock/forest logic and slot identities. |
| Verdict | Implemented and native-object/geometry verified; configured gameplay, native browsing camera, Alt parity and user acceptance untested. No full build or master promotion. |

The side-wall spacing was not necessary for clearance. Standard native shops
use four offers per side in a two-by-two index arrangement (EnOssan's shelf
order); this retains that familiar arrangement and the existing offer order
while placing the displays around this particular counter. They still float,
as requested; no vanilla shelf actor or entrance/randomizer routing is added.

## Placement and preview

| Position | Old coordinates | Current coordinates |
|---|---|---|
| Inner lower, each side | `(±72,64,48)` | `(±35,58,82)` |
| Inner upper, each side | `(±72,104,48)` | `(±35,82,118)` |
| Outer lower, each side | `(±120,64,48)` | `(±70,58,82)` |
| Outer upper, each side | `(±120,104,48)` | `(±70,82,118)` |

The upper row is set back so its spinning models and wrapping X seals stay
separate from the lower row. The keeper remains at `(0,1,130)` with her full
crossed-arm Sway and the existing Y41 counter. Normal/selected scales remain
`.25/.32`; bob, native item fitting, locks and selection logic are unchanged.

The preview reads and compares both native position tables, refusing to render
if they differ. Its single continuous overview now uses a 70-degree vertical
field of view to frame the counter displays. This is offline preview framing;
native gameplay camera behavior is not changed or established by that image.
Separate merchandise detail cameras also follow the new display positions.

## Verification

`check_shop_layout.py` first fails on the old wall-spread placement, then passes
on the compact native layout. It checks actual retained sample model/native-X
sweeps against every room triangle, all 32 actual keeper poses and other offers.
It also reserves the conditional authored-shop FrameFit envelope (76 local
spinning width, local Y[-22,62], complete bob and selected scale, conservative
native X inflation/quantization). Those reserved boxes are disjoint from the
room, keeper and each other, and fit inside the single-camera viewport without
being clipped by the preview legend. This envelope applies when the selected
resource obeys that authored fit contract; arbitrary native/Alt replacements
and procedural GI effects remain outside this proof.

Both actual native controller objects compile with current dependency hashes.
Both actual draw-body fixtures pass ASan/UBSan with real native GBI packets.
The first fixture invocation lacked its external JSON include directory; it
passes after restoring the recorded include environment. Packed keeper/stock,
single-scene video export and deterministic counter reproduction checks pass.
The counter report now accepts vertical OR front separation: a shop display
above the counter need not also sit entirely in front of it. The complete
room/keeper check independently verifies this tighter arrangement.

The POC13 asset remains byte-identical, SHA256
`bca46926d8474b3503653bdc243ee53ff0ab598c3db2b094827ad902a5ada4bf`.
No replacement O2R is required. The new positions require the updated native
source/build; installing the old pack alone cannot change compiled positions.

Source and asset identities are guarded across final rendering. Final media,
geometry evidence, native compile commands/dependencies, regression logs and
independent review are saved alongside the source checkpoint. Previous POC13
media/source remain available as the rollback baseline. Native playtesting
must still check framing and browsing from normal entry and both solo engines.

The refreshed video passes the source/pack guards and independent inspection:
72 frames, 7.2 seconds, 10 fps, complete 1050x760 viewport. All 17 recorded
render inputs still match the current source; the rendered slot coordinates
match the native geometry proof. Independent review found no remaining
actionable issue. The nested clearance evidence now lists actual native shelf
positions, replacing an ambiguous historical unchanged-position flag.
