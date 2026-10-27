# OoT inventory sub-page cursor lock

Baseline: `25e990332ef9d9d5147c2df4ef5413c61af7aa5b`, the recovered shop,
title and regional/Snowhead source candidate. Isolated candidate branch:
`fix/oot-inventory-page-cursor-20261024`. Public develop remains unchanged.

The user identified SoH/OoT as the affected host. The matching source case is
an empty inventory sub-page: native item search leaves focus on a page arrow,
but the extended-inventory shoulder-button handler was inside the grid-focus
branch. Re-entering that empty grid is impossible under native owned-item
navigation, so the player cannot reach owned items on another sub-page.
MM already handles its inventory-page button outside the grid-focus branch.

The fix handles the existing freed shoulder button before the grid/arrow
split. It retains both native tab-switch mappings, the available-page policy,
15-frame cooldown and native owned-item cursor search. It does not force a
cursor onto empty slots, grant/unlock items, modify inventory or save layouts,
or change MM. Debug/equip states and open item wheels retain input ownership.

A second reproduced defect affected simultaneous page/equip input: the old
page's item ID could be passed with the new page's inventory slot. Refreshing
the selected item immediately and deferring item actions for the page-switch
frame keeps both identities consistent. The following frame can equip the
actual newly selected item normally.

## Verification

`tests/oot_pause_inventory/run_tests.py --source-ref 25e99033 --sanitize`
fails on both original defects in solo SoH and Combo builds. The same runner
on the candidate passes with actual native PauseContext/SaveContext structures,
the unmodified production page-state helpers and the complete production
item-selection input block. Only graphics/audio and external effects are
fixture boundaries. Cases include both arrows, both shoulder mappings,
owned-custom/mask reentry, all available-page combinations, cooldown,
debug/equip/tab/wheel guards and simultaneous C-button input.

The existing sanitized OoT equipment-access fixture passes all 111 checks;
the existing sanitized MM pause-inventory fixture passes both navigation and
entry groups. All 21 CI policy tests and `git diff --check` pass. The new
sanitized runner is registered in the equipment CI group. LeakSanitizer cannot
inspect executor processes; address and undefined-behavior checks remain on
with the existing `ASAN_OPTIONS=detect_leaks=0` convention.

Full application/Windows compilation and configured controller/gameplay proof
remain pending for this candidate. Re-test the actual affected save: return to
Items with an empty sub-page selected, use the existing item-page button while
focus is on either arrow, enter an owned item and equip it. Repeat after native
tab changes and pause/reopen, with both NGCKaleidoSwitcher modes as relevant.
No runtime acceptance, public publication or develop/master promotion is claimed.
