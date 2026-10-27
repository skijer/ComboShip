# Regional surface replacement packs

The shipped executable looks for these stable resource paths every draw. Put a replacement `.o2r` in the OoT host's normal enabled mods directory and restart/reload the game resources. No source edit, CVar, ROM extraction or recompilation is required. A missing resource independently falls back to the listed native material. Base replacement entries apply with either vanilla or Alt Assets; an optional `alt/` entry uses the engine's normal Alt precedence. Do not replace whole scene archives to change these surfaces.

| PNG filename | Resource path (no `__OTR__` prefix in ZIP) | Native tile and format | Native fallback |
|---|---|---|---|
| `LakeStone.png` | `textures/comboship/regional/lake_hylia/stone` | 32×64 RGBA16 | `scenes/shared/spot06_scene/spot06_sceneTex_00FEB8` |
| `LakeWater.png` | `textures/comboship/regional/lake_hylia/water` | 64×64 I8 | `objects/gameplay_keep/gEffWaterRippleTex` |
| `SnowheadStone.png` | `textures/comboship/regional/snowhead/stone` | 32×32 RGBA16 | `objects/object_spot08_obj/gZorasFountainIceRampUpperSideTex` |
| `SnowheadSnow.png` | `textures/comboship/regional/snowhead/ascent_snow` | 32×32 RGBA16 | `objects/object_spot08_obj/gZorasFountainIceRampSurfaceTex` |
| `SnowheadIce.png` | `textures/comboship/regional/snowhead/ice` | 32×32 RGBA16 | `objects/object_spot08_obj/gZorasFountainIceRampMiddleSlopeTex` |
| `ValleySnow.png` | `textures/comboship/regional/snowhead/valley_snow` | 32×32 RGBA16 | `objects/object_spot08_obj/gZorasFountainIceRampSurfaceTex` |

Create a deterministic debug pack (only generated checker swatches):

```sh
python3 tools/regional_materials/build_pack.py --debug-swatches --scale 4 --output RegionalDebug.o2r
```

Or place any subset of the six named PNGs in a directory and run:

```sh
python3 tools/regional_materials/build_pack.py --input-dir my-regional-pngs --output MyRegionalTextures.o2r
```

PNG dimensions must be positive integer multiples of that material's logical tile. For example, use 128×256 or 512×1024 for LakeStone, and 128×128 or 512×512 for the snow/ice tiles. The tool does not resize or resample supplied pixels. Omitted PNGs use native materials. Pillow is the only nonstandard Python dependency. ZIP timestamps, order, permissions, resource metadata and generated swatches are deterministic.

Resources use OTEX v1 with the sampler's native type, `TEX_FLAG_LOAD_AS_RAW`, full RGBA32 pixels, `HByteScale = physicalWidth × 4 / (nativeWidth × nativeBytesPerPixel)` and `VPixelScale = physicalHeight / nativeHeight`. Thus a 4× RGBA16 replacement has horizontal byte scale 8 and vertical pixel scale 4; a 4× I8 water replacement has byte scale 16 and pixel scale 4. The native 16-/8-bit texture load still uses its logical tile size. The engine's texture factory and SETTIMG handler retain the resource, dimensions, flags and scales; the draw code never substitutes a bare pixel pointer.

Ascent tops use continuous world X/Z UVs at four N64 texture-coordinate units per world unit. Stone/ice side UVs follow cumulative station arclength and negative height, continuing across corners and each face's diagonal. The Lake cave retains continuous world-space UVs across adjacent rings; its water retains inward scroll. Color/shade and water alpha still multiply the chosen texture, so use a neutral swatch to judge brightness. Local snow flakes and the protected NEI opening shimmer remain their effect textures; surface replacements do not replace either effect.

Verify without archives:

```sh
COMBO_NEI_FIXTURE_CPATH=/path/to/native/header-dependencies python3 tests/regional_passages/run_material_tests.py
```

This runs ASan/UBSan portable ascent/grounded threshold/return/snowfall tests, deterministic pack metadata and actual native TextureV1/SETTIMG resource-lifetime tests. Native/Alt scene geometry and gameplay traversal are separate checks; this is not visual acceptance or configured runtime proof.
