# Magic Mirror choice and travel correction

> **For agentic workers:** Use superpowers:executing-plans for inline execution.

**Goal:** One Yes press closes the mirror prompt, completes the cast and returns
to the appropriate Lost Woods scene; the prompt owns gameplay input in both hosts.

**Architecture:** Retain the current early input hook and native cast/arrival
adapters. Let the native textbox finish closing before beginning the cast.
Trace MM's complete input path rather than assuming the local pad is its only
movement source.

**Tech stack:** Native OoT/MM C and C++, Python regression runners, ASan/UBSan.

**Spec:** User's 2026-10-21 runtime report; baseline commit
`8ab33deeb5000c6a66663dc9659d9060f702881a` from the current integration branch.

## Constraints

- Preserve the reported working OoT prompt movement lock and first-press No.
- Preserve cast animation, magic, respawns, progression, save ownership and forms.
- Preserve both solo routes and the combined route to MM Lost Woods.
- Arrival continues to follow the placed Prelude stump.
- Leave scene archives and the independent Saria/audio work unchanged.
- Native tests/builds are separate from configured runtime acceptance.

## Review focus

Closing-frame input, replacement messages/players, scene or slot changes,
transformed controllers, and native message drawing after Yes.

## Steps

- [x] Extend `tests/magic_mirror` to exercise real native message queries and
  closure. Reproduce the forced-NONE/live-text failure before changing code.
- [x] Correct the shared prompt-to-cast lifecycle in
  `combo/gameplay/MagicMirrorRuntime.inc.h`; run both host fixtures.
- [x] Reproduce and correct the mutable-eligibility ownership bypass at the
  native/Wolf input readers; retain cursor input and form/damage behavior.
  Exact MM runtime causality remains unproven; phase logs cover the next check.
- [x] Run affected mirror, inventory and integration suites, compile the changed
  native units, inspect the diff, and preserve the bounded commits for publication.
- [x] Record precise test evidence and remaining in-game checks in `docs/poc/`.

Final review found an additional raw-pad reader in the MM Mario driver. Both
hosts' existing Mario hold now includes prompt ownership. The entry-gate test
failed before correction and passed afterward; both complete native player
overlays compiled. No further Critical/Important findings remain from review.
Configured runtime acceptance remains outstanding.
