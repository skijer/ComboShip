# October 10 late-night batch

The user requested the late October 9 / early October 10 work and selected
**combine into existing PR41 for one fresh runtime build**, leaving `develop`
unchanged. The exact source parent is
`3abeff897e38f00de4a70d674c463212ec110575` on
`bridge/audit-merge-20261008`. Parent Actions run `38024948399` passed the
regression gate and Windows/Linux builds. Its packages apply to the parent,
not to this batch.

| Component | Recovery and preservation |
| --- | --- |
| Autumn forest writeback | Exact published `78590982`; retain the copper wallpaper/canopy tint after renderer pointer writebacks, Off and scene resets. |
| Autumn ground grass | Exact published `d8dbd01d`; scene-qualified grass/forest materials across eligible MM scenes, including shared native and selected Alt grass. Preserve unrelated terrain, alpha, geometry and Off restoration. |
| Sword GI polish | Exact local `e1407532`, recovered from its saved Git bundle. Upright custom pickups, readable independent particles and stronger themes; preserve accepted authored vanilla meshes, exact hex shimmer, held models and shelf fitting. |
| Tunic belts | Exact local `20ba282a`, recovered from its saved Git bundle. Fitted Champion/Sages/Spirit trim and rear harness; preserve current cloth, textures, materials, scale and Sages particles. |
| Mario SM64 POC3 | Exact saved archive `9980d41450bcaa741d2f86cee3658606eec76f558b4d73c5d26d7eccc6f7e52f`. Preserve the runtime mesh and optional 512px icon bytes; reconstruct inspection GLB and derive bundled 32px icons. Original procedural authoring history was unavailable. |
| C-Up tutorial coverage | Former unpublished `5a0b6ba3` source was unavailable. Rebuild the bounded routing sweep against this parent and record fresh regression results. Preserve C-Up, A cycling/equipping, selected identities, encoded byte lengths, ownership and gameplay. |
| Equipment preview transition | User reported the player view remaining fixed when navigating away. MM's old overlay uses fixed screen coordinates during the cube turn. Attach the existing framebuffer composite to the equipment page matrix; preserve its framing, poses and input behavior. |

The parent already includes Wolf eye/chain and audio/persistence work,
native-star ownership, summer firefly/cottonwood visibility, Hourglass/Crystal
receipts, metallic Ball & Chain, closed Time Gate sides, Sages medallion
particles, progressive vanilla fallback, bottles, Zora combat and the other
cumulative implementations recorded in `overnight-merge-bridge-20261009.md`.
Do not reapply superseded patches over that parent.

## Separate asset installs

`zzzzz_TP_BombBag_Wallet_Framing_POC1.o2r` remains an optional asset overlay.
It fits the TP donor's bomb bags to 70% and wallets to 60%; it depends on the
existing TP model pack and is deliberately not installed into the default
application asset tree. Archive SHA-256:
`9fe4324e01d986b2346bdfc8d0355abcd2a87147b34f05c119f85eb6b450ad46`.
The existing NEI inventory icon mod also remains a separate mod artifact.
Neither archive's presence or installed priority is established by a code
merge or executable build.

## Verification and status

The renderer/grass sanitizer fixture, MM weather and native seasonal weather,
progressive sword Alt toggle, upright sword native-header/camera fixtures,
four tunic-clearance checks and asset collision checks pass on the combined
source. The first full GI run exposed stale generated Mario/tunic bounds;
the regenerated bounds now compare against the exact serialized geometry.
Tests and user acceptance are separate: game/GPU appearance and performance
with the actual installed pack order remain untested.

All 113 canonical commands pass across the main run, its logged
remaining-section resumption and the newly added Mario/C-Up checks. The main
run ended without a diagnostic before the tunic resource query; that exact
query and the remaining section pass on rerun. The final build-input check
initially lacked Ninja in the diagnostic environment and passes after adding
it. No test assertions were removed or weakened to obtain these results.
The complete GI `--held --combo --sanitize` run also passes. The formatted
C-Up suite passes 205 production lookup/display/cursor checks in both normal
and ASan/UBSan modes; all four changed pause translation units compile with
actual headers. Archive/GLB/native icon parity, 64 serialized GI assets,
23 icons, asset collisions, conflict markers and canonical clang-format 14
checks pass. The equipment preview adds 323 production transition/render and
framebuffer-normalization checks, passing normally and with ASan/UBSan. Its
changed translation unit passes real-header syntax; adjacent ownership,
inventory and C-Up suites pass. Independent review has no unresolved blocking
findings. See `equipment-preview-transition-20261010.md` for the diagnosed
fixed-overlay and inherited-texture-dimension issues and the bounded fix.

Use the new executable and its matching generated `soh.o2r` and `2ship.o2r`
together. Review autumn Off/Autumn/reset in Termina Field and shared-grass
areas, all sword tiers with Alt off/on, three tunic belts through a spin,
Mario pickup/icon and C-Up on wheel selections, new equipment and forms.
Leave and return to MM equipment in both directions through the entire cube
turn, then cycle the L subpages with Link, Mario and transformation forms.
Retain the original parent and independently saved component checkpoints for
rollback. This batch updates draft PR41; it does not promote a gameplay master.
