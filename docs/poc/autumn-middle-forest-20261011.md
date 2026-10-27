# Termina middle trees, grass and autumn/winter tint

The middle tree wall was never the forest draw previously watched by the
autumn diagnostics. The user's Prelude selection identifies 32 faces in
`Z2_00KEIKOKU_room_00DL_003928`. Its primary RGBA color image was absent from
the material table, although its secondary IA intensity/alpha image was present.
The renderer tracks the primary render tile, so selecting only the secondary
image correctly left this draw untinted. The same omission affected the 26-face
backwall grass strip. This candidate fixes those primary identities and retains
the previously working canopy/ground selection and Alt fallback. Winter uses
the same verified foliage identities, with a softer cool treatment that keeps
some original green instead of whitening the textures.

| Surface | Primary color image | Secondary image | Native draw | Triangles |
| --- | --- | --- | --- | --- |
| Middle trees | `misc/scene_texture_08/scene_texture_08_Tex_005800`, RGBA16, 32x32 | `scene_texture_08_Tex_005000`, IA4, 64x64 | `003928` | 32 |
| Backwall grass | `misc/scene_texture_08/scene_texture_08_Tex_007800`, RGBA16, 32x32 | `scene_texture_08_Tex_006800`, IA4, 64x64 | `027238` | 26 |
| Distant forest / upper canopy | Termina `Tex_034098` / `Tex_037098` | Existing palette/detail loads | `0241B0` | 14 + 8 |

Both native image hashes and explicit `alt/` hashes are selected. Native and
generated `_scene` display-list namespaces remain supported. The actual native
image, tile, combine, alpha, vertex and triangle commands stay unchanged.
The frozen reference corrects the two formerly negative draws to positive;
all source resource hashes remain unchanged. The full reference contains 269
positive and 269 negative materials.

## Brightness and separation

The prior full-strength grayscale palettes multiplied an already shaded texture
by a dark color. Copper `B96848` reduced mean RGB intensity to approximately
47% and amber `D99C45` to 61%. A lighter hex color alone still multiplies those
shadows. This candidate adds opt-in grayscale command mode 2: its submitted
float RGB attributes are normalized to a mean of one. The existing OpenGL,
Direct3D and Metal shaders already multiply mean texel RGB by those attributes.
No shader template, backend layout or alpha treatment changes.

Mode 1 retains the exact legacy arithmetic. Mode 0 disables tint and clears the
normalized state; a later mode 1 command also clears it. Dispatching a mode
change invalidates prepared/packed vertices. Black tint remains finite and black.
Scene autumn and winter variants use mode 2; other grayscale users retain mode 1.

| Termina surface | Palette identity | Intended separation |
| --- | --- | --- |
| Middle tree wall | `FF9F91` | Soft autumn red |
| Upper canopy / distant forest | `F4B783` | Warm copper |
| Ground / backwall / stump / arch grass | `FFD078` | Lighter amber |

Normalization preserves arithmetic mean RGB before output clipping, not exact
perceived luminance or the source chroma. The original texture's lighting and
intensity variation remain. Very bright output channels can still clip. These
colors deliberately separate the middle wall from the upper canopy; their final
appearance and detail separation still require the user's game configuration.
Other scene material identities retain their existing palette hues while also
receiving the brightness-preserving scene treatment.

## Winter and grass coverage

Winter uses the existing `MMWeather_SeasonForPlay` eligibility and the same
material selection as autumn. No season controls, gameplay weather, story,
underwater or interior eligibility rules change. Registered grass in Termina
Field, Clock Town, the Laundry Pool and Milk Road receives the treatment,
alongside the exact backwall strip and private stump/arch grass atlas.

| Surface | Winter RGB | Blend amount |
| --- | --- | --- |
| Middle tree wall | `C4D2C9` | 176/255 |
| Canopy / distant forest | `B9CDD9` | 176/255 |
| Ground / backwall / stump / arch grass | `BED2C2` | 176/255 |

The blend is approximately 69% normalized cool tint and 31% original texture
color. The palette command's `B0` alpha is the shader interpolation amount;
it does not replace the texture's alpha. Mean RGB remains preserved before
clipping, while some source chroma and the original intensity variation remain.
Upper and middle tree layers use separate winter palettes as well as separate
autumn palettes. The intended result is muted frosty green grass, a cool canopy
and a slightly greener middle layer; actual presentation still needs in-game
acceptance under the user's textures, lighting and renderer.

Season changes restore originals before constructing or selecting the next
season's variant. Autumn and winter display lists have separate immutable
buffers and a common original snapshot. Validated renderer-pointer observations
are shared by source so the inactive season follows a later resource reload.
A season switch never overwrites a buffer that may still be queued.
Off, spring, summer, excluded scenes and ineligible play restore originals.

## MM season selector

Season icons are inset into their boxes and centered on the visible RGBA artwork,
excluding transparent margins and faint outer glow. The one-time measurement on
opening reads the same explicit `__OTR__@oot:` owner as the renderer. Invalid or
missing owners never borrow same-named MM artwork; unsupported textures retain
the ordinary canvas center. A small beveled X replaces the reset slot's power
icon. The hints underneath use the existing shaded/antialiased IA8 title arrows.
The style applies to the MM season selector; other item menus retain their layout.

