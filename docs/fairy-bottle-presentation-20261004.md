# Bottled fairy presentation candidate

Base: PR34 `122dd5f68cb37fcf515c3726f8a77043db5dcdf4`.

The MM native bottle previously exported only its two shell resources to OoT.
The foreign renderer therefore had no fairy contents or placement matrix to draw.
The new `CW_DRAW_KIND_MM_FAIRY_CONTAINER` recipe carries both, alongside the
existing animated material. The contents matrix loads through MM's resource
manager; drawing resumes after that resource-manager scope exits. Missing
foreign matrix/material resources fall back to the host's complete fairy GI.
The user's subsequent clarification selects the TP Blue Fire bottle mesh and
an actual bouncing fairy VFX. The completed route implements that request below.

`ComboFairyBottle.h` supplies the same motion and shell-selection policy to
native OoT, MM's legacy OoT-style bottle, native MM, and both foreign directions.
Only the contents move. The active host's `gameplayFrames` drives a periodic
360-frame path bounded by ±0.65 X, ±1.15 Y, ±0.35 Z local GI units. Contents
scale is 0.74–0.82 in X and 0.78 in Y/Z. `ComboFairyBottleDraw.h` draws the host's
normal fairy skeleton and its real wing animation at that moving pose, using a
GI-local scale of 0.004 and the normal actor glow-to-wing ratio. No EnElf actor
is spawned, no companion/Midna resource is changed, and no shared effect pool
or RNG is advanced. The active host frame selects the animation frame directly.
Pink VFX prim/env material colors are `#FFA0EB`, matching the single pink hex
shimmer. MM native fairy rows now export that hex color; the imported OoT fairy
wrapper skips its former extra overlay so it does not draw the same hex twice.

Missing/incompatible host fairy skeleton or animation falls back to the original
moving native contents. MM's placement matrix remains the native-shell anchor;
a selected Blue Fire shell uses its existing contents anchor (-8, -2, 0), and
its blue flame never draws. The selected shell's incoming transform is retained.
No internal model transforms are guessed or altered. The CPU matrix
is restored after drawing; foreign animated material segments are also restored.
The submission recipe retains OPA shell, XLU glass, then XLU contents.

These bounds are verified transforms. No extracted native archives or active
TP mod archive were supplied, so they do not establish silhouette containment
inside arbitrary replacement art. No new fairy/bottle mesh or raster art was made.

## Shell priority and pack resource paths

The owning game's selected archive and Alt state determine ownership through
the existing `ResourceMgr_IsModAsset` query. Its query checks the winning archive,
not whether an arbitrary mod happens to contain a similarly named resource.

1. A selected mod on either fairy-specific shell DL keeps the entire fairy shell
   pair, including partially overridden shells.
2. Otherwise, the selected OoT-owner mod on
   `objects/object_gi_fire/gGiBlueFireChamberstickDL` supplies the complete Blue
   Fire bottle shell. MM queries the registered OoT owner and carries that routed
   path as well. The stock chamberstick cannot enter this route.
3. Otherwise, a selected mod on either generic bottle shell DL selects that pair.
4. With none selected, the original fairy shell pair is retained.

The selected Blue Fire shell repeats the same path in both ABI shell slots.
Native and foreign consumers recognize that single-piece recipe and submit it
once. No `gGiBlueFireFlameDL` is part of the fairy recipe.

No other bottles change. Texture-only mods continue through normal texture
resolution; they do not trigger a geometry switch. Foreign appearance recipes
remain live (`stateDependent = 2`) so the owning game's Alt/mod selection is
queried again instead of retaining the first shell decision.

The following are archive keys. Runtime strings add `__OTR__`; foreign commands
add `__OTR__@oot:` or `__OTR__@mm:`. Those routing prefixes are not archive-key
prefixes. Optional Alt resources use `alt/` before the same archive key.

| Owning game and shell | OPA key | XLU key |
| --- | --- | --- |
| Preferred selected TP Blue Fire shell, OoT owner in either host | `objects/object_gi_fire/gGiBlueFireChamberstickDL` | Same shell is submitted once; no blue flame |
| OoT fairy-specific | `objects/object_gi_soul/gGiFairyContainerBaseCapDL` | `objects/object_gi_soul/gGiFairyContainerGlassDL` |
| OoT generic fallback | `objects/object_gi_bottle/gGiBottleStopperDL` | `objects/object_gi_bottle/gGiBottleDL` |
| MM native fairy-specific | `objects/object_gi_bottle_04/gGiFairyBottleEmptyDL` | `objects/object_gi_bottle_04/gGiFairyBottleGlassCorkDL` |
| MM legacy fairy-specific (`GID_FAIRY_2`) | `objects/object_gi_soul/gGiFairyContainerBaseCapDL` | `objects/object_gi_soul/gGiFairyContainerGlassDL` |
| MM generic fallback | `objects/object_gi_bottle/gGiEmptyBottleCorkDL` | `objects/object_gi_bottle/gGiEmptyBottleGlassDL` |

