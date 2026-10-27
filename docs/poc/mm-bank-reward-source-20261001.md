# Clock Town Bank reward attribution

Baseline: ComboShip PR31 head `b715c82e9227f033faaaf91415b0af5be23eb44d`.

cor reports that a silver-rupee check overflowed into the bank and unlocked an OoT dungeon key. The key's normal get-item animation played. This candidate clarifies the separate check's source; it does not change which item the seed assigns or when a bank milestone grants it.

The standard item textbox and existing notifications append ` (Clock Town Bank reward)` for the three bank checks. For example, the ordinary message reads `You found Water Temple Small Key! (Clock Town Bank reward)`. Native MM rewards, foreign OoT rewards, and notifications when cutscenes are skipped retain their existing item names, icons, fonts, colors, duration and delivery behavior. No separate popup or additional notification is created.

Attribution uses the check ID, so it also remains correct for a manual deposit. Other checks, unknown check IDs and shared-item synchronization keep their existing text. Foreign traps retain their existing message/disguise handling.

## Preservation and verification

- Auto Bank Deposit, its capacity and both threshold sets are byte-identical to the baseline.
- GiveItem's ownership/progression code is byte-identical after removing the one notification-suffix expression.
- CheckQueue changes are confined to its existing textbox/notification strings. Cutscene selection, foreign-item latching, grants, saves, broadcasts, obtained/cycle flags, draw callbacks and queue resets remain intact.
- Existing foreign presentation, progressive scale/save/replay lifecycle, and item-grant regression suites are run alongside the recovered dungeon-key suites. The scale fixture loads the new source-formatting helper to retain its production dependencies.

Full application builds and the actual combined MM/OoT runtime remain pending. The candidate has not been pushed, merged or accepted as a master.

Runtime check: collect the same silver-rupee check with a full wallet and the bank just below the relevant milestone. Confirm the rupee's normal presentation, followed by the bank reward's normal item presentation carrying the source text. Repeat with item cutscenes skipped, then collect an unrelated check and verify its text is unchanged.
