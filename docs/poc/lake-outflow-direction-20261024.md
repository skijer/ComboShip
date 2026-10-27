# Lake outflow direction correction

Superseded shape: the user rejected the shallow runout in this record. The
current explicit vertical curtain and distance-based UV correction are recorded
in `lake-vertical-spill-20261024.md`.

Baseline: `a9ac46d706dfbe6b879a5fca56fb46adc72d7322`, tree
`0e6b85a69c2bd3a0ce329c1d5c0b034af6c524b6`, with its saved recovery version.
The user clarified that the cave stream must descend outward into Lake Hylia.
The previous lake-to-cave motion and level water surface are not accepted.

Bounded correction: keep the cave, native bank, dry route, solid channel banks,
waterbox ownership and material repairs. Give the water a descending surface
from the cave to the open lip, then a short falling/fanning section beyond the
authored apron that joins the native lake level. Move the wave crests outward.
The walkway is only eight units above the full lake, so preserve that real
height relationship instead of inventing a tall full-lake waterfall. The
existing drained/dry variant stays intact; no long fall is drawn through the
exposed native basin.

The preview must include a clearly labeled lake-level context plane so it
shows the outlet joining a receiving surface. It remains an authored cutaway,
not a capture of the user's configured scene.

Checks: fail the prior flat surface and inward UV phase, then verify monotonic
descent, an actual outside-apron spill, continuous bank contact, outward phase,
lake-height connection, native/custom texture contracts, fog-safe alpha, dry
route/collision preservation and the existing full/drained lifecycle. Preserve
the original candidate and record runtime testing as unproven.

Evidence: the prior flat builder failed the descending-water check. A
read-only replay of the prior draw provider (`--provider-ref a9ac46d7`)
separately failed the actual queued-vertex descent check. The corrected native
Lake suite passed under ASan/UBSan, including full, two intermediate levels,
drained and refilled states, outward UV phase, wrapping, alpha packing and
collision/provider controls. Independent review sampled 525 water points at
each supported wet level against the authored geometry and measured native
collision control: no solid-floor intersections, 35 quads in each wet state,
and empty geometry when drained. No blocking review findings remain.
The complete integrated regional sanitizer runner also exited 0 after this
correction, including the native Snowhead, entrance handoff and boot controls.

The updated authored preview uses the actual builder and wave phase, with a
separate receiving lake-level context plane. It was inspected at several
animation phases. The drop is intentionally small at full water; the preview
does not prove the configured native lake mesh join, fog or in-game appearance.
