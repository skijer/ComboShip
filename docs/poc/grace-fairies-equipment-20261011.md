# October 11 item, equipment, shimmer, and Wolf Link fixes

## Candidate boundary

The cumulative parent is `298fdc6c38510f17d5e614b9bdad8d9907481350`
(`bridge/audit-merge-20261008`, PR #41). The prepared worktree also retains
the scoped autumn Alt fallback from `69a09fd0a006d3df62ab7285a65023351f9bcd7b`.
The discarded forest owner diagnostic worktree is excluded.

The user authorized integration of the Grace facing and Alt Termina fixes,
then added stray fairy dialogue/counters, cleanup of the stale equipment
entries, and a Show/Hide option specifically for the Magic Cape's A action.
The same pass also includes the Shield of Ikana pickup orientation and
fuller bottle, sword, and shield receipt/tutorial text.
The user subsequently added an audit of mask/equipment shimmer palettes
and correction of the bottled fairy's casing and relative size, then reported
Wolf Link's persistent dash sound, human appearance during scene transitions,
and a white, glowing body in Clock Town.
The recovered follow-up adds Wolf paw contacts, climb exertion huffs, and
white eye catchlights. The fresh-chat bridge also repairs the reported OoT
equipment cycle skipping the additional equipment page.
This integrates into the cumulative candidate; protected develop/master
and in-game acceptance remain separate.

## Behavior

- Grace's wings follow the player's existing shape yaw in both engines.
  Its glow limb retains the native fairy's independent orientation and
  animated world center. Position, pulse, size, animation, and flight
  controller behavior are preserved. Casting, transform, and revert
  particles use the existing pink fairy palette; spell-forced effects
  retain their previous colors and cadence.
- The previously unpublished Grace commit `12a59ad4` could not be recovered
  in this workspace. These narrow changes reconstruct its reported behavior;
  they are not presented as a cherry-pick of that missing commit.
- Randomizer fairy receipts name the shuffled fairy's dungeon and show the
  updated count, using the same pre-grant receipt boundary as skull tokens.
  The four temple counters are independent of the pickup scene. The
  Clock Town actor's special dialogue and native fifteenth-fairy completion
  logic are unchanged, as are receipt-skip settings.
- OoT's existing guarded grant recorder advances the four legacy fairy
  mirrors after a real grant. This lets consecutive receipts advance on
  older MM-first saves while leaving the existing FC pending/applied
  increments intact. Receipt construction remains read-only; the FC store
  is not rebased to older native counts, which would duplicate fairies in MM.
- Dungeon highlights use standard engine message color codes: Woodfall
  pink (`%p`), Snowhead green (`%g`), Great Bay blue (`%b`), Stone Tower yellow
  (`%y`). The existing MM fairy icon identities `0xF1`–`0xF4` are preserved.
  These are not arbitrary RGB overrides from the Cosmetic Editor.
- Both hosts' two lower grey equipment entries were unconditional reserved armor
  placeholders without item identities. They are now empty and cannot
  receive the cursor. The actual Cape and Pendant icon/name resources
  already match their identities; Pendant ownership and moveset behavior
  are retained.
  The real strength and owned scale displays remain on the quest pages in
  both hosts, using the current upgrade levels; no upgrade ownership or
  display is removed.
- A on the owned Magic Cape opens the existing Show/Hide Cape choice with
  the current appearance selected. Hiding changes cloth appearance only;
  ownership and magic savings remain. B/Start cancel through the pause
  description lifecycle without changing equipment.
- OoT's equipment cycle always visits the additional equipment page before
  the optional crossover page, under both L/Z mappings. The legacy gameplay
  enable flag no longer hides navigation. Browsing grants or equips nothing;
  selecting valid owned gear enables its behavior after the ownership, age,
  retired-item, Pikachu, and Trident guards. A valid saved loadout restores
  that enable flag on initialization, matching MM. Explicit gameplay disabling
  still clears active equipment and performs its existing cleanup.
- OoT Fish, Bugs, Poe, Big Poe, Kokiri Sword, Giant's Knife, Biggoron's
  Sword, and the three vanilla shields select their authentic get-item
  message-bank IDs. Localized native bodies retain their encoded page,
  button, color, icon, and textbox data. This covers native OoT pickups and
  donor receipts delivered in MM. The Master Sword has no ordinary native
  get-item receipt in the table; it receives an expanded localized
  equip-and-use lesson. A complete lesson also covers MM-first pickups
  before the OoT banks have loaded. Existing enhanced weapon messages
  retain their specific registry text.
- MM's own vanilla sword and shield rows prefer their native tutorial
  receipts. The imported Shield of Ikana keeps its item identity.
- The Ikana pose reader now measures the selected resource graph's width
  as well as height and depth. After the existing flat-model correction,
  a mesh more than 5% wider than tall receives a clockwise quarter-turn.
  Both native pickup drawers and donor descriptors apply roll before
  X tilt; the OoT donor descriptor follows live appearance changes.
  Already upright and near-square meshes retain their current pose.
  Equipped models and shield materials are not edited.
- Fairy pickups select the same casing as the implemented empty bottle,
  including its bundled private shell and explicit empty-bottle mods.
  The Blue Fire chamberstick is no longer promoted into a fairy casing;
  selected packs can embed fire inside that display list even when the
  separate flame pass is skipped. The actor-derived fairy's local scale
  increases by 50%, without scaling the casing, actor assets, animation,
  glow palette, or bounce envelope.
- The mask shimmer audit covers all 24 MM masks and nine OoT masks. Blast
  changes from orange to blue/white; Stone changes from grey to purple.
  The remaining mask identities retain their existing themed palettes.
  Added armor, shields, boots, Cape, Pendant, and extended weapons receive
  intrinsic item shimmer, including Four Sword's existing four-color stars
  in both hosts. Each equipment palette remains separate from its original
  geometry/effect identity, so authored bounds, fits, and energy meshes do not
  change. The donor metadata carries that separate identity through both
  dispatchers, including when an authored renderer declines and falls back.
  The winning OoT English-name Pendant catalog alias also receives the
  gold/blue overlay without changing its existing two-pass imported model.
  Ordinary inventory tools retain their existing optional effects.
- Both hosts' Wolf material now uses `TEXEL1` and `TEXEL1_ALPHA` in the second
  combiner cycle. The renderer swaps the texture selectors in that cycle:
  the previous `TEXEL0` selectors sampled a neighboring tile the Wolf material
  never loaded. The fix retains the lit body, unlit eye mask, and opaque output
  while removing the dependency on the previous actor's texture state.
- MM's Wolf dash dust uses `NA_SE_PL_ROLL_DUST - SFX_FLAG`, matching the native
  rolling code's expiring request. The startup sound does not retain a looping
  channel after the dash's movement boost ends.
- Wolf selection and visual presentation stay active during MM scene exit
  and entrance controls. Native transition/input ownership suspends Wolf
  actions, attack collision, movement audio, speed multipliers, and collider
  overrides without unloading the skin. Actual actor/scene teardown still
  releases resources; the destination restores the selected skin before
  native entrance control ends. Death, disabled mode, native transformation,
  worn masks, conflicting owners, and save/title lifecycle retain their
  selection cleanup behavior.
- MM Wolf contacts on ordinary ground, sand, concrete, dirt, grass, snow, and
  ice use the native Wolfos paw cue at the existing animation contacts, without
  Link's equipment layer. Water, magma, metal/glass, and unknown surfaces keep
  their native cues. Native climb-end voice contacts select one of three bundled
  Wolf exertion huffs. Inactive or rejected Wolf selection retains Link's audio;
  retained Wolf presentation also uses its contact cues during native entrance
  control, without restoring Wolf actions or movement overrides.
- HD Wolf eyes in both hosts retain blue irises and dark pupils,
  with small white catchlights and eye-only full-bright texels. Deterministic
  builders retain exact non-eye resource bytes and identical host assets;
  no full-bright body material is introduced. Standard model resources remain
  unchanged.

### Shimmer palette choices

Hex values below describe shimmer vertices; white centers remain white.
Two-color entries alternate their two hues rather than averaging them.

| Item | Shimmer colors |
| --- | --- |
| Blast Mask | Blue `#4090FF` / white |
| Stone Mask | Purple `#875FCD` / white |
| Divine Shield | Purple `#875FCD` / silver `#CFD9E6` |
| Sheikah Shield | Red `#DC4141` / blue `#4090FF` |
| Shield of Ikana | Lavender `#AFB4DC` / gold `#FFD45A` |
| Magic Cape | Gold `#FFD45A` / cloth crimson `#B80B0E` |
| Spirit Breastplate | Orange `#EB6E14` |
| Sages' Tunic | White `#F0F4FA` / gold `#FFD45A` |
| Champion's Tunic | Cyan `#30CAFA` |
| Pegasus Anklet | Gold `#FFD45A` / teal `#16AB94` |
| Climbing Boots | Yellow `#CFAC2E` / silver `#CFD9E6` |
| Trident and Roc's Boots | Existing gold `#FFD45A` |
| Cane of Byrna | Existing blue `#5A90FF` |
| Pendant of Memories | Gold `#FFD45A` / blue `#4090FF` |
| Iron Knuckle Axe | Silver `#CFD9E6` |
| Four Sword | Existing green/red/blue/purple |

These are authored default shimmer palettes. Pack-defined Cosmetic Editor
labels do not supply stable semantic cloth/gem bindings for every item;
arbitrary editor color inheritance is not claimed.

## Verification boundary

The complete fresh-chat candidate passes all 117 commands in
`scripts/diagnostics/run_combo_nei_regressions.sh`, including the scoped autumn
fallback. `october11-verification-20261011.json` records command results and
log hashes. Production player/Wolf/equipment syntax, asset collisions,
conflict-marker checks, clang-format 14, build metadata, and timing-output
regressions also pass. The original OoT placeholder helper fails eight new
socket checks; the fixed helper passes. Both quest-page source files remain
byte-identical to the published parent, preserving strength and scale displays.

Focused production-boundary regressions cover both hosts' native matrix
drawers and particle emissions, all four fairy counters and donor receipt
routes, consecutive old-save grants, equipment ownership/draw/cursor
navigation, Cape popup routing/selection/cancellation, localized native
bottle/weapon bank selection and cold donor fallback, actual selected
Ikana vertex/matrix graphs and donor pose/cache routes, and the existing
preview streams. Grace's existing flight/lifecycle regressions and both
native player unity translation units were checked as well. The full
OoT randomizer translation unit also passed a native-header syntax check.
The bottled-fairy suites additionally exercise repeated live Alt/editor
changes through the real MM selector, donor export, and OoT cache/latch,
and retain protected bottle geometry, pixels, and resource hashes. Shimmer
checks inspect emitted star vertices and native/authored/donor/fallback
routes while retaining geometry and effects-toggle boundaries.
The OoT access fixture executes the production cycle, ownership/equip,
loadout initialization, and disable cleanup across both button mappings,
legacy enable states, crossover states, and passive ownership/empty sockets:
111 checks pass normally and
under ASan/UBSan. Wolf host/audio checks include surface contacts and native
climb voice dispatch. Eye checks require identical host palettes, 268
full-bright texels and 11 white catchlight texels per eye, preserved pupils,
and exact protected non-eye bytes.

Normal and ASan/UBSan runs are recorded in the integration review. Local
sanitizer runs disable leak detection because process inspection is blocked
in this environment. Whole-module build and canonical regression results
are tracked by the PR #41 Actions run for the published head.
Wolf's complete MM host/audio fixtures pass their native UBSan runs; the core
also passes float-cast-overflow and optimized fast-math modes. Both hosts'
material regression passes normal and ASan/UBSan, and the actual bundled
standard/HD model loaders and skin/draw boundaries pass their existing
UBSan and fast-math modes. Both Wolf translation units and the MM player
unity unit pass native-header syntax checks.

No game was launched here. Runtime proof still requires the matching
ComboShip executable and game archives with the user's Alt Termina pack:
Grace facing and pink transitions, correctly counted fairy text in each
game, the cleaned equipment page, Cape Show/Hide/cancel, and the restored
bottle/weapon receipt pages. The autumn
acceptance case remains the Termina Field wallpaper and canopy under an
active Autumn season, followed by season reset/reactivation. See
`autumn-alt-fallback-20261011.md` for its exact asset and archive scope.
Also validate the requested shimmer palettes, the enlarged bottled fairy,
the OoT additional equipment cycle and saved-loadout behavior, Wolf dash sound
expiry, paw contacts, climb huffs, HD eye catchlights and standard asset preservation,
and Wolf appearance through an exit/arrival sequence.

The active Fados/Crescent Ikana replacement from the screenshot was not
available for geometry inspection. The provided older material archive
and repository-authored shield were measured, including derived sideways
and arm-local variants. This verifies the source correction, not that
the screenshot's specific pack meets the span classifier. Near-round
geometry or face artwork rotated within an upright rim still requires
that pack's in-game visual check.

The Wolf texture-slot fault was reproduced through the actual emitted GBI
packets, the production shader generator, and the DirectX shader-input helper
in both hosts. The previous material math fixture supplied the same texture
to both cycles and missed this dependency. Normal and ASan/UBSan checks now
require only the owned Wolf texture slot and retain body/eye/opacity coverage.
This is source evidence consistent with the white silhouette; the exact
screenshot's GPU appearance still requires in-game acceptance.
