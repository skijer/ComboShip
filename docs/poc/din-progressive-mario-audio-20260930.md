# Progressive Din swords and sound cleanup candidate

Baseline: `a6ae6e1b84108de857dbf75d65b3a734346c5cb4`, the current MM Time Gate
candidate stacked on the combined NEI branch. Its Windows/Linux build workflow
`36674687326` passed. Existing branches and the original Din archive are retained.

The user requested a combined candidate ready for tonight's merge. The sword
scope explicitly includes NEI upgrades and excludes Cane of Byrna, Trident and
Four Sword. See `din-progressive-fire-20260930.md` for its resource and rendering
contract. The companion O2R remains
`Din_Fire_Sword_Shield_Progressive_Combo_POC1.o2r`, SHA256
`5f7972b03e494aa3c309860dc2e91d85ab3bbe18c61244c06d063f8205e27306`.

## Audio

The supplied log records Metal cap activation at 01:59:11.650 and its flag
clearing at 01:59:12.308. In both game implementations the cap activation cue
had no Zelda SFX cleanup, although libsm64 music was stopped. Native sound-bank
tests reproduced Metal's sound surviving toggle-off before the fix.

Cap cues now use their own stationary source. The shared deactivation path
stops active and queued sounds from that source on toggle-off, switching caps,
expiry and suspension. This preserves the chosen cues, cap states/cooldowns,
libsm64 music handling and identical sound IDs owned by other actors or the
normal centered source. No global sound-bank or sound-ID stop is used.

The actual sound heard in the user's recording was not captured or identified;
the source-level lifetime defect is proven, but attribution of that audible
symptom still needs the user's runtime check. OoT Time Gate cleanup is also
covered as a distinct potential source of the boss-warp sound; its existing
controls and action behavior remain authoritative.

## Rendering diagnostics

The latest log's 1,985 FILEPATH texture failures occur in Lost Woods (scene
0x5B), Alt Assets enabled, at 01:53:02-01:56:01. The old message omitted the
resource path, so it cannot establish an offending pack or a safe asset repair.
The separate unresolved display-list target in Zora's River remains unattributed.

FILEPATH failures now identify the requested path, active resource manager and
Alt state, once per context. Retained contexts are capped at 128 with one
overflow message. Resource loading, owner routing, fallback, command state and
valid rendering are preserved. This makes the next failure actionable and
removes repeated log writes; it does not repair or hide missing resources.

## Evidence and remaining runtime checks

- Native OoT/MM cap state machines and native sound engines pass ASan/UBSan
  for four caps, four deactivation routes, pending requests and repeated use.
  Other sources' sounds and proportional cooldown values are checked.
- The full OoT Time Gate handler with native audio passes ASan/UBSan for Yes,
  No, damage, unequip, invalid state, queued cancellation and repeated use.
  B and long hover retain OoT's existing non-cancelling behavior. Magic and
  age-switch values, other actors' matching sounds and other player SFX survive.
  The original source failed the confirmation sound-lifetime check.
- The five focused sword renderer/hand/damage suites pass after formatting.
  They cover upgrades, original weapon damage, optional fire, Alt/assets-off
  fallbacks and other equipment ownership.
- The production FILEPATH handler fixture passes named/bounded diagnostics,
  source-context separation, recovery, routing and unchanged failure state.
  ASan/UBSan passes with leak detection disabled for the local environment.
- The companion archive preserves all 259 baseline entries and passes CRC and
  the complete 66-entry resource dependency check.

Local tests are not full application builds or audible/visual gameplay proof.
Remote platform builds and independent review are recorded in the PR.

Runtime checks: toggle Mario's Metal cap on/off, switch caps rapidly, leave
Mario mode and change scenes; the cap sound must end and ordinary sound/music
must continue. Try Time Gate confirmation and No, then repeat a cast. Draw and
swing each upgraded sword with Din fire enabled, then toggle fire/Alt off.
Revisit the Lost Woods north entrance with the same packs to obtain any named
missing texture. Keep the prior build/archive as rollback. Runtime acceptance
is still required before promotion of the working master.
