# Goron lifetime and Somaria GI candidate

Baseline: `2069d37e2a9338494acff3a70ca56ddaf2e035fa`, the entrance
candidate on draft PR #43. This checkpoint is independent of the generation
reset correction. Neither checkpoint promotes the accepted base branch.

## Report and reproduced defects

The reported Goron rolling sound survives voids and scene transitions until
the mask is removed. OoT actor sound cleanup does not own the separate MM
sound bank and direct mixer. Scene initialization, reset, death, player
destruction, room/scene transitions and action replacement could leave the
charged rolling entry alive there.

The report also includes walking through bombable Hyrule Field boulders,
punches failing to break them, and ground pounds failing to open hidden
grottos. A textbox interrupt can replace a ball action with idle without
restoring its body collider: the pound's persistent `OC1_NONE` survives into
walking and a later punch. Full form exit also failed to restore the body
AT/AC/OC flags. Separately, the pound supplied only `DMG_HAMMER_JUMP`;
OoT's bomb/hammer grotto accepts the swing bit, so it rejected that impact.

Fresh baseline walking and punching already pass against the native boulder
and collision routines. The user's custom `Hyrule Field - FINAL.o2r` pack is
not available locally. The interrupted lifetime is reproduced, but these
tests do not establish every reported boulder failure's cause.

The Somaria report is an invisible GI in an MM grotto reached from OoT, with
the item granted. A reproduced draw defect discards an OoT-only legacy cane
override: authored presentation declines for an override in either owner,
but the MM fallback always loads MM's cane. Raw MM roots also lack an
explicit MM resource scope under an outer foreign draw. The supplied log
contains no Somaria receipt trace, so the exact reported route is unproven.

## Correction

- Stop all six Goron roll/charge/slip sound IDs at lifecycle exits and after
  dispatch when the current action no longer owns rolling audio. Stop even
  if MM playback was disabled while an entry was alive. Preserve normal
  rolling, airborne ball and pound audio ownership.
- Restore the body collider and leave ball state before the textbox early
  return and existing OoT yield. Restore the body flags on full form exit.
- Supply the native swing and jump hammer bits in both pound impact windows.
  Native damage selection retains jump damage. No explosive bit is added;
  Song of Storms grotto requirements remain intact.
- Preserve MM override priority, route an OoT-only cane override through the
  existing stable `@oot` resource path, and bracket native MM cane roots with
  MM ownership. Item grant/receipt identity and other GI callbacks are unchanged.
- Run the new audio, collision and cane checks immediately after checkout,
  before dependencies and the long regression/build stages.

## Verification and limits

The final audio fixture fails 24 baseline exit cases on a stale charged-roll
entry; both active-roll controls pass. The candidate passes all 26 cases
under ASan/UBSan through sequence and direct audio routes, checking six IDs
per case. It compiles production lifecycle/update bodies, bank, dispatcher
and stop routines. Audio synthesis/output and unrelated form services are
boundaries; initialization/reset exercise their cleanup prefixes.

The collision fixture passes all nine cases under ASan/UBSan. Against the
baseline, five fail: both pound windows, textbox interruption, the following
punch interaction and form restore. Fresh walking, punching, jump damage and
Storms controls pass. It executes native cylinder/quad intersections,
`ObjBombiwa_Update`, `DoorAna_WaitClosed` and the production punch action.
Only inside the fixture, the pre-existing signed `(1 << 31)` fixed-damage
macro is represented as the identical unsigned `(1u << 31)` to avoid its
unrelated UBSan shift error. This is not a native collision macro change.

The cane fixture fails the baseline on the discarded OoT override, then
passes normally and under ASan/UBSan. It covers base/Alt overrides, MM
priority, live cache changes, stock fallback, nested resource ownership,
tint/flames and native/foreign dispatch. The changed callback compiles with
real MM headers and GBI in Combo and native modes. Existing quest-held,
authored GI, MM presentation/progression/icons and sanitized static GI bridge
checks pass. Related Zora, mask and Leaf audio/action checks pass. Independent
review reran all three new sanitizer fixtures and found no blocking defect.

Local full translation-unit attempts lack `imgui_internal.h`; the broader
cumulative NEI runner stops at asset packaging because `cmake` is absent.
Configured Windows/Linux builds remain required. No in-game run or listening
test has been performed with the user's installed configuration.

Runtime proof still needed: roll normally and at full charge, then void,
change rooms/scenes, die, reset and remove the mask; confirm silence after
each exit and normal audio on the next roll. Repeat walking and punching at
the reported boulders, including after pound/textbox interruption; pound
bomb/hammer grottos and confirm Storms grottos still require their song.
Obtain Somaria through the reported OoT-to-MM grotto route with the installed
model stack and verify its GI. These are implementation candidates, not
accepted runtime fixes.
