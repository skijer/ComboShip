# Static Story Actor Catalogue — POC6

Place these as **Cutscene Actors (`En_Viewer`)** in Prelude and enter the full
four-digit value in **Params**. These placements use self-contained static
adapters: they do not call the source NPC's story actions, grant rewards, start
cutscenes, or set progression flags.

## POC6 expansion page

| Param | Actor | Pose / behavior | Collision | Blink | Tracking | Water requirement |
|---|---|---|---|---|---|---|
| `0x7E01` | Darunia | Standing idle | Soft pushable | Yes | While conversational | None |
| `0x7E11` | Darunia | Complete four-part native dance cycle | Soft pushable | Yes | No | None |
| `0x7E02` | Nabooru | Standing, hands on hips | Soft pushable | Yes | While conversational | None |
| `0x7E03` | Adult Ruto | Grounded idle | Soft pushable | Yes | While conversational | None |
| `0x7E13` | Adult Ruto | Hold at native water-surface height | Soft pushable while surfaced | Yes | While conversational | Valid waterbox |
| `0x7E23` | Adult Ruto | Repeating dive, submerge, and rise cycle | Surfaced only | Yes while visible | No during performance | Valid waterbox |

If a water-mode Ruto cannot find a valid waterbox, she safely falls back to her
Prelude Y placement instead of running story logic or searching indefinitely.

## Existing POC4 page (preserved)

| Param | Actor | Pose | Collision | Blink | Tracking |
|---|---|---|---|---|---|
| `0x7F01` | Impa | Idle | Soft pushable | Yes | Yes |
| `0x7F02` | Child Malon | Idle | Soft pushable | Yes | Yes |
| `0x7F12` | Child Malon | Singing animation | Soft pushable | Yes | No |
| `0x7F22` | Child Malon | Singing animation with vanilla vocal pattern | Soft pushable | Yes | No |
| `0x7F03` | Saria | Arms at sides | Soft pushable | Yes | Yes |
| `0x7F13` | Saria | Hands behind back | Soft pushable | Yes | Yes |
| `0x7F23` | Saria | Playing ocarina | Soft pushable | Fixed performance face | No |
| `0x7F33` | Saria | Seated | Soft pushable | Yes | No |
| `0x7F04` | Adult Zelda | Native standing idle (repaired in POC6) | Soft pushable | Yes | No |
| `0x7F14` | Adult Zelda | Native standing idle alias | Soft pushable | Yes | No |
| `0x7F05` | Sheik | Idle | Soft pushable | Yes | Yes |
| `0x7F15` | Sheik | Arms crossed | Soft pushable | Yes | Yes |
| `0x7F25` | Sheik | Playing harp | Soft pushable | Yes | No |
| `0x7F06` | Adult Ruto | Idle | Soft pushable | Yes | Yes |
| `0x7F16` | Adult Ruto | Hands on hips | Soft pushable | Yes | Yes |
| `0x7F26` | Adult Ruto | Looking down-left | Soft pushable | Yes | Yes |
| `0x7F07` | Child Ruto | Hands behind back | Soft pushable | Yes | Yes |
| `0x7F17` | Child Ruto | Hands on hips | Soft pushable | Yes | Yes |
| `0x7F27` | Child Ruto | Sitting | Soft pushable | Yes | No |
| `0x7F08` | Kokiri Girl | Idle | Soft pushable | Yes | Yes |
| `0x7F18` | Kokiri Girl | Arms behind back | Soft pushable | Yes | Yes |
| `0x7F28` | Kokiri Girl | Hands on hips | Soft pushable | Yes | Yes |
| `0x7F38` | Kokiri Girl | Sitting, head on hand | Soft pushable | Yes | No |
| `0x7F48` | Kokiri Girl | Seated Great Fairy leg motion, level head | Soft pushable | Yes | No |
| `0x7F58` | Kokiri Girl | Sitting, arms and legs crossed | Soft pushable | Yes | No |
| `0x7F09` | Fado | Idle | Soft pushable | Yes | Yes |
| `0x7F19` | Fado | Arms behind back | Soft pushable | Yes | Yes |
| `0x7F29` | Fado | Hands on hips | Soft pushable | Yes | Yes |
| `0x7F39` | Fado | Sitting, head on hand | Soft pushable | Yes | No |
| `0x7F49` | Fado | Seated Great Fairy leg motion, level head | Soft pushable | Yes | No |
| `0x7F59` | Fado | Sitting, arms and legs crossed | Soft pushable | Yes | No |
| `0x7F0A` | Adult Malon | Idle | Soft pushable | Yes | Yes |
| `0x7F1A` | Adult Malon | Holding basket | Soft pushable | Yes | Yes |
| `0x7F2A` | Adult Malon | Singing animation | Soft pushable | Yes | No |
| `0x7F3A` | Adult Malon | Singing animation with vanilla vocal pattern | Soft pushable | Yes | No |

