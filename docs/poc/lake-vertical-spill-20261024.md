# Vertical spill correction

Baseline: `e9354ab329a5a993051824c62bd2dd5ef2cb5f89`. The user rejected its
longer, shallow runout: the extension must fall downward into the lake.

The earlier representation made height a function of X, so it could not form
a vertical water face. Its UV distance also depended on X alone, which would
not animate downward along such a face. Replace that representation with an
explicit short rollover and a vertical curtain at the front edge. There is no
projecting horizontal runout. Flow UVs follow distance along the course and
then down the curtain, and the curtain ends at the native lake surface.

Preserve the solid channel/cave, collision, dry route, native water ownership,
texture replacement contract, fog-safe material and drained variant. Bound the
variable curtain subdivisions and use that maximum for the frame-arena guard.
The full-lake fall retains the actual scene height; the preview must show the
vertical face close enough to judge rather than describe a shallow extension
as a waterfall.

Required checks: actual vertical faces immediately outside the lip; no distant
runout; downward UV distance in queued vertices; exact lake-level contact;
native/replacement and water-level states; frame-owned geometry and bounded
allocation; independent geometry review and an inspected authored preview.
Configured gameplay and user acceptance remain pending.

The prior runout fails the new edge/vertical-curtain control. After changing
geometry, the old X-only UV writer separately fails the native queued-vertex
check for downward texture distance. The corrected Lake fixture passes under
ASan/UBSan for native/replacement textures, full/intermediate/drained/refilled
levels, fog-safe packed alpha, vertical motion and exact lake contact.

Independent review checked 145 water levels and 69,330 geometry samples:
continuous surface/UV joins, outward-facing curtain normals, distinct rounded
rows and no solid-floor intersections. Generated counts are at most 34 quads,
within the 36-quad allocation guard. The integrated regional sanitizer suite
passed. A final uniform curtain-opacity adjustment removes visible horizontal
opacity bands; the focused native Lake suite passed again after that adjustment.

The authored preview includes a close-up at uniform scale to make the small
vertical face inspectable. Its lake-level context continues out of view rather
than appearing as a detached rectangular platform. It uses actual geometry,
flow-distance UVs and native pixels with approximate shading; it is not a game
capture. The small full-lake drop and configured native-water join still require
in-game inspection and user acceptance.
