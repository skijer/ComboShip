# Seasonal runtime corrections — October 14, 2026

Baseline: published PR #41 source `6501dc7d114c3255e6fce5ba1c7ff7b40ce2917c`,
including the boot fix, reviewed blossom revision and existing weather work.
Work is isolated on `debug/season-lighting-20261014`; accepted develop remains
`a82852a3a02c8cabc541cce7c124da71c5755efb`.

Cor supplied `MedalTVScreenRecording20261014004122627.mp4`,
`image(20261014-054406).png` and `MedalTVScreenRecording20261014005130453.mp4`.
The files were inspected locally. The clips do not identify their executable
commit, so this record separates observed/reported runtime defects from source
reproductions at the published baseline.

## Scope and preservation

| Boundary | Candidate scope | Preserved behavior |
| --- | --- | --- |
| MM winter atmosphere | Preserve simulation through the native null-graphics update envelope; guard graphics allocations at draw | Existing mist/aurora meshes, colors, timing, terrain, snow independence and visibility gates |
| Elemental rods | Prevent the competing MM native charge handler from catching brief rod taps; bound native spin types in both games | Accepted projectile interpolation epochs, custom held-charge threshold/release, native sword and beam behavior |
| Road to Southern Swamp foliage | Exact decorative trim, green vine pixels, relevant custom/carry grass draw paths | Shared palette, brown trellis, stone, alpha, general Spring ground, reward flowers, geometry/collision/drop behavior |
| Spring grass, both hosts | Raise the existing blush blend from 224 to 255 for grass/understory | Existing hue and lift, canopy treatment, ground, Summer and Winter recipes |
| OoT falling particles with rain | Resolve outdoor particle eligibility independently of the skybox-disabled flag | Weather/light resolver, native/authored rain, audio, indoor/story/underwater gates and sky-dependent effects |
| OoT lighting | Diagnose inherited lighting and sky behavior | Existing seasonal lighting recipe; no new scene-wide color correction |

The separate Kokiri Forest overworld/archive investigation is deferred as cor
requested. No scene archive, save payload, collision, geometry or UV is edited.

## OoT lighting: confirmed behavior beyond texture tinting

The inherited SoH Rod implementation already changed the environment in two
ways. `soh/mods/items/logic/item_rod_of_seasons.c` at `0914f957e` explicitly
requests gloomy sky and subtracts per-channel light for Autumn/Winter.
The current presentation controller retains those recipes in
`soh/soh/Enhancements/audio/SeasonWeather.cpp`.

Winter subtracts ambient RGB `(54,42,20)` and directional RGB `(46,34,14)`
before nighttime protection. Summer has no seasonal subtraction.
The current nighttime protection retains half the available native light,
but its baseline is the already-selected native weather palette. It does
not preserve half of the clear Summer palette.

`GlobalOutdoorRain::UpdateOvercast` requests `gloomySkyMode=1`.
Native `func_8006FB94` then selects outdoor lighting table row 2 (scene light
indices 8–11) and switches from clear to cloud sky artwork. Thus the same
weather request affects both sky selection and world lighting. The foliage
registry does not target sky textures, and the mist renderer does not write
the environment light or fog colors.

A throwaway production-function replay held the clock at 20:45, used the
real outdoor interpolation/clamp/controller paths, and used the twelve Hyrule
Field lighting rows already preserved by the existing regression fixture:

| Selection | Light table | Ambient RGB before/after seasonal shade | Moon RGB after shade | Fog RGB |
| --- | --- | --- | --- | --- |
| Summer | 0 | `40,40,70` / `40,40,70` | `50,50,100` | `0,0,30` |
| Winter | 2 | `70,70,90` / `35,35,70` | `15,15,66` | `0,0,10` |

These are Hyrule Field control measurements, not extracted values from the
scene shown in the new clip. They establish a darker blue-biased lighting
mechanism independent of texture tint. The forced cloudy route also gives a
specific connection to investigate for Gerudo Valley's black sky. They do not
prove the exact installed sky resource or every cause of the black appearance.
The existing Spring nighttime blackout remains unresolved.

