# ComboShip seasonal visibility correction — October 13, 2026

Baseline: PR #41 `bridge/audit-merge-20261008` at
`4cf4866d2dd33551e8a1372a5cce8bd7ad40196e`, including the audio-completion boot fix.
Cor reports the installed game now boots, accepts Termina's winter appearance,
and reports weaker adjacent MM frost, weak spring actors/brush/grass, missing
MM mist, difficult aurora visibility, missed Hyrule Field materials and night
blackouts in winter and spring. The three supplied screenshots were inspected.

This is a separately reviewed source candidate. Installed-pack/GPU acceptance
and develop/master promotion are separate from the checks below.

## MM tint continuity and spring readability

The existing MM registry covers 171 exact identities and their native, Alt and
`_scene` aliases. No missing private MM identity was established without the
currently mounted MM packs. The source nevertheless restricted winter grass
and tree actors to Termina, gave other scenes a weaker blend without the white
lift, and omitted branching-tree frost entirely. The candidate applies the
accepted Termina palette and lift consistently in every eligible outdoor scene.
Tests explicitly cover Road to Southern Swamp, both swamp variants, Woodfall,
town/Milk Road and shared resource transitions in both directions. A shared
display-list instance now has the same winter treatment across scene aliases.

Spring's former 32/255 grass blend produces approximately `43,103,29` from
native `40,110,25`, which still reads green. Selected grass/understory now uses
`F6C1D9` at 224/255 with the existing 1.8 lift; the same sample becomes roughly
`109,95,95`. Selected scene canopies use the matching lift, and actor canopies
use full blossom color. General ground remains native in spring. Summer and
Native restore original resources. This applies in both ComboShip modules.

Pixel-copy paths preserve alpha and non-green source pixels; draw-scoped actor
checks preserve native command streams, bark/stumps, interaction/drop behavior
and separately emitted reward markers. Decorative foliage in arbitrary HD
actor replacements remains an installed-pack visual boundary.

## Hyrule Field exact resource coverage

Source: `!Hyrule Field - FINAL(1).o2r`, 194,513,783 bytes, SHA256
`6a3c2662e03598a1b3aa4eaa6d421466dac9d0a295f121af0d5c7d864b5d49ad`.
The source archive is unchanged. Inspection covers 12 Alt room headers, 44 mesh
roots and 2,166 reachable lists with no unresolved hashes. The candidate adds
51 exact missing identities: 40 ground, 9 understory and 2 canopy paths,
including private exported/imported castle grass.

Four atlas paths use explicit foliage bounds, also included in the immutable
copy cache key. They protect masonry, wood and exposed trunks even when those
pixels are greenish. The same-image/different-bounds regression verifies that
copies cannot bleed across aliases. Road pavers, water, caustics and an
unproven mixed building/tree atlas remain outside the registry. The conservative
Ki02 canopy bound leaves its lowest fringe native to protect the exposed trunk.
Exact identities, bounds and source hashes are in
`season-visibility-targets-20261013.json`.

## Winter atmosphere

Both auroras placed their colored cores above native downward-facing gameplay
cameras. Lower fixed compass arcs retain their geometry budget, colors, alpha,
night/dusk/dawn timing, sky gates and depth occlusion. Native-camera tests cover
10/15-degree pitch, evolving arcs and all full-night hours. There is no 03:00
activation gate.

MM's original thin blue-white mist had poor contrast against Termina frost and
small projected footprints. Only MM changes to RGB `150,190,225` and depth/FOV
halfheight bounded at 16–48, preserving 0.24 alpha and terrain grounding.
Modeled emitted footprints are 10.76–11.28 native scanlines, with peak canopy/
grass contrast 7.73/11.48 RGB levels. OoT mist production rendering is unchanged,
matching cor's report that it is already visible there. These measurements are
projection/blend models, not GPU screenshots of the candidate.

## Shared weather, lighting and lightning

Actual supplied Field palettes reproduce autumn/winter red/green ambient
clipping at 03:00 and 21:16. Night protection caps the seasonal subtraction at
half the available native ambient and usable directional light, with gradual
dusk/dawn easing. Daytime offsets and native adjustment ownership remain exact.
Spring, summer and Native retain their original light behavior. The native
lightning flash path bypasses seasonal shade, then restores it after release.

The broader native time-table, overcast handoff and restoration checks cover
240 manual/Follow-Rod, season, time, independent rain and rain-mode cases plus
40 thunder flash/release cases. MM shades proportionally and reconstructs its
raw light channels each call; no equivalent MM clipping/feedback defect was
demonstrated, so its weather controller is unchanged.

The supplied Field spring palette does not explain the spring blackout.
That remains unresolved. Existing **Weather Audio Diagnostics** now records
bounded `[light-probe]` snapshots after world draw: scene/room, day/palette
clocks, manual/resolved/raw saved selection, raw/final/adjusted light and fog,
light directions, native indices, gloom/flash/native weather, camera/story and
debug overrides. It does not call the self-healing saved-season getter. The
decisive control is Spring versus Native at the same nighttime location and
clock with this checkbox enabled, followed by inspection of the resulting log.

## Verification and acceptance boundary

Focused production/state/renderer, real-header and ASan/UBSan checks cover both
games' seasons, native rain/sky/lightning, texture/cache restoration, region
protection, actual archived pixel samples, private RNG, pause/re-entry and
actor/save/environment ownership. The hosted gate additionally runs MM foliage,
grass, scene-material and real-texture regressions. Local leak detection is
disabled because the container cannot inspect process task files.

Full hosted Windows/Linux compilation and packaging belong to the published
candidate run. No new game/GPU appearance or universal blackout fix is claimed.
The original boot/audio fix, scene/archive bytes, geometry, collision, UVs,
scrolling, weather progression and accepted Termina winter recipe are retained.
