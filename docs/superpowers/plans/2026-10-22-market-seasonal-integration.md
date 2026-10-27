# Cumulative Black Market and seasonal tint integration

> Execute inline with the executing-plans workflow; preserve the already reviewed implementations.

**Goal:** Resume publication of the cumulative Black Market candidate with PR #56's seasonal paving grass treatment included.

**Architecture:** Merge published PR #56 without rewriting either implementation. Preserve the complete market/forest candidate and the current develop baseline; publish the resulting candidate for review.

**Tech stack:** C/C++, native OoT/MM resources, Python asset preparation, Git.

**Spec:** `docs/poc/market-forest-followup-20261022.md` and PR #56 (`d21ded5c9976f43db485d8f9c2b05e6fce62d5c4`).

## Constraints and review focus

- Market parent: `2b8fa033b7f6c86554aeab4a7033db7b0fa7552b`; develop: `c3b0c76b81766ce51378e573dcf9816ab3c59473`.
- Preserve market dialogue close/re-talk, camera cleanup, title border/fade, exit door, native destination titles and prior forest/Mirror/entrance work.
- Copy PR #56's four files exactly; target only `s00_road_01_41180` and `s00_road_02_46580` with the stricter grass mask. Preserve stone, alpha, source allocation and summer restoration.
- Preserve native/Alt resource routing and ordinary terrain's existing mask/cache behavior.
- Preserve source archives, geometry, collision, actors, weather/audio and workflow/task inventory. No recovery ZIPs or unrelated NPC archives enter this integration.
- Source publication is authorized. Configured gameplay, full executable linking and runtime acceptance remain separate evidence; existing executable-build hold is retained.

## Task 1: Merge and verify the integration

- [x] Recover exact source parents and confirm develop has not advanced.
- [x] Merge PR #56 into the existing isolated market candidate without conflict.
- [x] Verify PR #56 files are exact and the complete 118-file market change is retained.
- [x] Run seasonal native/HD binding sanitizers and actual uploaded paving-pixel checks.
- [x] Run native market browsing/text-release, door/title, entrance and asset checks on the combined tree.
- [x] Retain prior native compilation evidence when source/dependency hashes remain exact; compile the changed seasonal production unit.
- [x] Obtain a fresh integration review and resolve blocking findings.
- [x] Commit the verified merge and record results in the POC handoff.

## Task 2: Finish publication

- [ ] Publish the complete candidate source history and assets to `poc/market-forest-followup-20261022`.
- [ ] Verify the remote commit/tree and PR #56 ancestry; create the cumulative draft PR against develop.
- [ ] Report the exact published candidate and remaining configured runtime checks.

## Decisions

- Resume existing source work instead of reimplementing it; PR #56 and the market candidate have disjoint changed paths.
- Publication completes the durable source checkpoint. Do not add a recovery ZIP.
- Prior tests are retained evidence only. Fresh integration checks distinguish source verification from gameplay proof.
- The only fresh test error was the runner's missing CMake PATH; correcting its environment made the exact packaging case pass. Two optional donor skips retain their prior evidence.
- The first publication attempt was automatically rejected for current-conversation authorization; cor then explicitly approved the push to the named repository. No alternate publication route was used.
