# GI particles for placed Saria and bridge portals — 2026-10-20

| Field | Record |
| --- | --- |
| Baseline | Bug-fix integration input `a2b774ec0c0ef528fc35d23022e50376f9d5219c`; recovery note `c1be555b5`. The reported scrolling-circle appearance is a defect, not an accepted visual baseline. |
| Candidate | Bounded source change on `fix/bugfix-integration-20261020`; no master promotion or configured runtime acceptance. |
| Hypothesis | Reuse the real GI glow/star sampler primitives and safe GI shimmer renderer around the existing actor and doorway anchors, replacing the independent hollow rings and connected rim band. |
| Permitted scope | Particle sampling/submission, one OoT C viewer render bridge, and corresponding regression checks. |
| Preservation | Actor transforms/parameters, pose/blink/props, interaction/reward claims, geometry, collision, scene headers, resource ownership, textures/materials/scrolling, audio and progression. No Woods of Mystery assets or other asset payloads are added or changed. |
| Evidence | Actual production draw bodies, real GI renderer/GBI fixtures under ASan/UBSan, existing actor/routing/doorway regressions, native object compilation and C symbol checks. |
| Verdict | Implemented and source/build verified. Linked game, GPU appearance, exact installed pack combinations and user acceptance remain untested. |
| Recovery | Retain the input branch/assets. The bounded source commit can be reverted independently; no archive restoration or regeneration is needed for this repair. |

## Actual GI path and replacement

The working GI path is `NeiGi::SampleShimmer` in
`soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h`, which uses `Glow` and
`Star`, followed by the common `NeiGi_DrawMesh` renderer. Native and foreign
item shimmers use `NeiGi_DrawShimmerMesh` to clear incoming grayscale, reserve
restoration commands and restore material/matrix state.

`combo/actors/ForestGiParticles.h` now uses those actual `Glow` and `Star`
primitives. Each separate particle has a small filled green hex core with a
small GI star glint inside it and a soft GI glow. There are no hollow inner
contours, connected doorway bands or emitters scrolling around the doorway.
The particle cores are approximately 0.65–1.2 world units in radius; their
halos remain below 2.3 units. Ten particles follow Saria, sixteen follow each
portal. Meshes are finite, frame-derived, camera-facing and bounded to the
existing GI mesh capacity; seated Saria uses the lower emission height.

`NeiGi_DrawWorldParticles` is a narrow C entry into the existing safe GI
shimmer renderer. It preserves that renderer's arena reservation, grayscale
boundary, material restoration and matrix handling. World callers supply the
existing world origin and doorway yaw, with no GI item rotation or scaling.
No GI pickup effect policy, elemental effects, model geometry or resource
selection is changed.

The production callers are:

- MM `ComboSaria.cpp`: all six scenery/song-giver pose variants use the common
  particle sampler, still following the actual authored actor position.
- OoT `EnViewer_DrawStaticSaria`: the four placed static poses call the C render
  bridge in `ComboForestActors.cpp`. Native story cutscene draws retain their
  existing route. The old waiting-only solo forest Kira emitter is removed to
  avoid a second, inconsistent aura.
- Both hosts' `ComboSoloForest.cpp::DrawForestPortal`: only the effect draw and
  its include change. Entry/return state, native forest generation, title cards,
  doorway size selection, anchor and yaw retain their existing behavior.
- MM `ComboLostWoodsPassage.cpp`: opening forest and Woods of Mystery effect
  draws use the GI sampler. The opaque passage geometry and native/pack mesh
  ownership code are unchanged. Existing pause/cutscene/mode guards remain,
  with explicit pause suppression added to the two effect draws.

**Palette mapping:** Saria and both bridge/portal particle routes are green,
including the opening forest and both solo hosts. The exported OoT return
tunnel's existing purple scrolling material is unchanged. No active world
particle caller retains a purple particle palette. The old offline preview
headers (`ComboForestPortalEffect.h`, `WoodsMysteryHexEffect.h`,
`SariaHexEffect.h`) are no longer used by the production draws; an old preview
exporter using them is not evidence of this candidate's runtime effect.

Legacy opening-forest pack versions 1/2 retain their prior exclusion from the
additional finish draw, including their existing resource/placement behavior.
Versions 3–8 and the native fallback keep their original anchor selection.

## Verification

