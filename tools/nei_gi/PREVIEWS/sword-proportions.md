# Sword GI proportion correction

Master, True Master and Gilded from this earlier pass are superseded by the
[user reference image pass](sword-reference-pass.md) and the
[latest two-sword adjustment](sword-tuned-pass.md).

This preview implements the requested correction to the six sword silhouettes.
The original Nintendo GI vertex payloads are unavailable in this environment.
Razor, Gilded and Great Fairy use recovered public held-sword geometry as a
secondary shape reference; absolute GI scale remains a visual reconstruction.
Master and True Master retain identical dimensions, as their actual fallback
uses the same pedestal sword.

## Previews

- [All nine swords rotating](swords_proportions.gif)
- [Guard-aligned models at one common scale, effects off](swords_proportions_measured_scale.png)
- [Six before/after comparisons at the same scale](swords_proportions_before_after_scale.png)
- [Kokiri variants and corrected Razor](swords_proportions_group_1.gif)
- [Gilded, Master and True Master](swords_proportions_group_2.gif)
- [Biggoron, Great Fairy and Four Sword](swords_proportions_group_3.gif)

The camera fits all nine models and actual particle samples together. Each sword
uses the same world-to-screen scale within a view; models are never individually
resized to fill their cells. The comparison sheets translate guard centers to a
common baseline. Offline lighting and reflections approximate the game renderer.
These are exact exported meshes and sampled production particle triangles,
not gameplay captures. In-game appearance has not yet been checked.

## Changes

- Biggoron has a longer blade, narrower guard, slimmer blade proportions and a
  longer two-handed grip. Its upright full length now exceeds Master.
- Great Fairy has a much longer blade and grip, with botanical reliefs moved
  coherently to fit the new shape. Blade/width and grip/blade ratios follow the
  recovered held outline; full length is approximately 1.724 times Gilded.
- Master and True Master have a more compact blade and wing guard. True Master's
  gold blade, white handle and blue gemstone are preserved.
- Gilded's blade fraction and guard width are tuned against the recovered held
  reference while retaining its three fitted gold diamonds.
- Razor has a connected asymmetric slotted blade: root bridge, two internal
  bridges, two enclosed holes, a shorter lower spur and a hooked main tip. The
  prior two independent straight prongs are removed.
- Particle emitters follow the corrected blade spans. Razor and Biggoron no
  longer apply the old baked lean. Larger two-handed swords have separate
  shelf-only poses; their pickup scale is preserved.

The two Kokiri models, Four Sword model, approved hex palette and effect themes
are retained. See [the palette](sword-particles.md#palette-and-particles).

Current full model lengths after the exported scale matrix, before host actor
transforms (authored model units, not measurements of original Nintendo GIs):

| Sword | Length |
|---|---:|
| Razor | 73.44 |
| Gilded | 75.56 |
| Master | 70.82 |
| True Master | 70.82 |
| Biggoron | 104.63 |
| Great Fairy | 129.88 |

## Reference scope

[Public MM held-model sources](https://github.com/hylian-modding/Z64Online/tree/d1c59355532d7fd81209a40e73dbf586a4738aa9/src/Z64Online/mm/models/zobjs)
supply the secondary Razor, Gilded and Great Fairy silhouettes. Recovered held
Gilded blade length/width is 8.909 and blade/full length is 0.777; Great Fairy
blade length/width is 5.657 and blade/rear span is 3.649. These describe the public
held-model payloads and do not establish native GI dimensions. Biggoron and
Master proportion changes are visual review candidates. Exact native GI scale
calibration still requires original GI vertices or a traceable native GI render.

## Verification

- All 60 serialized GIs pass geometry/winding, vertex-cache, reference, matrix,
  texture and transparency checks. Razor's five blade solids also pass closed
  shell and outward-winding checks.
- Focused shape checks pass Fairy/Gilded part ratios, Master/True geometry
  equality and unchanged hashes for the two Kokiri models and Four Sword.
  All 32 textures on the four corrected forged swords retain the bundle bytes.
- GI combo diagnostics pass native/shared/foreign palettes, intrinsic effects,
  optional shimmer, routing/fallbacks and the actual foreign shop callback.
  Both enlarged swords clear local and foreign OoT shelves; model and particle
  matrices share the same pose. World/pickup size is preserved.
- The particle policy passes strict compiler checks and Address/Undefined
  Behavior Sanitizers. All 648 sampled model/effect rotations fit the shared
  camera; all four GIFs contain 72 distinct looping frames.
- Independent review found no remaining critical or important issues.

At this preview's original publication, the broader native MM diagnostic suite's held-item fixture
error (`Audio_PlaySoundGeneral` is undeclared in unchanged `equip_helper.c:272`)
remained outside this correction. The fixture include is now corrected and the
full MM suite passes; see the [integration verification record](../PUBLICATION.md).
This preview does not claim a full game build
or in-game visual verification.

The original preview branch published media and notes. The implementation is
now included on the `slate-element-preview` development branch; later shape
adjustments are recorded in `sword-tuned-pass.md`.
