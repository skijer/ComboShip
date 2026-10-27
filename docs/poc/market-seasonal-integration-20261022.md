# Cumulative Black Market and seasonal paving integration

## Candidate and preservation

Resume the complete Black Market/forest source candidate
`2b8fa033b7f6c86554aeab4a7033db7b0fa7552b` on
`poc/market-forest-followup-20261022`, and merge published PR #56
`d21ded5c9976f43db485d8f9c2b05e6fce62d5c4`.
Their shared develop baseline is
`c3b0c76b81766ce51378e573dcf9816ab3c59473`.

Every one of the 118 cumulative market files remains byte-identical to the
market candidate; all four seasonal files are byte-identical to PR #56. Their
changed paths are disjoint. The market candidate's native keeper text and
close/re-talk cleanup, browsing/camera, purchase/stock, bordered title/fade,
native exit door, interior resources, forest presentation, Mirror and native
destination/entrance behavior are retained. Source input archives and the
existing workflow/task inventory are unchanged by this merge.

The seasonal change selects grass baked between the Hyrule Field paving tiles
in `s00_road_01_41180` and `s00_road_02_46580`. It retains ordinary terrain's
existing selection and includes the stricter selection in texture-copy cache
keys. This source integration neither changes the supplied Field archive nor
adds the separately delivered caustic candidate, trims actors or promotes an
accepted runtime master. No new recovery ZIPs are included.

## Verification

- Fresh native/HD seasonal pixel and production binding tests pass under
  ASan/UBSan, including native/Alt paths, source/alpha/metadata preservation,
  cache isolation, reload/cleanup and summer restoration.
- Rebuilt the actual uploaded Alt-paving image fixture against the combined
  production code. For each non-summer season, the two textures change 21,610
  and 58,005 selected pixels respectively. All source pixels and alpha remain
  intact; sampled stone faces remain exact. Summer changes zero pixels.
- The complete entrance runner exits zero, including native title/fade,
  OoT cross-game title lifecycle, all MM destination scene rows, Fairy and
  Woodfall routes, arrival fade, portal camera and return guards.
- `SeasonFoliage.cpp` compiles against real native headers in both Combo and
  standalone modes. The four retained market/title Combo objects still match
  their original checksums and all 1,096 recorded dependency hashes. The four
  corresponding standalone sources and object checksums remain exact.
- A fresh independent integration review found no important correctness or
  integration defect and independently confirmed exact preservation of both
  source candidates.

The complete early regression sequence passed its decoder, actor, particle,
market entry/door/text/title/resources/readiness, interior, cache, lifetime and
palette components. The final forest batch discovered 57 tests: 54 passed,
two optional original-donor cases skipped, and its CMake packaging case initially
errored because the verification runner lacked CMake on `PATH`. Adding the
already restored CMake directory to the runner environment made that exact
case pass; no production or test-source change was required. Both browsing
adapters and the full MM/OoT market transaction/entry/return runners then passed.

Detailed current-session command output is retained in the workspace verification
directory; the underlying implementation's prior evidence remains in
`docs/poc/market-forest-followup-20261022.md`.

## Publication and runtime status

cor explicitly approved the push to `marsh6487/ComboShip` in this conversation,
resolving the automatic publication review's earlier authorization rejection.
Publication targets `poc/market-forest-followup-20261022` and a cumulative draft
PR against develop. The published PR records its exact remote source head and
tree. PR #56 remains available as the standalone seasonal source candidate.

Full executable linking, actual GPU/native/Alt appearance, configured live
browsing/purchase/close/re-talk, exit-door returns and shuffled destination
title presentation remain untested. Native fixtures and source compilation do
not establish configured gameplay acceptance. The accepted runtime master and
develop remain unchanged.

## Recovery and next action

The merge commit preserves both exact parents in Git history. Check out the
published candidate commit to recover the combined source; the market parent
and PR #56 remain distinct rollback points. Publication verification checks the
remote commit/tree and PR #56 ancestry. Retain the recorded executable-build
hold unless separately released.
