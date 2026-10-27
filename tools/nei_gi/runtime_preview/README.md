# NEI GI runtime revision preview

The current asset set contains **60 serialized GI models**. The sword and Slate
preview tools load the current exported GLB checkpoints and sample the production
C++ effect policy. See the [approved sword adjustment](../PREVIEWS/sword-tuned-pass.md)
and [Slate rings attached to the model](../PREVIEWS/slate-rings-attached.md).
Approved sword hashes are recorded in
[`../SOURCE/sword_visual_approval.json`](../SOURCE/sword_visual_approval.json).

Use Python 3.11 or newer with NumPy/Pillow, a C++20 compiler, Mesa EGL/OpenGL
libraries (`libEGL.so.1` and `libGL.so.1`), and the DejaVu Sans font. On Linux the
renderer expects the font at `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`.
The two GIF tools compile their C++ exporters automatically. From the repository
root, using a Python environment with the recorded dependencies:

```sh
python3 -m pip install -r tools/nei_gi/SOURCE/forged_swords/requirements.txt
python3 tools/nei_gi/runtime_preview/render_swords.py /tmp/nei-gi-previews --stem swords_current --groups
python3 tools/nei_gi/runtime_preview/render_slate.py /tmp/nei-gi-previews --slug sheikah_slate --stem slate_current
```

The sword output includes all nine swords at a common camera scale, three larger
group GIFs, effects-off views, a guard-aligned scale sheet, and camera metadata.
Use `--before-root` with an earlier checkpoint directory to add comparison pairs.
The Slate tool accepts each of the six slate slugs through `--slug`. These tools
preserve model materials and depth testing and use quantized production effect
triangles. Lighting and reflections remain offline approximations; the previews
do not verify gameplay appearance or a full game build. The older video renderer
below additionally requires ffmpeg.

## Historical POC5–POC7 revisions

The descriptions, budgets and verification results below record the earlier
rod/spell preview work. They are not fresh measurements of the current sword
and Slate changes.

POC7 removes all nine stationary decorative ice crystals from `a4e7705e6`:
six formations in the effect code and three `Small ice splinter` parts baked
into the ice-rod mesh. The eight collapsing/tumbling fragments and the entire
orb effect remain byte-identical across 180 frames. All retained model parts
have identical positions, normals, UVs, indices, and material bindings. The
central focus, staff, and pommel remain intact. The revised ice model has
3,168 triangles and 107 native vertex loads. Other models are unchanged.
Replace the previous GI archive with the POC7 archive when testing this code,
so an older mod pack cannot restore the three baked splinters. Both base and
Alt entries are included. Equipped gameplay models are separate from these
shop, freestanding, and overhead get-item resources.

POC6 changes only the ice rod's intrinsic accents on top of `2508f9ae9`.
The user approved the cores but found the surrounding ice ribbons/glints too
electrical. Ice now uses six pointed hexagonal crystal formations plus eight
tumbling, falling frost fragments. Hard planar face colors and explicit rear-face
culling keep the crystals legible in the translucent pass. No core texture,
orb mesh, accepted model, fire/light/spell effect, or checkbox shimmer changed.
Use `--mode ice` for the full-rod/close-up animation. Focused production tests
pass; the eight-slot shop uses 75,728 vertex bytes and 2,119 XLU commands at the
fixture frame. A byte comparison confirms all non-ice accents and every orb
export are identical to POC5 across 180 frames. Runtime acceptance is pending.

POC5 is based on POC4 commit `511cf2d75`. All accepted model, texture, GLB, and display-list resource bytes remain unchanged. POC4's shop clearance, overhead dispatch, approved light-rod energy, optional shimmer, and green Deku Leaf particles are preserved.

The C++ exporter evaluates the same effect geometry and texture policy that the game renderer uses, including 1/16-unit vertex and s10.5 texture-coordinate quantization. It writes the existing triangle stream and an adjacent `.orbs` stream containing native I8 texels and their curved meshes. The Python renderer loads accepted GLB checkpoints, applies native matrix/draw scales, and composites opaque depth, textured orb energy, unlit accents, then the crystal shell. Orb RGB matches the runtime primitive/environment gradient; I8 intensity controls alpha. There is no added bloom. Lighting and the fixed preview camera are offline approximations, not gameplay evidence.

The fire body is turbulent with rising embers; ice uses cloudy density with angular frost veins beneath the existing jagged rings/glints. Spells keep their crossing motes and discharge around a substantial animated core; Demise retains a black center. The original procedural texture sequence uses 48 frames per profile, 64×64 I8 (4 KiB per frame), and four profiles totaling 768 KiB. It is generated once, uses immutable addresses, and requires no new archive or copied reference pixels.

From the repository root (Python with NumPy/Pillow, system Mesa EGL/OpenGL, ffmpeg, and a C++20 compiler):

```sh
c++ -std=c++20 -O2 -Isoh tools/nei_gi/runtime_preview/export_effects.cpp -o /tmp/nei-gi-export
/tmp/nei-gi-export /tmp/nei-gi-effects.bin
python tools/nei_gi/runtime_preview/render.py /tmp/nei-gi-effects.bin /tmp/nei-gi-previews
python tools/nei_gi/runtime_preview/render.py /tmp/nei-gi-effects.bin /tmp/nei-gi-previews --mode energy --close-up
python tools/nei_gi/runtime_preview/render.py /tmp/nei-gi-effects.bin /tmp/nei-gi-previews --mode ice
python tools/nei_gi/runtime_preview/shop_fit.py /tmp/nei-gi-previews/shop-fit.png
```

The two videos contain 180 frames at 20 fps. The first shows intrinsic effects with optional shimmer off; the second compares shimmer off/on for Roc’s Feather and the green Deku Leaf effect. The clearance still uses the real mesh bounds and a schematic shelf at EnGirlA's local Y=-24.

Validation covers actual common/shop/overhead draw functions, NEI feather resource parity in all three, missing and Alt-only resource fallback, matrix restoration, serialized mesh clearance, bounded/contained geometry, native texture commands, UV-aware triangle-cache roundtrip, and an eight-slot potion-shop draw budget. The new draw fixture uses 75,472 arena vertex bytes and 2,166 of 4,096 XLU commands. A comparison against the POC4 exporter confirms identical production geometry/colors/alpha across 180 frames for light, both shimmer palettes, and the retained spell motes/arcs. Actual runtime appearance, interpolation, and scene-wide buffer headroom still require in-game review.

The earlier review seed mistakenly placed the separate randomizer `Roc's Feather`. The corrected seed changes only Market Bazaar Item 5 to `Progressive Roc`, the NEI feather entry. Use a fresh save and inspect that slot before collecting the house feather, because an owned NEI feather makes the next progressive copy a cape.
