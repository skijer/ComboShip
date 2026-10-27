# Complete Morning Integration Implementation Plan

**Goal:** Publish one recoverable ComboShip candidate containing the October 23 original fixes, Rod stowing, regional passages, completed spin styles, native title retouch, and optional vanilla held swords; run and monitor combined CI.

**Architecture:** Restore the saved combined source checkpoint `2fe6a9c51a533d113ac7fc389268ccdd54c13f58`, based on regional checkpoint `9747ea7bf9d1dc149a1855b3627dd93bad11d416`. Merge verified held-sword contribution `acb00a3ac51350ea0592109b881c0f1e6f136f83`, preserving both parents and every subsystem's existing behavior.

**Tech Stack:** Existing C/C++, native ABI regression fixtures, deterministic resource preparation, GitHub Actions.

**Spec:** Existing `2026-10-23-elemental-spin-styles-design.md` and `2026-10-23-native-title-parity-design.md` in the sibling specs directory, the vanilla-held-swords plan, and cor's request to combine and build all audited morning source work.

## Constraints and review focus

- Leave develop/master unmerged. Preserve all original morning fixes and existing texture parts.
- Preserve every spin style, elemental combat/visual independence, both cosmetic channels, both games, and solo/Combo guards.
- Ship native title artwork in normal assets; retain the optional smooth HD replacement and native fade/override rules.
- Keep the shared live vanilla-held-swords control, default authored appearance, Four Sword fallback and custom equipment priority.
- Preserve Rod A-button stow, all three regional routes/returns, colored shimmer and nearby waves.
- Scene actor-cleanup archives remain separate replacements; no source change or executable rebuild is required for them.
- Keep CI's full required matrix, deterministic assets and all regression registrations. Distinguish source/harness/build evidence from configured runtime acceptance.

## Task 1: Restore and combine

- [x] Restore combined checkpoint from the durable incremental Git bundle into an isolated checkout.
- [x] Inspect repository AGENTS.md and compare saved spin/title identities with actual source history.
- [x] Run the real held-menu test against the unmodified combined baseline and observe absent-checkbox failure.
- [x] Merge `acb00a3` without dropping combined source changes; inspect all conflicts and the full merge diff.

## Task 2: Verify and preserve

- [x] Run spin policy/resources/render/native/combat checks normally and with ASan/UBSan.
- [x] Run title asset/preparation/presenter/market/forest checks, Rod/region checks, held-sword and Four Sword checks, and CI inventory/formatting checks.
- [x] Review shared input, rendering, resource preparation and CI boundaries independently.
- [x] Preserve all 219 spin resource files byte-for-byte through a content-addressed pool, split into 106 base64 parts of at most 128 KiB. Restore with the standard library before collision checking and both archive generation targets; verify real CMake ordering, corruption rejection, bounds, repair and idempotence.
- [ ] Commit the combined candidate, save a recoverable bundle and exact source/tree identities, and independently verify restoration.

## Task 3: Publish and build

- [ ] Publish missing assets individually with blob hash verification, then publish the exact combined tree/commit to a candidate branch and fast-forward the cumulative branch with a lease.
- [ ] Update the cumulative draft PR to describe the final source and outstanding runtime acceptance.
- [ ] Run and monitor full Windows/Linux and regression CI; diagnose, verify and publish scoped fixes if needed.
- [ ] Report actual final run/commit status and any remaining configured runtime checks.

## Integration decisions and evidence

The clean held-sword merge is `de7d0ad7accc2937565c2dc3536cafae50c316ca`; its source tree is `98d7898c3f74fda9fc4f75118e22fa2ee3df8a6a`. A durable incremental source bundle was saved and independently fetched into a separate bare repository using only public prerequisite commits, with the same tree and no object alternates.

Independent source review found one Important issue: the full spin renderer/combat harness ran before preflight installed native headers. Local execution reproduced missing `nlohmann/json.hpp`. Keep policy/resource checks in preflight and move the complete normal/sanitized gate into the existing provisioned render regression group; all six groups and both platform builds remain required.

The expanded spin tree contains 219 paths totaling 188,684,244 bytes, with 73 unique byte sequences. Direct binary blob publication would recreate the earlier large-upload problem. The existing authorization explicitly calls for bounded individual parts. Store a lossless pooled source with per-part, compressed-pool, decoded-pool and per-resource hashes. No resolution, texture metadata, geometry, UV, material, gameplay or resource name changes are permitted. The original source bundle retains the raw resources for recovery. Generated resources remain at their original paths for both packers and are ignored by Git after preparation.

The new bundle-loader tests first failed because preparation was absent, then all four passed, including a real CMake dependency target and independent verification of every restored resource against the original spin manifest. Normal/sanitized full spin, held-sword/aura, native titles, original morning fixes and regional native routes passed in this environment with the appropriate native headers. Native regional presentation is sanitized by default; its CLI does not accept a `--sanitize` switch.