## Kokiri/Fado seated-motion candidate

The September 19 candidate replaces pose 4 (`0x7F48` / `0x7F49`) with the
Great Fairy's sitting leg loop at native playback speed. It retains the Kokiri
root, torso and arms and levels the head from the first sampled frame. Only the
six leg joints take donor rotations; skeleton proportions and actor placement
remain native. Pose 5 (`0x7F58` / `0x7F59`) retains the original arms-crossed
static pose. Other poses and actor parameters are unchanged.

Verification commands:

```sh
python3 scripts/diagnostics/run_kokiri_pose_tests.py /path/to/oot.o2r
python3 scripts/diagnostics/run_mm_rendering_regression.py --viewer-only
```

The first command reads the supplied archive without changing or redistributing
its assets. It exercises the production pose adapter and native sampler through
initialization, two loops, fractional frames, the wrap seam, independent actor
state and missing-donor fallback. The second checks the viewer bridge's timing
and the surrounding actor lifecycle/render callbacks. These are static tests;
the visible result, feet/clothing intersections and native/Alt parity still need
an in-game check with both characters and a comparison to their static pose 5.

## Runtime test priorities

1. Verify native Child Malon, Child Saria, and Child Ruto story placements with
   ordinary entrances; debug-menu warps can suppress vanilla NPC setup state.
2. Test `0x7E13` and `0x7E23` in both shallow and deep valid waterboxes, then
   leave and re-enter the room.
3. Let `0x7E11` run long enough to see all four Darunia dance clips repeat.
4. Test Adult Zelda under vanilla assets and alternate assets.
5. Unsupported `0x7Exx` values are intentionally rejected rather than aliased
   to an existing actor.

## MM ordinary actors

These entries require their complete resources from `mm.o2r`. Missing or incompatible
resources reject the placement before skeleton initialization.

| Param | Actor | Pose | Frames | Tracking |
|---|---|---|---:|---|
| `0x7E09` | Happy Mask Salesman | Idle | 29 | Head, while conversational |
| `0x7E19` | Happy Mask Salesman | Hands clasped | 29 | No |
| `0x7E29` | Happy Mask Salesman | Arms out | 29 | No |
| `0x7E0A` | Keaton | Idle | 36 | No |
| `0x7E1A` | Keaton | Chuckle | 36 | No |
| `0x7E2A` | Keaton | Celebrate | 30 | No |
| `0x7E0C` | Lulu | Look down | 30 | Torso and head, while conversational |
| `0x7E1C` | Lulu | Look left | 30 | No |
| `0x7E2C` | Lulu | Sing | 72 | No |
| `0x7E3C` | Lulu | Look around | 87 | No |
| `0x7E0D` | Anju | Seated umbrella crying | 43 | No |

All eleven animations loop at their ordinary speed with authored root translation.
Lulu's singing is animation only, with open eyes and mouth; it does not start audio,
ocarina, or quest actions. The unmodified 87-frame look-around clip remains a raw
loop pending an observed visual seam check. Happy Mask Salesman uses his happy
closed eyes and smile. Keaton does not bind face segments.

All four use scale `0.01`. Happy Mask Salesman and Lulu use collider radius/height
`22/70`, focus height `60`, and talk distance `100`; Keaton uses `18/50`, `35`, and
`70`. These are static placement defaults. Collider Y shift is zero.

