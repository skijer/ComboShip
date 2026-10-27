# Saria Prelude poses and green hex shimmer — 2026-10-19

Bounded candidate on PR44 source `ab7e30385b0bf21a0e58e8c2d0dd910bf33648d2`,
tree `9cb42dac25299cfb74acefe342e74c6a7b6ac6df`. Local equivalent parent
`dee321b9abb28e69ea5cc585a53815b90e9c8aa3` has the identical tree. Preserve
the cumulative forest/Mirror, Woods and baked Chateau integration.

## Prelude entries

In **MM Prelude**, add **En_Viewer, actor `0x001C`**. Set the parameter below,
then edit XYZ and yaw. The actor's authored transform remains fixed; the
animation and aura follow it. No new source build is needed for subsequent
placement edits after this implementation is in the executable.

| Entry | Scenery params | Termina Field song-giver params |
| --- | --- | --- |
| Saria — waiting, hands behind her back | `0x7F13` | `0x7E13` |
| Saria — playing her ocarina, with instrument | `0x7F23` | `0x7E23` |
| Saria — seated | `0x7F33` | `0x7E33` |

For the NPC outside South Clock Town, place **one song-giver entry in Termina
Field room 0**. Any of the three reward parameters suppresses the automatic
south-gate Saria. Move/rotate that entry beside the exported Moon tree and set
Y to the desired floor/seating level. Scenery parameters do not suppress the
Termina Field reward NPC. The default automatic reward remains seated.

For **2Ship Lost Woods room 0**, use the scenery column. An authored Saria
overrides the automatic waiting placement. Scenery never gives a reward.
Reward parameters give the fixed song only in Termina Field under the existing
save/mode guards; changing pose cannot reset the shared once-per-file claim.
The item pool, other story actors, receipts and hint behavior are preserved.

These are parameter variants of Prelude's existing En_Viewer entry, not a
replacement editor build or a new mesh recipe. In **OoT Prelude**, the existing
scenery variants use En_Viewer `0x002A` with the same `0x7F13/23/33` parameters.
The MM reward parameters are not OoT actor parameters.

## Saria resources and object bank

The ocarina entry resolves Saria's rig, **65-frame** playing loop and
`gSariaRightHandAndOcarinaDL`; it no longer falls back to arms-at-sides in MM.
The private model's fitted playing clip, right-hand/ocarina prop and fitted
limb overrides keep their existing precedence and resource owner. Waiting and
seated use their existing 24-frame loops. Playing keeps closed performance
eyes and opens them for conversation; waiting and seated retain blinking.

OoT's native Saria bank is **OBJECT_SA `0x00BC`**. MM's reserved viewer host
uses **GAMEPLAY_KEEP** and resolves the Saria resources from the existing OoT
donor; do not insert OoT object number `0x00BC` into MM's different object table.
Solo 2Ship still requires its existing `oot.o2r` companion. ComboShip uses the
OoT owner/Alt selection; solo 2Ship uses MM-owned snapshots and MM Alt selection.
Select one retained Saria model pack in the appropriate game's mod order.
No donor, model pack, scene geometry, collision or texture archive was changed.

## Green hollow hexes

Both MM placements, in ComboShip and solo 2Ship, now draw twelve small,
translucent green hollow hex rings around the actor's current XYZ. They drift
upward, turn toward the camera and fade; seated rings stay lower. The aura
uses local world units rather than Saria's model scale, follows authored
relocations, and is suppressed during pause and beyond its existing distance/
height limits. It uses untextured native translucent geometry and no new pack.
The existing native solo-SoH forest sparkle implementation is unchanged.

The helper stays at file scope with the real frame-interpolation C declarations;
placing OPEN_DISPS inside an anonymous namespace creates invalid private
interpolation references. The linked draw fixture covers this boundary.

## Verification and remaining runtime check

Focused actor tests exercise all six requested parameter combinations, authored
reward overrides, shared one-time grant routing, transforms, collider, blink,
closed playing eyes, resource rebinding, cleanup, actual hex vertex/triangle
commands, hollow-ring geometry, green vertex colors and pause/distance guards.
The native/Alt resource fixtures include the 65-frame loop, prop selection,
missing-instrument rejection, root preservation and retained ownership.

Reproduce with:

```sh
python3 scripts/diagnostics/run_forest_actor_tests.py
python3 scripts/diagnostics/run_mm_placed_saria_resources_test.py --sanitize \
  --native-oot OOT_ARCHIVE --native-mm MM_ARCHIVE \
  --saria ANIME_POC12_ARCHIVE --saria RENDER_POC11_ARCHIVE
python3 scripts/diagnostics/run_solo_forest_actors_tests.py
python3 scripts/diagnostics/compile_forest_actors.py \
  --source mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp \
  --source mm/2s2h/Enhancements/Gameplay/MmPlacedSariaResources.cpp \
  --source soh/soh/Enhancements/cosmetics/PlacedSariaResources.cpp \
  --output OUTPUT_DIRECTORY
```

Supply the existing public dependency include paths through CPLUS_INCLUDE_PATH.
The real-pack sanitizer run checks both selected packs, native/Alt ownership,
all four animations and repeated cache eviction. The solo routing fixtures
preserve the other mode paths. This is source/compiled-fixture evidence, not a
linked game build, GPU preview, installed Prelude test or gameplay acceptance.

Verified results for this candidate:

| Check | Result |
| --- | --- |
| Focused forest actor suite | PASS, 14 reported test families |
| MM resource snapshots, ASan/UBSan, native/Alt and both retained packs | PASS |
| Native archive/rig/clip/prop contracts with both packs | PASS |
| Solo forest routing/placement/dialogue and stationary horse fixtures | PASS |
| Affected native production units | PASS, four object configurations |
| Interpolation linkage inspection | Global C symbols; no private namespace references |
| clang-format 14.0.6 and diff whitespace | PASS, all seven C/C++ payloads |

In game: place each variant, verify its instrument/hand fit, move the reward
entry beside the Moon tree, confirm a single Saria and matching aura, then
talk/close, receive the song once, re-enter/reload/change pose and confirm no
refill. Check the Lost Woods aura and each selected native/Alt configuration.
The cumulative baseline and scene masters remain unpromoted.

## Exact retained asset inputs

| Input | SHA-256 |
| --- | --- |
| Native OoT donor | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |
| Native MM donor | `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59` |
| Saria Anime POC12 | `94a6235b27c2329feac19b5297aa517a67f425e667edf918f723718e4b6e070f` |
| Saria Render POC11 | `b47bd839335e91ff123d04c0c362cfc08b1f6bba9cf9aece01e12bb10e5bd0f5` |

Recovery: retain the cumulative parent, then apply only this candidate's git
diff on a new branch at that parent. Read the candidate commit/tree with
`git show --format=fuller --stat` and `git rev-parse HEAD^{tree}`. Existing
external archives remain byte-identical; no old forest snapshot is reapplied.
