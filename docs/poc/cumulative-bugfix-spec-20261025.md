# Cumulative ComboShip bugfix publication — October 25, 2026

The user authorizes consolidation, validation and public publication to
`marsh6487/ComboShip` of the latest completed source checkpoints for this pass.
The required room-material What's New font accents are included.

## Baseline and preservation

Use public `83eed9fff57d7b5dfa1f27ecf5fbe6b65e411832`, complete tree
`54ca0f4860ea91affc0498d8f36c8d5b03872097`. Its Build Artifacts run
40169007871 passed Windows, Linux, all six groups and the actual final gate.
The separate Windows preflight 40169007701 passed. Keep that candidate,
PR #61, earlier cumulative PR #59 and all accepted source/scene inputs intact.
Keep develop at `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`; master was absent.
Neither develop nor master may be merged or promoted.

## Completed source inputs

| Input | Original checkpoint commit | Integration scope |
|---|---|---|
| Black Market refinement v2 | b090cb26960975ab57164378cbb60a1a925960a8 | Rear props/steam removal, forward shelves, selected 1.5x stock, counter and Granny repairs |
| Spin themes | dd4c758eb474eb905047149bd06f2fe25f46ff10 | Theme identity at both spin levels; original native actor behavior |
| Ikana native GI/icon v1 | a23e60103ffb72309045e97b10f2222603016451 | Native MM GI priority and session-aware icon/foreign-owner routing |
| Fountain effects v1 | 6469c2f4ff119503df0403506e85083772b77223 | Transparent snow/mist, full-valley shimmer, normal bridge arrival; includes prior a69ac0da repair |
| Magical Room Materials accents | 519f63380e4355ddc560ca15d1461afdb71f539d | Color-only gold accent on checkbox and four independent room colors |
| Domain silver-hook POC1 | c9b6289a31525fb9f4a14684f3fa04a0af5b1bd5 | Missing dungeon/group safety while preserving native unregistered actors |

The five supplied full-tree expectations independently reconstruct from the
public baseline. The silver-hook input provides no original complete tree;
its production baseline is byte-identical to the public baseline and its
bounded patch independently applies. Its full restored tree is recorded in
the inclusion ledger. Preserve the accepted R9 caustic archive unchanged.

Retain geometry, collision, UVs, native setup headers, waterboxes, progression,
standalone/combined ownership, source/asset guards and previously accepted
integrations outside each checkpoint's explicit scope. The older Snowhead
transparency archive is subsumed by the latest Fountain checkpoint; do not
apply the older evening-cumulative source over the passing baseline.

## Publication contract

Use isolated candidate/cumulative branches with recoverable input hashes,
restore recipe, source checkpoints and an inclusion ledger. Use the proven
lossless zlib/base64 pools, at most 96 KiB decoded and 128 KiB encoded per part.
Reuse unchanged parts and verify reconstructed assets and the complete remote
source tree. Do not publish saves, dumps, private logs or original game archives.

Retain every required regression assertion and CI gate. Run formatting,
source/asset preflight, world/entrances/items/equipment/systems/render groups,
full Windows/Linux builds and the final required-check gate. Correct in-scope
failures and rerun against the final candidate. Verify the actual CI checkout
tree equals the published candidate tree.

Deliver the complete matching Windows package only after every required check
passes: Fleet.exe, runtime DLLs, soh.o2r and 2ship.o2r. Record CI/artifact links,
package/per-file hashes, installation notes and focused gameplay checks.
Build/static verification and configured game/user acceptance remain separate.
Final promotion requires the user's separate acceptance.
