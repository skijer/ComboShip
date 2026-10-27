# ComboShip seasonal weather — October 12, 2026

Target: `marsh6487/ComboShip`, PR #41, `bridge/audit-merge-20261008`.
Preserved baseline: `fea48b50907f070820abfe07220b9a134a17fe1c`, including
the completed SoH 9.3 merge and Windows CRLF scroll-patch repair.
The OoT port is implemented in this repository's `soh/` module. The winter
atmosphere additions are also implemented in this repository's `mm/` module.

## Scope and preservation

Skijer's saved Rod ownership, season functions, scene/world effects, water
display lists and randomizer mode/gate handling remain intact. The SoH 9.3
pins, save schema, shared grants, repaired dungeon keys and authored assets
are unchanged. ComboShip remains 0.4.0 with archive version 1.27864.26741.

Manual Audio Editor selections change presentation only. They bypass the
self-healing saved Rod getter and do not grant the Rod, unlock seasons,
change the saved gameplay season, consume gameplay RNG, or write seed/story
progression. Follow Game / Rod uses the existing ownership/getter functions.
Native removes the added seasonal presentation. Interior, story, game-over,
disabled-sky and underwater-camera restrictions apply.

| Season | OoT presentation in ComboShip |
|---|---|
| Spring | 96 drifting pink petals; pale-pink tree canopy and light understory blend; dry season default with independent rain modes |
| Summer | Native-green foliage; dandelion fluff, fireflies and optional sunbeams with independent controls |
| Autumn | Accepted warm foliage recipe, 96 falling leaves using the exact MM artwork, steady season-default rain at density 30 |
| Winter | Accepted cold foliage/shading recipe; supplemental snow and optional flurries; independent freezing mist and nighttime aurora |

Spring uses `F6C1D9` with canopy blend 224/255 and understory blend 32/255.
Tint copies always start from the immutable source texture. Only green
foliage pixels change; source shading, bark, rock, flowers and transparency
are preserved. Spring ground remains native. Packed RGBA5551 and raw RGBA
textures retain their original metadata. Native and Alt cache bindings
restore only the copies they own, retain queued texture allocations, and
clear the GPU texture cache before retiring scene cohorts.

The original exact foliage material list contained 92 mapped resource paths.
The [OoT actor bridge](oot-actor-tints-bridge-20261013.md) adds the supplied
Hyrule Field ground identity and missing Kokiri Forest / Lost Woods grass and
trim, and scopes ordinary actor foliage at draw time. The current texture table
contains 103 paths, including separate shuffled-grass greenery. This does not
claim comprehensive vanilla scene coverage; unmapped scene resources retain
their original path.

Native rain requests and scripted weather retain precedence in OoT. Seasonal
sky presentation yields to living native weather tags and the original
queued/applied overcast restoration handshake. Seasonal shade is applied at
the final displayed-light clamp; native shared light-adjustment arrays,
lightning and bomb-flash decay remain intact. Native snow actors suppress
only duplicate supplemental snow, leaving mist and aurora independent.

## Winter atmosphere in both modules

Both games have twelve soft terrain-grounded mist wisps and three blue/green
aurora ribbons. Mist can appear by day or night. Aurora holds fixed compass
directions, fades in from 18:00–20:00 and out from 04:00–06:00, and renders
only against the normal outdoor sky. Depth testing and far-clip gates keep
the geometry behind foreground surfaces. Missing floor queries suppress
mist rather than placing it in the air.

Private state/RNG, frame deduplication, pause handling, live control changes,
scene/room reset, same-address re-entry and frame rollback protect lifecycle
and interpolation ownership. Rendering restores the game's native material
setup. The MM implementation follows the existing native summer update/draw
integration and does not replace skybox textures or native precipitation.

## Audio Editor

OoT has a dedicated **Weather & Seasons** tab with the selector, per-season
sections, outdoor rain, rain audio/thunder and diagnostics. Its existing 9.3
audio import/archive features remain available. MM extends its existing
season section with **Winter Freezing Mist** and **Winter Nighttime Aurora**.
Both controls default on and remain independent of Winter Snow and Flurries.
Both games label the spring checkbox **Spring Falling Petals** while retaining
the existing `Spring.Blossoms` CVar for settings compatibility. OoT and MM
retain their separate settings namespaces.

## Verification

- Production OoT seasonal controller/native rain/actor/light fixtures pass,
  including saved-state snapshots, actual native weather tags, sky handoffs,
  private rain cadence and bomb-flash decay.
- OoT falling/summer/winter state and actual native renderer checks pass under
  ASan/UBSan, including command/matrix budgets and material restoration.
- Actual Fast::Texture pixel and cache-binding checks pass under ASan/UBSan
  for spring scope, source/alpha preservation, native/Alt transitions,
  external replacement guards and scene retirement.
- MM winter state and real-header renderer/controller checks pass under
  ASan/UBSan for terrain grounding, emitted dusk/dawn alpha, paused camera
  changes, normal-sky/far-clip gates, reset generations and actor/save/env
  snapshots. LeakSanitizer is disabled because this container cannot enumerate
  process task files; this is not a leak-test result.
- Existing native rain, Rod ownership/mode/save/lifecycle, seasonal selector,
  MM weather/audio/sky, autumn foliage and summer state/renderer/preview
  checks pass. Native/preview summer parity covers 241 frames.
- Changed production OoT and MM audio/atmosphere objects compile with the
  correct ComboShip native build flags. Independent review found no blocker.
- Clang-format 14, whitespace and asset-collision gates pass: 1,881 shared
  paths, 182 known differing-content collisions, no new differing collision.
- The rebuilt OoT archive passes CRC and retains all 6,973 baseline members
  byte for byte, adding only four exact shared leaf textures (6,977 total).
  Candidate SHA256: `780fb475f5f98078fdd762cd6cabe89c199cd42113890f8f33f17a9347ea5edc`.

CI now runs the four new seasonal/weather/foliage/winter diagnostic commands.
Linux CTest registers the new native seasonal controller/renderer, actual
foliage tests and MM winter renderer/controller tests.
The controller and both games' renderer harnesses also pass through the
configured CTest entry points (3/3). Full combined Release rebuild and
deployment checks continue separately from this source publication.
See [command and source-hash evidence](combo-season-weather-verification-20261012.json).

### Windows compiler compatibility follow-up

The complete hosted regression gate passed on `13cbdaf9`; its Windows build
then failed in the SoH compile step. The particle renderer used `near` as a
local distance variable. Windows headers define `near` and `far` as empty
macros; compiling the actual renderer with those definitions reproduces a
missing identifier and invalid clamp expression at that declaration/use.
The variable is now `nearestDistance`, with the same fade calculation.
The existing native renderer regression always compiles with both macros,
so the Linux gate also detects this Windows compatibility boundary.
Production object compilation with normal and Windows-macro flags, and
the complete renderer/state ASan/UBSan suite, pass after the rename.
The corrected hosted Windows build remains a separate verification step.

## Acceptance boundary

Implementation, static review, native compilation and fixture evidence are
separate from installed-pack/GPU gameplay acceptance. Cor's accepted winter
appearance remains the baseline. In the matching ComboShip build, inspect
Spring petals and pink trees with Alt off/on, switch all four seasons and
Native, test the editor live and paused, and check Winter mist/aurora in both
games through night, dusk/dawn, rooms and game switches. PR #41 remains the
candidate; this work does not promote develop/master.
