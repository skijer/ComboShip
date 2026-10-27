# Cumulative weekend recovery — 2026-10-19

Historical checkpoint record. See `final-cumulative-audit-20261019.md` for the
current cumulative source inventory, selected NPC revisions and verification.

Status: recovery record only. The cumulative source upload is not complete, and this checkpoint does not claim a linked build or gameplay acceptance.

Repository: marsh6487/ComboShip. User authorized publication and periodic recovery checkpoints on 2026-10-19.

Published parent: PR44, integration/big-merge-20261016, commit 43131342882972eeb4a257599ec8b061c7dc6d43. Preserve develop a82852a3a02c8cabc541cce7c124da71c5755efb until the candidate passes its required gates.

Restore the exact weekend source from ComboShip-Weekend-Merge-Ready-20261018.zip (Library libfile_7305733ebf108191ab8ef4f1bb434e88), importing weekend-source.bundle over PR44. Expected source 520d7b6dc64ab5742226b9b2f998a34bcd6b4a9e; tree 35535ac14ed5c3324655f79dd09bcacdd4e2de36. Follow the README in that kit. Its audit identifies all retained Friday/weekend features and external packs.

Additional source donors:

| Input | Exact head | Recovery file |
| --- | --- | --- |
| Elemental swords, selector/receipts, built-in auras and MM ice arrows in OoT | 4b39ddeb9cf51c573164bf1d7fa7265c7482a6db | ComboShip-Elemental-Swords-MM-Ice-Arrows-20261019.zip; libfile_ff16d39401f88191bc9e67c321a9d5fe |
| Chateau 3DS model, gold/burgundy shimmer, tutorial preservation | a4367a1070dc2d8965fccc92e84c3c876dc202a0 | ComboShip-Chateau-Romani-MM3D-Shimmer-20261019.zip; libfile_f05f0150e6cc8191809f8dd5b3530845 |
| Spring blush grass | 81707c123d9397f8689edb4547a954fab90d2146 | ComboShip-Spring-Grass-Blush-Source-81707c123.zip; libfile_dd785268e66c81918c550c6dade33c52 |
| Always Thawed Zora's Domain, immersion only | 7d58e2c4f8aa16d55f681e24289a8b3a708e606a | ComboShip-Always-Thawed-Zoras-Domain-20261019.zip; libfile_7709050dfa9c8191a11d6ed19c94d030 |

Each donor shares the weekend parent. Apply only its bounded delta. Retain all canonical-runner entries when reconciling overlaps. Never replace the cumulative tree with an older donor snapshot.

Latest NPC work to reconcile: accepted Anime Saria POC12 and Traditional Saria POC11; accepted twins POC9; young Malon iris POC12 candidate (final acceptance and in-game checks outstanding); preserve adult Malon POC5, blink, tracking, and Fado POC2. The latest NPC Python reconciliation completed 122 tests successfully in scratch before interruption; it is not yet applied to the cumulative branch.

BossSoul-Flame-GI-Control-20261019.zip is an optional A/B diagnostic, not a confirmed fix or a default installed pack. Keep approved flame artwork unchanged.

After restoration: apply clang-format 14.0.6 using repository selection; run the canonical regression gate, repair missing extracted-fixture declarations without changing production semantics, then build matching executables plus both generated port archives. Keep full runtime and external scene-pack recovery holds explicit. No gameplay master promotion is implied.

Interruption at approximately 03:23 America/Chicago: local branch integration/cumulative-weekend-20261019 had reached d4a643ee (elemental donor and first Chateau commit). Pending sequencer: Chateau tutorial guard, Spring grass, Domain. Local audits and reconciliation inputs survived; background tasks stopped. Continue the saved sequencer rather than reapplying completed commits.
