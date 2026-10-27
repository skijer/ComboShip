# Sword GI particle preview

The initial geometry preview below is superseded by the
[user reference image pass](sword-reference-pass.md) and the
[latest two-sword adjustment](sword-tuned-pass.md). The palette and particle
themes remain current.

The approved particle pass uses the seven exact models supplied in
`NEI_Sword_GI_Latest_20261003.zip`, plus the existing Razor and Biggoron models.
The two Kokiri variants retain their own appearance and effect color in either
game. True Master has the supplied gold blade, white handle and blue gemstone.

## Rotating previews

- [All nine swords](swords_rotation.gif)
- [OoT Kokiri, MM Kokiri and Razor — larger view](swords_rotation_group_1.gif)
- [Gilded, Master and True Master — larger view](swords_rotation_group_2.gif)
- [Biggoron, Great Fairy and Four Sword — larger view](swords_rotation_group_3.gif)
- [Four-angle contact sheet](swords_rotation_contact.png)
- [Models with effects disabled](swords_rotation_models_only.png)

Each animation samples 72 frames of a complete model rotation. These are exact
exported meshes with particles sampled from the shared C++ runtime policy;
camera, lighting and reflective-material shading are offline approximations.
Opaque geometry occludes particles behind the blade. In-game appearance has
not yet been checked.

## Palette and particles

| Sword | Shimmer hex | Added particles |
|---|---|---|
| Kokiri (OoT) | `#78C850` | Green leaf flecks |
| Kokiri (MM) | `#A46CFF` | Purple wisps |
| Razor | `#BDD6EA` | Silver edge sparks |
| Gilded | `#FFD45A` | Slow golden motes |
| Master | `#6F8FFF` | Blue blade wisps |
| True Master | `#FFF4D6` | Ivory ribbons and `#F4C95D` gold motes |
| Biggoron | `#FF9A42` | Forge embers |
| Great Fairy | `#79BE84` ↔ `#9382C4` | Alternating green/violet petal spirals |
| Four Sword | `#315B2F`, `#D8232D`, `#2289CF`, `#6D3593` | Four colored blade trails |

Great Fairy's shimmer smoothly cycles green → violet → green over 360 gameplay
frames. Four Sword retains five moving glint clusters in green/red/blue/violet/
green order. The blade trails and particle emitters rotate with their sword;
small glints and soft ribbon edges face the camera.

The existing always-on shimmer remains on standard swords; Four Sword's shared
shimmer follows the NEI item-effects option. The intrinsic particles accompany
the authored models even when the optional shimmer is disabled.

## Verification

- All 102 supplied payload hashes match; all seven imported GLBs and 75 game
  resources reproduce the supplied files byte-for-byte.
- All 60 serialized GI models pass geometry/winding, references, vertex-cache,
  matrix, texture and transparency checks.
- GI combo diagnostics pass native/shared sword palettes, intrinsic effects,
  both host draw paths, distinct Kokiri routing, missing-resource fallbacks,
  optional shimmer, and prevention of duplicate shimmer. The approved Slate
  ring attachment and rune colors also pass.
- Independent code review reported no findings.
- At this preview's original publication, broader native MM diagnostics passed
  rendering, ownership, dispatch and Fire/Ice interpolation, then stopped in the held-item fixture because unchanged
  `mm/mods/items/helpers/equip_helper.c:272` calls undeclared
  `Audio_PlaySoundGeneral`. Later checks in that suite were not reached.

The fixture include is now corrected and the full MM suite passes; see the
[integration verification record](../PUBLICATION.md).

The original preview branch published media and notes. The implementation and
subsequent geometry adjustments are now included on `slate-element-preview`.
