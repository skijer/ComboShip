# Termina winter frost correction

| Field | Record |
| --- | --- |
| Source baseline | Cumulative PR #41 source `aa0acc3f31c47bd396620d1bcc86b5b663a4c623` |
| Candidate | `poc/termina-winter-frost-20261011`, isolated from the active Ikana work |
| Runtime evidence | cor accepts the autumn forest screenshot and reports correctly colored winter grass in Clock Town. The inspected winter screenshot shows snow, green foreground foliage and bright green cuttable grass in Termina. Screenshot build identity is not embedded. |
| Scope | Stronger winter foliage presentation in MM `SCENE_00KEIKOKU`; existing selected materials and grass/tree actor draws |
| Preservation | Accepted autumn palettes/commands; other scenes' winter treatment, including Clock Town; trunks/bark, texture alpha, fade alpha, geometry, collision, material bindings, weather, season eligibility and reversible resource ownership |
| Recovery | Original source baseline and archives remain intact. Candidate is separate; no sacred-master promotion. |

## Confirmed causes

The original winter scene palette interpolated 69% toward cool gray-green and
retained 31% source color. Its normalized grayscale mode preserved mean RGB,
so dark HD foliage could become cooler without becoming snowy white. The
cuttable grass, bush, grouped grass and native tree-canopy draw paths separately
checked only for autumn, leaving them green in winter.

## Candidate behavior

| Termina surface | Winter RGBA | Treatment |
| --- | --- | --- |
| Ground, backwall and mixed-atlas grass | `E5EDF3FF` | Full cool-white tint, 1.8 mean-RGB lift |
| Upper canopy and distant forest | `CCDCEEFF` | Full cool-white tint, 1.8 mean-RGB lift |
| Middle tree wall | `D8E2ECFF` | Separate cool-white layer, same lift |

Opt-in `G_SETGRAYSCALE` mode 3 normalizes the palette and lifts mean RGB by 1.8.
Modes 0, 1 and 2 retain their original arithmetic. Every mode command clears
the previous treatment flags. The existing GL/D3D/Metal float attributes and
shader interfaces remain unchanged. Highlight clipping can occur; the result
is still subject to scene lighting and must be judged in game.

Only Termina selects this mode in winter. Shared textures retain the accepted
partial winter blend when used in another scene. Grass/bush OPA and faded XLU
draws receive scoped frost; cut stumps remain native. Native tree frost wraps
only canopy submission, preserving the opaque trunk and impact-leaf behavior.
Autumn continues to use its original actor commands and scene palettes.

The POC4 mixed stump/arch atlas uses a new owned texture copy: only pixels with
green greater than red and blue receive the same white tint/lift. Every other
pixel, alpha byte and metadata field remains exact. The original archive and
source pixels are unchanged. Existing per-season caches and restoration guards
are retained.

## Verification

The new tests first failed against the baseline for missing winter actor tint,
the old scene palette and absent brightness lift. After the correction:

- All 538 frozen native scene materials pass in autumn and winter, including
  positive/negative controls, native/Alt identities, both scene namespaces,
  real renderer pointer writes and Off restoration.
- The actual 1024x2048 POC4 atlas passes four fixed snow-white grass samples,
  two bark controls, every non-green/alpha byte and immutable-source ownership.
- Grass/bush/grouped-grass and native tree tests pass, preserving native draw
  commands, trunks/stumps, fade alpha, replacement routing and Clock Town.
- Actual renderer submission passes the frost lift, autumn/legacy return,
  black-color guard and mode barriers. All 16 baseline output digests remain
  exact across 16,384 render-state cases.
- AddressSanitizer and UndefinedBehaviorSanitizer pass the full scene, real-atlas,
  grass, tree and renderer suites. LeakSanitizer cannot inspect this container's
  process tree; the successful runs use `ASAN_OPTIONS=detect_leaks=0`.
- Independent read-only review found no critical, important or minor findings
  against the cumulative source baseline. Runtime appearance, active pack order,
  actual GPU submission and full application build remain outside that review.

The grass fixture has existing controller-macro redefinition warnings, also
present before the changes. These checks compile production functions and
native actor translation units; they are not a full platform/game build.
In-game candidate appearance and final visual acceptance remain untested.

## Next runtime check

Use the next cumulative executable and matching assets. At the same Termina
view, select winter and inspect ground/clumps, middle trees, canopy/backdrop
and arch grass; wood must remain brown. Cycle winter → autumn → Off and re-enter.
Confirm the previously accepted Clock Town winter grass still looks the same.
No diagnostics-only build or new logging is required for this check.

Evidence image SHA256 values:

- `image(20261011-195746).png` (autumn):
  `d19c85bcb24e1cdf4e0b33b741df0c98d5b857c9a1155cb4ac49ca27ea827149`.
- `image(20261011-200316).png` (winter baseline):
  `35ec4d7810a78f0a4d54382f403b632cc5ebdd3b0fc4a0e71f859109694dba60`.
