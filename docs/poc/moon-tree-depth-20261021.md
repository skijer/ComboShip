# Termina Moon tree depth repair — POC1

The supplied tree draw lists load `G_SHADE` as the complete geometry mode.
That erases `G_ZBUFFER`, although their RDP state still requests `Z_CMP` and
`Z_UPD`. The renderer therefore disables depth comparison. Far-side bark,
branches and leaf cards can overwrite nearer tree surfaces in submission order.

## Exact candidate

- Input: `1 - Termina Field Moon Tree FINAL.prelude(1).o2r`.
- Input SHA256: `9fc14e4cc00a40aa0a95c17828008bee6c0fa1d303804bedb1999e36615bd062`.
- Input Library identity: `libfile_2c514817abf08191bc8d0d909b8e33b5`.
- Output: `Termina_Field_Moon_Tree_DepthFix_POC1.prelude.o2r`.
- Output SHA256: `d5d244e0e7fe5c0efc365d18038909ee6ef00867300ae156dbaffd6c285107df`.
- Scene: `Z2_00KEIKOKU`, with the existing Moon-Tree-RootSeamFix and
  Moon-Tree-Base-Foliage placements. No tree replacement or relocation.

The repair changes only `0x00000004` to `0x00000005` in the full geometry-mode
reset of each of the 134 tree draws. This adds `G_ZBUFFER` while preserving
every other material bit. Their 129 pooled stored Prelude draw lists receive
the same bit; five other stored lists already reference their runtime resources.
The project JSON and blob descriptors remain exact.

## Verification

The independent archive audit fails on the supplied input with 134 depth-state
failures, then passes on the candidate for all 2,811 tree triangles. Stored and
runtime draw bytes match in both archives. Only 135 of 453 resources change:
134 runtime draw lists and `prelude/project/blobs.bin`, totaling 263 byte edits,
each exactly `04` to `05`. All 318 other resources are byte-identical. Candidate
ZIP CRC passes and the original input hash remains unchanged.

All vertices, triangle commands, UVs, bark/leaf/fern images, alpha, colors,
placements, collision, actors, native room/scene headers, culling and unrelated
Bazaar draws are byte-identical. Native alpha-cutout leaf modes remain intact.

A headless asset inspection renders the actual packed bark from three angles
and reverses all triangle submission orders. The input changes 163,028–184,105
pixels with order reversal. The repaired bark changes only 7–13 pixels at
coincident edges. The inspection uses the archive-derived depth state and exact
packed vertices/textures under neutral lighting; it is not a game capture or
configured runtime proof.

## Reproduce and recover

The two scripts use only the Python standard library. Their original directory
layout is sufficient; a full repository or game build is unnecessary.

```sh
python3 -B scripts/repair_moon_tree_depth.py \
  --input '/path/to/1 - Termina Field Moon Tree FINAL.prelude(1).o2r' \
  --output '/new/path/Termina_Field_Moon_Tree_DepthFix_POC1.prelude.o2r' \
  --receipt '/new/path/receipt.json'
python3 -B scripts/diagnostics/check_moon_tree_depth.py \
  '/new/path/Termina_Field_Moon_Tree_DepthFix_POC1.prelude.o2r'
```

The repair refuses an unexpected input hash or an existing output/receipt.
The source checkpoint records the branch, commit/tree and original asset
identity so this work can be restored independently.

## Configuration and status

Use one version of this Termina archive at its existing MM host/load-order
position with Alt Assets enabled. Keep the original unchanged for rollback.
The export was authored against `zz_Termina_Field_3DS_POC4.o2r`; retain the
existing scene dependencies. This candidate adds no new dependency.

Archive/command and focused asset-render checks pass. Actual ComboShip runtime,
seasonal rendering, pause/re-entry and user acceptance remain untested. No
engine executable, source integration or accepted master is promoted here.
The decisive check is to circle the same tree, including Saria's root nook,
then pause/unpause and re-enter the field. In Prelude, reopen the candidate
and re-export it before repeating that check to confirm editor parity.
