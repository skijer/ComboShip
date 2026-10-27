# Forest placed actors candidate — 2026-10-18

**2026-10-19 follow-up:** [Saria pose entries and green hollow hexes](poc/saria-prelude-poses-hex-20261019.md)
supersedes this document's unsupported MM ocarina pose and native-star shimmer
details. It adds waiting/playing/seated song-giver variants while retaining the
same once-per-file claim and authored-placement precedence.

Parent: recovered cumulative ComboShip source `78e44582` (audio regression
candidate, including the prior Black Market/forest/fade source work). Public
develop remains `a82852a3a02c8cabc541cce7c124da71c5755efb`. This candidate does
not promote the accepted build or scene archives. Existing model fitting work
is a separate dependency and remains untouched.

## Current mode routing

These features support the launcher's **ComboShip, solo SoH and solo 2Ship**
modes. The original independent builds retain their existing behavior.

| Mode / scene | Room 0 | Room 1 |
| --- | --- | --- |
| ComboShip / MM playable Lost Woods | MM waiting Saria + green shimmer; MM stationary Epona opposite the portal | MM reclining Skull Kid |
| Solo 2Ship / exploration forest copy `0x03`, entrance `0x1600` | Same MM entries and effect | Same MM Skull Kid entry |
| Solo SoH / exploration forest copy `0x6F`, entrance `0x0618` | Native OoT waiting Saria + green shimmer; native stationary Epona opposite the portal | Existing OoT cutscene-actor Skull Kid, same reclining loop |

Prelude uses each host game's actor IDs, rather than sharing actor IDs between
engines. MM entries remain En_Viewer `0x001C` / Saria `0x7F13`, En_Horse
`0x000D` / Epona `0x7E40`, and Dm_Stk `0x0191` / Skull Kid `0x7E50`.
Solo SoH uses En_Viewer `0x002A` / Saria `0x7F13`, native En_Horse
`0x0014` / Epona `0x7E40`, and En_Viewer `0x002A` / Skull Kid `0x7E08`.
The OoT monkey entries below remain usable in ComboShip's OoT and solo SoH.
Matching authored entries override automatic placements without a source build.
All automatic routes wait for a ready room and preserve the native story scenes.

Both ComboShip MM and solo 2Ship also retain the seated, once-per-file Saria's
Song gift in Termina Field (`0x001C`, params `0x7E33`). Solo 2Ship writes its own
native save and enables the same existing song/hint behavior; it initializes no
foreign save, game or UI subsystem. No Termina Field check is added to OoT.

**Assets:** solo SoH's imported monkey/Skull Kid require the existing `mm.o2r`
donor. Solo 2Ship's Saria requires the existing optional `oot.o2r` companion,
which MM already mounts at lowest priority; without it Saria fails closed,
including the song-giver. The native MM archive and private Saria packs alone
do not supply her native rig/animations/eye textures. Install the selected
`NativeNpcV1` Saria pack in solo 2Ship's own mod order and enable **MM Alternate
Assets** there. Solo 2Ship validates and retains MM-owned typed resources and
routes draws through `@mm`, including fitted poses and blink wrappers. ComboShip
keeps its existing OoT-owned Saria bridge and **OoT Alternate Assets** policy.
Solo SoH keeps its existing OoT NPC model/blink adapter. No donor or model
archive was edited or added to this checkpoint.

## OoT monkey

Place **En_Viewer / actor 0x002A** in OoT Prelude with these parameters:

| Params | Pose |
| --- | --- |
| `0x7E2E` | **Jumping up and down — requested main pose** |
| `0x7E0E` | Native MM monkey idle |
| `0x7E1E` | Native MM monkey scratching |

Use authored XYZ and yaw; pitch/roll are normalized by the existing placed
actor host. Invalid poses fall back to idle. No scene archive or placement was
changed: cor will place the monkey near the Woods of Mystery entrance.

The existing MM loader owns the 22-limb/20-matrix skeleton, 16-frame native
high-jump loop, 12-frame idle and 71-frame scratching clips, three blink textures
and actor lifetime. The jump keeps native animated root motion (about 26 world
units of vertical lift) while the actor's authored placement stays fixed. Scale is
the native `0.012`. This is an animated stationary NPC with collision and short
generic dialogue, without the native monkey quest/path/cutscene controller.
Requires the same native `mm.o2r` donor used by the existing MM actors in OoT.
No new monkey asset pack is needed. Metadata was read from the native archive.

Focused catalogue and viewer lifecycle/render tests pass. No full executable
link, live game, Alt rendering or scene re-entry acceptance is claimed.

