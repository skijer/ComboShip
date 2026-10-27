# Regional entrance visual refinements — October 24, 2026

Candidate source refinements after the independently restored 120-file checkpoint
`7f3f7bbd9adfbe50d22aecb53938a06412c26644`. Public upload, full executable
builds and configured gameplay acceptance remain pending. Accepted archives,
masters and the proven encoded texture payload are unchanged.

## Visible corrections

The marked native grave's old shimmer was at local Y=2 inside the closed stone.
It now uses the same 20 complete NEI twinkles in a crown and an external side
ring. The crown clears the native 48-unit stone; normal depth tests remain in
use. Closed and pulled source renders show the beacon throughout its cycle.
Native pull restrictions, other graves, departure and return behavior remain.

The Lake Hylia cave loses the bank switchbacks, landings and tall ramp supports.
Its hollow 322-unit vault, inlet, dry strips and return pose remain, with a low
vertical shoreline plinth embedded in the native terrain. Natural lake/shore
access replaces the former dry walk down from the laboratory bank. Full/refilled
water is 8 units below the lip; the sampled dry north lip rises 10–20 units from
the native basin. Both-age native ledge rules and complete provider fixtures
were inspected; actual swimming/climbing still needs gameplay observation.

The cave uses the adjoining bank's existing stone resource
`scenes/shared/spot06_scene/spot06_sceneTex_00FEB8`: RGBA16, 32×64, masks 5/6.
Continuous planar UVs use native-like texel density. No new texture pixels,
part files, downscaling or asset substitution are introduced.

Regional fields retain 20 twinkles, their art, pulse, clock, colors, alpha and
camera facing. Whole twinkles are distributed over each full black face:
the Lake's 180×216 piecewise roof, the Snowhead 160×180 rectangle and MM's
existing integer-packed arch. The original 128 MM shell positions/shades and
the Woods of Mystery/forest particle implementation remain unchanged.

## Snowhead visibility and native adult ice

The original 200-unit throat used solid blue interior/back colors, and its old
endpoint was hidden by a front-facing native cliff. The cave now darkens from
its icy rim to a black recessed cap. Its shared mouth is (1840,400,0), yaw −16384,
front X=1740, back X=1940; the complete face is 15.619 units in front of the measured
native cliff. The shimmer is 10 units in front of that cap. Native terrain and
its render/collision resources are unchanged.

An actual intersection was found with native adult ice actor 33, params 0x11,
home (−214,20,−1393). Its visual and integer-transformed collision triangles
crossed the former shelf's foundation, even though the walking floor was
higher. The revised early shelf passes north of that complete moving solid;
the native ice actors, their behavior and positions are preserved.

| Check | Native/source result |
|---|---|
| Every native adult ice actor, including spawned orbital twin | Full-orientation mesh spheres plus complete 100-unit orbital disk; minimum horizontal separation 6.58694 units after a further 2-unit rounding reserve |
| Native Ice Cavern exit polygon 87 | Exact minimum complete shelf/exit footprint distance 141.80033 units, exceeding the retained 120-unit reserve |
| Whole shelf collision header | Bounds derived from every actual rounded vertex; all axes contained |
| Continuous native cliff/roof gap | Exact3D triangle clipping over all 22 walking/apron floor triangles: minimum 85.973258 units in both render and collision geometry |
| Native ground joins | At most 0.360699 collision / 1.231754 render units above shelf; ordinary ground joins are recorded separately from overhead |
| Ordinary native standing body | Native adult GetHeight 68 and ceiling 56/radius 18; child 44 and ceiling 40/radius 14; complete 80-unit body-space guard above the 3-unit ground-join allowance |
| Ordinary native walking visual bound | Independent 552-frame idle/walk/run source-model sample: adult maximum 64.255595, child 44.298698; adult overhead margin 21.717663 |

The old 180-unit whole-ascent probe was more generous than native standing
requirements and made the usable corridor unnecessarily narrow. The selected
northern corridor is 70 units wide, with the measured roof gap above. Its cave
throat retains 180-unit headroom. The earlier sampled 98.231855 estimate was
superseded by the exact 85.973258 minimum; no sampling estimate is presented as
a continuous lower bound. No native obstacle was removed to pass these checks.

The native actor sweep includes all yaw/roll orientations and the full orbit,
so bob/sink changes cannot defeat its positive horizontal separation. This is
source geometry evidence, not configured gameplay proof. Custom/Alt actor or
model replacements, jumping, raised weapons, procedural pose adjustments and
mounted traversal were not visually established by the native walking sample.

## Verification and remaining gate

Component red→green checks reproduced the hidden grave, undersized fields,
old ramp/material/UV reset, blue cap, actual ice intersection and collision
header bounds mismatch. Native/sanitized component and combined checks pass:

- `tests/regional_passages/run_tests.py --sanitize`: combined regional native handoff, geometry and lifecycle checks.
- `tests/regional_passages/run_presentation_tests.py --syntax`: fitting, native draw/audio/arena and both graphics translation units.
- `tests/regional_passages/run_snowhead_bridge_tests.py --native-archive /path/to/native.o2r`: post-format complete native scene/ice checks.
- `python3 -B -m unittest discover -s tests/ci -v`: 21 tests.
- clang-format14 and `git diff --check`: clean.

Formatting was followed by an exact C++ pose/geometry/color dump comparison
and native fixture reruns. Logs and independent review accompany the private
source checkpoint.

Lake checks used the full 1050-polygon native scene and a separate portable
inherited-Alt control. No missing custom Alt tree was fabricated. The native
roof proof uses 596 render and 531 collision triangles with exact clipping,
including vertical faces. Whole-input archive identities remain private;
only derived resource/fixture identities accompany source evidence.

Before gameplay acceptance, inspect all three entrances and returns with the
actual pack order and Alt setting; both ages and water states; grave rules;
walking the northern shelf and approaching the original Ice Cavern; camera
behavior, local snow/chill, native moving floes and wave audio. Source renders
approximate lighting/filtering/blending and are labelled accordingly. Keep
develop/master and accepted runtime baselines unchanged until acceptance.
