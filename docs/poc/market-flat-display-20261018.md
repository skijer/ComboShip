# Flat shop presentation correction

| Field | Record |
|---|---|
| Baseline | Source `3f3e630d`; retained POC13 counter/keeper archive. The staggered display was rejected visually, not accepted as a runtime baseline. |
| Candidate | `poc/black-market-flat-display-20261018`; source-only display correction. |
| Request | Straight, regular-shop item presentation without an unexplained shelf-like depth angle. Keep the floating models. |
| Change | Both rows share Z110. Each side uses columns X35/70 (mirrored) and rows Y58/102, preserving slot order. |
| Preservation | Keeper position and crossed-arm lean, raised counter, all asset bytes, item fitting/spin/bob/selection, seals, stock/price/grant/save/unlock and forest routing. |
| Status | Geometry and both native controller object compilations pass. Offline media is separate from native gameplay/camera proof. |

The rejected layout used Z82 for lower items and Z118 for upper items. That
made the upper anchors smaller and shifted them inward in the frontal view,
giving unsupported floating displays the perspective of angled shelves.
One common depth removes that effect; larger vertical spacing keeps the
selected models and scrolling X seals separate without staggering them.

The first common-depth Z100 probe intersected a left-side room decoration
with the conservative outer-upper fit envelope. Z110 clears that geometry
for all eight offers while keeping the display flat. No room, prop or keeper
geometry was edited. POC13 stays byte-identical, SHA256
`bca46926d8474b3503653bdc243ee53ff0ab598c3db2b094827ad902a5ada4bf`.

The layout check first fails on the retained staggered source, then passes
with the shared native depth. It checks actual retained sample model/native-X
spin and bob sweeps, conditional authored FrameFit reserves, every room
triangle, all 32 keeper poses, inter-offer clearance and preview framing.
Both complete native controller objects compile with recorded dependency
hashes. Preview code reads both native tables and checks source/pack hashes.

Fresh evidence uses the `market-flat-*` paths under `verification/` in the
source checkpoint. Earlier counter/layout evidence remains historical
rollback context. The previous source checkpoint is preserved separately.

These are compiled-source positions: POC13 alone cannot update them. The
next executable build must use this source. Native browsing/talk cameras,
entry in both engines, external/Alt replacement models, procedural GI effects
and gameplay remain untested. No full build or master promotion was performed.