Preserve all existing actor IDs/parameters, progression, original/combined game
guards, forest portals, room resources, collisions, audio and other actor work.
Runtime check: place all three poses; verify jumping, blink, talk/close, authored yaw and feet,
collision, re-entry and vanilla/Alt assets. The monkey must remain stationary
without granting items, setting story flags or starting a quest cutscene.

## Saria in MM Lost Woods

A small MM NPC host uses the same OoT-owned Saria assets, without porting the
forest scene or Saria's story controller. It has native 24-frame standing poses,
blink, ordinary short dialogue, an overlap collider and circle shadow.

| MM En_Viewer actor `0x001C` params | Pose |
| --- | --- |
| `0x7F03` | Arms at sides |
| `0x7F13` | **Hands behind back waiting — automatic placement default** |
| `0x7F33` | Seated scenery NPC (dialogue, no reward) |

Reserved Saria params bypass MM En_Viewer's native lighting controller before
its counters or lighting actions run. All three native lighting modes retain
their original path. Unsupported Saria poses fall back to arms at sides;
pose 2 remains unsupported.

On a combined-game exploration visit (Lost Woods spawn zero/layer zero, room
zero), or the solo 2Ship exploration copy, Saria is placed once near the return portal: its resolved anchor
plus X=100/Z=70, using a floor ray for Y. An authored MM placement suppresses
the automatic one. Missing floor or missing donor resources suppress that NPC.
Prologue/flashback cutscenes, file select, other rooms and active
transitions are excluded. This default location remains provisional pending
in-game review; no scene archive, entrance or portal geometry was edited.

Saria's location dialogue is:

> These woods are a bridge
> between worlds...
> The forest will guide you home.

Her waiting pose keeps native timed blinking. The custom head follows the same
open/half/closed/open sequence through all three private face wrappers. Focused
checks cover both standing poses, and both retained model packs supply distinct
blink heads without a waiting-pose head mesh overriding expression selection.

In ComboShip, load ONE existing Saria Render or Anime `NativeNpcV1` pack in its usual OoT
owner and enable **OoT Alternate Assets**. The games deliberately keep separate
Alt settings; this foreign model follows OoT's model selection, not MM's switch.
No second model pack or regenerated textures are required. The matching full
NPC adapter checkpoint remains separate for Saria's ordinary OoT placements;
this small MM bridge reads the same pack contract independently. Private fitted
standing/seated animations and limb meshes are supported. Missing/invalid private rigs
fall back to native Saria; missing native resources fail closed for that actor.

MM owns the actor, animation state, fixed joint arrays, collision and dialogue.
In ComboShip, the narrow ABI borrows validated OoT-owned normal animation/rig resources and
routes every model/face draw through their OoT owner. It passes no actor or save
context between engines. Live OoT Alt switching rebinds the snapshot while
preserving the current frame. Fitted clips must keep native root translations.
Snapshot cleanup covers normal actor/scene destruction and direct window close.
The native fallback explicitly binds its required segment-C empty display list
and native opaque environment alpha. The monkey and Lost Woods/scenery Saria
do not write progression or rewards.

## Seated Saria in Termina Field

The user selected an **always-fixed, one-time MM Saria's Song gift**, not a
new shuffled seed location. Existing randomizer item pools are unchanged.

Place **MM En_Viewer / actor `0x001C`, params `0x7E33`** for the reward variant.
It uses seated pose 3 (`gSariaSitting3Anim`, 24 frames) with native or either
existing private model. Both retained Render/Anime packs already contain the
fitted seated clip and three distinct blink heads. No model archive was edited.

An automatic reward Saria spawns once per room visit in combined or solo MM Termina
Field room zero at **X=-720, Z=2000**, southwest of Clock Town's south gate,
facing north (yaw zero). Y comes from a floor ray starting at 300. The retained
native collision yields Y=-77 there. An authored reward Saria suppresses the
automatic one. Pause, inactive play, cutscenes, transitions, missing floor,
invalid save slots are excluded. Position and visual fit remain
provisional until in-game review. Lost Woods Saria keeps her existing location.

The original seated loop carries an elevated cutscene root. The host subtracts
7003 model units from its root Y each sample, keeping its native breathing and
all limb rotations while putting the hips initially 9 world units above the
authored floor. Actor XYZ stays fixed. Seated focus is 20 and the cylinder is
radius 12/height 24; native and custom heads retain timed blinking.

Talk to her, then close this dialogue:

> Even here, I can hear
> the forest calling...
> Let me teach you its song.

