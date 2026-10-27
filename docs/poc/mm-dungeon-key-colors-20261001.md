# MM dungeon key models and independent palettes

Baseline: PR31 head `b715c82e9227f033faaaf91415b0af5be23eb44d`. This candidate retains its item-grant, shop audio, mask/remains, transformation and replacement-skeleton fixes.

The eight key models restore the original SoH body/teeth and retain the approved Woodfall blossom, Snowhead snowflake, Great Bay fish and Stone Tower ornament/red jewel. The asset archive contains only private Alt resources under `alt/objects/cor_mm_keys_poc2/`; the established TP map/compass archive is unchanged.

## Behavior

Four independent emblem rows immediately follow their owning dungeon rows in **MM Cosmetic Editor → Link & Items → Dungeon Items**. The existing dungeon-color checkbox controls body tint. Custom key bodies keep neutral steel/gold until their dungeon row is edited; the ornament metal uses that same body channel. Emblems use their own rows, including Stone Tower's red jewel.

Palette resolution is independent of replacement-model selection. With the key pack absent, partial, or Alt disabled, all four dungeons' vanilla small keys, boss keys, maps and compasses still use the dungeon picker when body colors are enabled. The same palette-only helpers feed native MM and MM items presented in OoT. Maps retain the established lighter tint; compass glass keeps its original untinted XLU material and setup. Editing an emblem row also tints the vanilla boss key's separate gem, even with the body checkbox off; resetting the row restores the native gem. A vanilla small key has no separate crest geometry.

The owning MM dungeon travels with the randomizer item, regardless of its location in either game. Unknown owners remain untouched. Custom keys use their own optional model selector and retain the quarter-scale required by their integer vertex grid. Tab disables the custom mesh while edited vanilla colors remain available.

OoT re-resolves dungeon-item appearance after color/Alt changes, including after the grant-time latch. Progressive-tier latching remains unchanged. No shared ABI layout, grants, save schema, progression, bank, audio, actor or scene behavior is changed.

## Verification

- The real-header native item-visual suite passes all existing owner/tint, scene fallback, missing-resource and bottle-shimmer cases, plus vanilla boss-gem edits. The full modified `z_draw.c` passes real-header C syntax compilation.
- `run_mm_dungeon_key_tests.py` passes normally and under ASan/UBSan. It compiles production palette/model/native draw functions and the actual MM export, OoT resolver/cache/latch and OPS replay with controlled resource and graphics services. All 16 owner/GI combinations are checked with Alt off, absent/partial key packs, body colors off/on, independent emblem edits and state cleanup. It also verifies progressive tiers, save-slot/generation cache sweeps and routed-path lifetime.
- Production group rendering with inert UI services confirms all four emblem rows are adjacent to the matching dungeon rows. Exported GLBs and O2R vertices agree; the eight donor bodies retain the original geometry/normals; emblem resources remain byte-identical to the approved fish candidate. Archive dependency/type/index/triangle checks and CRCs pass.
- Changed source formatting and `git diff --check` pass. CI includes the native and cross-host tests.

A full application build is unavailable locally (`cmake` is not installed). CI builds and real in-game MM/OoT appearance remain pending. These are offline checks, not runtime acceptance or master promotion.

## Installation and runtime acceptance

Use `zzzz_MM_Dungeon_Keys_POC3.o2r` in `mods/2ship/` with a build containing this candidate. Replace previous POC1/POC2 key packs and enable Alt for the custom meshes. Keep the established TP map/compass pack. The palette functionality works without either asset pack.

Test each family in MM and at an OoT foreign check: edit body and emblem colors independently, obtain the item, change colors again, toggle Alt and test once with the custom key pack removed. Confirm native boss gems, clear compass glass, correct owning dungeon, native/custom scale and an unchanged progressive pickup. Asset SHA-256: `2b4948ff7d784dbce5943f264ed8049b1e7e2a57b8744071fcf30b39c06b83c5`.

The original POC2/fish archives remain the rollback. No merge or master promotion is part of this candidate.
