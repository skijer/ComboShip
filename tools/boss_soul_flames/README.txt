BossSoul Realistic Flames POC5 — alpha encoding repair — October 20, 2026

INSTALL
Close the game. Disable the older boss-flame POC archives and any temporary
native-flame control. Copy the included mods folder into your game directory:

  mods/2ship/zz_BossSoul_Realistic_Flames_MM_POC5_AlphaSafe.o2r
  mods/soh/zz_BossSoul_Realistic_Flames_OoT_POC5_AlphaSafe.o2r

Keep your boss-model packs enabled. Enable these packs and the owning game's
Alt Assets. In Pak Loader, these packs must win over any other replacements
of the same flame paths; filename order alone does not establish priority.
Restart after changing selection. This is an archive-only repair: no build.
Standalone 2Ship/SoH: use only the matching archive in its ordinary mods folder.
Only files under mods are installable. The inputs/native directories are
recovery and test data, and should not be copied into your game mods folders.

WHAT WAS WRONG
The POC4 replacement reaches intensity 66/255; the original native moving
texture never drops below 107/255. The MM flame material uses its moving alpha
in the calculation

  alpha = fixedEnvelope + (movingAlpha - 1) * 192/255

In the renderer's two-cycle shaders, values below -0.51 wrap upward before
clamping. The replacement can reach about -0.558 and become fully opaque,
including where the fixed flame envelope is zero. Those pixels appear as
bright detached fragments. The earlier control's software audit assumed a
simple clamp and therefore missed this wrap. Its report was not a repair.

THE REPAIR
Only the MM moving texture's physical alpha values below 86/255 are raised to
86/255. This keeps the worst intermediate value above -0.5, safely above the
shader wrap threshold. 4,271 alpha bytes changed: 3.26% of texture pixels.
The flame's RGB artwork remains byte-for-byte identical; there is no new art,
no blur, no geometry crop and no change to its scroll, color or native envelope.
The native MM material also draws OoT boss souls in the MM host.

The OoT material uses LOD 128/255 and cannot cross this wrap threshold with
POC4. Its two resources are unchanged byte-for-byte in the included OoT pack.
Both packs remain limited to the same two Alt texture paths as POC4. These
paths are shared with native MM EnLight effects and OoT blue-fire GI.
The accepted soul text, boss models/jaw, animation and progression are intact.

VERIFICATION
Baseline: realistic POC4 archive and original native resources in inputs/native.
Exact hashes and changes are recorded in manifest.json.

The regression first failed with the original MM archive on a headless
OpenGL/Mesa llvmpipe render of the native flame combiner, texture domain,
scroll and production shader wrap stages. Across 64 phases, the old archive
produced up to 2,978 nearest-filtered, 2,723 bilinear-filtered and 2,789
three-point-filtered visible pixels outside the zero native envelope.

The candidate passed all three filters with zero escaped pixels across 256
quarter-frame MM phases and 1,024 quarter-frame OoT phases at 256x512 output.
Header/payload validity, ZIP CRC, two-entry scope, unchanged fixed envelopes,
unchanged RGB channels and complete OoT resource equality also passed.
The alpha bound applies at every scroll phase, including interpolated samples.
GLSL/HLSL/Metal source snapshots show the same wrap threshold; only OpenGL
was executed. This is rendering-test evidence, not a configured game capture.

Replay the same Gohma pickup with the repaired pack to verify your installed
mod stack. cor's supplied clip confirms the existing boss-soul text presentation
and shows the original particle failure; it is not footage of this candidate.
The candidate has not been promoted to an accepted master.

RECOVERY / REPRODUCTION
inputs/POC4_MM.o2r and inputs/POC4_OoT.o2r are unchanged originals.
To roll back, disable these POC5 packs and restore the prior pack selection.

Python dependencies: NumPy. GPU verification also needs Linux desktop OpenGL
and EGL with the surfaceless extension (Mesa works).

  python3 repair_pack.py
  python3 verify_flame.py --mm mods/2ship/zz_BossSoul_Realistic_Flames_MM_POC5_AlphaSafe.o2r --oot mods/soh/zz_BossSoul_Realistic_Flames_OoT_POC5_AlphaSafe.o2r --quarter-phases

Baseline expected failure:
  python3 verify_flame.py --mm inputs/POC4_MM.o2r --out baseline-gpu-failure.json

The recipe changes serialization alpha only, without editing the art source.
source-shaders contains the inspected renderer snapshots; verification.json
and baseline-gpu-failure.json retain the passing/failing rendering evidence.