The standard MM item event queue then presents and grants `RI_SONG_SARIA`,
using its existing localized receipt and song model. The event retains no NPC
pointer, suppresses duplicate queued/active gifts, and checks the originating
save slot at delivery. `NeiSaveData.comboSariaSongClaimed` is appended and saved
with the actual reward. Older saves default to unclaimed. Ownership or the
remaining hint count is not used as completion, so using the hint/song,
re-entering, changing worlds, reloading or resetting the cycle cannot refill
this fixed gift. The claim is local MM state and is not mirrored to OoT.

This uses 2Ship's existing item behavior: a song/hint grant also sets its MM
quest bit and mirrored quest-page icon. In randomized saves, her gift enables
the existing Saria hint hooks even when adding the song to the shuffled pool
was disabled. In ordinary combined or solo MM saves it teaches the melody and fixes its
native fanfare without installing randomizer hint logic. Other shuffled copies
retain their existing behavior. Later conversations use:

> Wherever you wander,
> our song will carry the
> forest with you.

## Green shimmer and portal Epona

This bounded polish extension starts at source `ef3fedb3`. Both Sarias now
emit drifting green native Kirakira sparkles. The emitter uses each actor's
current XYZ, so Prelude placement changes move the aura with her. It keeps
standing particles around her full height and seated particles lower, with
one particle every three updates and a 24-update lifetime (about eight live
particles per NPC). Pause and distance guards bound the effect; it does not
recolor the model, replace blink heads or alter her reward state.

A stationary **native MM Epona** occupies the other flank of the MM Lost
Woods return portal. It uses the existing 46-limb Skin rig, 70-frame idle,
three eye textures, native `0.00648` scale and MM Epona cosmetic draw wrapper.
Its narrow controller owns animation/blink and two overlap cylinders. It
never runs riding, summoning, racing or story/save logic. Ordinary horse
parameters and original builds keep their native initialization path.

