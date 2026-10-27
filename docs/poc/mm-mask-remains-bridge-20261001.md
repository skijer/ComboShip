# Imported MM mask and remains presentation repair

| Field | Record |
| --- | --- |
| Baseline | PR #31 at `d196518008067debf922372ab60bc9649d440c8b`; cor reports item-grant repair runtime-passed |
| Scope | All 24 imported `RG_MM_MASK_*` plus the four `RG_MM_REMAINS_*` GI recipes and their MM-host rendering |
| Preservation | Item/save grants, icons, name messages, native gameplay, remains equip/wear/actions, existing draw-kind ABI values |
| Evidence | Supplied Romani diagnosis: correct item/name/icon with a blue-rupee GI; current producer lacks these custom recipes |
| Verdict | Production recipe/resolver/draw dispatch checks passed; actual visual result remains untested |
| Recovery | Parent stack retained; two kinds appended without changing struct layouts or existing kind values |

OoT's imported masks use `Randomizer_DrawMmMask` with a table of two native MM display lists. A foreign check in MM asks OoT to describe that presentation. Previously the bridge could not describe the custom callback, so it returned failure and the MM consumer displayed its blue-rupee sentinel. Five legacy masks also had valid OoT GIDs and overlapping object paths; merely allowing their GID fallback would select OoT's child-trade assets.

The producer now describes the entire family from the existing 24-mask table. It preserves the original OPA/XLU or two-OPA division. The MM consumer routes these recipes through `__OTR__@mm:` and clears palette lookup mode on each used stream. Romani specifically uses `gGiRomaniMaskCapDL` and `gGiRomaniMaskNoseEyeDL`. Ordinary foreign OoT items retain their `@oot:` routing.

The four remains had the same missing-custom-recipe risk. A shared pure path selector now serves both their existing native draw and exported recipe. Each recipe uses the correct `object_bsmask` single OPA display list and the established 0.02 scale, routed through MM's resource manager. Wearable remains use a separate Moon Child face-fitted mask (`object_ob`) path, equip handling and action hooks. Those files and their inventory/grant logic are byte-identical to the parent. Both OoT and MM still contain their wearable-remains implementation; this repair does not claim to have runtime-tested those abilities. Twinmold's existing OoT ally limitation remains outside this repair.

The new regression executes the production table, descriptors, effective GI boundary, foreign resolver, simple host renderer and complete draw dispatch. It covers all 28 imported items, exact paths, pass split, scale, palette-state reset, retained routed-string lifetime, invalid bounds, unavailable-provider behavior and deliberate unknown-check fallback. Supported items must never render the blue-rupee sentinel. Existing icon/cane/progressive/foreign texture/lifecycle and NEI GI tests remain in the gate.

Runtime check: obtain Romani through a foreign check in MM, inspect both layers and correct message/icon; inspect representative two-OPA masks and the five overlapping-path masks with Alt assets off/on. Preview/obtain each remains, then check the established remains equip/wear behavior. Item grants and dungeon-reward pool policy must remain unchanged. No master promotion is implied.
