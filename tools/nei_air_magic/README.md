# MM air and lightning visual POC 01

Candidate branch: `poc/nei-lightning-wind-20261006`, based on
`af54f452a99ec59800bb59ba770f58d70deefbff` from the existing MM GI routing POC.
This candidate is separate from the baseline and has not been promoted.

The MM Storm Rod retains its travelling projectile, aim, lifetime, collision,
damage and magic gates. Its draw becomes a narrow bent bolt with a white core,
warm rim and branching crackle. The weather cast is unchanged.

The MM Tornado Rod draws one translucent silver wind envelope around Link,
scaled to the native player height. Upper strands trail against movement. Its
toggle, magic drain, launch boost and hover behaviour are unchanged.

The MM Gust Jar draws loose helical wind strands within its existing nozzle,
aim and dimensions, flowing inward for suction and outward for blowing.
Suction remains white; the default wind blow becomes silver. Other elemental
colours are preserved. Draw direction follows the update's captured mode,
including the last timed blow frame. Sampled strands replace the old six
persistent ribbon effects for this item.

The two private RGBA32 materials are byte-identical copies of the established
medallion `fire_wisp` and `light_rays` resources. Donor paths and hashes are in
`material_manifest.json`. The MM renderer checks availability and draws a soft
geometry fallback if textures are absent. Deferred textured commands use the
existing OoT resource-owner bracket. Shared tornado assets and SoH item logic
are unchanged.

## Preview

`export.cpp` exports the same pure samplers used by the native MM draw, with
native 1/16-unit position quantization. `render.py` uses those triangles,
colours, alpha and material texels, standard alpha blending and an opaque
neutral height-44 scale reference. It adds no bloom. It produces a nine-second
20 Hz MP4, a looping GIF, a still and the material sheet. Lightning is shown
in a stationary tracking view to reveal its shape; projectile travel remains
native. The fixture is not Link's model or an in-game capture.

Run from the repository root:

```sh
c++ -std=c++20 -O2 -Isoh tools/nei_air_magic/export.cpp -o /tmp/nei-air-export
/tmp/nei-air-export /tmp/nei-air-samples.bin
python3 tools/nei_air_magic/render.py /tmp/nei-air-samples.bin /tmp/nei-air-preview
python3 -B tests/nei_air_magic/run_tests.py
python3 -B tests/wand_modes/run_tests.py
```

The renderer needs NumPy, Pillow, FFmpeg and the repository's headless EGL
preview support. Native tests use normal repository dependencies; an isolated
header fixture can supply `NEI_TEST_DEPENDENCIES` (and `CPLUS_INCLUDE_PATH` for
the wand runner).

## Evidence and limits

- New sampler checks pass under `-O2` and `-Ofast`: 180 phases, three camera
  orientations, horizontal/vertical aims, multiple body heights, bounded mesh
  sizes and UVs, soft alpha and invalid-input rejection.
- Native MM renderer fixture passes with `-O2 -ffast-math`: present/missing
  textures, balanced resource ownership, restored matrices, NaN/Inf rejection
  before transform entry and safe refusal on exhausted graphics arenas.
  Four simultaneous preview effects peak at 40,304 OPA bytes and 1,071 XLU
  commands; one sampled effect peaks at 1,368 triangle vertices before packing.
- Wand regression runner passes: six MM modes, ownership, C/D input, wheel,
  magic, spawns, native update/cleanup, cast poses, meteor movement, native Storm
  weather and Shadow collision, plus SoH Sand coverage.
- 41 non-presentation gameplay function bodies match the base revision after
  removing comments and whitespace. Non-wind Gust Jar palettes match the base.
- Existing medallion policy baseline passes. The wider `nei_used_fx` runner is
  blocked by its historical `c77c18587a976f6d6cb5c8f91f27593286469218` fixture,
  which lacks `NeiUsedMagicPolicy.h`; this precedes the candidate changes.
- No complete game build, configuration-specific runtime proof, interpolation
  appearance, hardware frame-rate proof or user acceptance is claimed.

Acceptance still needs MM gameplay observation of lightning against locked
targets and walls, Gust Jar suction/blow with every element, and the wind
envelope during movement, boosted jumps, hover, damage cancellation, scene
changes and player forms. Promotion follows that runtime review.