Anju uses the native `gAnju2UmbrellaCryAnim` loop, sad eyes, closed mouth and the
umbrella attached to right-hand limb 8. This is the seated crying loop, not the
sit-down transition. Her dialogue is "...Kafei... I promised I'd wait for you."
through catalogue text `0x8F26`, with talk range `90`. Conversation leaves her pose
and placement intact. She has no tracking, schedule, quest or audio actions.
Her world XYZ and yaw stay at the authored placement. There is no gravity, floor
snap, furniture collision correction, hovering or automatic foot-height offset.
The pose's root translation remains in the skeleton, so align the seated body with
your ledge, chair or log manually; the actor origin is near the feet, below the seat.
She uses an immovable `20/50/0` collider and focus height `40`. The umbrella graph
retains its vertices and textures across scene/cache eviction.

An optional HD Anju pack uses `objects/object_anju_hd/v1/` and requires Alternate
Assets enabled. It supplies all 19 drawn limbs, three complete head states and the
umbrella as one archive-owned graph. Missing or invalid resources fall back to the
whole native model. The custom heads blink open/half/closed/closed/half at 20 Hz;
the original blue eye artwork and closed mouth are retained. The native fallback
keeps its fixed sad eyes. Each actor owns its blink timer; additional render calls
do not advance it. The custom mesh's 476-unit fit is applied only to three local
draw branches, never to world placement, collision or the native skeleton.

Child Kafei (identity 21) uses `0x7E0B` for the 89-frame idle and `0x7E1B` for
its 48-frame gesture. Both are private, native-rate full loops with no tracking,
player animation hooks, quest behavior, pendant or audio. Open eyes and closed
mouth follow these clips' neutral appearance words. Its placement defaults are
scale `0.01`, collider `18/60/0`, focus `45`, and talk distance `80`.
The native MM root translation adjustment applies only inside the draw callback;
placement, collider, and sampled joints are unchanged. Dialogue is
“I've made a promise to Anju.”

## Regional actors (append-only 2026-10-24)

Existing catalog IDs and `0x7F`/`0x7E` placements are unchanged. New catalog
IDs are Stalchild 25, Mikau 26 and Darmani 27. Stalchild uses the last `0x7E`
identity nibble; Mikau and Darmani start the `0x7D` family. `0x7D00` remains invalid; `0x7D03` is allocated to child Zelda below.
`0x7D04` remains invalid. Each row below is an ambient presentation, with no
quest actions, hostile AI, native path requirements or talk prompt.

| Actor | Params | Pose | Skeleton limbs / matrix reservation | Frames |
|---|---|---|---|---|
| Stalchild | `0x7E0F` | Idle | 19 / 0 (standard) | 30 |
| Stalchild | `0x7E1F` | Sit laugh | 19 / 0 | 20 |
| Stalchild | `0x7E2F` | Sit tap toes | 19 / 0 | 20 |
| Stalchild | `0x7E3F` | Swing on branch | 19 / 0 | 30 |
| Stalchild | `0x7E4F` | Salute | 19 / 0 | 34 |
| Mikau | `0x7D01` | Idle | 22 / 20 | 30 |
| Mikau | `0x7D11` | Guitar | 22 / 20 | 12 |
| Darmani | `0x7D02` | Wait | 19 / 17 | 29 |
| Darmani | `0x7D12` | Fly | 19 / 17 | 39 |

The machine-readable presets are `combo/actors/RegionalActorPresets.json`.
All OoT rows use En_Viewer `0x002A`. Native MM rows use En_Rail_Skb `0x0212`,
En_Zog `0x0224` and En_Gg `0x01F7`, respectively, with the same params. The
native MM reserved rows replace callbacks before native quest initialization;
En_Gg2 and all nonmatching original actor params retain their original code.
Native MM presentation support is available in Combo builds, including MM-only
mode. OoT presentations use the MM importer and fail closed if MM resources
are unavailable. Solo modes do not create regional default groups.

Mikau has 19 visible limb display lists but reserves 20 flex matrices. His
native guitar loop is `object_zog_Anim_00931C`; its additional
`object_zog_DL_0280A8` is required and attaches at `OBJECT_ZOG_LIMB_11` (17).
Missing or mistyped imported props reject the presentation. Darmani uses
`object_gg_Skel_00F6C0` with wait `object_gg_Anim_00F578` or fly
`object_gg_Anim_00AF40`. His placed copy always uses the XLU stream, independently
of Lens, healing and mask flags. Stalchildren use their actual standard skeleton,
not a flex reinterpretation.

