# Forest and Black Market native arrival fades

| Field | Record |
| --- | --- |
| Baseline | Music/shelf source `df56c2c1b69b8883836a02ac5db0ed51f12526f3`, tree `49d1fdec6e5ea5f0a39824d1ebe4ecaa8106e32f`; `ComboShip-Forest-Market-Music-Source-20261018T173206Z.zip` |
| Candidate | `poc/forest-market-native-fades-20261018`, based directly on that source |
| Hypothesis | MM's opening Lost Woods entrance inherits the native prologue's instant arrival (`0x858B`). Explicitly staging a black fade after the paired save loads lets Play_Init reveal the exploration destination gradually. |
| Scope | Native black arrival override for the two combined forest routes in both hosts. No new transition renderer or asset pack. |
| Preservation | Route destinations, solo guards, remembered origins, placed arrivals/cameras, native prologue/flashback behavior, title art and timing, geometry/collision, shelves, actors, weather, inventory/progression, Farore's Wind, and music remain unchanged. |
| Status | Implemented; native compilation and source behavior verified. Full executable build, rendered gameplay, Alt/Djipi visual parity and user acceptance remain untested. No master promotion. |

MM_ApplyComboPortalArrival now stages TRANS_TYPE_FADE_BLACK after consuming
the pending forest request and after native save loading. This covers the
playable 2Ship Lost Woods and Woods of Mystery arrivals. SOH_ApplyComboPortalArrival
does the same only for an allowed forest placement. Play_Init consumes the
override once. The existing native fade owns alpha, duration and rendering;
the existing title-card hook already waits for the transition and player entry
control to finish. The normal Faster Scene Transitions setting still applies.

Woods of Mystery's native entrance already selects black, as do the OoT
Lost Woods return and the existing local Black Market/solo forest transitions.
Those behaviors are retained and covered. The explicit forest override also
prevents a stale instant effect from replacing the requested reveal.
Normal handoff cancellation continues to discard forest route requests when
composed dungeon/boss, reset, owl-save or Farore travel owns the switch.

## Verification

- The new `tests/lost_woods_entrance/run_arrival_fade_tests.py` uses the real
  native pending-effect default and entrance metadata, production boot/arrival
  bodies, native Play_Init selection, native transition initialization and
  actual TransitionFade arithmetic with native save/play types.
- The baseline fails on the opening-forest instant arrival; OoT's baseline
  failure is separately reproduced with an explicitly stale instant effect.
  The candidate passes in both engines under ASan/UBSan: both forest routes,
  all three slots, one-shot consumption, black-to-clear multi-frame alpha,
  absent/superseded route guards and existing local shop/solo forest fades.
- The complete forest suite `python -B tests/lost_woods_entrance/run_tests.py`
  covers boot/handoff cancellation, native arrivals, cameras, title lifecycle,
  save/slot/progress preservation, native prologue boundaries and weather.
- Existing MM/OoT native shop and solo forest suites pass, as does
  `python -B tests/black_market/run_scene_music_tests.py --native`.
- Both changed translation units, `mm/2s2h/BenPort.cpp` and
  `soh/soh/OTRGlobals.cpp`, compile to objects with real dependency headers and
  build CVar definitions. Compile commands, dependency/source hashes and
  results are retained in the checkpoint. No full link is claimed.
- Independent code review found no blocking production defects after correcting
  the new fixture to use native defaults and destination-specific entrance data.

Disk I/O, transition factory dispatch, resource ownership availability and GPU
output are fixture boundaries. Source tests cannot establish rendered smoothness
or the installed Djipi title-card appearance.

## Runtime check and recovery

After the cumulative build, cross both OoT forest tunnels and return, then visit
forest → Black Market → forest. The destination must begin black, reveal smoothly,
then show its existing title card without a clear-frame pop or a duplicate fade.
Check the Skull Kid/Clock Tower handoff, solo SoH and solo 2Ship routes, and re-entry;
include the normal/Alt assets and current Djipi load order. These remain runtime
checks, not acceptance evidence. No replacement O2R or load-order change is required.

The checkpoint contains the bounded patch, source files, incremental Git bundle,
verification evidence, exact base/candidate identities and restore instructions.
Restore its retained music/shelf baseline first, then fetch/cherry-pick the new
candidate commit or apply the patch to an equivalent cumulative tree. The
standalone recovered history must not be merged as a replacement for main.