A separate throwaway eight-row scene-palette control produces an ASan
out-of-bounds read in the native outdoor lighting blend after the forced
overcast request selects indices 8–11. The prior fixture tested only a full
twelve-row palette. Whether any supplied mod scene has an incomplete palette
requires actual archive readback; this is not attributed to cor's clip and is
not silently folded into a new lighting recipe.

## MM mist/aurora: confirmed native update lifetime defect

Native `Play_Main` saves `state.gfxCtx`, sets it to `NULL`, calls `Play_Update`,
restores it, and then calls `Play_Draw`. The baseline winter controller rejects
a null graphics context in its **update** path and resets all simulation state
on every gameplay tick. The later draw therefore repeatedly starts mist at
age zero and restarts the aurora fade. The earlier fixture always retained a
graphics context during update and missed this boundary.

The regression runner now extracts the actual `Play_Main` envelope and drives
the existing production renderer/controller through it. Before the correction,
90 update/draw cycles submit zero mist meshes and zero aurora ribbons.
The minimal correction allows graphics-free simulation and checks the
graphics context before allocating draw packets.

The corrected native-envelope fixture submits 12 mist meshes and 3 aurora
ribbons at update rates 1, 2 and 3, verifies positive mist age, and checks that
repeated draws and a missing draw context cannot reset or advance simulation.
This proves the source lifetime correction; installed GPU appearance remains
a runtime acceptance check.

## Rod ancestry and native charge/spin seams

Shipwright commit `5fadb2d07f0e768974dd62c278220245d1539aaf`
(“Restore rod effects and interpolation; fix Deku Leaf completion”) added
projectile launch epochs and per-head interpolation scopes. Those corrections
are already carried into both current game modules. They do not change Link's
input or charge state.

The MM custom rod controller requires ten eligible held updates before custom
charge begins. Separately, native `Player_ActionHandler_8` can catch a single
tap while melee recovery `unk_ADC == 1`, before the normal melee handler. It
enters `Player_Action_30`; that action defers its release check until the charge
entry animation finishes. This is a concrete short-tap charge-pose path,
conditional on the native B/held-button policy and recovery window. Rods now
retain their own custom charge owner rather than entering that competing path.

The baseline MM native spin spawn also subtracts the Kokiri sword item-action
from custom rod actions `0x74/0x75/0x76`, producing invalid effect types
`113/114/115`. Native damage arrays support only regular types 0–3 and charged
indices 4–7. The SoH rod melee classification similarly produces a fourth
index for a three-entry native spin damage table. Explicit rod-only mappings
use a valid native two-handed spin type in each engine; legacy native swords,
Master/Fierce Deity beams and magic-cost bits retain their original mapping.

The broader MM rod model-group alias/two-handed animation mismatch is recorded
but is not redesigned in this candidate. The source reproductions explain
specific charge/bounds defects; final acceptance of the reported snapping
still requires repeating the user's actual tap/swing/hold sequence.

## Road foliage: exact native identities and draw ownership

Native readback identifies this connecting area as `SCENE_24KEMONOMITI`
(`Z2_24KEMONOMITI`, Road to Southern Swamp). The native MM control archive is
36,251,870 bytes, SHA-256
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
It establishes native material identity, not the user's mounted HD pack.

The 52-triangle trim display list `room_00DL_005FF0` primarily samples
`misc/scene_texture_08/scene_texture_08_Tex_007000`. Its secondary IA4 texture
already had seasonal coverage, while the primary RGBA16 texture did not.
The new primary entry is Road-scoped because the same texture is used by Milk
Road. Spring coverage is confined to this decorative trim.

The six-triangle bat-tree vine display list `room_00DL_003CE0` samples scene
CI8 texture `Z2_24KEMONOMITITex_005730` with shared palette
`Z2_24KEMONOMITITLUT_007950`. This is a separate surface from the bark and
canopy. The renderer decodes the active native/Alt palette into an immutable
RGBA copy and tints green pixels only. Brown trellis pixels and alpha are
preserved; the shared palette is never mutated. Source and palette lifetime,
raw-upload scaling, cache identity and Summer/Native restoration are tested.