Automatic placement is restricted to the same combined or solo MM playable Lost
Woods visit as Saria. The horse uses the portal's current anchor/yaw, chooses
the side opposite the first authored Saria (or Saria's default position), and
stands at local lateral +/-145 and forward depth -90, facing away from the
door. A floor ray supplies Y. This leaves the doorway and forward approach
clear. For the current return anchor/yaw, its default is approximately
**X=-722.66, Y=0, Z=-481.40**; the native collision floor at this point is zero.
This position is provisional until live fit/clearance review. Earlier portal
packs use their resolved anchor/yaw rather than the latest coordinates.

To author or move Epona in Prelude, place **MM En_Horse / actor `0x000D`,
params `0x7E40`** in Lost Woods room zero. Set ground-level XYZ, yaw as desired,
pitch/roll zero. An authored reserved horse suppresses the automatic one.
It is a Combo-only scenery entry for playable Lost Woods; it is not the
ordinary rideable horse or a progression check. Native cutscene,
other scenes/rooms and missing-floor automatic spawns are excluded. No scene,
portal, grotto or model archive was edited for this addition.

## Skull Kid on the second Lost Woods area's outer hill

Place **MM Dm_Stk / actor `0x0191`, params `0x7E50`** in Prelude, in playable
Lost Woods room **1**. This is the same native 40-frame
`gSkullKidRecliningFloatAnim` selected by OoT's placed Skull Kid pose 0: the
reclining crossed-leg pose. The spread-leg floating/arms-crossed clip is not used.
The native 21-limb/20-matrix MM rig, normal head/eyes, Majora's Mask and open
left hand render in MM's ordinary asset owner. No donor bridge or new model
archive is required. The optional private OoT 3DS adapter remains unchanged;
this MM host uses native MM resource paths and MM's normal Alt resolution.

Automatic placement is once per combined or solo MM exploration visit to room 1,
at **X=2200, Z=200**, facing inward at yaw **-20480 (`0xB000`)**. A floor ray
starts at Y=500, then places the actor 35 units above the hit: native collision
gives floor **89.27945**, actor Y **124.27945**. This is a provisional raised
outer-edge location, not a live-accepted custom-scene placement. A placed
`0x7E50` Dm_Stk suppresses the automatic one. Authored XYZ/yaw remain fixed;
the animation and a gentle +/-10-unit visual hover animate independently.
Move/rotate the actor and export the scene to change placement without another
source build once this feature is in the binary. No scene archive was edited.

The reserved Init path runs before any native story setup. It uses actor-owned
22-entry joint/morph arrays, an OC-only cylinder, a circle shadow, and dedicated
draw/update/cleanup. It never runs Skull Kid's story actions, telescope/moon
timers, cutscene cues, item delivery or save writes. Ordinary Dm_Stk params and
standalone builds keep their original path. Invalid scene/room/save/prologue
placements fail closed. Automatic spawns also wait through pause, transitions,
inactive play and player cutscenes. The reserved parameter alone selects the
always-loaded GAMEPLAY_KEEP bank at the pre-Init spawn check, allowing authored
placements without adding OBJECT_STK/STK2/STK3 to the scene's object list;
the host resolves the skeleton/animation/display lists by native resource name.

Focused checks cover exact clip selection, authored transforms, bounded hover,
mask/hand draws, collider and skeleton cleanup, bank isolation, no save writes,
automatic placement guards and authored precedence. All three automatic hosts
wait for room loading to finish before scanning authored actors or marking a
spawn attempt, preventing a duplicate from racing a newly loaded Prelude entry.
The earlier Skull Kid milestone compiled seven native object configurations.
The solo-routing extension adds native checks for both host games and its local
MM snapshot loader; evidence is recorded separately in the recovery archive. Native archive
checks confirm the rig, 40-frame clip,
render resources and raised outer-edge floor. Full executable linking, the
active Lost Woods pack's hill fit, silhouette/clearance, native/Alt rendering,
shadow and room re-entry remain runtime acceptance work.

## Verification and remaining acceptance

- Focused production tests cover monkey parameters/rig/clips, MM viewer lighting
  isolation, typed Saria rig/fitted-pose fallback and retained resources,
  independent NPC lifecycle/blink/Alt rebinding, native draw commands, idempotent
  teardown, and exploration/default-vs-authored placement guards without save writes.
- The existing OoT viewer lifecycle/render and placement-culling controls pass.
- Native Linux object compilation checks both new C++ units, the affected song
  hint unit and both games' viewers in Combo and original build modes. This is
  not a platform link.
- Read-only real-archive checks cover all three monkey clips, all three Saria
  loops and both retained
  Saria Render/Anime POC4 packs, including hierarchy/matrix counts, blink heads
  and fitted root parity. Input hashes are retained with the checkpoint.
- Independent review found the native segment-C and direct-close cleanup gaps;
  both are corrected and covered by focused checks.
- Seated gift tests cover deferred delivery, queued/active duplicates, NPC
  destruction, file/mode/scene guards and completion after consuming the song.
  Actual JSON conversion checks roundtrip the claim and default older saves
  without changing unrelated completion state.
- Polish checks cover both green emitters following authored placement,
  emission bounds, pause/distance suppression, opposite-flank portal math,
  authored horse override and duplicate/cutscene/wrong-mode exclusions. The horse
  fixture covers native idle/blink/cosmetics, collider/skin cleanup, and no
  mount, song-summon or save side effects. The complete focused suite has
  fourteen PASS families after solo routing. Affected hosts and horses compile in their
  applicable Combo/original configurations; prior
  unchanged object configurations remain recorded separately.
- Solo routing checks exercise the three mode choices, both exploration scene
  copies, ready-room/authored precedence, the solo MM local asset bridge and its
  once-per-file song/save flow. Solo SoH production policy, horse and full
  spawn/dialogue fixtures pass UBSan with warnings treated as errors.
- Solo MM snapshot checks use its real resource classes and the production MM
  binary skeleton/limb factory methods with the actual OoT donor plus both
  retained POC4 Saria packs. They cover poses 0/1/3, Alt cycles, distinct blink
  wrappers and cache clearing. Top-level loads stay exact; child resources are
  retained only when they own the factory-selected pointer, and body display
  lists follow MM's render Alt policy. These checks do not render the game.
- Seventeen actor/resource object configurations compile for the extension.
  The new exact-resource helper in `BenPort.cpp` separately compiles against the
  real resource-manager API. A full `BenPort.cpp` diagnostic object attempt was
  blocked by missing libzip `zip.h` in the local include dependency set; those
  failure logs are retained. No complete executable build or link is claimed.

Reproduce focused checks with `scripts/diagnostics/run_forest_actor_tests.py`.
The C++ resource fixtures need the project's real spdlog/glm/json headers in
`CPLUS_INCLUDE_PATH`. `compile_forest_actors.py --output PATH` compiles affected
production units; use normal dependency include paths or `--dependencies` with
a mapping containing `include` fields. `verify_forest_actor_assets.py --help`
documents the read-only archive check. No game assets are redistributed here.

Still untested: full executable/Windows link, native/custom rendering in MM,
default placement visibility and grounding, talk/close and collision in game,
seated model fit, the song receipt/grant and hint use, reload/cycle-reset behavior,
green particle appearance/density, Epona's native/Alt rendering and portal clearance,
re-entry, live asset switching, and window-close shutdown. Test Saria with each
model pack separately and with OoT Alt off. The candidate is implemented and
build/static verified; runtime proof, user acceptance and master promotion are
not claimed. Roll back the isolated commits to return to the parent source;
accepted scene archives and unrelated model-fitting work remain unchanged.
