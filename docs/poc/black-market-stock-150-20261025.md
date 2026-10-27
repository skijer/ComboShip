# MM Black Market: 1.5× stock

cor chose the 1.5× preview. Parent: `917932f3151b06c87e122b47d197542890699028`
(tree `a0e39b4779be21bef10869a823dc27557adb287f`). Candidate branch:
`poc/mm-market-stock-150-20261025`. This remains an isolated source candidate.

## Implementation

- Private MM stock scale increases from `.18` to `.27` during initialization
  and after every real EnGirlA update: 1.5× the previous scale, 108% of native
  MM/OoT `.25`. The same scale persists during selection and return.
- Both shelves retain their full 12-unit forward move. Upper stock anchors
  lower from 79.5 to 76; middle shelf lips shorten from 94 to 90 in local Z.
  Lower stock anchors and shelf collision bounds remain unchanged.
- Confirmation endpoints change from `(±38,53,96)` to
  `(±39.25982,53,92)`, keeping the close-up on the bank viewing ray while
  reserving complete larger model/seal bounds above the dialogue box.
- MM seal padding and ribbon thickness use a `.5` profile. Captured model
  bounds and model scale do not shrink. Other callers retain the original
  default profile. The exporter and renderer consume the same profile.
- Fixed camera eye/pitch/FOV, bank pivots, keeper position/pose, native shop
  behavior, Granny initialization, counter boundary, rear-pot removal,
  ordinary shops and OoT stock are preserved. No texture/asset recipe changes.

## Verification

- The new all-eight-slot shelf readability assertion failed against the old
  display, then passed. A 40-unit reference projects at 17–18 native pixels
  on the shelf (previously 11–12) and 28 in confirmation. The original >=20px
  and >=1.5× confirmation assertions remain required. B restores exact anchor
  and scale after actual native actor updates.
- Scale-only full clearance failed at the shelf. The fitted version passes
  every native selection/return interval and interpolation against the full
  authored model/seal envelope, 7,577 room and 144 shelf triangles, and the
  original UI margins. Exported bounds reserve 125 actual native seal cases.
- A native MM seal-padding assertion failed before the compact profile,
  then passed. Actual host GBI/allocation/capture/restoration tests pass in
  both hosts, including original OoT padding and invalid profile rejection.
- Fifty default-profile mesh/color samples are byte-identical to parent,
  SHA256 `c8b25a97f66f39c7d94af7bf0196e83c15a9ff71ffef29b9ad96e832bf6a15f0`.
- Both-host shelf collision/material and interior-effect fixtures pass.
  All 17 Python Black Market tests and native-preview seal agreement pass.
  Native fixtures use ASan/UBSan. Changed C/C++ lines use clang-format 14.0.6;
  whitespace checks pass.

## Limits

No full application build, installed Granny/Alt visual proof or configured
gameplay acceptance is claimed. Arbitrary replacement models beyond the
authored bounds remain untested. Offline renders use the base keeper and
representative stock with simplified effects. No publication or master merge.
