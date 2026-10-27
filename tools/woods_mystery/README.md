# Woods of Mystery visual POC6 — baked bridge, fitted canopy and detailed grass

This additive scene candidate fits the existing nine-room Woods of Mystery to
cor's requested Lost Woods treatment. All four openings in every room use the
recovered MM3DS external mouth and a cropped interior sleeve (36 mouths).
The separate OoT/2ship bridge keeps its approved fitted wood shape. Vanilla
collision, daily maze routing, actor placement/logic, transitions and audio remain unchanged.

The donor has 246 faces and 321 source vertices, recovered from the unchanged
`zzzz_MM3D_Lost_Woods_Black_Market_POC14.prelude.o2r`, SHA256
`7b8f2b820ba0bf4441889693f820d280f8dace6f4664387f847bf0fdeecbdce9`.
The long donor is cropped rather than compressed. Maze mouths start at local
175 and end at 450; width1.20 and height1.0 are retained. The approved bridge's
first120 units and its30-unit continuation retain all166 wood triangles and28
terminal black-cap triangles. The floor correction below changes only their
lowest serialized Y1/2 vertices to-3/-2; all other attributes remain exact.

The supplied grass, rock and vine textures remain scoped to this scene.
The main floor and added coverage use one detailed grass sampler with a shared
world UV phase; original floor positions and normals are preserved. Original Henriko ENVIRONMENT canopy
and forest-edge pixels/alpha and native foliage normal bytes are retained. Only
the shared bridge-clearance pivot changes position as described below.
Donor RGB stays unlit; native wall and hidden backing normals stay lit.
The inward plank-band UV/color seam repair remains unchanged.

## POC6 correction and preservation

cor explicitly accepted the POC5 fitted tunnel geometry, especially the diagonal
OoT/MM bridge. The floor correction changes only low Y1/2 vertices in the9 room
wood vertex resources; all9 wood display lists remain byte-identical. This
protects all36 maze mouths, every donor UV/color, all194 bridge
wood/continuation/cap triangles and added tree geometry. Henriko foliage and
private texture bytes remain exact. Vanilla collision and daily routes stay native.

The new screenshots expose close-camera gaps outside the tunnel shoulders.
The original fitted wall ends near depth206 while hidden backing started210.
A horizontal ray from(190,50,100) through(208,50,114.5) passes through the gap.
Changing only the non-floor backing's near edge to200 overlaps the retained
wall by at least3.375units on the sides and3.4375units at the roof. Backing
sides remain±116 and roof145, outside accepted wood by6/10units. The movement
applies to all72 shoulder sides in all9 rooms, with the far450 joins intact.
Existing grass underlay and corridor wings remain below/alongside the wood.
An independent762048-ray control found2430 POC5 local misses and zero with200.

The grass screenshot shows a different issue: the native main floor still used
a two-stage distance blend while the added corridor/underlay used a single
texture sample. POC6 uses the same detailed supplied grass material everywhere.
All grass UVs use S=-6.4worldX and T=-6.4worldZ. This removes the native per-room
offsets as well as the material-boundary mismatch; positions, normals and collision
stay unchanged. Single-stage sampling, equal tile shifts and matching global
phase are checked at every grass triangle, including all main-room floors.

The floor-contact revision keeps grass visible inside the tunnels. The original
transparent corridor fade rose fromY0 toY10; making it opaque exposed a ramp
above the native flatY0 collision. Added corridor grass, wings and tails now
sit atY0. The original main floor staysY0 and the hidden clearing underlay stays
Y-1. Serialized wood/cap low vertices atY1/2 move down4units toY-3/-2, including
their matching annulus vertices. Every X/Z/UV/normal/color and higher wood vertex
is preserved. The same correction is built into the independent native bridge.
Collision, triggers and daily routing remain native. Tests decode the actual
collision header and protect the precise19scene/2native vertex-resource delta.

Curved native shoulder remnants protruded in front of the logs. Only portions
inside the existing opaque backing volume are trimmed: depth200..450,
across+/-116 and Y-4..145 in each cardinal direction. The backing remains intact,
and flat native depth240 fascia are retained. New cuts interpolate signed
lighting normals and UVs from the original faces. The fin regression checks
all nine rooms, fascia area and removed-surface rays. A view looking back out
through a tunnel's intentionally open near face can see the clearing; closed
side/roof joins must retain a local opaque hit. No new collision is introduced.

The low foliage curtain already overlaps the wall geometry, but the round crown
artwork has a transparent border that leaves a floating sky band. POC6 maps only
those16 triangles per room to the existing Henriko forest-edge strip and fits its
opaque lower band over the walltop. Its logical tile is32x16; T is75% of the
normal forest-edge mapping (equivalent to37.5% of the previous canopy T).
S coordinates, normal bytes and vertex alpha remain exact.
The higher crown and forest-edge lists keep their accepted artwork and mapping.
An alpha-aware control reproduces187 sky rays around Room1; the fitted mapping
closes all23328 sampled rays across nine rooms and two texture conventions.
This verifies sampled cutout coverage, not exact renderer rasterization.

The low curtain's shared Room1 pivot at(213,88,-214) crossed the diagonal
bridge roof in14 triangle pairs. Raising only that pivot toY119 clears the
actual wood by at least4units in the sampled roof profile while preserving
the walltop coverage above. The native bridge has an eighth built asset:
the original38-vertex canopy resource with only indices0/27 moved toY119.
Four scoped filepath vertex-load patches preserve native culling, materials
and all other commands; they verify and restore together with the wall patch.

Seasonal targeting includes these stable scene-local texture paths beneath
`custom/cor_woods_mystery/poc1/textures/`; the historical namespace is retained:

