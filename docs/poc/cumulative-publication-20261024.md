# Audited cumulative publication, October 24, 2026

The user explicitly authorized combining, verifying and publishing this pass
to `marsh6487/ComboShip`, including source, generated assets, compressed texture
parts, tests, build/CI repairs and a matching executable/host-archive handoff.
Candidate/cumulative branches and a PR may be published. `develop` stays at
`196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`; the initial `master` ref is absent.
Final promotion requires separate explicit approval.

## Exact included source

| Contribution | Authorized commit |
|---|---|
| Recovered native shops/cameras/purchases, title cards, D-pad transfer, regional shortcuts/Snowhead ascent/actors/effects/material support | `25e990332ef9d9d5147c2df4ef5413c61af7aa5b` |
| Final What's New/cosmetics/weather UI | `87e6cf7c97e467a45c035455d7feb2596396d293` |
| Authored intact/broken Giant's Knife and casing fit | `3794ff0f41546d6e8fc68c3e5c0d483b787d7a78` |
| Stronger Ice/Light casings, Fire preserved | `8c1504c44a0f430dadf3fe48bc50a63a48d8e09f` |
| OoT inventory paging/cursor recovery | `2f0d77ce562210b35499951d181e21bd53bc39e2` |
| Correct continuous native monkey call | `39ad19c9f4c737e388a09f14b5d7fe313e1ebad2` |
| Young Fado archive-owned 1.25 scale | `c32978d923fd7ac37db3113dee0af66abbc221bc` |

Input recovery artifact: `ComboShip_All_Unmerged_Source_Checkpoints_20261024.zip`
version 2, SHA256
`1d4d185bad2edf1053a7d07dfa14dd1707a4277104d46554f6629e481b9d9840`.
It and `ComboShip_Premerge_Audit_20261024.txt` version 2 identify the seven
original snapshots, their evidence, excluded older POCs and restoration paths.

The local integration starts at Fado, which already inherits pause, monkey and
the recovered base, then merges the exact UI, Knife and casing heads. Initial
combined source is `e6de3d5c6de78f516dc16754e6583e98f220d320`, tree
`350bb6f7b70eb6e77a216b4310bdb1e3e2cdcf6b`. All seven heads are ancestors.
The only overlapping implementation paths are the Knife/casing recipe and
generated aura header; their combined deterministic generation check passes.
The recovered 128 source entries remain: 114 original blobs, 14 intentional
evolutions and zero missing entries. The 221-entry CI registry retains every
base check and includes the new pause regression.

Publication may represent this verified combined tree as one new snapshot
commit with the existing public develop commit as parent. This avoids uploading
intermediate rejected/superseded asset versions. The durable source bundle
preserves the original 36-commit integration graph and all seven input heads;
the published snapshot has its own commit identity. Any later source repair
must have a recorded delta, checks and a new recoverable checkpoint.

## Assets and installation

Preserve the existing spin/market lossless pools and ordered encoded parts.
Upload only changed or verified missing blobs, with per-file size/Git-SHA
receipts and resumable state. Existing packed-part limits are 96 KiB compressed
and 128 KiB encoded text where applicable. Verify exact decoded resource hashes
and final remote tree before guarded candidate ref updates. No accepted art,
pixels, resolution, UVs, geometry or resource path is changed to fit a transfer.

Rebuild the executable, `soh.o2r` and `2ship.o2r` together. The MM native shop
requires its matching prepared scene/room/camera metadata. Older visual packs
that shadow those records must retain the new metadata. Selected scene, actor,
NPC, sky and optional HD-title packs remain separate; preserve explicit load
order instead of treating them as executable inclusions.

Young Fado's separate candidate is
`Young_Fado_Shoulder_Lining_Scale125_20261024.o2r`, SHA256
`1f465322b395d02c2596ba876ec05288fe2d75acf05328340006b05d661101e3`.
Use it with this rebuilt loader in `mods/soh/`, OoT Alternate Assets enabled,
replacing the older Young Fado pack. All 174 original resource payloads remain
byte-identical; one DrawScale scalar is added. Adult Fado is unchanged.

## Verification and runtime boundary

Run clean asset/preflight checks, combined sword/held/GI checks, all six required
regression groups, independent integration review, and Windows/Linux CI on the
published cumulative source. Final commands, results, source/tree identities,
upload receipts and CI/build links belong in the saved publication checkpoint
and PR handoff. This document records scope and initial integration evidence;
it does not claim that pending CI or configured gameplay has passed.

Targeted runtime acceptance remains separate: affected OoT inventory save and
controller; D-pad transfer; native shop browse/purchase/receipt return with
matching metadata; regional child/adult traversal/returns in vanilla/Djipi;
new actor/prop appearance; Knife and Ice/Light visibility; continuous positional
monkey cadence and cleanup; Fado size, feet/contact, poses and live Alt switch.
The reported configured Woodfall opacity observation remains unidentified.
The older full native-resource helper's Anju synthetic-poison assertion is a
named preexisting limitation; required assertions must never be suppressed to
claim a green result. Older Demise draft #42 remains outside this pass.
