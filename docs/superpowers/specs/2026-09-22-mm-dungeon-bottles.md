# MM dungeon items and bottles

Approved scope: retain MM models where the TP donor has no matching model; enhance small keys, boss keys, potion bottles and fairy bottles. Port TP dungeon maps, compasses and Poe/Big Poe bottle shells. Keep the existing animated bottle contents and add sparse, content-coloured shimmer. Other items are excluded.

One editable colour per MM dungeon drives small keys, boss keys, maps and compass bodies. Resolve randomizer ownership from the item ID, never its location. Use the current key palette as defaults: Woodfall #EC78BA, Snowhead #81AD46, Great Bay #635AB7, Stone Tower #B1A553. Preserve shading, legibility and clear compass glass. Vanilla dungeon items use their actual dungeon scene; outside a dungeon an unknown owner remains untinted.

Provide separate switches for dungeon colours and bottle shimmer in Cosmetic Editor > Link & Items. Both start off in the POC so an existing installation retains its settings until enabled. The model pack works independently; the live effects require this code candidate. Shimmer uses three deterministic billboards in bottle-local space, advances on gameplay frames, changes no game RNG or effect-pool state, and restores the matrix and render state.

Baseline: ComboShip 639b526150898cee98ffdd09ccfa6ccd6a40f8ba, remote integration/shared-items-soh-20260921. Isolate on poc/mm-dungeon-bottles-20260922. Never modify donor archives or promote a master without configuration-specific runtime proof and acceptance. Static and compiler checks are not runtime proof.
