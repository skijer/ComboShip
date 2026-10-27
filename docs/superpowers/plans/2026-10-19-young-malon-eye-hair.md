# Young Malon oblique eye and hair-root POC11

Baseline: POC10, SHA256 40eef326945f6d3d8181c1ad1e77e4f96bcd099ad928ba533d500e6bebb7072c.
The user accepts the result except a patch crossing the eye at 45 degrees and off-color hair roots.
No game build, merge or master promotion is part of this repair.

Evidence: archive readback assigns the oblique patch to R0/EyesNeutral, not hair or Base.
Its folded upper-inner lid overlaps the iris. Source texture ownership confirms mesh 25.
Source hair meshes 1 and 24 contain pale gray/pink strands and a peach root stain.
Two registered UV crop edits supply corrected root colors. Only audited hair-owned texels
at the roots may consume those edits; the original body atlas stays byte-identical.

1. Isolate and correct the eye join while retaining the accepted orbital shape.
2. Transfer the two root edits into a private head diffuse with a mesh/height/UV guard.
3. Verify exact baseline lineage, protected resources and source contours, then inspect
   front, both 45-degree views, steeper profiles and all blink states.
4. Deliver a bounded archive delta, reproducible source, hashes and offline previews.

Protect all poses, body skin, brooch, facial artwork, eye-state routing and accepted Twins.
Final status requires visual inspection; Python/archive checks alone are insufficient.
Runtime remains untested; user review is the next acceptance gate.

Result: POC11 f8b915f53716b3b54e32286bdbb6cac022dc49e02f6a4c2c1167f737bb27ba69.
The final surface uses concave-rim ear clipping and shared-interior-edge refinement,
not a radial fan. The original14/16 perimeter edges and1818 retained subtriangles
remain exact in position, normal and UV. Allfive eye states share the new cavity
sheet. The original body/rig/skin/poses/eye artwork are byte-identical to POC10.
Hair uses the existing head-private atlas binding;22 records change,316 remain exact.
105 Python tests, canonical125-route decoding, all protected-byte checks and
front/45/65-degree/blink inspection pass. Independent review found no blocker;
tiny quantized projected slivers remain a documented minor risk without an
observed defect. Runtime, user acceptance and promotion remain pending.
