# Raised main counter POC13

| Field | Record |
|---|---|
| Preserved source | POC12 `zzzz_Skiejer_Black_Market_POC12_KeeperLean.o2r`, SHA256 `761fa811149f860b6a174901bdda1ad7fdf5610a0064f0e3efe5927853bb1352` |
| Source branch | `poc/black-market-counter-20261018`, preserved base `e5d857e2`; root's separate single-camera correction is `7f5965a5` |
| Candidate | `zzzz_Skiejer_Black_Market_POC13_RaisedCounter.o2r`, SHA256 `bca46926d8474b3503653bdc243ee53ff0ab598c3db2b094827ad902a5ada4bf` |
| Requested result | Raise the counter toward the existing crossed hands, preserving the grounded keeper and full Sway animation, with room props and merchandise clear. |
| Implemented result | Main counter X[-51,58] top rises from Y30 to Y41; trim underside rises Y26 to Y37. Bases stay Y0. Left counter X[-160,-51], including its own low seam endpoints, stays unchanged. |
| Preservation | 188 of 192 payloads byte-identical; keeper model/animations/placement, all scene headers and routing, other room scenery/props, materials, textures, original UVs, scrolling and cosmetics unchanged. |
| Status | Offline asset/geometry/clearance verified; configured gameplay, Alt parity and user acceptance untested. No full game build or master promotion. Exact continuous flat palm contact is not claimed. |

## Height and actual hand evidence

All 32 native Sway frames were measured using the complete 1,266-triangle left
hand, 1,280-triangle right hand, both forearms, and the full 16,304-triangle
keeper mesh. Geometry is clipped to the actual central countertop's XZ region,
including triangle edges/interiors. The stationary keeper remains at
`(0,1,130)`, yaw `-32768`, scale `.01`; no animation or model payload changes.

Y41 is the highest integer flat top avoiding the complete hand/body loop.
Minimum model clearance over the counter is `0.16887088`. Lowest hand-surface
gaps range from `0.16887..1.96431` on the left and `1.02018..2.45409` on the
right. Area-weighted downward hand-surface gaps range `1.42910..3.02312` and
`2.09427..2.93021`, respectively. These additional measurements avoid treating
a wrist position or dangling fingertip as proof of flat palm contact.

At Y42, left-hand geometry would penetrate by up to `0.83113`. Even shortening
the counter from front Z93 to Z108 would leave hand geometry at Y42.1118 below
a proposed Y43 top; further narrowing removes palm support. The full animation
therefore remains a non-intersecting lean with varying gaps, rather than hands
continuously flush to one static plane.

Contact inspection artifacts are in `../market-counter-candidate/contact/`:
front/side projections at frame 0, frame 3 (least simultaneous contact), and
frame 12 (closest left hand). They show exact posed geometry with textures and
lighting omitted. Root owns the separate textured room/keeper preview.

## Exact geometry, seam and collision delta

Only these entry payloads change; no resource paths are added:

- `custom/skiejer_market/geometry/mahouya_room_0DL_006548_vtx_2`: 46 original
  Y values change by +11 and eight side-cap vertices are appended. Every
  original X/Z/UV/color value and all other vertex rows are retained.
- `custom/skiejer_market/geometry/mahouya_room_0DL_006548_tri_2`: two native
  four-vertex loads and two `Triangles2` commands are appended before the
  existing end command. All original command bytes remain unchanged.
- `scenes/nonmq/skiejer_market/collision`: 34 original counter records change
  and four matching seam records are inserted in actual draw order. Existing
  4,552 collision vertices and all surface/camera/water metadata remain intact;
  21 vertices are appended, giving 4,573 vertices and 7,823 polygons.
- `custom/skiejer_market/manifest.json`: records the counter and collision audit.

The X=-51 join follows the actual exposed profile. The upper face covers
Y30..41/Z93..123 and faces -X. The retained lower lip end covers
Y26..30/Z120..123 and faces +X. Both inherit the existing scrolling
GranmaShop21 material. Opposite winding is necessary under native backface
culling. The empty forward underside receives no blanket face. Original low
left endpoints (661,726,727,730,731,734,735) remain unchanged; the donor already
provides separate right endpoints, so no original triangle becomes a ramp.

The regression independently compares every drawn nondegenerate face with
collision in native order, recalculates every collision normal/distance, and
checks both new normals and the two exact profile regions. Original UVs,
materials and native cache limits remain intact.

## Retained scenery and merchandise

All 16 left counter faces are byte/geometry-equivalent to POC12. Every changed
or added counter face was screened against all 7,865 non-counter room triangles
in newly occupied Y26..41. No unresolved AABB overlap pairs remain. The 160
nearby retained book/dish/prop faces are at least ten units away in X. Lower
side planes and fixed ground edges retain their original footprint.

Eight actual production-generated sample offers use retained checkpoint
geometry: ice rod, switch hook, fire rod, Great Fairy Sword, shovel, lantern,
Mario mask and spinner. Their complete continuous 360-degree sweeps, +/-2 bob
and .25/.32 caller scales clear the raised counter. Minimum vertical/front
separations are `14.42778 / 37.19805`. Native wrapping-X envelopes include
minimum radii, ribbon width, second-loop offset and 1/64 vertex quantization;
their minimum vertical/front separations are `8.31283 / 32.39738`. The analytic
envelopes were also checked against real C++ `BuildLock` output on 64 actual
AABB cases per item. Shelf positions need no change for these samples.

This evidence covers those eight authored samples and their native X geometry.
It does not establish every catalog/native/Alt/replacement model, omitted GI
procedural effects, runtime visibility, interaction or fixed-point pose error.

## Reproduction, rejected probes and recovery

Run from the repository root with new output/report destinations:

```sh
python tools/black_market/counter.py \
  --source ../market-keeper-candidate/zzzz_Skiejer_Black_Market_POC12_KeeperLean.o2r \
  --output ../market-counter-candidate/zzzz_Skiejer_Black_Market_POC13_RaisedCounter.o2r \
  --report ../market-counter-candidate/counter-report.json \
  --json-include ../verification-deps/nlohmann_json/include
CPATH=../verification-deps/nlohmann_json/include python tests/black_market/check_counter.py \
  --source ../market-keeper-candidate/zzzz_Skiejer_Black_Market_POC12_KeeperLean.o2r
```

The green regression creates two candidates and compares complete ZIP bytes,
all unaffected payloads, collision geometry/planes, prop clearance and source
hash preservation. It first failed before implementation; strengthened prop
checks subsequently rejected both the full-counter and ramp probes. Captured
final output is `../market-counter-candidate/evidence/check-counter.log`.

`REJECTED_POC13_WholeCounter.o2r` (SHA256
`67720ef4bef7798710b6bb5ca1aab82cb0f4b903e22ba84c15f6b888f9f55371`)
and its report/note preserve the rejected whole-counter probe, which would
intersect left-side books. `REJECTED_POC13_Ramp-note.json` records the ramp
rejected before output because low dishes occupy that section. Neither is the
final POC13. Keep the unchanged POC12 input to discard this experiment; the
three-file bounded source commit and exact patch recipe reproduce the final
candidate. Root saves the shared durable milestone and owns final independent
review, native/room checks, rendering and acceptance status.