| Role | Resource suffix |
| --- | --- |
| Detailed grass | `grass` |
| Green trim in wall and ruin tiling | `rock`, `vines`, `ruins` |
| Walltop curtain and high forest strip | `henriko_forest_edge` |
| Overhead leaf crowns | `henriko_canopy` |

The matching source targets the four requested groups: ground grass, green
wall trim, native foliage geometry and grass actors. Private filepath images
use green-only seasonal pixel variants with their original dimensions, scale
metadata and alpha. Native seasonal materials and grass palettes are reused.
Standing, carried and replacement-grass draws include Woods; replacement
flowers/stems retain their existing material scope. Spring ground/actor grass
uses the current spring-grass palette. Stone/bark base tones, black masks and
portal/shimmer colors keep their existing roles. Seasonal behavior requires
the matching source build, separately from the editable scene archive.

The earlier fitted black boundaries remain at their original440 depth and only
the12 pre-existing exits. Unfogged black state is restored correctly before lit
backing. All28 bridge cap perimeter edges remain welded to the forest annulus.
Aperture-disjoint original wall faces remain intact, preserving the repaired
upper silhouette. Broad outer-rim probes did not establish a new uncovered
perimeter. The former protruding wood threshold is now submerged beneath grass.

The native baked bridge integration and its tests are recorded in the final
source/recovery handoff. It preserves the194 accepted bridge triangles and
provides its own textures and bridge-only wall cut through built-in resources;
the complete Woods makeover remains an optional scene pack. Native entrance
collision and route triggers are not redefined by the visual model.

## Loading and rollback

Use `zzzzz_Woods_of_Mystery_OoT_Makeover_POC6.prelude.o2r` as the active Woods
candidate. Remove earlier Woods POC overrides, enable Alt Assets, and let this
pack win over other `Z2_26SARUNOMORI` room replacements. Reload the scene.
In Prelude, load native MM plus this override and choose **Continue editing**.
Open **Woods of Mystery**, **Room1** for the OoT bridge. All nine rooms contain
four fitted maze mouths. This is an editable baked scene layer. Export as a
**Prelude project** to retain it; original and bare-path ZIP layouts are supported.

Animated green hollow hexes still require the matching ComboShip source build.
The archive alone does not add that effect. Inherited resource-ownership,
Alt Assets, combined-game and solo-addon availability guards are unchanged.
The POC6 native integration ships the accepted bridge separately in the built
2ship asset set. Regenerate `2ship.o2r` through the normal build workflow to include
the eight committed assets; a previously built executable/archive does not gain
them by loading the scene pack. The purple intro treatment stays separate.
Remove this archive and reload to roll back. Earlier candidates remain separate;
no accepted master is replaced or promoted.

## Rebuild and recovery

Python3, NumPy, Pillow and Node are required. From the source root:

```sh
python -B tools/woods_mystery/build_candidate.py \
  --native /path/Woods-of-Mystery-Native-Resources.zip \
  --poc /path/tunnel-source-resources.zip \
  --donor-archive-sha256 7b8f2b820ba0bf4441889693f820d280f8dace6f4664387f847bf0fdeecbdce9 \
  --textures /path/textures --foliage /path/foliage --output /path/output
python -B tests/woods_mystery/verify_candidate.py \
  /path/output/zzzzz_Woods_of_Mystery_OoT_Makeover_POC6.prelude.o2r \
  /path/Woods-of-Mystery-Native-Resources.zip
```

The original full donor POC can replace the exact donor subset; omit the
provenance SHA argument in that case. Every used donor resource is separately
hashed. Sorted archive names and fixed ZIP timestamps support exact rebuilding.

The compact recovery contains the complete asset-source snapshot, all required
asset inputs, cached Prelude parser reference and cumulative full-source patch.
Run `python -B rebuild.py` in a fresh extracted folder. Small historical builder
snapshots reconstruct POC2/3/4/5 and the original POC6 controls from the same inputs and require their
original archive hashes before running preservation tests. This avoids including
five nearly identical full texture packs. No network fetch is needed.

For full game source, independently retain the original
`Woods-of-Mystery-POC1-Source-Recovery.zip`, SHA256
`5d450ee7e52599371af204283634a8715b0c541cc7148f8dcfba49a509942925`.
In another fresh extracted POC6 recovery folder run:
`python -B rebuild.py --base-recovery /path/Woods-of-Mystery-POC1-Source-Recovery.zip`.
This restores that full baseline, applies the cumulative patch and requires the
exact final Git tree recorded in the manifest. `--native-tests` additionally
runs inherited effect/ownership and forest tests with POC1's saved dependencies.
The compact asset restore is self-contained; full game restoration depends on
that separately retained POC1 baseline.

## Evidence and remaining proof

New shoulder and grass tests first fail against POC5 for a close local sky ray
and the native two-stage grass material. They protect grass XYZ/normals, a common
UV phase and every accepted wood/cap face. Prior seam/backdrop tests continue
to protect all36 mouths, upper walls, black silhouettes, serialized cap closure,
actual room culling bounds and native non-mesh header bytes. The actual Prelude
manifest validator and layer restorer check original and canonical ZIP layouts.

Reports distinguish the native build integration from the optional scene archive.
Static geometry, compiler/harness checks and exact asset restoration do not prove
configured runtime appearance, antialiasing, fog, live resource switching/culling,
performance or traversal. The accepted POC5 wood remains the preservation baseline;
new native ownership, shoulder closure, grass and canopy rendering need runtime inspection.
No accepted master is replaced or promoted by this candidate.
