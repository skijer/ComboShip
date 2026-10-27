# Native Magic Mirror visuals

The round ALTTP-style handheld mirror is generated from
`combo/gameplay/MagicMirrorGeometry.h`: blue casing and short grip, gold inner
rim, and pale reflecting glass. The silhouette and palette follow cor's supplied
reference images. Both engines and the software preview use the same generated
geometry. The optional forest-glass probe reflects a fixed view of the actual
Lost Woods scene, with the shop, trunk, purple portal and reference-derived Saria
silhouette. Its exact source and render provenance are in `reflection/forest-scene.json`.

## Native interface

Include `combo/gameplay/MagicMirrorDraw.inc.h` after the native `global.h`.
MM defines `COMBO_MAGIC_MIRROR_MM` before including it; OoT leaves it undefined.
Call `ComboMagicMirrorVisual::Draw(play)` after setting the current object matrix.
The function preserves the caller's CPU matrix and reloads that matrix into both
display streams, then restores the native opaque/translucent setup state. Before
allocation it verifies space for the complete draw: aligned data, opaque and
translucent commands, and the overlay scratch cells used by native debug brackets.
Insufficient room queues nothing and leaves native arena cursors untouched.

The object is approximately **32.4 units wide × 41.2 units tall × 3.15 units deep**:
X −16.2..16.2, Y −19..22.2, Z −1.2..1.95. The round face is centered at Y=6. Caller
scale 1 uses these ordinary game units; the draw function internally uses a
1/16-unit vertex precision to retain slender bevels and highlights.

The mesh has 1043 vertices, 960 triangles, and 83 vertex batches (at most 16
vertices per batch). The blue casing/grip and gold inner rim use the opaque
pass. The recessed double-sided reflecting pane and confined shimmer use the
translucent pass. The restrained 960-frame reflection shimmer is deterministic and wraps
exactly; there is no positional bobbing or rotation inside the visual renderer.

The GI, stump reward and inventory icon share this mesh.
On pressing the equipped Mirror button, Link puts away his held item and performs the native
Farore's Wind main casting clip once at 1.25 playback speed, with a short morph
from his current pose. The green seal/wisps begin immediately, including during
put-away, and continue scrolling for the entire Yes/No prompt. The motion plays
once and holds its finish. Yes waits for native textbox teardown and animation
readiness without replaying the cast; No/B restores control, camera and ritual
state. Prompt duration is unbounded; only post-confirmation completion has a
watchdog. There is no floating item or Farore spell actor. The probe uses a magical shimmer when the
Mirror opens its prompt, then native Farore warp audio at committed departure
and actual arrival. Animation start, cancellation and put-away stay silent;
the Farore wind effect is never used. Time Gate ritual framing uses an owned subcamera.
MM receives a private real KEEPON4 setting; its legacy TURN_AROUND alias is NONE.
Its native cutscene manager ownership guard protects the camera and stale return
targets until the scene resets.
The native player bridge remains in `combo/gameplay/MagicMirrorPlayer.inc.c`.

The cast also has private botanical seal and rising vine-wisp textures. The seal
adds one transparent floor quad; existing green ritual geometry and timing stay
in place. Time Gate keeps its own material. Missing textures leave the green
geometry-only ritual. Original built-in ImageGen artwork and prompts are in
`cast/ART_PROVENANCE.md`; rebuild with `build_cast_textures.py` and inspect the
production mesh/material comparison with `render_cast.py --output /path/cast.png`.

Recall and the fixed pickup share the stump cap in the loaded scene collision.
Arrival retains the validated cap/floor fallback and initializes the player before
the camera starts. The exact original editable Prelude input and complete scene
identity are preserved in [scene/README.md](scene/README.md). Moving that stump
in Prelude moves both anchors when geometry and collision are exported together.

## Rebuild the native texture files

Requirements: a C++17 compiler, Python, NumPy, Pillow, and the existing bundled
`soh/assets/custom/fonts/CenturyGothicBold.ttf`. Preview captions use the runtime's
DejaVu Sans font.

```sh
python tools/magic_mirror/generate.py
python tools/magic_mirror/generate.py --preview-dir /absolute/scratch/preview
```

This writes identical 32×32 RGBA32 inventory icons and identical 128×16 IA4-exact
`Magic Mirror` name textures to both native asset trees. Name rendering imports
the existing item-name generator without altering its item list. Registering
these texture identities in asset headers/archive recipes belongs to the native
inventory integration.

The probe additionally writes the same 64×32 RGBA16 forest reflection in both
ports. Its optional 512px `.o2r` companion uses the same key in base/Alt and keeps
the existing NEI icon shadow. The native tile fits exactly 4096 bytes and textured
glass has neutral opaque modulation; separate glints retain its polished look.
POC2 requires its matching HD companion if a previous Mirror companion is installed:
replace POC1 rather than mixing its older logical texture scale with the new UVs.
Missing reflection assets retain the original plain
glass. Rebuild the HD companion with `build_forest_addon.py --output FILE.o2r
--preview-dir DIRECTORY`; see the POC report for the pinned scene-render command.

The optional front PNG, contact sheet, and animated GIF rasterize the actual
mesh after the same 1/16-unit native quantization. Their object geometry, colors,
alpha, and animation phase match the shared builder. They are software previews;
configured game rendering and get-item scale still require runtime verification.

## Verify

```sh
python -m unittest discover -s tools/magic_mirror -v
python tools/magic_mirror/run_draw_tests.py
```

The standalone C++ test checks all 960 animation phases plus the wrap boundary,
large frame values, finite coordinates, scale bounds, deterministic results,
local triangle indices, nondegenerate triangles after native quantization, opaque
metal, and translucent/transparent shimmer. Asset tests check identical game
copies, native dimensions, icon coverage, and exactly representable IA4 names.

The native draw fixture requires the engine's external include dependencies
(`nlohmann/json.hpp` and spdlog, supplied through `CPATH` when needed). It compiles
both real native `GraphicsContext`/GBI headers and executes verbatim production
allocator, setup, and debug bracket bodies. Guarded storage proves rejection
before any mutation when data, OPA commands, XLU commands, or overlay scratch are
short, and proves the exact accepted boundary stays in all three arenas. The
current draw needs 16,816 aligned data bytes, 379 opaque commands, 229 translucent
commands, and two overlay scratch commands. Optional sanitizer flags can be
passed with `MIRROR_TEST_CXXFLAGS`; the draw fixture also links the real C-named
interpolation boundary, so namespace/linkage regressions fail at link time.
The forest-glass path uses 244 translucent commands; the exact boundary and
missing-asset fallback are both checked under the native guarded arenas.
