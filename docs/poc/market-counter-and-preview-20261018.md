# Counter and single-view preview continuation

Baseline source `e5d857e2` and Black Market POC12 remain preserved unchanged.
This bounded candidate is on `poc/black-market-counter-20261018`. cor requested
removal of the doubled shopkeeper in the item view and a raised counter under
her crossed arms, with merchandise clearance checked.

## Doubled figure diagnosis

The previous offline presentation exported three separately rendered cameras
per video frame. Two opposing merchandise views both included the same keeper;
cropping and pasting them adjacently put their partial keeper images together
at the center seam. The stock-overview PNG had the same composition problem.
This was a preview composition error. No native duplicate-actor behavior is
inferred from those images; configured gameplay remains untested.

The main video and overview now render one continuous room camera. Keeper and
merchandise detail stills remain separate; item-only detail stills explicitly
hide the keeper. The ordinary room view retains her and all eight offers.
There are no native actor, item, stock/grant or travel changes in this fix.

The new export regression first fails on the missing single-view exporter,
then passes through real FFmpeg/FFprobe: exactly one scene render per output
frame, the complete unchanged viewport/aspect, one stable camera and the actual
requested frame count. The old crops and multi-camera composition are removed.

## Counter candidate

POC13, implemented at `9394a7f1`, raises the keeper-side top from Y30 to Y41.
The left display and all books/dishes stay unchanged. Two stone seam quads
close the join, including the lower lip with its opposite native winding.
Collision matches all 7,823 drawn nondegenerate faces. Only four of the 192
resource payloads change; all keeper, scene, material and texture bytes remain
preserved. The exact recipe, archive hash and geometry audit are recorded in
[market-raised-counter-20261018.md](market-raised-counter-20261018.md).

All 32 actual Sway frames clear the raised surface. Y41 is the highest tested
integer top avoiding finger/body penetration; the hands still move above it,
so continuously flush palm contact is not claimed. All eight actual sample
offers clear it through the full spin, bob and selected-scale envelopes,
including their native X ribbons. This does not establish all 79 catalog
models, arbitrary replacement resources or Alt/runtime parity.

## Final verification and delivery

The refreshed media uses the final POC13 hash
`bca46926d8474b3503653bdc243ee53ff0ab598c3db2b094827ad902a5ada4bf`.
Source/pack guards pass across rendering; all 17 recorded source hashes still
match afterward. FFprobe confirms 72 frames, 7.2 seconds, 10 fps and the complete
1050x760 viewport. The video and both overview PNGs contain one continuous room
view. Textured keeper, merchandise and left/right stone-step details remain
separate images. The keeper/room/step stills were directly inspected.

The asset regression passes, including deterministic duplicate generation,
all 188 unchanged payloads, preserved left scenery, both cap windings and every
collision plane. Fresh independent resource, room-boundary/collision/repair/
cosmetic/OoT-scene and native keeper/stock preview checks also pass. Existing
native controllers, forest routes, grants and item presentation code are
unchanged from their recorded prior compilation and regression evidence.

The isolated source branch and verified counter milestone are saved before
final packaging. The final checkpoint must include the combined source and
forest-only control, original milestone history, an independently restored
standalone bundle, POC13 plus unchanged rollback inputs, exact identities and
verification evidence. Media is saved separately.

Full executable linking, configured native camera/actor behavior, gameplay,
normal/Alt runtime parity and user visual acceptance remain unverified. This
is an offline source/asset candidate, with no master promotion.
