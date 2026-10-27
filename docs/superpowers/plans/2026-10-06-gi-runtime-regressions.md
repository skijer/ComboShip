# GI runtime regression recovery

Baseline: `92b72bf29d15b80f3bfd88402319bae09b74c895` (PR #34 candidate). Work only on `poc/gi-runtime-regressions-20261006`; no master promotion.

1. Reproduce OoT-imported MM dungeon items, boss souls and bottles resolving to the sentinel. Preserve placement and grant identity; render through their existing native MM handlers.
2. Exercise tunic resource availability with an MM-first, dormant OoT owner. Preserve tint, particles and local-mod priority.
3. Keep compass receipts to title plus assigned boss and actual placed reward sprite, including missing-sprite cases. Preserve the map entrance shuffle condition.
4. Recover real custom sword packs and test the selected, multipart model graph and receipt transforms. Correct the active-path defect rather than guessing another uniform scale.
5. Preserve broad item scales and lift held framing slightly. Keep every supported player form inside its camera envelope.
6. Trace summer sun geometry and animation interpolation; preserve its appearance while eliminating the discontinuous transform.
7. Run relevant diagnostic suites and host syntax/build checks, obtain a fresh code review, publish a draft candidate and save a downloadable recovery checkpoint. Distinguish implementation/build evidence from user runtime acceptance.

Acceptance remains the user's MM-first all-items plando run with the actual loaded mod stack, receipt views, tunic bodies, native MM imported items, hints and summer sun. Candidate build success does not imply runtime acceptance.
