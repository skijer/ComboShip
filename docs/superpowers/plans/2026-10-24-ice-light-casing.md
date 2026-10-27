# Ice and Light sword casing visibility

User request: make the existing Ice and Light casings appreciable at normal
gameplay distance. Keep the accepted Fire/Din effect. This is bounded visual
tuning on top of protected shortcut checkpoint
`25e990332ef9d9d5147c2df4ef5413c61af7aa5b`.

The translucent material multiplies primitive alpha, I8 intensity and vertex
alpha. Thin bright patterns over a low intensity floor therefore fade when
filtered down. Raise Ice's core/envelope alpha from 120/190 to 200/225 and
Light's phase-zero alpha from 115/165 to 185/220, retaining its +/-20 pulse.
Broaden the existing procedural veins/rays and raise the texture floor to 48.
Keep their colors, scroll speeds, transparency, geometry and model identities.
Do not change Fire, gameplay, sound, selectors or other checkout work.

Execution:

1. Change the shared material and deterministic texture recipe; regenerate.
2. Synchronize the existing source preview's phase-zero opacity values.
3. Run the existing elemental regression suite and native host compilation.
   Compare all generated geometry and Fire maps byte-for-byte with baseline;
   measure the before/after material and filtered source appearance.
4. Request an independent read-only review, record evidence and runtime limits,
   commit only owned paths and save an independently restorable checkpoint.

Source projections and native GBI checks can establish a stronger submitted
material, preservation and compilation. Actual gameplay blending, installed
assets, animation and normal-camera appearance remain a configured runtime
acceptance step. Do not merge or publish the cumulative candidate while the
separate What's New handoff is outstanding.
