# Eyelid artwork inputs

These are front-projected texture assets for the original supplied GLBs, not
model appearance previews. `projection.json` pins the source SHA and source-XY
bounds. The compiler projects them onto native expression parts. Inspect the
exported 3D face to judge appearance; the flat sheet changes the apparent aspect.

The original 4K diffuse remains byte-identical. The actor's private face wrappers
select these additional textures. Blink meshes have one joined skin surface;
the surrounding face and native skeleton topology remain intact.

Artwork was produced with the built-in image-editing tool, then limited to
explicit source-space masks. The prompts were:

- For each model, edit a two-panel copy of its own source face. Left: upper lids
  half lowered while retaining the iris and lashes. Right: gently closed eyes,
  curved lashes and matching skin shading. Preserve face, brows, nose, mouth,
  skin colors, lighting, hair, panel layout and landmark positions. No redesign,
  makeup, skin disks, polygonal eyelids, captions or borders.
- Saria Render: center iris/pupil gaze in the neutral and half-open states;
  preserve eye openings, lids, lashes, original blue color and all other artwork.
- Young Malon: remove only the purple dash and pale green dot beside the nose in
  neutral/half/closed states. Preserve everything else. The generated cleanup
  changed an eye, so that change was discarded: only the two small cheek masks
  were applied to the previous artwork. Neutral eyes match the source projection.
- Young Malon mouth: repair the stray dark dimple/crease outside the mouth
  corners, especially the viewer's right. Keep the delicate closed smile and
  all other features. Only an ellipse around the mouth was composited from
  the result into `mouth-neutral.png`; the approved eyes were not copied.
- Adult Malon mouth: gently raise the corners for a warmer closed smile;
  preserve all other features and alignment. Only the mouth ellipse was
  composited into `mouth-neutral.png`.
- Young Malon crown: replace the pale triangular crown defect and thin pale
  outlines with matching orange-red strands. Keep the hairstyle and exact
  projection. `hair-top.png` retains the source outside the crown mask;
  `hair-projection.json` records its separate bounds. No face pixels from
  this edit are used.

The attempted generated clasp reconstruction was rejected by the image tool
and is not used. The final clasp uses the supplied adult model's existing
Bowser artwork on a rebuilt surface, with donor provenance in the O2R manifest.

The actual source GLBs and native game archive are not stored in this directory.
These textures remain visual-review candidates; in-game acceptance is separate.
