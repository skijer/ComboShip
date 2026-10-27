# Forest NEI twinkle correction — 2026-10-21

cor explicitly requested the actual NEI twinkle on Saria, the world Mirror,
and BOTH 2Ship bridges: Lost Woods and Woods of Mystery. Portal particles
must fill the black entrance face, including its center. Border emitters and
filled hexagon particles are removed. Hexes belong to the scrolling portal
material, whose authored primitive RGB is separately changed to green.

Baseline: `e62668e077d699dd51e91af891a5d3567f012332`. Candidate branch:
`poc/native-forest-shimmer-mirror-20261021`. Source publication and merging
are explicitly authorized; configured gameplay and visual acceptance remain
unverified.

`ForestGiParticles::Emit` calls the existing `NeiGi::SampleShimmer` with its
green Leaf identity. Every source twinkle's 72 vertices are copied, moved as
a unit and uniformly scaled for the world emitter. Geometry, spin, pulse,
RGB and per-vertex alpha are retained. No replacement particle primitive or
additional bloom is authored. Five twinkles are sampled in each phase group.

Each portal uses a staggered five-column/four-row field on a plane in front
of the black opening. The Lost Woods native authored portal is radius68,
centerY66; the old geometry-only fallback retains radius44/centerY65.
Woods of Mystery uses its own anchored radius60/centerY66 opening. Both
production draw paths submit the complete field through the guarded native
GI world-particle renderer. Saria uses ten twinkles around the placed actor,
with seated and standing heights. The uncollected Mirror uses five; owned,
queued and overhead casting states suppress that world emitter.

Evidence:

- A new canonical-shape assertion failed the old filled-hex policy first.
  It then passed exact per-vertex comparisons to the real NEI sampler over
  camera bases, frame overflow, seated/standing and all entrance sizes.
- Entrance tests require central, upper, lower, left and right coverage in
  front of the entrance plane. Both actual bridge draw bodies must submit
  their exact expected mesh, including native old/authored Lost Woods sizes.
- `scripts/diagnostics/run_forest_gi_particles_tests.py` passed the policy,
  actual OoT viewer bridge, both Saria bodies, both bridge bodies, both solo
  forest bodies and both native GI renderer boundaries under ASan/UBSan.
  The renderer checks vertices, color/alpha, matrices, grayscale restore and
  exhausted-arena behavior. Existing mask, bottle and season cases passed.
- Both native Mirror fixtures passed with the world-only callback and the
  separately tightened stump-cap pickup condition.
- The existing GI effect source is unchanged. Freshly compiled baseline and
  current exporters produce byte-identical180-frame energy/shimmer streams
  and native orb streams.
- The offline preview exporter includes these production meshes and the
  production Mirror geometry with runtime1/16-unit vertex quantization.
  Mesa uses standard source-alpha blending, without extra bloom. Portal
  borders and actor silhouette are schematic guides, not game captures.

Preview reproduction:

```sh
c++ -std=c++20 -Icombo tools/nei_gi/runtime_preview/export_forest_twinkles.cpp -o /tmp/export-forest
/tmp/export-forest /tmp/forest-twinkles.bin
python3 -B tools/nei_gi/runtime_preview/render_forest_twinkles.py /tmp/forest-twinkles.bin /tmp/forest-preview
```

The decisive configured check remains both bridge directions with the actual
installed mod stack, Saria in each placed pose, and an unowned Mirror before
and after walking the new log onto its stump. Particle preview is not proof
of scene load order, live camera placement or native gameplay acceptance.

Mirror palette follow-up: cor accepted the preview except the Mirror's green
color and delegated a suitable replacement. Starting from merged source
`09d26152330c946f4e8e2934ee97a06c75308b7e`, the Mirror alone now samples
NEI's existing Neutral palette: pale-blue halo `#A8E9FF`, icy glints `#E6F8FF`
and white core `#FFFFFF`. Saria and both entrances retain Leaf green. Shape,
pulse, motion, alpha, size and pickup behavior are unchanged. The existing
canonical test first failed on the old green RGB and then passed the full
native particle/renderer suite. Comparing all 180 exported frames confirms
that only Mirror RGB differs; all other streams and Mirror geometry, positions
and alpha are byte-identical. This is verified source/offline rendering;
the new palette has not been captured in configured gameplay.