| Resource | Archive key |
| --- | --- |
| OoT fairy contents | `objects/object_gi_soul/gGiFairyContainerContentsDL` |
| OoT fairy texture | `objects/object_gi_soul/object_gi_soulTex_000000` |
| MM legacy fairy contents | `objects/object_gi_soul/gGiFairyContainerContentsDL` |
| MM legacy fairy texture | `objects/object_gi_soul/gGiFairyContainerFairyTex` |
| MM native fairy contents | `objects/object_gi_bottle_04/gGiFairyBottleContentsDL` |
| MM native placement matrix | `objects/object_gi_bottle_04/gGiFairyBottleBillboardRotMtx` |
| MM native material animation | `objects/object_gi_bottle_04/gGiFairyBottleTexAnim` |
| MM native cork texture | `objects/object_gi_bottle_04/gGiFairyBottleCorkTex` |
| MM native fairy texture | `objects/object_gi_bottle_04/gGiFairyBottleFairyTex` |
| MM native glass texture | `objects/object_gi_bottle_04/gGiFairyBottleGlassTex` |
| Shared bottle glass texture exposed by both owners | `objects/gameplay_keep/gBottleGlassTex` |

| Actual host fairy VFX | Skeleton | Wing animation |
| --- | --- | --- |
| OoT | `objects/gameplay_keep/gFairySkel` | `objects/gameplay_keep/gFairyAnim` |
| MM | `objects/gameplay_keep/gameplay_keep_Skel_02AF58` | `objects/gameplay_keep/gameplay_keep_Anim_029140` |

The OoT skeleton references the native `gFairyWing1DL` through `gFairyWing4DL`,
`gFairyWingTex`, and `gGlowCircleTextureLoadDL`/`gGlowCircleDL`/`gGlowCircleSmallDL`
under `objects/gameplay_keep/`. MM's normal fairy uses
`gameplay_keep_DL_029990`, `gameplay_keep_DL_029A58`, `gameplay_keep_DL_029B20`,
`gameplay_keep_DL_029BE8`, `gameplay_keep_DL_029CB0`, `gameplay_keep_DL_029CF0`,
`gameplay_keep_DL_029D10`, and `gameplay_keep_Tex_029150` under that same folder.
These resources remain owned by the active host, including its normal Alt
resolution. Private `objects/midna_navi/...` models are never requested.

Generic bottle XMLs declare the two DL roots, with no separate named cork
texture. Preserve every vertex/texture/matrix dependency used by the replacement
DLs, including any private TP texture paths. Changing only a held/player bottle
resource does not replace a GI shell. MM can now select the registered OoT
owner's Blue Fire shell; a generic-bottle-only MM pack still needs MM's distinct
generic keys above. The existing
`scripts/mods/build_mm_tp_dungeon_bottles.py` covers Poe/map/compass assets and
does not establish fairy-pack compatibility.

## Verification and remaining runtime check

Commands run from the checkout root:

```sh
source /workspace/scratch/480e6acc0018/test-env.sh
python3 -B scripts/diagnostics/run_fairy_bottle_tests.py
python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo
python3 -B scripts/diagnostics/run_mm_item_visuals_tests.py
python3 -B scripts/diagnostics/run_mask_shimmer_tests.py
git diff --check
```

All passed. The new regression executes unchanged production export/draw bodies
with instrumented resource, matrix and GPU-command boundaries. It checks all 32
selected-shell ownership combinations, the full periodic transform envelope,
time variation, unmodified shell transforms, OPA/XLU order, native MM placement,
foreign owner scope exit, material/matrix restoration, complete-recipe capacity
guards and six missing-resource fallback conditions. Both compiled host variants
also execute the new fairy helper, confirm real skeleton/animation calls and
changing frames, pink VFX material, a single selected TP shell, no blue flame,
and native-contents fallback. The hex regression executes both native and
imported MM fairy rows, requiring exactly one 360-vertex pink overlay.
Before the fix, the MM
export assertion failed because it returned two resources rather than four.
The existing gates also compile the real-header native `z_draw.c` translation
units for both games; the Combo gate exercises the new dispatch kind.
The focused fairy runner also compiles the new shared effect helper against
both games' actual C++ headers, including callback types and resource APIs.
Its integrated composition case executes the actual outer foreign dispatcher,
native `GetItem_Draw`, native shimmer selector and native fairy draw. Every
foreign owner/matrix/material failure emits exactly one pink overlay, whether
or not the foreign recipe requested an overlay. The complete foreign recipe
also emits one. This case failed before the review fix because the native
fallback and outer dispatcher both drew shimmer. The fairy helper now reports
that the fallback owns its overlay, and the caller restores its matrix and exits.

Runtime follow-up: with the user's exact archive order and Alt setting, inspect
native OoT, native MM, MM's imported OoT bottle and both foreign routes in world,
shop and get-item views at several frames/camera angles. Confirm the selected
TP silhouette, cork/glass texture dependencies, translucent composition and
contents containment. That configuration-specific visual proof remains pending.
