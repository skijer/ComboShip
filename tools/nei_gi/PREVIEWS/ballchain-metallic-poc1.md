# Ball & Chain metallic POC1

Baseline: `56c83a878562052ca873b7d414f7d0e1175b6325`, the October 9
ComboShip audit bridge gate repair. Candidate branch:
`poc/ballchain-metallic-20261009`. cor approved brushed silver steel,
polished bevels/spikes and GI links, retaining the existing shape and fitting.
The supplied Twilight Princess reference remains at
`tools/nei_gi/REFERENCES/BallChain_Reference.png`.

The GI and held ball reuse their accepted meshes with the existing forged sword
reflection exporter. Only `iron`, `edge`, and `steel` textures and their material
bindings change. Normal-generated texture coordinates move the light bands as
the model/camera turns. The seam and patina textures remain byte-identical.
Each display list clears texture generation and disables texturing on return.

The mesh resources, including positions, normals and UVs, and scale matrices
remain byte-identical to the baseline. GI triangles/load counts remain
8,982/429; held ball counts remain 4,388/309. Item logic, damage, collision,
animation, physics positions, GI effects and shop placement have no changes.
The separate native gameplay tether retains its existing hookshot chain draw;
the GI's display coil and the held ball/shackle receive the new metal finish.

Rebuild both candidate resource sets and an optional isolated archive:

```sh
python3 tools/nei_gi/SOURCE/build_ballchain_metal.py --install --pack /tmp/zz_BallChain_Metallic_POC1.o2r
python3 tools/nei_held/verify_assets.py
```

Use the dedicated builder for this candidate. The original historical
`models.ball_chain()` recipe is preserved as the matte recovery baseline.
The optional archive contains only these two resource directories, with
byte-identical base and Alt copies (32 entries). It needs an existing build
with NEI GI and held replacement routing, as the baseline has. Embedded asset
integration places the resources in the existing shared SoH custom asset tree.

The comparison renderer reads the exact candidate and baseline GLBs, their
textures, and exported reflection metadata. It matches camera, light, geometry
and fit for each before/after pair. Fixed textures use the installed display
list's neutral texture tint; no extra specular shader, bloom or GI particles
are added to the preview. Rotation is a turntable, not player animation.

```sh
python3 tools/nei_gi/runtime_preview/render_ballchain.py /tmp/ballchain-preview --before-root /path/to/baseline --frames 120
```

`baseline/gi/ball_and_chain` and `baseline/held/ball` must contain the original
GLB and checkpoint files. The baseline commit retains both for recovery.

Verification passed: exact resource/GLB parity across all 63 GI models and 39
held components; scoped byte preservation; per-material reflection selection
and return-state cleanup; deterministic rebuild; base/Alt package parity;
120 matched preview frames with no OpenGL errors; whitespace check.

Status: implemented and statically verified; offline preview rendered.
In-game lighting, vanilla/Alt runtime presentation, held/throw/return animation
and shop pickup appearance are untested. The accepted baseline is not promoted
or overwritten. Next review: inspect the comparison, then use an existing build
to check the GI/shop view and equip/throw/return under vanilla and Alt assets.
