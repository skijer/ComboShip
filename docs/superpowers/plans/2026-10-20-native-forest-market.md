# Native forest and market implementation plan

> Implement in the current isolated worktree using test-first changes and bounded source checkpoints. The user requested execution now.

**Goal:** Supply stock forest layout and shop access through the game assets, with visual packs optional.

**Spec:** `docs/superpowers/specs/2026-10-20-native-forest-market-design.md`

## Global constraints

- No MM3D forest, updated textures, custom texture pixels, masks or poster in the native bake.
- Preserve stock geometry, UVs, collision, prologue, saves, grants and all three game modes.
- Saria is beside the portal; authored placements and the field song giver retain precedence.
- Preserve the protected source baseline and immutable input archives.
- Native forest generation uses the supplied verified stock donor; normal MM ROM assets remain the runtime owner of unchanged resources.

## Review focus

- Port/ROM archive priority must not restore old stump headers.
- Missing optional textures must not disable native entry or select a partial keeper.
- Culling must follow the moved stump and building.
- OoT solo must bind native segments and source native MM resources before entry.
- Real runtime validation remains distinct from extracted-body tests.

## Tasks

1. [x] Audit stock edit, POC9 and FINAL. Confirm source identities, cap/landing, door, three-room framing, stock culling and optional segment ownership.
2. [x] Add failing native actor placement cases; implement a shared collision-checked Saria side pose; run both suites and compile changed units. Commit milestone.
3. [x] Generate the real 39-resource native bundle from the supplied stock donor; verify the complete graph and both asset trees. Controlled graphs remain test-only; no texture pixels are packaged.
4. [x] Add failing production scene-load tests; route combined exploration only to the native resource path and keep original prologue loads. Add complete per-host dependency inventories, native/legacy readiness and stock/optional draw binding paths for both solo hosts. Bind from active mesh ownership, including mixed optional markers.
5. [x] Add native market generation tests; supply geometry/collision/native material fallback to both port asset trees. Add production readiness/keeper tests and integrate vanilla keeper fallback without changing transactions. Preflight and cache the OoT native keeper before entry.
6. [x] Run focused native forest/market/Mirror/actor source checks, native object compilation and asset collision/format checks. Independent review and narrow re-review pass after the three findings below were fixed. Durable recovery verification is recorded separately in the checkpoint receipt.
7. [x] Integrate the supplied stock donor, handle its native Empty material sentinel, retain host-native ambient particles and consume the real bundle through each runtime dependency gate.
8. [ ] Build complete game/port archives and perform configured gameplay/Alt acceptance. Local full CMake configuration stops at missing SDL2 development files; source/asset checks do not establish runtime acceptance.

## Current source checkpoint

The source implementation includes the native portal arch (92 original local vertices, 46 triangles and exact native transform), moved stump ancestors/culling, building, current collision/door framing, complete dependency metadata, native material parameters, both solo adapters and the stock keeper fallback. Stable runtime contracts are generated from the recipes. All custom texture payloads remain excluded.

The first independent review found three important gaps: missing donor-derived dependencies at readiness, a legacy material marker overriding native mesh bindings, and OoT keeper graph failure detected after entry. All three were corrected with failing-then-passing focused cases. Cheap base-archive membership avoids repeated resource loads/logging in readiness checks. Final review and checkpoint verification are recorded in the recovery receipt.

The production forest source bundle is now checked in with a generated inventory. Both normal port trees prepare successfully from its 39 resources, with zero texture payloads. Full archive/executable builds, configured rendering/traversal/Alt checks and user acceptance remain pending. Local root CMake configuration is blocked by missing SDL2 development files. No protected master or installed game was changed.

The supplied-donor review independently reproduced the real bundle, checked both prepared trees and passed all 31 native tests plus focused runtime-body groups. It found no Critical issue and one Important compatibility gap: the old full-scene FINAL pack does not target the new combined native resource identities. That compatibility remains unimplemented; the original broad spec promise has been narrowed to the actual legacy solo path, with the native visual-only contract documented separately. This source checkpoint does not claim the old FINAL combined presentation works.

## Rulings for supplied-donor continuation

- Accept the exact newer uploaded donor alongside the historical donor after checking immutable hashes, native three-room framing and all changed stump vertex records. Cost if wrong: an unobserved stock-version difference would require a donor-specific correction; arbitrary archives remain rejected.
- Preserve OoT ambient fairies through its native Object_Kankyo params0 actor rather than dropping the incompatible MM actor ID. Cost if wrong: host-native particle appearance may need adjustment after configured runtime review; no new texture or custom effect is introduced.
- Treat the temporary CMake asset-gate harness as preparation/dependency verification only. Its named archive targets do not run packers; it is not evidence of complete port archive or game builds.
- Ruling: preserve the user's final geometry-only scope instead of restoring FINAL's full scene or inventing a texture/UV conversion. The review finding is valid for the old combined presentation, but that pack also owns the MM3D scenery explicitly excluded from the native bake. Document the incompatibility and native material targets; leave the old inputs unchanged. Cost if wrong: the user may require a separate visual-only migration before using the existing FINAL appearance. This is a known compatibility limitation, not a completed migration.

## OoT solo fountain report

The user reports their installed OoT solo route still reaches the former Great Fairy Fountain. Source inspection confirms the pink tunnel's existing floor exit remains available when `ForestPack()` fails. The current source instead selects private entrance `0x618`, scene `0x6F`, after the full native dependency gate succeeds. The new `run_solo_forest_tests.py --native` mode consumes the actual generated dependency payload and resource inventories while running both production entry/return/save bodies; both hosts pass without optional scene art. It preserves the existing missing-resource cases. This establishes the source route with the new assets, not the installed game's behavior or the cause of that user's observation.

The native OoT forest/keeper still require the normal stock `mm.o2r` owner. OoT solo's general setup permits running without that archive; this MM area is then unavailable. Do not describe that case as working without MM base resources, or imply that a supplemental Lost Woods archive can replace that owner. Full installation/runtime diagnosis remains pending.
