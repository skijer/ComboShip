# Native MM soul flame and mask/remains shimmer

Baseline: PR #32, `c68835e93411d2f7665906a710318b442a545e42`.
Candidate branch: `poc/mm-native-oot-soul-flame-20261002`.

cor requested using 2Ship's native soul renderer after the imported OoT flame
remained absent on Volvagia's Trading Post shelf. The exact shelf's selected
resources and depth state are still unknown. This is a replacement rendering
candidate, not a claimed diagnosis of the original failure.

Trace: `DrawGoht`/`DrawGyorg`/other MM souls call `DrawEnLight`, which submits
`objects/gameplay_keep/gameplay_keep_DL_01ACF0` on XLU with native prim/env
colors, billboard rotation and a segment-8 scroll (second tile steps 2, -6).
The native list samples `gameplay_keep_Tex_01A8B0`,
`gameplay_keep_Tex_01AAB0` and `gameplay_keepVtx_01ACB0`.

The actual archive's quad is x +/-400, y -480..1440. OoT blue fire is x +/-12,
y 2..42. Thus native MM scales are oldScale.x/z * 0.03 and oldScale.y / 48;
native translation.y is oldTranslate.y + 12 * oldScale.y. At the current
OoT recipe (scale 5, y -70), both quads occupy x +/-60, y -60..140.
cor explicitly accepted the conversion after learning the raw mesh dimensions.

`DrawSoulFlame` contains the unchanged native DrawEnLight body with an explicit
PlayState parameter. Existing MM souls retain its shared random initial phase
and once-per-frame update. `DrawOotSoulFlame` calls that same renderer with
the converted transform, MM resource ownership and original per-boss color.
It restores the matrix, XLU segment 8, grayscale and neutral prim/env state.
The animated foreign path, simpler foreign skull path and native imported
OoT soul IDs all use it. OoT's own host flame and native MM souls retain
their existing rendering choices.

Preserve: the accepted TP head, loaded flex-skeleton/jaw dispatch, model
owner, transforms, masks/remains, keys/emblems, bank/grants, Great Spin,
audio, all archives and approved dramatic texture bytes. MM's native flame
samples MM texture paths, so the existing OoT-only dramatic pack is not
automatically applied to this new MM-host effect. Its archive is unchanged.

Important limit: both native flame lists share the same combiner and render
mode. Selecting MM's native renderer removes the imported effect-resource
path; it cannot by itself prove a depth/occlusion problem is resolved.

GI shimmer: the native MM table adds five animated sparkle motes to its 24
inventory masks, Sun Mask GI and all four remains. OoT's eight native mask GIs
also shimmer. The imported MM masks/remains in OoT and both cross-game recipe
consumers retain their selected models, materials and owning ResourceManager,
and add the independent overlay after restoring the incoming item matrix.
The legacy Skull/Spooky/Gerudo mask imports and Mario Mask callbacks also
receive the overlay. No archive is rewritten and no effect pool or gameplay
RNG is used for the shimmer. A missing sparkle resource skips only the effect.

| Item | Shimmer environment hex |
| --- | --- |
| Ordinary masks (including Mario and Sun) | `#DCE1F0` |
| Deku transformation | `#32DC5A` |
| Goron transformation | `#F04040` |
| Zora transformation | `#4090FF` |
| Fierce Deity | `#000000`, dim silver `#505060` highlights |
| Odolwa's Remains | `#911485` |
| Goht's Remains | `#0A8A2E` |
| Gyorg's Remains | `#1363A5` |
| Twinmold's Remains | `#A8B414` |

The remains colors match the corresponding native MM boss souls. Ordinary
mask colors remain neutral; transformation hex colors apply to the effect.

Local validation:
- Both engines' native rigid/flex drawer fixtures pass normally and under
  ASan/UBSan. The MM fixture executes the actual native flame body/wrapper,
  checks converted quad bounds, MM effect/OoT model ownership, tint, cleanup,
  repeated Alt changes and preserved jaw matrices.
- Both engines' real-header shimmer fixtures pass normally and under
  ASan/UBSan: every eligible native GI, transformation hex, black highlights,
  deterministic animation, resource fallback, unscaled remains overlay and
  matrix/color/resource-owner cleanup.
- Production OoT imported recipes -> MM resolver/dispatch covers all 24
  masks and four remains, including effect colors and missing-effect fallback,
  normally and under ASan/UBSan.
- Dungeon palette/keys/emblems, progressive latching, item/icon/cane selection,
  native MM item visuals, full native z_draw.c syntax and Great Spin checks
  retain their passing behavior. Both engines' Great Spin renderer sanitizer
  checks pass. Local sanitizer leak detection is disabled because the container
  prevents LeakSanitizer's thread inspection; no leak-proof claim is made.
- Asset collision checks pass and clang-format 14 is applied. CI runs the new
  shimmer checks in both normal and sanitizer modes.

These are source/service/command-stream checks, not scene rasterization or
full application build proof. Full CI builds and visual acceptance are tracked
on the separate draft candidate.

Decisive runtime check: same Trading Post shelf and TP pack order. Confirm
visible red flame and attached jaw, browse adjacent items, leave/re-enter,
check the two owner Alt selections independently, and check acquisition.
Check Deku/Goron/Zora/Fierce Deity, ordinary masks and all four remains in MM,
OoT imports and opposite-game checks, including shelves and held-up rewards.
Confirm the motes are readable and correctly sized with the selected custom
models. Candidate remains unaccepted in game; no master promotion.