The new OoT `ComboRegionalActors` module queries the endpoint every update.
It floor-fits two seated Stalchildren near Ikana, Mikau with fallback Lulu near
Great Bay, and a floating Darmani near Snowhead. Offsets follow endpoint yaw;
no final Snowhead world coordinate is embedded in the NPC module. It keeps the
central return lane clear, rejects missing floors/exits/overhead obstruction,
and retries endpoint readiness without repeated failed actor spawns. An explicit
matching actor in the current room suppresses its automatic family, including
all automatic Stalchildren if one is authored. The module removes its own copies
when mode/route/room changes and never removes authored or native quest actors.
Native MM receives no extra automatic groups.

The fixtures establish catalog/resource contracts, lifecycle and emitted draw
commands. They do not establish visible game rendering, exact body/floor fitting
or traversable portal clearance in either configured scene presentation.

## Zelda catalogue additions — 2026-10-26

Place these as **Cutscene Actors / En_Viewer (`0x002A`)**, using the full params.
The normal child Zelda actor and its courtyard proximity/story sequence are not
used by these entries. Child Zelda uses safe optional A-button dialogue, with no automatic
conversation, quest reward, progression change or forced cutscene. She uses
the catalogue's standard soft collision.

| Actor | Params | Presentation | Blink | Model replacement |
|---|---|---|---|---|
| Child Zelda | `0x7D03` | Standing idle | Yes | Native/custom public skeleton and face resources |
| Child Zelda | `0x7D13` | Hands clasped | Yes | Same |
| Adult Zelda | `0x7F24` | Hands clasped, native 20-frame loop | Yes | Existing adult adapter and face matrices |

The two existing adult entries `0x7F04` and `0x7F14` both deliberately retain
their prior standing idle. `0x7F24` is an additional distinct pose, with
tracking disabled to preserve the clasped silhouette. All previous actor IDs
and poses retain their values. Child Zelda is appended as catalogue ID 28,
using OoT `OBJECT_ZL4` for both model and animations.

Child Zelda uses `objects/object_zl4/gChildZeldaSkel` through the same public
ResourceMgr/SkeletonPatcher replacement mechanism used for adult Zelda's
`objects/object_zl2/gZelda2Skel`. Compatible custom packs replacing those
resources remain eligible for mod load-order and Alt selection. Every child
draw binds left/right eye segments `0x08`/`0x09` and neutral mouth `0x0A`,
uses the loaded skeleton, and follows the normal child's hidden-under-skirt
limb convention. Blinking is driven by the independent catalogue actor.
No vanilla-only display-list copy is baked into the adapter.

Machine-readable presets: `combo/actors/ZeldaActorPresets.json`. Scene
placement remains explicit in Prelude; put the child and adult entries in
the corresponding normal age/setup actor lists. The POC4 addon below supplies explicit age-specific Chamber placements;
no native automatic actor spawning is used.
Rebuild the executable containing this source before using the new params.

Native/source checks and replacement-boundary fixtures are recorded with the
candidate; the actual custom model pack, Alt toggles, pause/reentry and
in-game pose/face appearance still need configured runtime acceptance.

## Blue Chamber Zelda variants — 2026-10-26

| Actor / pose | Params | Dialogue | Blue aura |
|---|---|---|---|
| Child Zelda standing | `0x7D03` | Fairy-boy greeting | No |
| Child Zelda hands clasped | `0x7D13` | Same fairy-boy greeting | No |
| Child Zelda hands clasped, Chamber echo | `0x7D23` | Mysterious Chamber line | Yes |
| Adult Zelda standing / alias | `0x7F04` / `0x7F14` | Existing safe standing dialogue | No |
| Adult Zelda hands clasped | `0x7F24` | Same dialogue as standing | No |
| Adult Zelda hands clasped, Chamber echo | `0x7F34` | Mysterious Chamber line | Yes |

The aura combines a soft light-blue halo and existing NEI twinkles local to
the selected En_Viewer instance. It changes no scene lighting, model texture,
custom model selection or normal Zelda story actor. Existing plain variants
remain free of the aura. Dialogue only offers the A prompt; it never starts
from proximity alone, changes progression, grants an item or runs story
actions. All existing ordinary catalogue actors keep their prior dialogue.

The full POC4 addon places blue child/adult Zelda at the center of the
authored Chamber platform, facing its Light approach, using normal age/setup
headers. The matching rebuilt source is required for both new params, aura
and dialogue. See the placement evidence for the inherited night fallbacks.
