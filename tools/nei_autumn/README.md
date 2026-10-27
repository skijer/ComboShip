# Generated autumn leaf textures

`leaves-generated.png` is the unchanged transparent atlas generated for this POC. Its four cells contain crimson maple, burnt-orange oak, golden maple, and copper-brown oval leaves. The source and packaged resource hashes are recorded in `manifest.json`.

Art direction: a transparent square 2-by-2 atlas, one clearly separated leaf per cell, painterly fantasy-game texture style, distinct silhouettes and visible veins, equal cell sizes, no labels, glow, shadows, or painted background. The actual generated image is 1254 by 1254 pixels.

Run `python3 tools/nei_autumn/build_textures.py` from the repository with Pillow installed to reproduce the four binary resources. This only crops the generated cells, downsamples to 128 by 128, and packages native OTEX V1 RGBA32 textures with retained alpha and scale 4. The logical drawing tile is 32 by 32.

Resources use the private `objects/nei_autumn/leaves/` namespace in MM. Both the seasonal snowfall replacement and the occasional ambient tree leaves draw these textures. Stock tree-shake impact leaves retain their existing model and texture.

Rebuild **Generate2ShipOtr** and deploy the regenerated **2ship.o2r** with the rebuilt game DLLs. Replacing code while keeping an older archive will omit the generated textures. The CMake target already includes the complete `mm/assets/custom` directory; no external texture pack is needed.

`python3 tests/seasons/run_generated_leaf_texture_tests.py` reads all four resources through the native texture factory and verifies dimensions, format, alpha, scaling, and retained buffer ownership. It does not exercise a GPU or deployed archive.
