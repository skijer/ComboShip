# Fairy bottle receipt routing — 2026-10-09

Baseline: consolidated local candidate
`421a7f1a85157eb24a80e317f5f6496364f33616`, on the existing October 9
audit-bridge review. Cor reports a fairy receipt displayed as a vanilla empty
bottle despite an earlier preview showing the fairy inside the authored TP
bottle. That preview establishes the intended appearance; it does not identify
the executable, receipt route or loaded pack configuration of the failure.

## Disconnect and correction

The existing fairy drawer and private TP/Peyron casing are already present.
Three OoT receipt recipes still selected `OBJECT_GI_BOTTLE / GID_BOTTLE`:
randomizer `RG_BOTTLE_WITH_FAIRY`, shop `RG_BUY_FAIRYS_SPIRIT`, and native
`GI_FAIRY` in `VanillaItemTable_Init`. Consequently the native drawer and the
cross-game export could select a two-pass empty bottle without fairy contents.
Correct all three to `OBJECT_GI_SOUL / GID_FAIRY`, matching the native fairy shop
shelf's object and drawing recipe. Keep item IDs, get-item IDs, grant behavior,
refill behavior, text, categories and price unchanged.

The corrected recipe reaches `GetItem_FairyBottleShell`, whose bundled paths
are `objects/combo_bottle_gi/EmptyOpaque` and `EmptyXlu`. The latter uses the
unchanged authored `BottleShell` dependency graph. The recipe retains the fairy
contents and reaches the existing animated fairy renderer in native OoT and
the MM foreign drawer. Its selection still respects explicit replacement
packs, and foreign descriptions retain their live owner-appearance refresh.
The independent empty-bottle correction cannot repair an item that requests
the wrong drawing kind; both corrections belong in the consolidated candidate.

## Evidence and limits

`tests/fairy_bottle/run_receipt_tests.py` compiles production catalog rows and
the Item constructor, the real vanilla table initialization/item manager, and
both native draw/export tables. All three OoT cases fail against the previous
recipes with a two-resource simple bottle, then pass with the complete fairy
recipe, private TP casing paths and unchanged receipt identity. The existing
MM native and imported fairy recipes pass unchanged.

The new receipt checks run from the existing fairy diagnostic, so they are
included in the normal regression batch. Separate both-host GPU-boundary
checks execute the native and foreign fairy production drawing bodies,
including the TP casing, animated wings/glow and motes. Fairy, empty/potion,
bottle contents and bottle-polish suites pass, including real engine header
compilation and the protected casing fingerprints. Independent review is
recorded with the consolidated candidate.

No geometry, textures, renderer or grant code changes are required. Actual
appearance in Cor's game remains to be confirmed with the matching build:
test ordinary bottled-fairy, shop refill/fanfare, native and cross-game receipts,
and owner Alt Assets off/on with the installed replacement packs recorded.
