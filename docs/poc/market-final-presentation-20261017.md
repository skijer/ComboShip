# Market presentation and forest continuation

This continues cor's approved shop/forest design on the isolated
`poc/black-market-cross-lock-20261017` candidate. The recovered visual source
`01acb9e8` and verified forest source `9c063dcd` are combined at `3128176d`.
Both original branches and the pre-continuation standalone recovery archive
remain recoverable. No master, GitHub publication or full build is involved.

## Preservation and intended result

Preserve the complete 79-entry catalog, deterministic eight-offer selection,
native exact grants and transactions, boss-clear gates, title cards, private
forest access/returns, weather and save recovery. Keep POC9 and POC11 unchanged.
Room geometry/collision/materials and keeper model/animation bytes are protected.

Replace the offset rings with two thin, scrolling translucent ribbons forming
a wrapping X around each locked item's actual rendered world bounds. Native
GI selection, fit, rotation, bob and selected scale determine those bounds.
Unlocked selected items retain their scale highlight without a seal. The keeper
uses her existing crossed-arm forward lean above the counter, with her feet
behind it and above the actual floor. Updated previews must decode those exact
resources and shared native X geometry; offline previews are not game captures.

## Milestones

- The source merge is clean and saved in
  `ComboShip-Work-Recovery-20261018T001719Z.zip`.
- Recovered capture and X mesh checks pass before controller changes. All 13
  retained legacy fallback branches report the selected model after its final
  transforms; unlocked captures perform no resource queries.
- Native controller fixture first fails because the old ring API cannot consume
  a model frame. Both controllers now capture only locked offers and consume
  that frame directly. Their actual draw bodies pass ASan/UBSan with real host
  vertices and GBI packets: exact 64-vertex/16-quad submission in two native
  batches, IA8 texture identity, modulation, texture-LUT reset, explicit
  translucent blending/depth testing without depth writes, and restored state.
  Invalid/missing bounds or allocation failure submit no stale geometry.
- Nine native frame/resource/legacy units compile against real dependency
  headers. Per-unit commands/hashes and output are retained with the checkpoint.
- Keeper source milestone `35dd5418` identifies the actual native Sway resource
  as the crossed-arm lean, places its carrier at `(0,1,130)`, and preserves
  all model/animation bytes. POC12 changes only the MM carrier placement and
  manifest; the other 190 resource payloads match POC11 byte-for-byte.
- Both actual native initializers fail on the former upright idle then pass on
  the shared lean; the native OoT spawn fixture fails on the old position then
  passes on the common placement. Native full controller objects compile.
- The updated preview calls the same C++ X mesh/IA8 texture and fits it around
  the selected sample model's actual posed bounds. Preview adapter and packed
  keeper/blink/stock checks pass. Preview generation now guards source and pack
  identities across the complete render. Final refreshed media passes those
  guards: 7.2 seconds / 72 frames at 1280x720, plus room/keeper/locked/unlocked
  stills. The central room, keeper and locked merchandise stills were inspected.
- The cumulative solo gate (three compiled probes and 18 production-boundary
  suites) and combined Lost Woods regression gate pass after native changes.
- All 14 additional catalog/pricing/transaction/renderer/resource-query/legacy
  and effect suites pass. Previously blocked real-header receipt-query and
  native sword-pose phases now pass with the restored dependency headers.
- Fresh whole-change review at `36bea817` reports no Critical, Important or
  actionable Minor findings. It independently reruns the native capture,
  13 legacy fallback cases and both native X draw fixtures under sanitizers,
  and confirms the exact two-resource archive delta. Its report is retained
  with the recovery checkpoint. Final documentation changes do not alter the
  reviewed production source or the recorded preview source identities.

## Final source/asset candidate

POC12 is `zzzz_Skiejer_Black_Market_POC12_KeeperLean.o2r`, SHA256
`761fa811149f860b6a174901bdda1ad7fdf5610a0064f0e3efe5927853bb1352`.
It supersedes POC11 for this candidate; keep the unchanged POC11 only for
reproduction/rollback. Both solo modes require POC12 plus the separately saved
Native Solo Forest POC2 in their active native engine; combined uses POC12
plus the accepted POC9 forest in MM. Do not enable two market versions.

The complete source checkpoint contains the combined shop/forest branch,
retained forest-only control, original milestone history/patches, an independent
standalone source bundle, both market packs, asset identities and all verification
logs. It must pass restoration into an empty repository before delivery.

No intended source implementation item remains open in this continuation.
Full executable linking, configured gameplay, normal/Alt runtime parity,
visual acceptance and promotion remain unverified. The next decisive checks
are native round trips through forest/shop/stump and OoT Clock return/MM Clock
destination, keeper talk/blink, purchases across all special grant categories,
boss unlocks, save/reload/void/death recovery and a combined/prologue control.
