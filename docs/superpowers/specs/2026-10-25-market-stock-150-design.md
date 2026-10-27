# MM Black Market stock at 1.5×

cor chose the 1.5× size study: increase the private MM shop's actor scale from
`.18` to `.27` (108% of native MM/OoT `.25`). Baseline is `917932f3`, retaining
the forward shelves, rear-pot removal, Granny initialization and counter fixes.

## Approaches and decision

The shown alternatives were existing size, 1.5× and 2× at identical cameras and
anchors. cor explicitly chose 1.5×. Implement that uniform scale in the private
MM stock actors. Adjust shelf depth/row spacing and confirmation endpoints only
as needed for the existing complete item/seal geometry and UI bounds.

Builder fit: retain the full forward shelf translation. Use half-sized seal
padding/ribbon thickness only in MM, lower upper anchors by 3.5 units, shorten
middle lips by four, and move confirmation endpoints four units back while
keeping them on the same bank viewing ray. Model scale stays `.27` throughout.

Keep the fixed camera eye, pitch, FOV, bank pivot, keeper position/pose, ordinary
shops, OoT shop, receipts, stock generation, compressed textures and prior
corrections. Stock must remain visibly larger after native updates, and each
selection/cancel path must preserve a useful close-up without clipping.

Verify with actual EnOssan/EnGirlA native actions, authored item/seal envelopes,
room/shelf triangles and native UI bounds. Render the implemented trace and
save an independently restorable source checkpoint. Full application and
installed Granny/Alt gameplay remain untested; do not publish or promote.
