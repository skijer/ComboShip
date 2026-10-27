# CI fail-fast recovery checkpoint

Base: `develop` at `54c0e5d2c4302116f998142284b502da2191a40a` (tree `2995b61682e497984fc215d6d2b7b00939998730`). Candidate branch: `fix/ci-failure-visibility-20261021`.

The base's hosted run [39639205569](https://github.com/marsh6487/ComboShip/actions/runs/39639205569) compiled both platforms successfully. The `world` regression failed at about five minutes because forest draw fixtures still referenced removed `ForestGiParticles::kHexVertices`; the final gate waited until the Windows build finished at about 62 minutes.

The candidate puts both builds and all six existing regression groups in one native fail-fast matrix, retains their existing implementations and command inventory, emits immediate regression errors, and links final diagnostics to the original failure. The first checkpoint deliberately retains the existing broken forest fixtures so a draft-PR run can prove that a real regression stops both builds. That checkpoint is not mergeable. A subsequent checkpoint must update and run the stale fixtures, preserve production game code and assets, and pass the CI helper/graph checks and workflow lint before promotion.

The remote candidate branch and its PR retain the complete source tree. Recover it with `git fetch origin fix/ci-failure-visibility-20261021` and `git switch --detach FETCH_HEAD`; compare the commit/tree recorded in the PR. The base remains independently recoverable by its exact commit above. Reverting the bounded follow-up commits restores the earlier parallel workflow and fixtures; no game assets, generated archives, upstream pins, build targets or release workflows are changed by this follow-up.

## Fixture repair

The second checkpoint replaces the two obsolete particle vertex-count constants with the current `kTwinkleVertices` and provides the message-engine stubs now required by the actual Saria update body. The lifecycle fixture exercises event text waiting for input, actor-owned textbox closure and deferred reward delivery, while retaining its original placement, color, bounds, duplication and save assertions. Production code is unchanged.

Local verification passed: all 21 CI helper/graph tests; full workflow actionlint; forest actor ABI/resources/lifecycle/save checks; both MM and OoT solo forest draw checks; and the full forest GI particle suite, including ASan/UBSan native draw/renderer checks and independent NEI geometry/palette comparisons. The remote first checkpoint was fetched into a fresh bare repository and reproduced commit `68046b5ecb6992d56a1b14c4f275eeb18359ed97`, tree `b8c1a65310d5e3d77a0895579bf68830729b28b1`.

Source checks and native fixture tests do not establish configured gameplay acceptance. Hosted cancellation and the subsequent complete CI result must be recorded separately in the PR before this work is described as verified.

## Hosted cancellation proof

Draft-PR run [39650821732](https://github.com/marsh6487/ComboShip/actions/runs/39650821732), source `68046b5ecb6992d56a1b14c4f275eeb18359ed97`, started at 18:13:47 UTC. The original forest fixture failed at 18:19:09 (5m22s). Windows stopped at 18:20:00 and Linux at 18:20:12. All five other regression groups were cancelled by 18:20:12, within 63 seconds of the failure. The final status failed at 18:20:23 and the run completed at 18:20:24 (6m37s total), rather than waiting for hour-long builds. Its log/summary linked the original forest job and failed step and separately counted seven cancelled checks. No manual cancellation was used.
