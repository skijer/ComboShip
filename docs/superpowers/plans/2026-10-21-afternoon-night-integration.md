# Afternoon source integration

Goal: merge the verified afternoon source checkouts into the current
`marsh6487/ComboShip:develop`, retaining the existing executable-build hold.
Scope and evidence: `docs/poc/afternoon-night-audit-20261021.md`.

## Constraints

- Preserve baseline `66bde021ccd651a3728db2338251961b1a44c9f7` for rollback.
- Retain the reviewed runtime/test bytes, current workflows and baseline tasks.
- Exclude new recovery ZIPs, checkpoint receipts and optional asset inputs.
- Keep durable recovery separately; do not install archive/model candidates.
- Distinguish saved native verification from new publication identity checks
  and from configured gameplay acceptance.

## Completed composition and verification

- [x] Compose PR53, PR54, PR55, title cards and the native Fairy source.
- [x] Run combined entrance, crash, world, forest, effect and CI-policy checks.
- [x] Compile the 13 affected native production objects.
- [x] Independently review and restore the original verified implementation.
- [x] Restore the saved candidate without changing its source bytes.

## Source-only publication

- [ ] Exclude new checkpoint entries and supplemental recovery packages.
- [ ] Compare every included production/test/tool file with the verified tree.
- [ ] Verify workflow preservation, regression inventory and source-only scope.
- [ ] Publish the source-only candidate and merge using the expected head.
- [ ] Verify the final remote tree and report the merge and runtime limits.
