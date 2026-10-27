# ComboShip bugfix roundup — 2026-10-21

cor requested one cumulative source merge into `develop`, including the
unfinished pipeline, then explicitly authorized publication of these fixes
and their source checkouts. PR48 is the integration target. PR49 was stacked
on that integration branch; it had not independently merged to `develop`.

The cumulative baseline is `8ab33deeb5000c6a66663dc9659d9060f702881a`.
Its existing Windows compile preflight and Windows/Linux artifact builds
passed, as did those for the stacked Mirror head `a7b5244e`. Those older
packages do not include the new forest corrections below.

Included source changes since that cumulative baseline:

| Change | Recovery / evidence |
|---|---|
| Mirror choice, native closing and movement ownership | Existing `a7b5244e`; both native-layout fixtures, forms/inventory/save/action and real player input gates. |
| Saria dialogue advance | `6d171889`: handle the custom event-text advance and close in the actor; real decoder and actor checks. |
| Native transformation audio | Retained input commit `58e5c4bd0ed850bbbfb5f2619c3e0ab840258638`, tree `36cd680e2c3332cf5c0c9a53f89938c5b465d992`; two-part high-rate sample rendering rather than truncated native cue audio. Eight real DSP cases, 116 native controller cases and audio-lifetime checks pass. |
| NPC archive repair recipes | Existing `26accab6` history plus input-alias protection `e62668e0`; 190 Python tests, 22 optional-input skips, native actor/routing/pose/draw checks. Model archives remain separate mod installs. |
| Seasons do not activate OoT Storms puzzles | `641451ca`: weather application no longer sets environment flag5. New fixture first reproduces actual native grotto opening/chime, then passes every season/presentation mode. A real song flag still opens the grotto. |
| Native forest authored presentation | `76029ea9`, `05313bb2`: original three-room game framing with exact authored shop/poster/bench/bridge resources and six scroll tracks. Only the portal mouth primitive RGB changes to green; alpha96 and purple shop/stump/bench materials remain. |
| Natural Mirror pickup | `ec4bc59b`, `039a8f41`: grounded static stump-cap contact, no base/ramp/platform grant; a visible collision-backed fallen log provides access. Original collision records/tail are preserved with additive16 vertices/28 polygons/1 wood surface. |
| Actual NEI twinkles | `a1f6c40e`: use the actual sampler for Saria, the uncollected Mirror and both bridges. A five-by-four field covers each black entrance face, including its center. The old border/filled-hex emitters are removed. |
| Optional MM3D appearance | Separate appearance-only recipe and candidate pack retain native authored additions/log and change only six room mesh command10 records. |
| BossSoul flame alpha repair | Retained archive-only POC5 tooling, pinned inputs and shader snapshots under `tools/boss_soul_flames`. Its mod packs remain separate installs; no executable/material/model rewrite. |

Source recovery already verified: branch
`checkpoint/bugfix-roundup-before-forest-20261021`, remote commit
`0257e242b1b924140be4bc491c9e3b5bbae09649`, tree
`aa7e9fbe18dfc4ac55402c65845cc6dd3cd290ae`. A fresh remote fetch matches
local checkpoint `e62668e077d699dd51e91af891a5d3567f012332` exactly.
The remote commit preserves the Mirror/Saria/NPC commit parents. The retained
transformation-audio input archive also contains its original Git bundle and
exact ref/tree. Recovery input hashes and recipes remain in their POC records.

The final cumulative candidate is published on
`checkpoint/native-forest-nei-mirror-20261021`; its publication receipt and
PR48 record the exact remote commit/tree corresponding to the local candidate.
The source API preserves the tree bytes; its commit metadata differs from the
local integration commits listed above. Clone this branch to recover all final
source, not only the earlier pre-forest checkpoint.

The new native canonical ZIP is85 resources /49,704,908 bytes, SHA256
`b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5`.
GitHub's blob API rejects that size. Source storage uses verified numbered
parts; reconstruction must produce this exact ZIP before existing CRC,
membership and per-resource checks. This changes storage only. The source
manifest and reader are the authority; a leftover local ZIP cannot mask a
missing/corrupted tracked part.

Additional completed checks:

- New full particle suite: exact source twinkle vertices/pulse/palette,
  camera bases, overflow, central entrance coverage, actual draw bodies for
  both bridges and hosts, both renderer boundaries and arena restoration.
- Both native Mirror fixtures under ASan/UBSan; four physical log/collision
  tests; all earlier choice/cast/return, forms and ownership cases retained.
- Independent complete compilation of both Mirror adapters, both solo forest
  units and the MM passage unit against real dependency/native headers.
- Independent native forest suite and production solo entry/return/save
  fixtures. The two original stock/full-donor tests skip when those optional
  original inputs are unavailable; committed source-bundle checks run.
- The final native forest suite accounts for48 tests with2 optional-donor
  skips. A separate fresh Git-archive restore loads only committed source parts
  and the reader, with no ZIP cache, and reconstructs the exact pinned bytes.
- Actual production NEI preview exported with runtime vertex quantization
  and source-alpha blending. Original GI energy/shimmer/orb streams remain
  byte-identical. Preview is offline rendering, not captured gameplay.
- Independent review found no Critical/Important core blockers. Full clang
  format14.0.6 pass applied across1991 repository-selected files; its one
  new guard-wrapping correction is included. No unrelated behavior changed.

Source integration does not establish configured gameplay acceptance. The
remaining game check is fresh unowned pickup via the log, no base grant,
both black portal faces/directions, all Saria poses, shop traversal and
seasonal scene changes with the actual installed mod order. Optional MM3D,
repaired NPC and flame packs have their own installation/load-order records.
No accepted visual/runtime master is promoted by source compilation.

Restore the source by cloning the published checkpoint branch (or the merged
PR48 tree), initialize the repository's normal dependencies, and run
`python3 -B scripts/assets/prepare_native_forest.py` before asset packing.
No external full-forest mod is required for the authored native additions.
