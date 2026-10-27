# Autumn: every slot in view, finite preview

Baseline remains `a82852a3a02c8cabc541cce7c124da71c5755efb`. This follow-up
supersedes the leaf distribution in `mm-autumn-alt-density-20261008.md` and
retains its separate, source-tested Alt foliage changes. Neither candidate is
runtime accepted or promoted. No full build or remote push was performed.

The user's preview feedback exposed three distinct issues:

- The static red leaf was baked into the sampled background frame, separate
  from the simulated leaves. The new background is reconstructed from the
  fixed-camera capture with FFmpeg's temporal median filter. This removes
  transient foreground leaves; moving clouds/rain may show filtering artifacts.
- The 96-slot allocation still wasted foreground particles behind the camera
  and particles at the edges. All 96 slots now spawn in a jittered, permuted
  12×8 camera-view layout at different world-space depths. The allocation remains
  16/32/48; near leaves begin 700–1000 units away, keeping them small. Clock Town's
  compact middle/distant distribution remains, with a 600-unit minimum radius.
- Both preview renderers interpolated the last recorded frame back to the first.
  They now hold the final frame. The interactive comparison offers Replay.

The actual draw checks the current projection. Only offscreen or fully
distance-faded leaves are reseeded; their interpolation epoch changes and they fade in individually.
Edge opacity fades before exit. The fall/flutter/wind equations, leaf artwork,
camera/Link clear pocket, native snow allocation, other seasons, Alt foliage
corrections and intermittent-rain implementation remain preserved.

## Evidence and limits

The new density test first failed with only 25/96 submissions at a narrow FOV.
The revision passes all six outdoor areas at 4:3, 16:9 and 21:9, with FOVs
35/60/90 and upward/level/downward views. Every tested draw submits 96 slots.
Recycling is checked against the prior projection to reject visible teleporting
and against a per-update bound to reject a periodic whole-field restart.

The full native seasonal/weather harness passes ASan/UBSan/bounds checks, along
with real-header syntax for the complete production particle actor. Existing
season, rain, native snow/fog, RNG, ownership and Off restoration checks pass.
Leak detection is disabled because this executor cannot inspect `/proc` threads;
the other sanitizer checks remain active. Existing real-header warnings remain.

All 120 exported production frames contain 96 nonzero-alpha leaves. Strict
frustum bounds are checked before export; normalized JSON coordinates have
four-decimal precision. Preview decoding, motion, selection, pause, end hold and
explicit replay pass at their DOM/canvas boundary. The MP4 is visually inspected.

The preview is a production-motion simulation, not a game/GPU recording. It
does not model terrain occlusion, fog, the revised foliage colors or camera
movement. Onscreen slots can be faint during their transitions; the camera/Link
clear pocket and terrain can still hide pixels in game. Rendering-performance
and appearance acceptance with the user's actual Alt pack stack remain untested.

## Combined recovery follow-up

The combined summer/autumn candidate exposed a longer-lived failure that the
short preview did not cover. A sustained steep-downward view kept falling leaves
onscreen past the native 8000-unit distance fade. The first nonzero-alpha slot
was lost at frame 365, although the renderer still submitted 96 slots. Recycling
now also checks that exact fully faded distance, preserving opaque visible
trajectories and individual epoch/fade-in behavior.

Fresh ASan/UBSan/bounds tests cover 3600 frames each in Termina Field and Clock
Town. They retain 96 nonzero-alpha slots, with 304 and 175 individual recycles,
and reject opaque visible teleporting or a whole-field restart. The fixture now
projects through the actual native `guLookAtF`; its handedness assertion also
passes. The summer update/reset boundary is stubbed only in this seasonal
fixture; separate production summer and weather preservation tests pass.

The supplied 120-frame preview remains unchanged as checkpoint evidence. It
predates the long-view fix and the native projection correction; it was not
rerendered or accepted as gameplay proof. Full particle-actor syntax passes,
and the foliage/grass/material fixture checks remain green. The optional canopy
sanitizer variant cannot link its retained, otherwise-dead lifecycle helpers;
the standard canopy fixture and real-header foliage syntax pass. This fixture
limitation does not establish a production runtime result.

## Reproduce

With the normal dependency headers available, run:

```sh
python3 tests/seasons/run_native_weather_tests.py --sanitize
python3 tests/seasons/run_native_weather_tests.py --sanitize --preview-output=/absolute/path/all-96.json
python3 tools/nei_autumn/render_motion_preview.py --current /absolute/path/previous-preview.json --revised /absolute/path/all-96.json --background /absolute/path/filtered-frame.png --html /workspace/autumn-leaf-preview-all-96.html --video /absolute/path/preview.mp4
```

The recovery archive contains the exact baseline-relative patch, full changed
files, verification evidence, preview inputs and the superseded checkpoint's
source record. The prior recovery ZIP is preserved separately. Source recovery
does not require a full game build; review the motion before building.
