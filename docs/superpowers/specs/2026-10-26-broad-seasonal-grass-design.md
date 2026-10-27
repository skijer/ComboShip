# Broader seasonal grass coverage with unchanged treatments

User requests the Termina correction in source and broader Autumn/Winter texture queries comparable to Spring, in both games. Explicit preservation requirement: do not change existing tint formulas; add coverage without regressing accepted treatments.

MM: preserve the valid native `G_RDPSETOTHERMODE` packet that caused the exact merged Termina floor to be rejected. Broaden queries for the same reviewed grass identities and eligible foliage children Spring reaches. Preserve already-working draw-scoped tint routes and all palette, grayscale-mode, blend, brightness, pixel-mask and texture conversion arithmetic. Missing draw coverage may use existing selective surface copies; shared identities across covered/uncovered draws must not introduce double tint or change the accepted route. Unsupported/cold resources retain safe existing behavior, and source pixels plus queued allocations remain intact across season/asset/scene changes.

OoT already uses one shared query table for all three foliage seasons. Preserve its runtime code and characterize every eligible reviewed cohort in native/Alt modes across Spring, Autumn, Winter, repeated transitions, Summer and Off, including original/alpha/metadata preservation. Existing spatial masks and negative green actor/stone controls remain required.

No arbitrary green object discovery, recoloring baked into archives, source palette retuning, asset recompression, public merge or gameplay-acceptance claim. Record exact source and native verification separately from Windows/configured game testing. Update the existing durable follow-up checkpoint to include the source fix and revise the prior diagnosis-only note.
