# Slate and elemental palette preview

Candidate based on `poc/mm-gi-routing-followup-20261003` at `40cbef51`.

[Comparison sheet](slate-element-preview.png) shows the previous Slate, current
front/back geometry, five raised rune badges, and the Shadow Crystal.
[Rod and spell palette](elemental-rods-spells.png) shows the elemental GI materials
alongside their designated shimmer hex values. These are
software renders of the same quantized triangles exported to GLB and GI assets;
they are not game screenshots. The user approved the visual designs after reviewing the exact PNGs on GitHub.

The Slate remains upright, with a charcoal chamfered housing, raised bronze eye,
amber triangles/circuit terminals, wrapped top handle, and rear eye seal. Rune
iris, badge and optional shimmer share these colors:

| Rune | Hex |
| --- | --- |
| Slate | `#30CAFA` |
| Bomb | `#5FDCEB` |
| Master Cycle | `#64E6BE` |
| Stasis | `#FAC846` |
| Cryonis | `#96D7FF` |
| Sensor | `#C882FF` |

| Element/item | Hex |
| --- | --- |
| Fire | `#FA8B20` |
| Ice | `#357CFF` |
| Light | `#FDFF7B` |
| Hylia | `#FF96FF` |
| Zonai | `#64FFE6` |
| Demise | `#000000` |
| Sand | `#E8AC48` |
| Tornado | `#91EFBC` |
| Water | `#38A6FF` |
| Meteor | `#FF4818` |
| Storm | `#B5A8FF` |
| Shadow Scepter / Shadow Crystal | `#9E38DA` |

The Shadow Crystal retains its near-black core and translucent facets. Rod
focuses/inlays now match their existing effect hex values. Fire/Ice/Light and
spell geometry is preserved; their optional shimmer now follows their element.
Enable **NEI item effects** to see animated shimmer in game (default remains off).
Material lighting and procedural textures modulate the nominal hex colors.

Regenerate the changed mesh assets:

```sh
python3 tools/nei_gi/SOURCE/quest_revamp.py sheikah_slate slate_bomb slate_master_cycle slate_stasis slate_cryonis slate_sensor shadow_crystal sand_rod tornado_rod water_rod meteor_rod storm_rod shadow_scepter --install
python3 tools/nei_gi/verify_assets.py
python3 scripts/diagnostics/run_nei_gi_tests.py --combo --held
```

Validation: all 59 serialized GI models pass geometry/winding, vertex cache,
references, matrices, textures and transparency checks. Production GI tests
cover all 64 authored bindings, effects on/off, base/Alt resources and
common/shop/acquisition routes; new regressions check elemental shimmer,
distinct rune hues and the shared cross-game renderer boundary.

At this preview's original publication, the broader MM suite passed GI bindings/resource ownership, native renderer,
dispatch and Fire/Ice interpolation checks, then failed in
`tests/mm_nei/run_held_tests.py`: unchanged `equip_helper.c:272` calls
`Audio_PlaySoundGeneral` without a declaration under this compiler. Later MM
suite checks were not reached. Existing C-header warnings are also emitted.

The fixture include is now corrected and the full MM suite passes; see the
[integration verification record](../PUBLICATION.md).

Slate geometry increased from 1,404 triangles to 4,316 (base), with rune variants
between 4,372 and 4,796. GPU performance and appearance still need in-game
review; visual design approval applies to the exported mesh previews. Exported checkpoints remain marked `runtime_tested: false`.
