# Accepted NEI presentation and MM pickup rendering

## Baselines and authorization

- ComboShip parent: `ae2da508b60bedbde117432a6ce13ff0ab1a22dc` (PR 23, cumulative performance and PAK/menu fixes).
- SoH donor: `2ce2e5a28098261943c25f8a1ecebe730c1d06ea` (PR 17), relative to `628deea3cf851af19a18f915e0665d772294c383`. This includes the final local-player first-person correction on top of the accepted `5fadb2d` candidate.
- The user accepted the current third-person rod fit and reported that the latest Deku Leaf runtime no longer crashes. Din model wrist orientation is a separate task.
- The user authorized the first-person visibility fix and combined ComboShip push, including the previously queued MM pickup GI/icon correction. Master promotion is outside this task.

## Work plan

1. Hide all three local held rods while aiming in first person, retaining projectile/trail submission; exercise both ages and fallback paths.
2. Import the accepted SoH code/assets without replacing ComboShip's vendored dependencies or cumulative game integrations.
3. Reconstruct the waiting MM pickup fix: correct owning-game icon texture and dimensions; route custom GI presentation instead of native bottle row zero; preserve progressive award tier and fallback effects.
4. Run focused producer/consumer tests, source compilation, asset collision and preservation gates. Request an independent review of the changes before publication.
5. Publish the SoH correction and a stacked ComboShip candidate, then start and inspect runtime builds.

## Evidence ledger

- SoH hand fixture: baseline passes; new aiming assertions fail against the original drawer; corrected Fire/Ice/Light drawers pass, including single-shot and remote/third-person checks.
- Accepted donor patch applied cleanly to 657 files. Combo-specific vendored dependencies and existing workflow remain intact.
- The earlier MM item-presentation checkout/commit is absent from the restored workspace and was not published. Its diagnosis was recovered and the implementation reconstructed and verified anew.
- All 21 concrete NEI GI presentations route through native MM display-list submission, retaining opaque/energy/translucent layers, archive path lifetime, missing-resource fallback and legacy Cane effects. The SoH mesh submission body is shared without modifying its batcher behavior.
- The MM pickup message uses the owning game's resolved icon before granting the item, with its real dimensions and RGBA32/IA8 format. Custom GI callbacks cannot fall through to the native empty-bottle row.
- Progressive Cane presentation follows the native skill order for all 64 ownership masks. Raw previews resolve the current tier; a queued award keeps its pre-grant concrete tier. Concrete Roc Feather and unrelated items retain their identity.
- Passed: actual owner-descriptor/MM draw fixtures, icon producer-to-consumer fixtures, ASan/UBSan progression and icon tests, full affected C++ translation-unit syntax checks, and native MM message/rod C syntax checks.
- Passed: Fire/Ice interpolation including disappearing volleys, reused slots and impact heading; accepted Light charge/focus/release comparisons across 180 phases; Fire's Light-style charge using its private fire texture; Leaf, Whip, Lantern, Switch Hook, time-gate and item-stow regressions; exact resource/GLB parity for 21 GI models and 22 held components.
- Passed: PAK visibility, nested voice discovery, shared-item and grant-boundary preservation tests; asset collision check (1,733 shared paths, 181 known differing-content collisions, no new collisions); clang-format 14 and whitespace checks. Vendored dependencies and renderer performance implementations are unchanged.
- Independent reviews found and closed clone visibility and progressive Cane defects. Final review reports no remaining blocker within this scope.
- SoH PR 17's cumulative regression gate and formatting passed; its final first-person runtime build was started. ComboShip's existing platform workflow remains the build gate, with the new focused regression suite added.
- In-game proof for the new first-person hiding and ComboShip integration remains pending. Source tests are not runtime acceptance.

## Native MM held/use completion (2026-09-29)

The earlier accepted SoH use/effect evidence above did not establish native MM held rendering. PR 24's original MM work covered GI/pickup/icon/progression; the follow-up completes the actual native held/use paths. No extra artwork is required.

- Task 1 connects native MM Fire/Ice/Light wrist fits, held/tip/charge/release/trail/burst presentation, local first-person hiding, owner-aware resource routing and stable projectile interpolation identity. Accepted Light output and Ice trail material stay unchanged.
- Task 2 connects Leaf, Ball and Chain, Shovel, Mitts, Gust Jar, Beetle, Spinner, original Somaria, Whip, active Hook actor, Lantern and Time Gate presentation. Native human/LOD and final adult/custom hand selectors supply the fist/clasp. MM `save.linkAge` remains zero; fit selection reads the ready rendered rig through `AdultLink_UsesAdultPresentation`. Feather/Cape/Minish remain GI-only, and Din rig work is out of scope.
- Task 3 adds the native stow bridge and outgoing-action sound deduplication, persistent Lantern pocketing, rod teardown, held-button release suppression, native item-change input blocking, inactive Ball and Chain cancellation protection, raised Whip arm through native first-person release, and immediate Switch Hook selection/swap at the active `Arms_Hook` seam. Native MM save/form/input/camera/collision/audio structures remain authoritative. Stow cleanup ignores remote players; input/sound caches reset on relevant owner/frame-lifetime changes.
- Leaf already had native one-shot upper animation, missing-resource/no-magic checks and safe completion/collider cleanup; executable native tests now cover them. Native `MmSfx_Stop` remains its existing no-op compatibility stub, with no SoH bank/audio fix imported. Time Gate active prop suppression was already connected by Task 2. Native Ball and Chain breakables, inertia and late pose restamping are preserved. Rod inactive swing sentinels remain `0xFF`.
- Passed the cumulative `run_combo_nei_regressions.sh` gate, including native MM production-path presentation/lifecycle tests and accepted SoH suites; MM item visuals, PAK visibility, voice discovery, shared-item/grant checks, and asset collision checks also pass. Full native player unity and active Hook actor compile against production MM headers. New lifecycle tests are invoked by `run_mm_nei_tests.py`, already included in the existing CI runner. Changed-line clang-format 14 and whitespace checks pass. This is component/command/source evidence, not an in-game acceptance claim.

Install the new combined binaries and **both matching port archives (`soh.o2r` and `2ship.o2r`) together**. Native MM reads the accepted bundled assets in `soh.o2r` through the new SoH owner export. There is no separately generated NEI add-on O2R to install. Existing ROM-derived archives and saves remain separate user files.

The controller still owns independent final review, publication of the exact tested tree to PR 24, the complete remote gate, and Windows/Linux build start. Full platform builds require dependencies absent locally. Actual child/adult/custom/transformed presentation, first-person transition feel, sound playback, repeated/interrupted use and network scenes require the new build and user testing. No merge or master promotion is authorized by this evidence.
