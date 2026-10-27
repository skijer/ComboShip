# Termina Field partial seasonal ground tint: diagnosis and source correction

The initial read-only diagnosis established why the merged field lost part of its Autumn/Winter ground tint. The user subsequently authorized including the correction and broader Spring-equivalent ground queries in source, while preserving all existing tint formulas. The archive remains unchanged.

Inspected `Termina_Field_Henriko_Prelude_MoonTree_Optimized_POC1.o2r`, SHA256 `977080dcd5e538f50b431a6d7b0e667a83207e37fd8bef04c981e982aa9038a2`, its source checkpoint, and original `1 - Termina Field Final.prelude.o2r`.

The active Alt floor list `scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_005EC8` contains valid native `G_RDPSETOTHERMODE` opcode `0xEF` (`EF182C10 / C8112078`). It occurs in both the original Final export and the merged archive: the merge did not introduce or corrupt it. Native ground texture hashes `01C650` and `01DE50` are already in the seasonal material table.

Before the correction, production `MaterialCommandLength(0xEF)` returned zero, causing `BuildVariant` to reject this entire floor list. Other inspected ground lists `0153A8`, `015DF0`, and `016598` contain no such opcode and produced variants. Existing Moon-tree discovery is independent and remains intact.

The following are the original diagnostic results, before the source correction:

| Actual merged draw | Autumn words | Winter words | Spring draw words |
| --- | ---: | ---: | ---: |
| 005EC8 | 0 | 0 | 0 |
| 0153A8 | 3820 | 3820 | 3763 |
| 015DF0 | 1120 | 1120 | 1099 |
| 016598 | 2461 | 2461 | 2428 |

In a scratch command-vector copy only, omitting the single unsupported state command caused the rejected list to produce 107-word Autumn and Winter variants, selecting its 57 grass triangles correctly. Removing inserted tint commands reproduced that copied input exactly. This was a diagnostic control, not an archive fix: the render-state command must remain in real output.

Spring separately uses selective image copies for both ground textures. That path reaches the grass even when its draw variant is rejected, explaining Spring's fuller coverage. This source/native probe does not measure visible ground area or substitute for gameplay.

The source correction now accepts and preserves `G_RDPSETOTHERMODE` as a one-word command. The exact floor regression produces Autumn/Winter variants covering all 57 grass triangles, and removing only the inserted tint commands reproduces the original resource exactly. No new grass texture binding or archive rewrite was needed for these triangles.

The follow-up also extends Autumn/Winter's reviewed ground queries to Spring's cohort. Existing accepted draw treatments remain authoritative; selective copies using the existing formulas fill only missing coverage. Palette, tint mode, indexed conversion and pixel-tint function bodies are unchanged. Cold Alt resources are resolved before choosing a fallback, and unclassifiable accepted wrappers conservatively retain their existing treatment. See `mm-broad-seasonal-grass-20261026.md` for implementation, regression and runtime limits. All four actual grass roots and the supplied Moon-tree archive passed the native sanitizer gate; configured game appearance remains untested.
