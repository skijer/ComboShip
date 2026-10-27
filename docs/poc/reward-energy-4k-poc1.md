# Reward energy 4K POC1

Parent: `56c83a878562052ca873b7d414f7d0e1175b6325` on
`bridge/audit-merge-20261008`. Candidate workspace:
`poc/reward-energy-4k-20261009`. This source is now integrated on top of the
overnight bridge `ad8f96ca` for the authorized cumulative PR41 publication.
No master promotion is implied.

The six medallions and three spiritual stones share one scrolling material
display-list fragment. Identity selects an elemental image and matching hex
shimmer. The renderer copies triangles from the selected resource graph,
including supported replacement matrices, so the pass follows the existing
medallion symbols and gem facets. A restrained brushed-gold layer covers the
metal body/setting. Original native geometry remains the base draw.

| Reward | Energy resource | Energy and shimmer tint |
| --- | --- | --- |
| Forest Medallion | forest | #54D85B |
| Fire Medallion | fire | #FF593C |
| Water Medallion | water | #459DFF |
| Spirit Medallion | spirit | #FFA64D |
| Shadow Medallion | shadow | #A675EB |
| Light Medallion | light | #FFE58A |
| Kokiri Emerald | forest | #46E879 |
| Goron Ruby | fire | #FF4564 |
| Zora Sapphire | water | #459DFF |

The two-command material fragment sets a 32×32 logical tile's quarter-texel
offset and returns. It executes directly from aligned graphics-arena storage,
preserving every incoming GPU segment binding. All rewards use the same
128-frame diagonal wrap. The six RGBA energy masks and neutral metal image are
4096×4096, use the accepted V1 OTEX RAW|IMG format, and have unit resource
scales. Their names are private `objects/nei_reward_gi/*` keys.

OoT native rewards and MM foreign OoT rewards query OoT geometry. MM native
imports query MM's selected geometry, with resource selections refreshed on
each draw to follow Alt changes. Both hosts load the shared private material
images through OoT ownership. Missing textures or unsupported geometry retain
the base reward and matching shimmer. Eggs and the Sage's Tunic miniature
medallion fountain retain their previous recipes. No grants, progression,
item IDs, receive animations, held equipment, or scene assets are changed.

Verification completed on this candidate:

- `python3 -B tests/reward_gi/run_tests.py`: policy, real XML/native/hash
  triangle decoding, OoT/MM renderer boundaries, all nine native recipes,
  independent MM/OoT Alt selection, replacement refresh, GPU segment
  preservation, and foreign-jewel near-capacity arena checks.
- `python3 -B tests/elemental_arrow_gi/run_tests.py`: existing arrow/spell
  palette, geometry, native renderer, resource fallback and arena regressions.
- `python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo --sword-toggle-only`:
  existing shared/native/foreign GI checks and native C translation-unit checks.
- `python3 -B scripts/diagnostics/run_sword_mod_gi_tests.py`: both hosts'
  synthetic selected-resource fit regression; external archives were not used.
- Both OoT/MM variants of the new resource helper compile with the real
  Ship/Fast headers. The surface fixture passes AddressSanitizer and
  UndefinedBehaviorSanitizer with leak detection disabled because this runtime
  denies LeakSanitizer access to `/proc`.
- Actual texture headers, dimensions, payload lengths, alpha rules, periodic
  wraps, exact seven-key archive scope, CRCs and hashes pass. A complete second
  generation reproduced all payload hashes and the archive hash.
- Independent static review and `git diff --check` pass.

These checks are implementation and fixture evidence. No full game build,
in-game capture, configuration-specific runtime proof, or user acceptance is
claimed. The previews show texture art only. The next runtime check should
compare all nine rewards in both hosts with Alt Assets off/on, including a
receipt, shelf and freestanding draw, then confirm glyph readability, scrolling,
single shimmer, surrounding scene materials and the existing tunic fountain.

Each uncompressed RGBA payload is 64 MiB; all seven total 448 MiB before GPU
allocation/caching. Runtime memory and frame rate remain part of that check.
The standalone private asset archive is 13.0 MiB and requires this candidate's
renderer changes. Its contents do not override native object paths.

For source transport, the same deterministic archive is committed as
`tools/reward_gi/assets.zip`. A Python-standard-library CMake dependency restores
the exact seven resource hashes before both normal port-archive exports, checks
the private keys against MM assets, and preserves already correct outputs.
The full renderer checks pass on the combined bridge tree. The combined
fixtures reuse the bridge's shared OoT/MM graphics setup helpers; neither
duplicate helper changes production rendering. The CMake packaging fixture
checks clean-source generation, exact hashes, stale recovery, unchanged files,
and private-key collision rejection. All earlier in-game proof limits remain.
