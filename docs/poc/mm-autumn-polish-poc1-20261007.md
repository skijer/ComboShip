# MM Autumn Polish POC1 — 2026-10-07

The approved candidate broadens autumn leaves across the scene, adds gentle
intermittent showers, and applies reversible colors to selected scene foliage.
The follow-up request for a quieter foreground is included. This is a source
candidate with focused compiler/harness evidence; game appearance, platform
build, performance and acceptance remain untested.

| Field | Record |
| --- | --- |
| Source baseline | ComboShip `develop`, `13901c677d0084e0305eec4672c0557f4434aea7` (merge of cumulative fixes #36) |
| Isolation | Local branch `poc/mm-autumn-polish-20261007`; patch against the exact source baseline |
| Art baseline | Existing four generated RGBA leaves and accepted two-cycle combiner fix; byte-identical to the source baseline |
| Scene references | Termina Field Broad POC2 R1 material support, POC4 checkpoint documentation, Woodfall Surroundings Henriko POC2 texture archive |
| Active scene ownership | Runtime uses the currently loaded display lists; no older scene export is installed or substituted |
| Candidate status | Implemented and focused static/harness verified; no platform binary, deployment or master promotion |
| Recovery | `candidate.patch`, complete changed files and verification logs in the checkpoint ZIP; original archives unchanged |

## Leaves and foreground density

One live native snow actor owns the autumn presentation. Multiple native
blizzard actors cannot multiply its budget. The native 128-particle allocation
and actor lifecycle remain intact.

| Band | Leaves | Initial horizontal distance from camera | Size multiplier |
| --- | ---: | ---: | ---: |
| Foreground | 16 | 250–1,000 game units | 1.0 |
| Middle | 32 | 1,200–2,800 game units | 1.7 |
| Background | 48 | 3,200–6,000 game units | 2.4 |

The total stays at 96. Origins remain in world space as the camera turns or
moves; leaves drift with wind and flutter, then recycle after falling below the
view or moving sufficiently far away. Newly recycled leaves fade in over 24
actor updates. Distant alpha stays readable through 6,000 units and fades out
between 6,000 and 8,000.

Leaves fade to zero within 150 units of Link **or** the camera and reach full
opacity at 350 units. This fade follows the live positions, including movement
and wind, rather than relying on a spawn-only exclusion radius. The original
leaf textures, palettes, transparency/combine commands and maximum alpha are
preserved. All autumn presentation random choices use the private weather RNG.

Switching back to Winter or native snow restores the native particle volume
and fall speed on the confirm render, even before another actor update. This
prevents distant autumn positions from producing invisible native snow for a
frame. Reinitializing those cosmetic positions also uses the private RNG.

## Intermittent autumn showers

Autumn uses its own existing weather-state instance, independently of global
weather settings. It starts dry for approximately 2–4 simulation seconds, fades
into rain over one second, sustains a shower for 8–14 seconds, fades out over
one second, and rests dry for 20–25 seconds. Pause freezes the cycle. Scene/room
changes reset it with the existing weather lifecycle.

The added layer caps at density 15, audio gain 0.6 before the user's rain-volume
setting, and overcast 0.5. It adds no thunder. Leaves continue through both wet
and dry phases. Story/interior/underwater restrictions and Off remove the added
presentation. Native environment state is preserved; native Day 2 storms and
explicit global weather overrides can still provide stronger rain. Judge the
new cycle on Day 1 or 3 with the global weather override disabled.

## Selected foliage and edited scenes

The material treatment uses the renderer's existing RGB grayscale-color blend,
wrapped only around selected triangle batches in a copied loaded display list.
Its fourth color byte controls RGB interpolation; it does not replace texture
alpha. Original vertices, UVs, texture identity, primitive colors, combine and
render modes remain exact. Each batch ends with the treatment disabled.

| Scene/material identity | Treatment |
| --- | --- |
| Termina Field `Z2_00KEIKOKUTex_021650`, `_0216D0`, `_0218D0` | Burnt ochre `#D99C45`, the near/mid/far clover foliage slots used by the Djipi import |
| Termina Field `Z2_00KEIKOKUTex_034898` | Muted rusty red `#B96848`, the foliage-edge slot |
| Woodfall approach `Z2_21MITURINMAETex_0055D0` | Muted rusty red `#B96848`, one selected tree/vegetation backdrop material |

Both canonical scene namespaces (`Z2_…/` and `Z2_…_scene/`) and explicit Alt
texture hashes are recognized. An existing loaded list's cold Alt replacement
is resolved before patching. Unloaded vanilla lists and unrelated rooms are
left to the regular render path. Off, other seasons, excluded scenes, Alt
switches and resource-instance replacements restore the original source
command. Later patches to the same source deactivate this variant rather than
replaying stale commands. Queued copies retain their source until the existing
resource-registry teardown boundary safely releases them.

This handles edited lists that retain the selected material identities. It
does not promise coverage for arbitrary new texture names, segmented image
loads, unsupported custom commands, or pre-existing grayscale treatments;
those lists are preserved. The Woodfall binding and palette appearance in the
currently active scene export still need game inspection.

## Evidence and limits

Fresh verification logs accompany the checkpoint:

- Native weather harness with AddressSanitizer, undefined-behavior and bounds
  checks: 16/32/48 distribution, stable origins, distant alpha, Link exclusion,
  first-render native snow restoration, native weather/fog and gameplay RNG.
- Full production scene-foliage body with real MM/GBI structures, sanitizer
  checks and all six supplied serialized Termina materials: exact original
  command preservation, Alt cold loading/toggles, later patches, unused-list
  avoidance, instance replacement, season cleanup and teardown release.
- Existing weather state/mixer/bridge/audio suite: wet/dry cycle, audio/sky,
  pause, native-state preservation, story/view restrictions and restoration.
- Existing whole native canopy/ambient/impact-leaf regression suite.
- Four generated leaves loaded through the native texture factory, and the
  accepted single-owned-texture/two-cycle RGBA combiner check.
- Whole changed snow actor and scene-material module syntax against real
  integration headers; `git diff --check`.
- Independent read-only review found no unresolved critical or important
  source-POC defect after the first-render restoration correction.

The harness boundaries replace engine projection, GPU submission and archive
services where assets/game execution are unavailable. Historical material
fixtures prove command preservation, not the active edited pack's rendering.
No full CMake/platform build, link, generated game archive, game execution,
frame-time measurement or visual acceptance is claimed. Existing controller
macro and const-qualification warnings occur in the native-header checks.
Leak detection is disabled for the sanitizer runs; address/undefined/bounds
instrumentation remains enabled. An optional sanitizer attempt on the older
untouched canopy harness could not link its omitted collision/profile boundary
functions; its normal complete harness passes, and the new weather/material
harnesses pass with sanitizer instrumentation.

## Decisive game comparison

Build the patched MM module in a separate candidate installation, using the
existing generated-leaf archive and the current accepted scene packs/load
order. No new scene archive or leaf-art generation is required by this POC.

1. In Termina Field on Day 1 or 3, disable the global weather override, select
   Autumn and compare the same standing, running and combat views to the base.
   Check foreground restraint and visible middle/background motion while
   turning and traversing the field; assess size, depth, occlusion and fog.
2. Observe at least 90 seconds of wet/dry transitions with uninterrupted leaves,
   rain audio and gentle sky changes. Pause/unpause during a shower.
3. Check the selected field foliage and Woodfall approach backdrop with Alt
   enabled and disabled, including a cold scene entry and an Alt toggle.
4. Confirm Winter, Spring, Summer and Off; enter/re-enter an interior and the
   two outdoor scenes. Verify exact material restoration and native weather.
   Include a native Snowhead storm to check shared ownership/restored snow.
5. Compare frame cost with the same camera, save and packs. Acceptance requires
   the user's judgment of readability and unobtrusive gameplay, not draw counts.

Keep the accepted installation and scene archives unchanged until this exact
candidate passes those configuration-specific checks and is accepted.
