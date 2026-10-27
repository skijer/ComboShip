# Sword reference image pass

Gilded's length and Master's blade root from this preview are superseded by
the [latest two-sword adjustment](sword-tuned-pass.md). True Master was accepted
and is preserved in that pass.

Master, True Master and Gilded now follow the three images supplied in chat.
Their blades are longer than both the initial bundle and the preceding shortened
pass. Shape matching uses each image's component ratios; the images have
different framing, so their pixel sizes do not determine relative in-game size.

## Previews

- [Three before/after pairs at one common scale](swords_reference_before_after_scale.png)
- [Gilded, Master and True Master rotating](swords_reference_group_2.gif)
- [All nine swords rotating](swords_reference.gif)
- [All models guard-aligned, with effects off](swords_reference_measured_scale.png)

Master has a long straight blade, a narrow ricasso, distinct shoulders and a
short final tip taper. True Master follows the gold image's stepped blade and
rounded-ended bar guard, retaining the approved gold blade, white handle and
blue gemstone. Gilded has a shorter red grip, silver angular quillons and three
broad gold diamonds separated by silver triangular faces.

The two Master variants have independent blade/hilt ratios and guard geometry;
they are no longer forced to share an authored shape. Their full model lengths
remain equal. Gilded's long blade and short hilt follow its own image ratio.
Biggoron and Great Fairy retain their corrected models and remain longer overall.
The two Kokiri variants, Razor and Four Sword are also preserved.

| Sword | Previous full length | Current full length | Blade / hilt from guard |
|---|---:|---:|---:|
| Master | 70.8 | 92.4 | 3.67 |
| True Master | 70.8 | 92.4 | 2.78 |
| Gilded | 75.6 | 101.6 | 4.70 |

Lengths are exported model units before host actor transforms. Ratios describe
the authored components that emulate the supplied images, rather than recovered
Nintendo GI measurements.

The approved shimmer colors and particle themes remain current. Emitters now
reach the new blade tips. Local and foreign OoT shop poses fit the larger models
without changing their pickup/world size.

These previews use the exported geometry and actual production particle
triangles. Every model uses a shared camera scale; comparison sheets align the
guards using translation only. Lighting and reflective shading are offline
approximations. Original Nintendo GI vertex coordinates and in-game visual
verification remain unavailable; this is a reference-shaped authored pass.

The original preview branch contained media and notes. The implementation is
now included on the `slate-element-preview` development branch, with subsequent
shape adjustments recorded in `sword-tuned-pass.md`.

## Verification

- All 60 serialized GI models pass geometry/winding, vertex-cache, references,
  matrix, texture and transparency checks.
- The focused image-ratio check passes for all three rebuilt models and confirms
  the other six swords' GLBs and installed resources remain unchanged.
- Fresh combo diagnostics pass native and foreign draw paths, local and foreign
  shop fits, effect attachment, optional shimmer and missing-resource fallbacks.
- The effect policy passes AddressSanitizer and UndefinedBehaviorSanitizer.
- All four GIFs contain 72 distinct looping frames. The shared camera contains
  current and previous meshes and real particle samples without clipping.
- Independent source, exported-solid winding and preview review found no
  critical or important issues. These checks do not replace in-game viewing.
