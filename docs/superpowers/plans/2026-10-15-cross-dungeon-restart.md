# Cross-game dungeon entrance restart

Spec: `docs/poc/cross-dungeon-restart-20261015.md` (existing approved scope).
Execution: inline, continuing the user's implementation authorization.

## Global constraints

- Base `a82852a3`; isolated candidate; opt-in defaults off; no master promotion.
- Compose native OoT shuffles, including mixed pools, rather than replacing them.
- Seed route ownership is game-tagged; invalid enabled data fails closed.
- Foreign dungeon access must not imply destination-overworld access.
- Preserve age/time, pending rewards and existing portal behavior.
- Persist source checkpoints at boundaries and roughly every ten minutes.

## Tasks

1. Route model and deterministic permutation: shared `combo/rando/CrossDungeonEntrances.h`;
   behavioral tests for disabled identity, native-map composition, both directions,
   coupled/decoupled returns, deterministic replay and malformed data rejection.
2. Native graph adapters: export eligible endpoints; install graph disconnections and
   foreign arrival roots; expose reachable outgoing routes without granting overworld.
   Test real source adapters and cross-oracle fixed point, including retry restoration.
3. Launcher integration and persistence: generate after native entrances; feed routing
   into combined fill/playthrough; store versioned routes in consolidated seeds; restore
   loaded seeds transactionally. Cover disabled seed comparisons and stale data.
4. Runtime handoffs: guarded transition interception in both native hosts; destination
   title setup; route-specific return/death/save/boss warp recovery; preserve pending
   rewards, age and MM clock. Test native adapter behavior under sanitizers.
5. Checkbox and game-tagged labels; headless switch; relevant regression checks, native
   build and source checkpoint. Request one final independent whole-branch review.

## Review focus

- Mixed grotto/boss endpoints and decoupled returns must match runtime and graph.
- Dormant saves, retries and seed reload cannot retain another seed's routes.
- Native boss-warp progression/rewards must finish before game handoff.
- Save/death/reset during foreign visits must recover to a valid owned-game route.
- Checkbox-off seeds must preserve the prior RNG stream and access gating.

## Progress

- Checkpoint 0: fresh baseline, candidate and preservation contract established.
- Tasks 1–5 implemented in isolated candidate; independent review identified six Important issues,
  all corrected. Focused tests, sanitizers, source-extracted regressions and native syntax pass.
- Owned takeover at `da82ae67` preserves the recovered checkpoint 13 and verifies both
  MM boss-warp corrections with production-function regressions and sanitizers.
- Native incremental compilation/linking and Linux AppImage packaging complete. Linked and
  packaged-module probes, deterministic generation/replay, disabled generator parity and five
  native start/shuffle profiles pass. The headless affordability canary now honors existing NEI
  shared wallet groups; all five corrected forward traversals pass.
- The original mixed coupled-plus-Ganon seed also fails the original disabled generator's
  All Locations Reachable check. Its rejection is preserved; a controlled coupled 16-pool case
  passes in both generators. Detailed evidence and limits are in owned checkpoint 2.
- The final independent whole-branch review reported no actionable Important/Critical findings
  within its bounded scope. Candidate remains isolated; Windows and installed-game acceptance
  are unverified. No develop/master promotion.
