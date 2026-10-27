# Lake flowing inlet POC — 2026-10-24

Superseded direction: the user subsequently clarified that this stream must
flow outward and down into the lake. The current correction and evidence are
in `lake-outflow-direction-20261024.md`; this record preserves the earlier POC.

Baseline: reviewed material/ownership checkpoint `834bc321`, tree
`f36fc7a3846d431a35e7cb7f7e07ca531d25425b`. Its independently restored source
checkpoint is retained. The user's second clip rejects the turquoise strip as
a representation of flowing water; this is a new bounded channel revision.

The existing 20-by-412 trough uses a circular ripple sprite, sits behind an
unbroken front curb, and inherits an opaque setup despite its alpha. Correcting
the texture-slot defect alone does not address those presentation problems.

Use the existing Lake Hylia crest texture for the native fallback (RGBA16,
32x32), retaining the named replacement's existing I8 64x64 contract through a
separate load branch. The native resource was inspected in the actual archive;
its resource SHA256 is
`638b44b871a88f5abbf7be61e9113c09c50326f5e10f18fe3a27a9516a881fdb`.
The texture scrolls toward the dark cave through a translucent water surface.

The chosen channel treatment is a wider recessed course along the same south
side with mildly uneven banks and a flared, open lake intake. Cut only the
authored front curb at that intake. Preserve the native lake, lab bank, cave
vault, portal, dry main route and return, native waterboxes and drained gate.
The old narrow south rim becomes a decorative bank; it is not the travel path.
These are authoring choices for this candidate, not user acceptance.

Alternatives considered: replacing only the sprite retains the conspicuous
ruler-straight strip and closed intake; flooding the whole entrance removes
the dry main approach. The bounded side channel retains the existing route.

Verification must exercise a connected open intake, dry main body clearance,
actual solid bank/collision orientation, native/replacement texture tile
contracts, translucent render mode, forward flow and complete drained
suppression. A geometry/material preview can illustrate this candidate but
does not establish in-game rendering, swimming or user acceptance.

Independent review found that standard fog replaces shade alpha with its fog
factor. A new regression executes the actual production shader-input packing
loop for fog factors 0, 64 and 255; it failed the vertex-alpha version. The
corrected surface uses per-quad primitive RGBA with `MODULATEIA_PRIM/PASS2`, so
water opacity and contact fade reach the packed input while fog remains active.
The native command budget includes the added primitive-color packet per quad.
The four short lake-contact bands fade to a soft edge without vertex-alpha
dependence. The test also found and now closes the two small intake bevel caps,
with explicit collision rays at both corners failing before that correction.

Implementation steps:

- [x] Add failing geometry checks for the open lake mouth and dry main route,
  plus real draw checks for water texture format/size and translucent mode.
- [x] Replace only the authored trough/banks and front curb around the intake.
- [x] Use the native wave tile, preserve the replacement contract, and correct
  blending/flow at the existing draw boundary.
- [x] Run native Lake/material/regional checks and inspect a candidate preview.
- [x] Save a recoverable checkpoint and review the added channel delta.

The focused native Lake fixture and full integrated regional sanitizer suite
passed after the review corrections, including
actual GBI/combiner/packed-input consumption, texture format/dimensions,
translucency, UV direction/wrap, collision classification and provider lifetimes.
An authored cutaway using actual BuildGeometry and native texture pixels was
inspected: the watercourse, banks, dry route and moving crests read distinctly.
It is labeled as a CPU geometry preview, with approximate shading and no native
scene/GPU capture. Configured runtime acceptance remains pending;
develop/master stay unmerged.
