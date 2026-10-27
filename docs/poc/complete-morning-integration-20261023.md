# Complete October 23 morning source candidate

This candidate combines the original cumulative fixes at `523e71e949dd1e9d18db1fb47cf17ecbbc24b710`, Rod/regional checkpoint `9747ea7bf9d1dc149a1855b3627dd93bad11d416`, final spin handoff `f1c97f61adaad055232592d1413686bf1d673b84`, native-title handoff `4ef0c048ea4caa3a6e436a6f0ffdfe4b0f3953b0`, and vanilla-held-sword contribution `acb00a3ac51350ea0592109b881c0f1e6f136f83`.

Included: morning Black Market camera/floor/dialogue and cape fixes; monkey messages/audio; seasonal OoT rain preserving Song of Storms; Blast Mask tutorial/native A action; sword ice material/audio fixes; Rod A stow; all three regional routes with returns, shimmer and waves; eight independent spin styles with elemental visual/combat separation and live cosmetic channels; built-in native OoT typography for 67 MM scene titles plus market/forest; optional shared live vanilla-held-sword preference with custom equipment priority.

The optional smooth HD title pack replaces the same built-in title resources. It remains separate; normal native typography needs no optional pack. The corrected Lost Woods/Zora's River actor archives also remain separate scene replacements and are not compiled into this executable.

## Exact asset preservation and bounded publication

The spin source originally expanded to 219 resources totaling 188,684,244 bytes. Its 73 unique byte sequences are pooled losslessly and stored in 106 base64 parts, each at most 128 KiB. Per-part, compressed-pool, decoded-pool and per-resource SHA-256 checks protect restoration. Every restored path/hash matches the unchanged original `tools/spin_styles/manifest.json`; no texture resolution, pixels, UVs, geometry, materials or runtime resource names changed.

`scripts/assets/prepare_spin_styles.py` restores both generated trees before collision checking and archive generation. `CMake/SpinStyleAssets.cmake` is a prerequisite through `CheckAssetCollisions` for both `GenerateSohOtr` and `Generate2ShipOtr`. Fresh checkouts require no private donor archive or third-party Python package for restoration. For intentional resource re-authoring, regenerate from the preserved inputs, then run `python3 -B scripts/assets/prepare_spin_styles.py --pack` and the bundle tests before committing.

## Verification observed in this integration

- Full five-gate spin runner normally and with ASan/UBSan: PASS, including both complete native actor translation units in solo/Combo configurations.
- New source-pool tests: 4/4 PASS, including all219 original resource hashes, bounded parts/decoding, corrupt source rejection before writes, repair/idempotence, and actual CMake ordering before an asset consumer.
- Native title assets: 5/5 PASS; location presenter/resource bridge, both market title suites, opening forest, and OoT/MM title/return fixtures: PASS.
- Rod native A stow and lifecycle, regional native endpoints/handoffs/boot/returns, and default-sanitized presentation/wave fixtures: PASS.
- Held equipment, Four Sword visibility, both actual cosmetic menus and sanitized elemental aura: PASS. The unmodified combined baseline failed the added actual-menu test in both hosts; the merged source passes.
- Monkey, seasonal storm preservation, Blast Mask, cape, shop browsing, deterministic ice assets and sanitized native ice materials: PASS.
- CI inventory/routing/failure-handling tests: 21/21 PASS. Deterministic sword UI assets and custom asset-collision gate: PASS.
- Actual `run-clang-format.sh` with clang-format14.0.6 formatted 2,154 files without source differences; `git diff --check`: PASS.
- Independent feature integration review identified the spin preflight dependency issue. Full native normal/sanitized checks now run in the existing provisioned render suite; policy/resources remain in preflight. All six regression suites and both platform builds remain required.
- Independent bounded-storage review found no additional actionable findings and independently decoded all219 exact resource hashes.

Native fixture execution and source review do not establish full Windows/Linux builds, configured GPU/audio behavior or gameplay acceptance. Combined hosted CI and configured runtime traversal/appearance checks remain separate gates. Keep develop/master unmerged.

## Recovery

The incremental recovery bundle requires public prerequisite commits `9747ea7bf9d1dc149a1855b3627dd93bad11d416`, `acb00a3ac51350ea0592109b881c0f1e6f136f83`, `3c3cd4fe0c37ccfb009c2d138773e185d977c174`, and `21d278219bf95dfbf5f0b5e9b21c324b3a7cc6d8`. Fetch these in an independent clone, verify the bundle, then fetch its named candidate branch. Earlier source handoffs retain unchanged raw resources and donor input identities. Remote publication must independently match the verified candidate tree and every uploaded Git blob SHA.