An accepted change, including Off/reset, starts one soft eight-frame pulse using
the native screen-flash renderer. Its peak alpha is 128. Reconfirming the same
season, a rejected selection and B cancellation produce no pulse. Play/scene/room
changes, frame rewind, pause, cutscenes and either active transition field discard
the effect. Native weather state, global lightning state and the season's eligibility
remain under their existing owners. Visual UI acceptance still requires the game.

## Stump and wooden arch grass

The POC4 southern trunk/stump/arch assembly uses the exact mixed RGBA32 atlas
`scenes/nonmq/Z2_00KEIKOKU/mm3d_field_poc4/textures/tl_ki_02`. It contains grass
beside wood. Tinting its entire draw would also tint bark. The candidate instead
copies an already cached texture and changes only pixels whose green channel
exceeds both red and blue. Non-green bytes, every alpha byte, dimensions, flags,
scales and resource initialization data remain unchanged. The grass conversion
uses the same mean-preserving amber in autumn and the partial frosty-green blend
in winter, with byte rounding/highlight clipping.

The copy owns its pixel buffer and has a new GPU texture-cache address. Cache
bindings stay in the MM resource manager. Alt-off, season-off, scene exit and
reset restore an original only while that exact cache entry still contains this
candidate's copy; outside edits win. Copies are keyed by source texture and
season palette. Cold assets load normally and are inspected on a later update.
Originals and copies remain alive until renderer teardown to avoid queued-pointer
reuse. The checkpoint image and archive are not rewritten.

## Evidence and verification

- Native archive SHA256:
  `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
- Exact atlas RGBA SHA256:
  `cd21e5fec64adfa7c0dc2b3f482e3f66bd39cadb0453627a234ab0493c7899e9`.
- The original middle draw failed the new test with selected grass/forest still
  untinted. The backwall draw reproduced the same failure. The original mixed
  atlas test failed because it still resolved the green source pixels.
- The brightness test first failed because normalized vertex attributes still
  reduced mean texture intensity. It passes with the opt-in renderer mode.
- Before winter support, the new winter renderer test failed because the exact
  middle-tree draw still received no tint. It passes with the winter variant.
- Independent review reproduced four VTX-first failures and four existing-cache
  failures across both season directions and whole/material variants after a
  rendered Off frame. The common snapshot and shared validated observations fix
  both regressions. No-draw Off controls and intentional vertex-offset/pointer
  edits remain covered; outside changes still win.
- All 538 frozen native materials pass in both autumn and winter, normally and
  under ASan/UBSan (1,076 material/season cases before namespace/Alt variations), including
  exact 32/26-face fixtures, both texture layers, native/Alt identities, both
  namespaces, season-off frames, cache reloads and actual renderer writebacks.
- Autumn/winter transitions retain distinct cached display lists and texture
  copies, reuse the correct season's buffers, and restore original cache bindings
  for Off, spring, summer, Alt-off and ineligible play. Ground grass in Termina,
  Clock Town, the Laundry Pool and Milk Road also passes actual renderer tests
  in each season.
- Real `Fast::Texture` ownership tests pass normally and under ASan/UBSan with
  the exact 1024x2048 atlas: all non-green pixels and alpha bytes remain exact;
  unclipped grass preserves mean RGB within byte rounding. Four fixed grass
  samples per season and two wood controls verify the checkpoint's separate regions.
- Actual triangle submission passes normally and under ASan/UBSan: normalized
  tint, legacy mode, mode barriers, black guard and 16,384 render-state variants.
  All 16 legacy output digests exactly match the parent renderer.
- The real-texture icon-center fixture first reproduced the missing owner route.
  Distinct MM/OoT managers now verify native/HD visible bounds, same-name isolation,
  missing-owner rejection, active-manager preservation and malformed-buffer guards.
- Native MM selector C syntax and the actual A/B confirmation/flash fixture pass
  normally and under ASan/UBSan, including reset, unchanged/rejected selection,
  bounded alpha/duration and play/scene/room/transition cleanup.
- The first platform gate caught two missing selector declarations in the older
  Rod input fixture. Its engine boundary now records style and flash calls;
  actual Rod input passes with an accepted Off pulse and no pulse for unowned
  or repeated selection; all 16 existing season grant/equip/save/shop/particle
  fixture cases also pass. The separate complete native Rod lifecycle passes
  normally and under ASan/UBSan/bounds. The native weather fixture independently
  reproduced its missing flash declaration; its boundary now records one pulse
  per actual season change after the wheel unpauses. Native scheduling, rain,
  snow, leaf rendering and Off restoration pass normally and under the same
  sanitizers. Production behavior is unchanged by these fixture corrections;
  platform build results must correspond to the corrected candidate head.
- Texture-address and sanitizer failure-command regressions pass, including
  hash resolution, command position, HD metadata and recovery.
- The existing native tree/leaf suite passes, including actual C translation-unit
  syntax, season/eligibility restoration, separate trunks and leaf lifecycle.
- Independent review approved the seasonal fixes after the witnessed cache
  regressions were corrected. The UI owner-routing and transition-mode findings
  were also corrected and verified. Final follow-up review approved the seasonal
  and UI changes with no remaining critical, important or minor findings.
- Changed C++ ranges use clang-format 14; `git diff --check` passes.

The tests compile production functions and real texture ownership against
recording graphics/archive seams. They are not a full game launch. Local full
translation-unit syntax checking lacks the external `nlohmann/json.hpp`
dependency; the published candidate's platform Actions build supplies the actual
dependency/PCH environment. No in-game visual acceptance or sacred baseline
promotion is claimed. Use the new head's executable and its matching assets.
