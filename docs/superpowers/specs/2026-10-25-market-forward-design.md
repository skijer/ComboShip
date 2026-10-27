# Black Market display refinement

cor chose physically closer shelves and item anchors, keeping the current
shopkeeper camera. The clip also shows rear-pot vapor crossing the displays;
cor requests those rear pots removed, reports missing shop contents with the
Granny toggle, and can still climb onto the counter.

## Baseline and preservation

Start from local `cc11cfcdd2d157017cb7be37e61301fd8f63a8a3`, tree
`54ca0f4860ea91affc0498d8f36c8d5b03872097`, identical to public `83eed9ff`.
Preserve the fixed shop eye, keeper position/pose, native item selection and
receipt flow, seeded stock, talk range, customer floor, exit and solo guards.
Keep accepted archives, compressed texture pools and develop/master unchanged.
Source checks and offline geometry checks do not constitute gameplay acceptance.

## Approaches

Moving both shelf banks and their anchors toward the customer makes stock
closer throughout the interaction. Tightening the browsing camera would leave
the displays in place but add camera movement and could crop the keeper.
cor explicitly selected the first approach. Native framing checks settle the
forward translation at 12 units. Lower the upper anchors by 4.5 units, lower
the shelf roof by two, and shorten the middle-board front by five to clear
the full item/seal envelope and straight native selection path. Preserve the
confirmation endpoints and absolute size while the shelf offers grow.

## Implementation boundaries

- Repair Granny startup without relaxing camera/cutscene validation: accept
  only the owned optional shape tied to the validated original room shape.
- Identify a reproducible counter-climbing failure before changing its policy;
  keep native physical collision and talk-through behavior.
- Translate MM shelf mesh, collision and all eight stock anchors together.
- Remove both rear cauldrons, including their contents/attachments, from the
  private MM presentation; retain entrance-side decoration and source geometry.
- Remove the corresponding MM vapor emitters; preserve OoT presentation.
- Verify native shop/receipt/camera behavior, collision, optional-room lifetime,
  private resource closure and source preservation; checkpoint the result.

## Acceptance

The candidate must show visibly closer items, no rear-pot steam through the
shelves, the keeper/stock/shelves in Granny's room, a counter that remains
inaccessible while the shopkeeper can still be spoken to, and normal item
selection/return/purchase. Those configured runtime checks remain cor's smoke
test after a matching executable and port archives are built.
