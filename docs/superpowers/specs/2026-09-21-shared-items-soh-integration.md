# Shared Items and current SoH integration

## Intent and inputs

cor wants Varuuna's Shared Items update in their ComboShip fork, together with the current running SoH feature stack. They confirmed there are no saves/files requiring migration; retain upstream new-seed sharing semantics.

- Protected ComboShip baseline: `marsh6487/ComboShip:integration/soh-stabilized-20260912`, `cac526a8b9dc08c83f4c2ad7fece0113d284513d`. Windows build 34722809210 succeeded. This does not establish complete runtime acceptance.
- Upstream: `Varuuna/ComboShip:develop`, `94eb185e4abcc2d568aa8241fa02c43cdd86c439`, containing Shared Items PR 207 merged September 17.
- SoH donor: `marsh6487/Shipwright:poc/pedestal-stump-clearance`, `6f03c439ab83cbf149c4a368b529e53724f6fc47`. Build 35539499857 passed Windows, Linux, macOS and asset generation. Latest pedestal framing, prompt and clearance remain runtime candidates. A later pause-crash report is a known outstanding issue, not a proven fix in this donor.
- Previous SoH port donor: `fcc86528612ecabb192fe3642acaacf7c2766869`, recorded by the September 12 ComboShip integration report.
- Candidate: `integration/shared-items-soh-20260921`, based on the protected ComboShip branch.

## Ownership and preservation

ComboShip owns launcher, cross-game transitions, inactive save state, multiplayer, shared-items runtime, dual resource contexts, per-game clocks, MM, and platform/DLL build layout. Preserve the existing NEI expansions, progressive chains and Fleet shared counterparts. Combine those mappings with the new seed-selected shared families deliberately: do not trim the same pool copies twice or count the same acquisition twice.

Port the SoH donor delta onto ComboShip's OoT code. Preserve weather/BGM/SFX concurrency, actor adapters and optional-model fallback/lifetime, native Prelude material bindings and cache, PAK equipment selection, cosmetic models, Time Gate behavior and local time pedestal. Shared plumbing must retain host ownership; do not replace the complete SoH subtree. Scene/model archives are separate and unchanged by this code integration.

No change to randomizer upstream pins or save-version derivation is warranted merely by this custom donor port. Preserve upstream's per-seed shared mask, old-seed mask-zero behavior, Bombchu directionality and tier limits. Do not add existing-save migration.

## Verification and delivery

Run production-code tests covering combined shared pool/oracle behavior and progressive grants; port and run focused actor, audio/weather, material/cache and pedestal regressions. Validate conflict markers, formatting and asset paths, then run full Windows/Linux CI via a draft PR to the existing integration branch. Keep failures explicit. An integration candidate is build/static verified only when its named checks pass; gameplay acceptance remains separate.

Runtime checks: a new seed with selected shared families; pickups in both games; progressive upgrades without duplication; OoT/MM round trips and save reload; NEI equipment; pause/menu stability; static actors and materials with Alt assets; local sword ceremony; weather/music after game transitions. Preserve the original branches for rollback and do not promote the candidate as a proven master automatically.
