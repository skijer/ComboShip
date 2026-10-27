# Cumulative candidate verification

The candidate preserves public parent `fa81f7926038f2aa85c7f3e73c96fec043168b66`, including recorded ancestor `9f23b84859ac545a283c155b6642aea302aad414`. Exact checkpoint inputs are recorded in `inputs.json`.

## Approved chamber packaging

The normal package contains the supplied scene geometry and collision with native OoT texture artwork and static native materials. The optional visual archive supplies the authored textures, scrolling materials and other visual replacements. Geometry, UV layout, actor/setup data and travel behavior remain compatible with both configurations. Public publication notes describe these layers without naming third-party scene packs.

Walkable chamber collision uses the native stone/concrete footstep selection and the maximum native surface echo field. Only floor surface-type references and acoustic bits change; vertex positions, topology, polygon normals/flags, cameras and water remain protected. The optional visual archive receives the same acoustic metadata so it cannot restore old floor audio.

## Evidence

- Four independent source histories were combined: confirmed seasonal palettes, Mirror cast/audio/camera, Tycoon HUD, and Sages travel/cutscene catalogue. All 22 scoped source regression commands passed after the required formatter.
- Independent review found a built-in portal provenance defect: normal port resources were rejected as non-mods. The registered OoT owner's resource availability now accepts built-in assets; the native adapter regression failed against the old check and passes against the corrected one.
- Dedicated Lake sand is packaged at native and Alt paths, with 49.872% coverage measured from actual emitted draw vertices. Collision and water checks pass.
- New assets use the existing lossless pooled/zlib/base64 publication format: at most 98,304 compressed bytes and 131,072 text bytes per part. All 266 inherited source parts remain byte-identical.
- Revised chamber package: 4,096 native-texture resources in three bounded source parts. Eleven packaging checks pass, including whole-output cleanup and preservation of unrelated files when namespace symlinks are rejected before writes. The cleanup defect was reproduced first, then corrected.
- The native-base / optional-archive layer suite passes five tests with the updated archive. All 1,149 reachable optional visual resources match the prior full visual stack; geometry/UVs and camera/water data remain unchanged.
- Three collision acoustic checks pass, including the actual native sound and echo readers under sanitizers. All 229 walkable polygons use stone/concrete sound index 2 and surface echo 63.
- The exact CMake preparation targets run successfully for both normal port archive targets and collision checks. Required CI and full Windows packaging remain the next publication gate.

Source/native/static tests do not establish configured in-game appearance, audible echo, collision feel, camera transitions, or user acceptance. Develop/master remain unmerged.
