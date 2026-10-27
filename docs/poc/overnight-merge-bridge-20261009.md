# October 9 overnight merge bridge

The previous remaining-items conversation hit its maximum. This continuation
recovered its existing staged checkout and two worktrees, rather than starting
the feature work again. The source parent is PR41's build-verified
`56c83a878562052ca873b7d414f7d0e1175b6325` (tree
`01fc6e28b8458e892dcfb886080795767dd6536a`). Exact-parent Actions run
`37878395750` completed successfully, including Windows/Linux packages.
That result applies to the parent, not this new candidate.

| Requested work | Recovered integration |
| --- | --- |
| Wolf blue eyes and chain | Both bundled standard/HD Blobs, soft eye emission, HD model setting, deferred model switching and preserved registration; see `wolf-hd-bundled-20261009.md`. |
| Final autumn tint | Exact private brush leaf/stem registrations, with existing two forest-layer materials retained; see `mm-autumn-private-brush-tint-20261009.md`. |
| Pink night sky specks | Suppress native MM procedural stars only while the replacement OoT sky owns the visible sky. Preserve normal native skies and cover/weather policy. |
| Summer visibility | Preserve the parent’s 56 fireflies and cottonwood count/size increase; include sustained firefly pulse from `summer-firefly-glow-20261009.md`. |
| Hourglass/Crystal tutorials | Shared colored/button-glyph receipts in both hosts and corrected pause descriptions. |
| Metallic ball and chain | Authored held/GI display lists, textures, GLBs, previews and conversion source from the existing metallic POC1; shape and gameplay remain unchanged. |
| Progressive sword fallback | Freeze the awarded sword tier, while allowing live host Alt selection to refresh its appearance. Keep custom sword mods, effects, upright receipt fit and shelves. |
| Time Gate GI edges | Closed outward side winding on the authored gear body, with shape/UV footprints preserved and fresh exporter/resource parity verified. |
| Sage Tunic medallion particles | Restore the existing six-medallion effect on authored draws in both hosts, retaining medallion colors and optional shimmer. |

All recovered assets and the accepted parent remain available for rollback.
This integration changes no installed scene archive or pack load order.
Separate stone/medallion 4K redesign work is outside this remaining-items
checkpoint unless its own completed patch is explicitly included later.

## Verification ledger

- Recovery: the main checkout had its original seven patch domains staged;
  the autumn worktree contained its completed narrow registration/test change;
  the GI repair worktree was still clean. No prior edits were discarded.
- Integrated the four autumn source/test/documentation files only after
  verifying each destination had no existing modification.
- The interrupted weather runner needs the preserved local dependency include
  environment. Reproduced the missing-JSON failure without that environment;
  the complete rerun then reproduced unresolved fmt symbols from the newly
  extracted native sky-star logging code. The fixture now uses the existing
  standalone header-only fmt convention, with production logging unchanged;
  the complete weather command passes.
- All 93 canonical NEI commands passed, with the Wolf packaging command rerun
  after restoring CMake to the local PATH. Five additional commands passed:
  MM weather, MM summer, OoT sky, native-star ownership and both actual standard/
  HD Wolf resources through both production loaders. An initial misspelled
  summer command was corrected to the repository's `run_mm_summer_tests.py`.
- All 51 other CI commands passed; the timing and Ogg-decoder commands were
  rerun after restoring the preserved pkg-config paths. These are environment
  corrections, not weakened assertions. Together this covers the 146 complete
  Python commands used by the current CI gate, plus three additional probes.
- The actual POC4 brush and native forest fixtures passed with 6,869 leaf,
  586 stem and 22 forest triangles, exact command preservation, retained
  cache variants, Off restoration and safe teardown.
- Full repository clang-format-14 introduced no extra source changes.
  Asset collisions and both staged/unstaged whitespace checks passed.
- Cherry-picked the isolated GI repair as `ee1a01ad`. The Time Gate regression
  reproduced 288 inconsistent perimeter edges on the parent and passes with
  outward winding. All 2,972 triangles, 161 loads, bounds, scales, materials
  and position/UV footprints are preserved; 864 side faces reverse winding.
  Fresh temporary authoring export, shipped resources and GLB agree.
- Restored the Sage fountain using the six original medallion face/body
  resources. The production fixture reproduces the missing effect on the
  parent and passes with sanitizer and fast-math modes. Full launch/spin
  transforms match the intact fallback over 80 frames, across both shimmer
  settings, pickup/shop/foreign routes, missing resources and short arenas.
  Native MM renders all six through the OoT donor across all four host/donor
  Alt combinations, without changing the inactive owner query boundary.
- After the GI cherry-pick, all eight affected GI, native-MM, Sage-particle,
  tunic-material, flame-arena, mask-shimmer, serialized-asset and Gate-solidity
  commands passed on the combined tree.
- Independent read-only review found one important gap: OoT's Shadow Crystal
  entry gate still required the legacy loose Wolf file. The actual production
  gate regression failed with bundled resources and that file absent. The
  gate now uses the loader's selected-model archive query, retains disabled,
  malformed-resource and absent-resource policies, and preserves the legacy
  file fallback. Both real standard/HD loaders pass in sanitizer and fast-math
  modes, including registration/buffer preservation. The reviewer checked
  the correction and reports no remaining critical or important finding.
- The canonical runner now contains 96 commands, including the new Gate,
  Sage and actual standard/HD Wolf tests. Together with the 53 additional
  distinct workflow commands, all 149 distinct CI regression commands have
  passed locally through the recovered full runs and affected-command reruns.
  This does not mean the new full-application build has already passed.
- The unchanged real ZAPD exporter generated both normal port archives from
  the combined checkout. Both ZIP CRC/unique-path checks passed, as did the
  bundled Wolf/dependency check. All 964 Wolf/GI/held entries checked across
  the two archives match their source bytes. These normal port archives use
  base paths; per-model Alt lookup/fallback is covered separately by the
  real resource-manager fixture.

| Local export | Entries | Bytes | SHA256 |
| --- | ---: | ---: | --- |
| `soh.o2r` | 6,845 | 91,529,031 | `7ae2254abeaac8a1297c72d8270a438fa9e625f60071b747a40796601c706624` |
| `2ship.o2r` | 2,436 | 21,765,105 | `bf9e88a9b2bd5644db62fbfc1f14983b1eac430041587a409108c1f2451146dc` |

Exporter SHA256 is
`aee69da112daef64c32fad227d1316879136663effb057c37fd1522ab3e19cf7`.
Its OTRExporter/ZAPDTR/libultraship source and the port version
`1.23127.21329` are unchanged from the preserved build verification.

## Publication checkpoint

The authorized target is the existing draft
[PR41](https://github.com/marsh6487/ComboShip/pull/41), branch
`bridge/audit-merge-20261008`. Its description records the exact published
commit/tree and matching Actions status. Keep the prior parent's successful
packages separate from the new candidate's packages. Use one cumulative push
and build after local verification; these results do not promote `develop`.

## Runtime status

The new candidate is not accepted or promoted based on fixtures or compilation.
Remaining game checks are soft eye/chain appearance and HD toggling, Termina
brush/forest tint with the actual installed pack order and Off restoration,
native/replacement night skies, summer visibility, both item tutorials, the
metallic held/GI appearance, Gate sides through a full spin, Sage particles and
vanilla/custom progressive sword transitions. Regenerate both port archives
with the matching candidate executable when testing.
