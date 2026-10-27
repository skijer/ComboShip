# Ice and Light casing visibility checkpoint

Branch: `poc/sword-ice-light-visibility-20261024`.
Protected prerequisite: `25e990332ef9d9d5147c2df4ef5413c61af7aa5b`
(`poc/snowhead-regional-actors-20261024`). That branch and its saved source
checkpoint are retained. The separate What's New and broken Giant's Knife
checkouts are not modified or integrated by this pass.

## Result and scope

Ice and Light retain their existing translucent, fitted casings, colors and
animation. Ice's primitive alpha is now 200/225 instead of 120/190. Light's
phase-zero alpha is 185/220 instead of 115/165; its +/-20 pulse remains.
Broader frost veins and light rays, with a higher intensity floor, reduce
attenuation from texture filtering and multiplied vertex/material alpha.
The retained procedural maps are 64x32 I8, range 48..255.

All 2,153 generated vertex arrays, 126 display lists and geometry tables are
byte-identical to the protected prerequisite. The generated header differs
only in the two intended I8 maps. Both Fire maps, its material, scroll speeds,
opaque/translucent routing, and all blade profiles/fits are unchanged. There
are no new textures, vertices, draw commands, actors or state settings.
Gameplay, damage, sound, colors/overrides and selector behavior are unchanged.
The existing preview and Biggoron depth control use the new opacity values.

## Verification evidence

Passed on this candidate:

- `python3 -B scripts/assets/build_elemental_sword_aura.py --check`
- `ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_elemental_sword_aura_tests.py --sanitize`
  including actual native GBI, held identity, selector integration and complete
  renderer TU compilation in OoT/MM, each in standalone and Combo modes.
- `ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_biggoron_aura_tests.py --sanitize`
  including three-rig/eight-roll/four-section source coverage, native identities,
  reload/Alt boundaries and full native player-library syntax checks.
- `git diff --check`.
- Independent read-only review: no critical, important or minor findings;
  deterministic generation, full generated-data preservation, depth coverage
  and increased filtered texture contribution independently checked.

A separate native C sampler executes the actual shared material before/after
for all 1,024 animation phases and both layers. Normalized Fire GBI is identical;
texture bytes are independently identical. All element scroll values and
material command counts match. At reference vertex alpha 150, combining each
layer at the mean I8 intensity gives:

| Element | Previous opacity range | New opacity range | Gain at minimum |
| --- | --- | --- | --- |
| Ice | 0.1292 | 0.2671 | 2.07x |
| Light | 0.0922..0.1211 | 0.2370..0.2835 | 2.57x |

These reference calculations describe the submitted material, not measured
screen brightness. Actual geometry, overlap, UVs, backgrounds and filtering
affect perception. An offline phase-zero comparison was inspected at 48/96 px
blade size and in detail, using exact generated geometry/maps over a neutral
blade. It shows stronger casing while preserving the motifs; it is not a game
capture or a full RDP simulation.

The full elemental regression script initially stopped because the restored
environment lacked `nlohmann/json.hpp`. The repository-pinned v3.12.0 dependency
was restored outside the checkout (commit
`55f93686c01528224f448c19128836e7df245f72`). The full script is being rerun with
its include directory in `CPLUS_INCLUDE_PATH`; its final result is pending at
this source milestone. No production workaround or test exclusion was added.

## Handoff and remaining proof

This is an isolated, reviewed source candidate. Native test compilation is
fresh evidence for the changed headers; no fresh complete application build,
configured gameplay capture or user visual acceptance is claimed. The earlier
successful Linux build belongs to the protected shortcut prerequisite.

Next configured visual check: normal gameplay camera in both engines, Ice and
Light held/swinging across representative blade profiles and dark/bright scenes;
verify visible casing, readable metal, subtle Light pulse and unchanged Fire.
Keep the cumulative build/publication hold for the separate What's New handoff.
Do not promote master from these source controls.

The recovery artifact contains the candidate source bundle, a bounded binary
patch from the protected prerequisite, exact ref/tree identities, changed-file
copies, verification evidence and restoration instructions. It requires only
the identified public base; the previous protected private checkpoint is not
needed to restore this candidate. Original input archives remain unchanged;
normal aura regeneration uses retained repository inputs and needs no archive.
