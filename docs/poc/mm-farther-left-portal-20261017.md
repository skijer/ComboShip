# MM Lost Woods: farther-left placement (POC8)

The user judged POC7 still off center and requested more movement left. Compare
larger shifts from the same viewpoint, then translate the existing doorway by
X -14, Z +34: about 36.8 units left along its face, twice the prior adjustment.
Anchor (-698, 0, -306); yaw remains 12288. Depth changes by less than 0.1 unit.

Source parent: `b558091e94125f1a7c6db8060b8c9e40dd02952d`.
Cumulative base: `43131342882972eeb4a257599ec8b061c7dc6d43`.
Input POC7 SHA256: `caa96a5941c24a07a32cf109c9a26e3fffa149ab2b9a5eab696e613c400a37ff`.
Output POC8 SHA256: `9e62cc55104968710b795d3765f01ba607184438fc8e910f37da41fdece94ded`.

All five doorway resources move together (882 vertices); Y/UV/RGBA and all
textures, mushroom/stump repairs and other scene geometry remain byte-identical
to POC7. Exactly nine resources change, plus one centered_v7 wrapper. All four
setups select it; one doorway remains active. Static clipped native-root clearance
is 3.41 units. The final side-by-side preview uses identical cameras and the
actual packed geometry/materials.

Source v7 uses the new shared anchor for trigger, camera and native trim/shimmer;
v6 and all earlier pack poses remain supported. Archive and source fixtures fail
on the unchanged parent before the fix. Final archive checks, ASan/UBSan return,
camera and finish fixtures, and production renderer object compilation pass.
Use `--left-more` for the new archive/preview/check mode.

Load POC8 in place of the preceding Lost Woods archive and include its matching
source in the next aggregate build. This is an isolated source/asset checkpoint;
no full linked build, configured gameplay/collision acceptance, runtime shimmer
proof or master promotion is claimed. Final visual acceptance remains with cor.