Native direct `EnKusa` and `ObjGrass` paths already received seasonal tint.
Road randomizer draw overrides bypassed the direct grass scope. The candidate
wraps 38 exact bundled leaf child identities, excludes flower/stem and private
identities, and loads only those eligible children on a cold first draw.
Road native grass batches restore their scope after an override disables it,
and carried native grass receives the corresponding draw scope. Reward/drop
behavior, collision and the marker/stem materials are unchanged.

The default suite and complete 567-material native scene/object suite pass
normally and with ASan/UBSan, including native/Alt identities, both scene
namespaces, cold loading, mixed batches, shared-palette preservation and
restoration. Exact installed Djipi/private material identity remains a runtime
acceptance boundary.

## Spring grass: stronger existing blush in both games

Spring grass uses the existing `F6C1D9` blush with full blend `FF` instead of
`E0`; the existing 1.8 lift is retained. The change targets grass/understory
in both hosts, including the newly covered Road surfaces and override leaves.
General ground and canopy recipes are unchanged.

The real production tint path changes a dark-green HD control pixel
`40,110,25` from the old Spring result `108,94,94` to `118,92,104`.
Native bush mean RGB moves from the old Spring `136.9,119.7,121.5` to
`145.4,120.7,132.8`, against Summer `77.6,113.0,41.8`.
These are texture/draw measurements before lighting and GPU appearance.
The new full-blush assertions fail at the old blend and pass with the
candidate; production actor and FastTexture checks pass with ASan/UBSan.

## OoT rain actors and falling seasonal particles

The production renderer already supports rain density 0, 25 or 64 alongside
Spring/Autumn particles with a visible sky. There is no shared particle slot
or density limit causing that combination to fail.

The existing weather resolver also requires `envCtx.skyboxDisabled == 0`.
An outdoor scene can disable its skybox separately from its indoor flag,
while authored/native rain remains active. In that configuration the resolver
returns Native/Off and suppresses the seasonal particles. The exact flag in
the user's scene archive has not yet been established.

The new particle-only season resolver permits that outdoor configuration.
The weather/light resolver is unchanged. Indoor, custom sky filter, story,
game-over, underwater and saved/manual season ownership gates remain intact.
Aurora and sunbeams still require a visible sky.

The renderer fixture now links the actual seasonal controller, outdoor rain
resolver and native weather actor. The failing control has authored rain
density 40 and rain-loop gain 0.5 but no seasonal geometry with a disabled sky.
After correction, nine authored/native-rain combinations produce 96 Spring
petals or Autumn leaves, or 64 Winter snow slots plus 180 mist vertices.
Rain/audio/light/save fields and native snow ownership are preserved.
Both normal and ASan/UBSan particle suites and the full weather matrix pass.
This proves the eligibility correction, not every installed scene's flags.

## Verification and runtime handoff

Independent combined review reports no remaining critical or important
findings. Local normal and ASan/UBSan checks pass for both native rod spin
tables, the MM tap/held-charge sequences, MM winter's native update envelope,
Road grass/carry/mixed batches, native texture ownership/upload paths,
SoH actor/texture scopes and rain/particle coexistence. Leak detection is
disabled for ASan runs because the container does not support its process
inspection; address and undefined-behavior checks remain enabled.

The complete 567-material MM native archive suite, full MM weather suite,
SoH weather matrix, real-header MM foliage translation-unit compilation and
MM Fire/Ice/Light interpolation donor parity also pass. The latter preserves
the accepted projectile launch epochs, removal/reuse and stable impact
heading; protected projectile logic/draw files remain byte-identical to the
published baseline. The hosted gate now includes the native rod regressions.

This record does not claim a full platform build or installed GPU acceptance
for the new candidate. Exact commit and hosted results belong to the draft
PR/build record after publication; a baseline build is not candidate evidence.

Decisive runtime checks are: MM Winter mist after approximately one second,
aurora at night with both options enabled; each elemental rod tapped through
swing recovery versus deliberately held and released; and the Road trim,
bat-tree vines and cuttable/carried grass in Spring/Autumn/Winter followed by
Summer/Native restoration; and falling particles alongside authored/native
rain in the affected OoT outdoor scene. Keep the same packs, Alt setting,
scene and time when comparing lighting. The inherited lighting recipe,
Gerudo black-sky appearance and separate Kokiri archive diagnosis are not
claimed fixed. No develop/master promotion is implied.
