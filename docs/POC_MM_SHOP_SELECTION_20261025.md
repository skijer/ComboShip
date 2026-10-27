# MM selected-item inspection candidate

Baseline: `1b6102a0f`; all-form regression checkpoint: `9cc922b75`.
Candidate branch: `poc/black-market-selection-close-20261025`.

Selected merchandise now travels to `(±31.04,51.66,106.24)` using the existing
straight native take/return tween. The previous target was
`(±39.25982,53,92)`. Camera eye `(0,43,160)`, native bank direction, FOV60,
resting shelf anchors, .27 stock/GI scale, counter, purchase and receipt policy
remain unchanged.

The full authored item/seal reservation nearly filled the original vertical
letterbox area. Meaningful closer travel requires its top to use the inspection
area above the cinematic strips. Only an owned item inspection suppresses the
native letterbox scissor and rectangles through the existing
`VB_DISABLE_LETTERBOX` flag. Keeper dialogue and ordinary browsing retain their
existing bars. Existing DisableBlackBars preferences remain authoritative.

## Ownership and native timing

The latch arms when native action first enters `EnOssan_SelectItem`, before the
first take update. It binds the play, keeper, actual item, offer slot, file and
seed. Its draw predicate additionally checks the exact active private camera
pointer/UID/setting and native cutscene actor ownership, running/safe shop
context and pause state. Stale or foreign ownership immediately falls back to
native letterbox policy. A changed seed/file/slot cannot re-arm the same ongoing
SelectItem action. Ordinary scene and native-shop camera behavior are unchanged.

Suppression continues through confirmation, CannotBuy and the entire return,
then one settled shelf frame keeps restored bars clear of between-frame item
interpolation. Native shelf positions remain exact. A successful purchase can
reset tween0 while the actor still occupies its selected endpoint; the final
owned shop-camera frame remains clear until native camera handoff. The receipt
flag alone does not prematurely restore bars.

Neither native letterbox current/target size nor a user CVar is changed. Native
Data4 continues to request32. The existing flag skips both
`View_ApplyLetterbox` scissor commands and `ShrinkWindow_Draw` rectangles; it does
not alter viewport/projection construction. Other hooks' true results are
preserved. This avoids a target-retraction delay or global camera change.

## Measured result

All measurements use actual native camera poses at320×240 and the unchanged .27
actor scale. Reference height means a40-unit reference mesh, not a claim that
all item models share that height.

| Measure | Original selection | Candidate selection |
| --- | ---: | ---: |
| Eye distance | 79.154 | 62.679 |
| Continuous reference height | 27.962px | 35.222px |
| Native integer reference height, every offer/form | 28px | 35px |
| Shelf reference height | 17–18px | 17–18px |
| Right-bank complete reservation screen Y | 35.635–132.230 | 6.545–136.021 |

Continuous reference size increases26%; native integer size increases25%.
Every selected reference is at least1.9 times its resting shelf height. The full
reservation remains unchanged: relative minimum
`(-12.9663391,-9.00919628,-12.7663383)`, maximum
`(12.9663391,19.8091965,12.7663383)`. The native textbox clearance remains Y138;
visible top is decoded from the real native scissor/rectangle writers, with
its existing2px guard.

## Verification and limits

- `run_shop_camera_mm_tests.py`: real EnOssan/EnGirlA, native camera/cutscene
  manager, transactions, receipt/lifecycle and all five forms × eight offers.
  Native view/scissor/rectangle writers and real VB registry prove initial
  suppression, current/target32 preservation, cancel return plus settled frame,
  immediate CannotBuy, accepted-receipt first frame, external DisableBlackBars
  CVar behavior and stale/pause/owner/context release controls.
- The same runner checks all40 actual native forward/return action paths with
  the full reservation against7,577 prepared room triangles and144 shelf
  triangles, including1/32-frame interpolation and actual visible-top changes.
  Exact camera, .27 scale and shelf return are asserted.
- `run_native_tests.py`: native A/Open, ordinary door, receipt acknowledgment,
  tutorials, exactly-once debit/save, stale ownership, slot/seed and recovery
  controls pass. The complete MM `BlackMarket.cpp` translation unit compiles.
- Intended RED: the first native SelectItem frame retained both scissor and
  bars before implementation. The stronger projection guard also rejects the
  original28px/79-unit endpoint. No test envelope was shrunk to fit the target.

This is source/native-fixture and object-compile evidence. Configured gameplay,
GPU interpolation and arbitrary external model replacements remain unproven.
No archives were changed, no gameplay image is represented, and no master was
promoted. Runtime focus: both banks/all eight selections, B cancel/reselect,
CannotBuy and accepted purchase/receipt, with the user's existing bar preference.