| Check | Result |
| --- | --- |
| Original production draw regression before repair | Expected FAIL: no GI mesh submission |
| OoT placed viewer regression before adding bridge | Expected FAIL: no particle bridge call |
| Direct GI mesh submission with incoming grayscale | Expected FAIL before using the established shimmer wrapper |
| Portal green identity check against the earlier purple candidate | Expected FAIL, corrected before delivery |
| New particle policy, ASan/UBSan | PASS: green filled hexes, real Glow/Star geometry, finite/extreme frame values, three camera bases, small disconnected triangles, open doorway and stationary doorway emitters |
| Actual production draw bodies, ASan/UBSan | PASS: OoT four static poses, MM six variants, authored origins, both doorway sizes, solo/combined routing, pause/distance/height/cutscene guards and title behavior |
| Both native GI renderer fixtures, ASan/UBSan | PASS: exact candidate triangle vertices, colors and alpha; incoming grayscale; material and original transform restoration; failed allocation and arena exhaustion |
| Existing forest actor suite | PASS: model/resource lifecycle, blink, props, fixed transforms, collision, dialogue and once-per-file song grant |
| Existing solo forest actor suite | PASS: spawn/readiness/authored precedence, dialogue and stationary horse routing |
| Existing native solo doorway draw suite, both hosts | PASS: active native/packed sizing, anchors, title and guards |
| Existing opening portal and Woods ownership suites | PASS: legacy/native/pack selection, solo/combined behavior and no duplicate arch |
| Native production object compilation | PASS: eight affected actor/portal configurations, plus both GI presentation units |
| GI wrapper native symbol | PASS: unmangled `NeiGi_DrawWorldParticles` in both host objects |
| Diff whitespace | PASS |
| Linked executable, configured GPU/runtime visuals | Untested; unavailable in this environment |

The native object configurations are MM Saria; OoT forest actors and viewer
with Combo enabled and disabled; both solo forest units; MM passage; and both
hosts' complete GI presentation translation units. Existing header macro and
deprecation warnings remain in the named compiler/test logs; no new compile
failure or sanitizer failure remains.

Reproduce from the repository root with the existing public dependency include
paths supplied through `CPLUS_INCLUDE_PATH`:

```sh
python3 -B scripts/diagnostics/run_forest_gi_particles_tests.py
python3 -B scripts/diagnostics/run_forest_gi_particles_tests.py \
  --baseline-ref a2b774ec0c0ef528fc35d23022e50376f9d5219c # expected failure
python3 -B scripts/diagnostics/run_forest_actor_tests.py
python3 -B scripts/diagnostics/run_solo_forest_actors_tests.py
python3 -B tests/black_market/run_solo_forest_draw_tests.py
python3 -B tests/lost_woods_entrance/run_portal_finish_test.py
python3 -B tests/woods_mystery/run_active_pack_test.py
python3 -B scripts/diagnostics/compile_forest_actors.py \
  --source mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp \
  --source soh/soh/Enhancements/Gameplay/ComboForestActors.cpp \
  --source soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c --output OUTPUT
python3 -B scripts/diagnostics/compile_solo_forest_core.py \
  --unit mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp \
  --unit soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp \
  --unit mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp --out OUTPUT
```

The focused runner also executes the existing both-host GI renderer suite
with the exact new particle meshes. `--policy-only` runs only the pure particle
sampler and needs no game assets or external dependency headers.

## Smallest decisive runtime test

Use the candidate build with the same native donor/model/scene packs and load
order as the control. No new particle pack is required. Keep Woods of Mystery
assets outside the deliverable.

| Route | Configuration to record |
| --- | --- |
| OoT placed Saria | Static En_Viewer `0x7F03/13/23/33`; native and selected Alt model; solo OoT and combined launch |
| MM placed Saria | Scenery `0x7F13/23/33` and Termina Field reward `0x7E13/23/33`; solo MM and combined launch; native/Alt under the existing resource owner |
| OoT return doorway | Solo forest/native and installed pack treatment; verify purple tunnel scroll with separate green particles |
| MM opening forest bridge | Native and active pack route; existing return anchor/yaw and scene/setup |
| Woods of Mystery bridge | Native bridge and any already installed authorized bridge pack, both Alt states, combined and solo-MM route when available |

At one near gameplay camera distance, verify the effects look like small
separate green hex sparkles with a soft GI glow. Move a placed Saria and verify
that only the aura follows, with the seated height lower. Confirm no large
circle/continuous outline, clear doorway traversal, unchanged tunnel material,
proper pause/resume, re-entry stability and unchanged dialogue/song claims.
Record scene/room/setup, age, actor parameters, save flags, packs/order and Alt
state with the clip. A successful compile or CPU mesh/GBI fixture is not visual
acceptance and does not promote a sacred master.
