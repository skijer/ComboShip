# Cross-game return title cards — 2026-10-21

| Field | Record |
|---|---|
| Baseline | Published `integration/morning-roundup-20261021`, commit `724e0476b1dd2555f40faf7cc01b8d1cf0329ecd`, tree `f4d2092d09a511d030de1b5007188a9e7849b883` |
| Candidate | `fix/cross-game-title-cards-20261021`, isolated from the other ongoing fixes |
| Scope | Native destination title-card request on MM→OoT cross-game returns, including both forest bridges, Woodfall, composed entrance shuffle/boss/grotto arrivals and cross-game Farore recall |
| Preservation | Existing destination composition, spawn positions/rooms, respawn modes, transition fades, save contents, title art/language/layout/lifecycle, explicit hide-card setting and solo SoH behavior |
| Assets/configuration | No new assets, archive dependency or load-order change. The native destination texture continues through the existing resource manager and Alt/Djipi lookup. |
| Status | Implemented; focused sanitizer checks, native object compilation and review passed. Configured gameplay and full executable builds remain untested. No runtime master promotion. |

## Cause and change

The cross-game boot bypasses file select's `showTitleCard` initialization. Forest
returns also use `respawnFlag = 2` to preserve their authored position, which
excludes the native title-card branch. Composed shuffled entrances can have the
native entrance card bit unset; randomizer mode also hides cards by default.

Boot now arms a transient request after the final destination and Farore
composition. Player initialization admits that request through the respawn and
entrance-bit gates, selects the destination's native card, then consumes the
request even if the card is suppressed or the scene has no title texture. The
request remains visible through the native title-card hook so the randomizer's
implicit hide default does not suppress a crossing. An explicit Hide Title Cards
setting, native scene-layer/story eligibility and other hook vetoes remain in
effect. Ordinary same-game respawns keep their existing behavior.

This changes the shared MM→OoT return flow, rather than hard-coding Lost Woods
or altering entrance-shuffle maps. Reset/quit title boots clear abandoned
requests. MM's existing forest/title-card implementation is unchanged.

## Verification

- The native regression first failed with: `cross-game positioned return must
  display the Lost Woods card`.
- `python3 -B tests/lost_woods_entrance/run_oot_title_card_tests.py --sanitize`
  passed in Combo and solo modes under ASan/UBSan, including the real boot,
  portal placement, player title selection, native suppression hook, texture
  selection, language conversion and fade lifecycle.
- Tests cover all three paired save slots and forest route endpoints, composed
  shuffled and boss arrivals with card bits unset, positioned/grotto fallback,
  cross-game Farore recall, explicit hide/show settings, randomizer defaults,
  untitled/hidden/cutscene-layer consumption, abandoned boot cleanup and ordinary
  same-game respawn parity.
- Full production C/C++ object compilation passed for `title_setup.c`,
  `z_player.c` and `timesaver_hook_handlers.cpp`, each in Combo and solo modes.
  Native symbol inspection confirmed matching C linkage for the new helpers.
- Independent review found no blocking issues and reran both sanitizer cases.
- The existing Lost Woods suite initially stopped in `renderer_linkage_test.py`
  because the shared workspace ran out of temporary disk space. Completed checks
  before that stop passed; the failed renderer and remaining checks were resumed
  with compiler temporary files in `/dev/shm`. Final results are recorded in the
  recovery checkpoint.

The container checks do not establish visual correctness with the user's actual
pack stack. The decisive game checks are Woods of Mystery→OoT Lost Woods,
2Ship Lost Woods→OoT Lost Woods, a shuffled cross-game return, and a later
ordinary respawn without a replayed title. Repeat with Alt/Djipi enabled to
confirm the existing art substitution in the actual configuration.

## Recovery

The checkpoint archive preserves the exact implementation commit/tree, Git
bundle, owned before/after source, patch, verification output and restoration
instructions. The original integration baseline remains recoverable by its
immutable commit. Restore the candidate on an isolated checkout; no mod pack or
save migration is needed. Source publication does not publish an executable or
establish runtime acceptance.
