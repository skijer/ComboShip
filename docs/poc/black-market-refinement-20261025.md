# Black Market display and collision refinement

Baseline: local `cc11cfcdd2d157017cb7be37e61301fd8f63a8a3`, tree
`54ca0f4860ea91affc0498d8f36c8d5b03872097`, identical to public `83eed9ff`.
Candidate: `poc/mm-market-forward-20261025`. cor chose moving the shelves and
stock forward, removing the rear pots/steam, and reported counter climbing
and missing shop contents with Regular Granny Interior.

## Changes

- Both MM shelf banks, collision and all eight stock anchors move 12 units
  toward the customer. Upper anchors move down 4.5 units, shelf roofs down two,
  and middle separators become five units shallower. The fixed camera eye,
  keeper position/pose, FOV, native turns, confirmation endpoints and stock
  scale remain unchanged. Larger moves failed the full item/seal UI bounds;
  shortening the separator preserves the straight native take/return path.
- The private MM room removes exactly 342 triangles belonging to the two
  rear cauldrons, their contents and attachments. Remaining faces and flags
  are exact. The shared/OoT geometry and compressed texture parts are intact.
  MM emits vapor only from the two entrance-side pots; OoT retains all four.
  Optional magical room materials use the same corrected MM geometry.
- Granny's owned room swap retains the validated original room pointer.
  Native shop readiness accepts that exact owned replacement; foreign shapes,
  missing originals, invalid cameras/cutscenes and inactive replacements remain
  rejected. The former pointer check reproduced keeper initialization failure,
  which also prevented its eight native stock actors from spawning.
- The invisible counter boundary retains its native physical collision and
  no-ledge surface. A narrow owned-collision check prevents Mitts, the climb
  cheat and Slate overrides from adding the climbable flag to this boundary.
  Ordinary surfaces retain their climbing abilities. The reported runtime
  failure is reproduced at this native policy boundary, rather than inferred
  from the previous ordinary-movement test.

## Evidence

- Counter climbing regression failed with the original real wall-flags code,
  then passed with all three overrides; native collision/talk, human/Goron
  radii, camera pass-through, floors, aisle/exit, capacity and lifecycle pass
  under ASan/UBSan.
- Granny actual-resource regression failed on the original owned replacement,
  then passed within 128 typed scene/camera/cutscene/pointer/Alt/lifetime cases.
  Both native host attachment fixtures pass. Native shelf GBI/collision tests
  draw both magical and Granny finishes with real texture paths.
- Complete EnOssan/EnGirlA + native Data4/cutscene integration passes. All eight
  selection/return intervals, including interpolation, clear the full reserved
  item/seal envelope against 7,577 prepared room and 144 shelf triangles and
  the native letterbox/textbox. Both-bank cursors, all five forms, receipts,
  interruption, ownership and camera-pool cases pass.
- Confirmation remains at least 20 pixels for the fixture's reference mesh;
  shelf references are 11–12 pixels. The former 1.75x relative-growth assertion
  becomes 1.5x because the shelf reference is intentionally nearer; the original
  absolute confirmation size and full clipping checks remain required.
- Seventeen Python shop/asset tests pass. Exact connected-component deletion,
  retained-face flags, private graph closure, deterministic preparation, optional
  material ownership and original texture identities pass. Real XML loading
  retains 7,919 OoT / 7,577 MM triangles and all 30 scroll calls. Native effects,
  shelf collision/GBI and native optional-room draw-exit checks pass in both hosts.
- Generated OoT resources are byte-identical (200/200). MM adds only two private
  geometry owners (214 vs 212 resources); no resource is removed. Eleven existing
  MM resources change for the complete pot removal and removal of its previous
  rigid placement move. Every generated texture remains byte-identical.
- Project-pinned clang-format 14.0.6 and whitespace checks pass. There are no
  source changes under `soh/`. No texture recompression or public upload ran.

## Limits and runtime check

This is source/native verification, not a full application build or configured
gameplay acceptance. Granny's installed Djipi room/stock/shelf visibility needs
an in-game check; the external room/texture bytes are not part of this candidate.
Normal and optional magical MM room geometry are checked; external replacement
models outside the authored envelope remain untested. Original Granny props
are preserved. No master promotion, develop/master merge or shared-branch push
has occurred.

Build this source with freshly prepared matching port archives. Verify greeting,
both banks, all eight offers, select/cancel/purchase/receipt/continue, counter
talk and climb attempts with powers, Granny on/off and exit/re-entry. Offline
previews show source geometry and native camera/item traces using the base
keeper and representative stock; they cannot establish collision or installed
custom-keeper behavior.
