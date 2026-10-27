# Autumn Alt foliage and leaf motion candidate

This records the initial source candidate. Its leaf layout is superseded by
`mm-autumn-visible-budget-20261008.md`; the Alt foliage changes are retained.

Baseline: `a82852a3a02c8cabc541cce7c124da71c5755efb` (develop / merged PR #38).
Candidate branch: `poc/autumn-alt-density-20261008`.
Status: source and focused harness verification complete; no full engine build,
no packaged binary, no in-game acceptance, and no remote branch push.

The user reported green Alt foliage, sparse slow leaves, and no apparent leaves
in Clock Town. Their annotated field screenshots and 30-second gameplay capture
are the evidence. Intermittent rain is accepted and remains unchanged.

## Leaf behavior

Keep the existing allocation: 16 near, 32 middle, 48 distant leaves (96 total;
the native 128-slot actor allocation is unchanged). Place middle and distant
particles across the view instead of spending their budget behind the camera.
Their positions remain in world space. Distant particles recycle outside the
view after their fade-in interval; camera movement does not translate origins.

Increase fall, flutter and wind drift. Keep the existing art, billboard size,
rotation, alpha, season eligibility, fade-in and 150–350-unit camera/Link clear
pocket. Compact the middle/distant radii in all five outdoor Clock Town areas
to 30% of field radii, so they can occupy streets inside the city walls.

The perspective regression reproduces the baseline's 22 visible distant leaves
out of 80. The revision initially shows 65 with the same 4:3 test view and remains
above 40 through 600 updates (30 seconds) in the field and every outdoor district.
These counts do not model geometry occlusion or prove the on-device appearance.

## Reversible foliage coverage

| Marked foliage | Selected assets treated |
| --- | --- |
| Field ground | `01BE50`, `01C650`, `01DE50`, `02C050` texture identities, native and Alt |
| Forest wallpaper | `034098`, `037098` texture identities, native and Alt |
| Raised grass beds | Existing clover identities plus the six accepted POC3 logical roots below |
| Observatory/tree canopies | En_Wood02 and Obj_Tree selected canopy draws, including Alt models |
| Cuttable grass | En_Kusa bush/sprout and Obj_Grass opaque/translucent draws, including replacements |

Bed top roots: `0153A8`, `015DF0`, `016598`; side roots: `0158E8`, `016180`,
`016A38`. The accepted POC3 routing checkpoint identifies these roots as owners
of private child materials. When no recognized texture is exposed in a root,
execute its intact copied commands as a pushed child under the seasonal color,
then restore grayscale state. Private names, child calls and material bindings
are retained. The actual POC3 archive was not available in this workspace;
nested private-material behavior is covered by a faithful command fixture.

No texture bytes, UVs, geometry, alpha, culling, collision, sway, or replacement
ownership are rewritten. Off/ineligible draws retain native commands. Native
impact leaf ownership and the existing ambient tree-leaf cap are preserved.

## Verification

- Native seasonal-weather harness: AddressSanitizer/UBSan/bounds, perspective
  density, sustained town density, visible fall, bounded draws, origin stability,
  seasonal lifecycle, gameplay RNG isolation, native snow/fog and Off restoration.
- Scene-material harness: sanitizer coverage, six real serialized bed materials,
  eight real ground/forest materials, native/Alt hash identities, nested private
  Alt materials, resource reload/toggle, exact command restoration and teardown.
- Canopy harness: selected Alt canopies, trunk preservation, native impact leaves,
  ambient lifecycle and cap.
- Grass harness: sanitizer coverage plus full real-header actor syntax; opaque
  and translucent tints, alpha, hooks, stumps and exact Off command preservation.
- Full production Object_Kankyo and AutumnSceneFoliage real-header syntax checks.
- Weather suite: state/mixer, wet/dry cycle, sky/audio agreement, native weather,
  town/interior/story boundaries, seasons and Off restoration.
- Preview: production draw transforms exported under sanitizer; 12-second
  1280×720/30fps MP4 inspected; comparison, motion and pause controls checked at
  their DOM/canvas boundary. A browser executable was unavailable for UI QA.

Leak detection is disabled for the sanitizer runs because this managed executor
cannot inspect `/proc` threads. Address, undefined-behavior and bounds checks
remain active. Existing controller-header macro warnings remain.

Re-run the focused checks from the repository root with its normal dependency
headers available:

```sh
python3 tests/seasons/run_native_weather_tests.py --sanitize
python3 scripts/diagnostics/run_mm_autumn_foliage_tests.py
ASAN_OPTIONS=detect_leaks=0 MM_FOLIAGE_TEST_CXXFLAGS='-fsanitize=address,undefined,bounds -fno-sanitize-recover=all' python3 scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py
MM_FOLIAGE_TEST_CFLAGS='-fsanitize=address,undefined,bounds -fno-sanitize-recover=all' python3 scripts/diagnostics/run_mm_autumn_grass_tests.py
python3 scripts/diagnostics/run_mm_weather_tests.py
```

## Preview and recovery

The preview replays matrices, palette, alpha and interpolation identities from
the actual production particle functions. It uses the existing leaf artwork and
an unmodified frame of the user's capture. It does not render revised foliage
colors, terrain depth/occlusion, fog, or a moving game camera. It is motion
direction evidence, not a GPU/performance or runtime acceptance result.

Export both committed-baseline and candidate trajectories with:

```sh
python3 tests/seasons/run_native_weather_tests.py --sanitize --preview-baseline=a82852a3 --preview-output=/absolute/path/current.json
python3 tests/seasons/run_native_weather_tests.py --sanitize --preview-output=/absolute/path/revised.json
python3 tools/nei_autumn/render_motion_preview.py --current /absolute/path/current.json --revised /absolute/path/revised.json --background /absolute/path/frame.jpg --html /workspace/autumn-leaf-preview.html --video /absolute/path/preview.mp4
```

The recovery archive contains the baseline-relative patch, full changed files,
test evidence, actual material fixtures, preview trajectories and inputs. Apply
the patch to a clean checkout of the exact baseline. Review the animated fall
before spending a full build on further motion tuning. Runtime review must still
cover the user's actual Alt stack, terrain occlusion and foreground density in
the field and Clock Town, season Off restoration, and rain coexistence.
