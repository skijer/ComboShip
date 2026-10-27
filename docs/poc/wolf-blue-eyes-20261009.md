# Wolf Link blue eyes POC1

This records the initial bright-eye proposal. The later [bundled HD candidate](wolf-hd-bundled-20261009.md) follows the user's natural-eye reference and adds the cuff chain.

The requested change makes the existing TP wolf's blue irises readable at a normal viewing distance. The candidate enlarges the bright iris area within the existing baked eye texture, retains dark pupils, and gives only those blue texels an unlit material response. It adds no draw pass, geometry or particles.

| Field | Record |
|---|---|
| Source baseline | October 8 merge bridge `0914f957ed1260ad1c36102db16f2f67ce58191a`, tree `8c97a7ce974e54dea32945889d2214984851a6e2` |
| Asset baseline | Recovered TP mesh POC1; payload SHA256 `d6447975f22bcb1e62a6fac6009f5aa7ab7f78f65f499f19c9b4b7ce3216416e` |
| Candidate | `zz_Wolf_Link_Blue_Eyes_POC1.o2r`, payload SHA256 `033f981817589b1d2947553b18f8a43bf4429268bf2d8795cb66c8efabe4a61f` |
| Scope | 458 left-eye and 448 right-eye texels; the same native Wolf material change in OoT and MM |
| Preservation | Every other texture word, vertices, normals, UVs, weights, rig, all 144 clips and PCM audio are byte-identical to TP POC1; gameplay source is unchanged |
| Configuration | Both normal and Alt Blob paths carry the identical candidate; existing Wolf transformation and supporting pack remain required |
| Recovery | Remove the candidate and restore the original TP appearance pack; revert the two material changes to restore the original material |
| Status | Implemented candidate; local source/asset verification only. Full application build and game acceptance remain pending. No Actions run or publication was requested. |

The wolf's existing RGBA5551 atlas has opaque texture alpha at every texel. This candidate uses alpha zero as an inverse emission mask within the blue irises. Cycle one still computes texture times lighting. Cycle two computes `(lit - texture) * texture_alpha + texture`, preserving lit fur at alpha one and the full texture color at alpha zero. Bilinear alpha filtering blends the edge. Both cycles keep final opacity at one; the mask does not create eye cutouts. Existing fog, depth, culling, texture filtering and command allocation remain unchanged.

The installable archive contains only `objects/forms/wolf_link/gWolfLinkData` and its `alt/` counterpart. It contains no plugin binaries or host port archives. The loose `.bin` is provided only for recovery/tooling; an archive-selected Blob takes priority over the loose file.

On an existing binary the archive already gives brighter blue irises, with ordinary scene lighting. Persistent brightness in shadow requires this source change in the matching executable. The offline preview shows that combined candidate, not an unmodified game binary or captured gameplay.

Local verification records live with the delivery: the native material test reproduces the old lighting loss before the change and passes on both actual host material bodies after it. It checks ordinary body colors, self-lit eyes, filtered mask edges, culling on/off and opaque output. Native real-asset fixtures cover both game loaders and MM skin/draw/action/lifecycle paths in sanitizer and fast-math modes. The preserved TP skin verifier checks all 144 clips and 9,950 integer/half-frame poses per asset; the six exported pose-buffer streams are identical before and after. The retained small-detail integer-packing limitations and static open-eye bake from TP POC1 are unchanged.

For the smallest decisive game check, swap the active TP appearance override for this candidate, fully restart, transform, and face the camera from normal gameplay distance in a bright scene and a dark interior/night scene. Repeat in OoT and MM with Alt on/off. Confirm readable blue irises, dark pupils, ordinary fur shading and intact eye surfaces. Transformation in/out and a normal wolf action should match the baseline. Runtime acceptance and master promotion have not been inferred from the local checks.

Rebuild the exact asset with Python, NumPy and Pillow:

```bash
python3 tools/wolf_blue_eyes/build.py --source /path/to/zz_Wolf_Link_TP_Mesh_POC1.o2r --output /path/to/candidate --style bright
python3 tests/mm_wolf/run_material_glow_tests.py
python3 tests/mm_wolf/run_real_asset_tests.py /path/to/candidate/zz_Wolf_Link_Blue_Eyes_POC1.o2r
```
