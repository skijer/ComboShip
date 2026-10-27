# Cross-entrance expansion verification — 2026-10-15

Implementation: `affcc193ff9092f333f0481960ad4b2d9c96267f` on
`bridge/cross-entrance-recovery-20261015`, based on PR41
`913bc3298bfcdd57226f87e326eaf4bf72ccd33a`. Later documentation and diagnostic
commits do not change that implementation. The continuing original POC supplied
source through `039f4462` and its configuration evidence through `36bf28ae`;
the recovery branch adds the independently reviewed decoupled death fix.

This report separates source/build and headless native evidence from installed
gameplay acceptance. No develop/master promotion or push has occurred.

## Delivered scope

| Pool | OoT entries | MM entries | Total |
| --- | ---: | ---: | ---: |
| Dungeons, with expanded MM pool | 11 | 12 | 23 |
| Ordinary boss arenas | 8 | 4 | 12 |
| Interiors | 40 | 42 | 82 |
| Grottos/graves | 37 | 26 | 63 |
| All selected pools | 96 | 84 | 180 |

The seven additional MM dungeon destinations are the two Spider Houses, Ikana
Castle, Secret Shrine, both Beneath-the-Well entrances and Pirates' Fortress.
Final bosses and the existing Happy Mask Shop/Clock Tower game portals stay
outside the new pools. Every new option defaults off.

Farore checkpoint ownership and legacy migration, initial Stone Tower exterior
orientation, MM reset/owned-clock admission, native grotto materialization and
independent physical rewards/logical source completion are included. Runtime MM
time is preserved.

Boss death ownership uses the incoming native source, independently of reverse
travel. For example, native Fire ingress → Gohma and Gohma reverse → Water must
recover at Fire after cross routing into another boss. `deathCheckpoint` records
that parent. Foreign recovery reloads source-host state and applies the same
parent as its actual title spawn. Only recognized native arena recovery targets
are normalized to their canonical temple, avoiding repeated cross interpretation;
ordinary interior/exterior/grotto endpoints and intentional reverse/warp routes
retain their native identity. Stale provisional boss metadata fails exact
preflight; recorded maps are never silently rerolled.

## Source and build evidence

- The complete cross-entrance and Farore suites pass. Production-body fixtures
  cover native adapters, first/repeat boss warps, physical reward/world effects,
  native spawn and time state, recovery, codec and coordinator boundaries.
- New decoupled parent and actual foreign title-spawn regressions first failed,
  then passed under ASan/UBSan. The constrained-generation Coordinator fixture
  likewise failed with `StructureExhausted`, then passed with real cross-game
  movement, deterministic replay, empty temporary roots, runtime isolation and
  fail-closed rollback. Earlier Gyorg, native boss-exit parity, waiting-state,
  source-condition and Farore regressions have passing sanitizer evidence.
  Leak detection is disabled because this container cannot enumerate `/proc`
  tasks; address and undefined-behavior checks remain enabled.
- Both native engines, launcher, headless generator and UI compile/link. The
  corrected `native-candidate-final02` link expands response files and checks
  that every required rebuilt object is included. The final header-complete
  `native-candidate-final03` also passes exact replay and combined playthroughs.
  The earlier final-stage
  relink/matrix omitted response-file replacements and is excluded as final
  evidence. Source and artifact hashes identify the corrected stage.
- A linked public-API probe passes decoupled catalogue ownership, both runtime
  setters, validated foreign recovery requests in both directions and
  transactional invalid-metadata rejection. Internal native title/death
  consumption is exercised by production-body fixtures.
- Launcher reload and shared-item/slot/plando production fixtures also pass. Their
  diagnostic compiler flags now match the project's C++20 requirement.
- Whole-integration and scoped reviews have no remaining confirmed Critical or
  Important findings. Review and automated proof do not replace gameplay.

## Final linked native matrix

The accepted matrix uses corrected native stage02 and records actual exit
codes. Final stage03 additionally rebuilds every remaining user of the changed
common headers (12 units total), relinks both engines/launcher/headless/UI, and
passes the linked recovery probe, exact complete-JSON replay and all four
both-start/mixed playthroughs. Source and artifact hashes match their manifests.
`FINAL-ACCEPTANCE.json` records 21 accepted native cases and the stress rejection.

| Native profile | Generation | No-trick playthrough |
| --- | --- | --- |
| Boss-only | Pass | Pass |
| Expanded dungeons | Pass | Pass |
| Interiors | Pass | Pass |
| Grottos | Pass | Pass |
| All pools, OoT start | Pass | Pass |
| All pools, MM start | Pass | Pass |
| All pools, native mixed coupled | Pass | Pass |
| All pools, native mixed decoupled, open Door of Time, Ganon's Castle excluded | Pass | Pass |

The decoupled acceptance seed has Dungeon8/Boss0/Interior40/Grotto18 cross-game routes.
A selected pool is a within-category permutation, which can remain inside a game;
per-seed cross-game movement in every pool is not a generator guarantee. The default
combined seed and boss-only case separately prove actual cross-game boss routes.
An extra diagnostic assertion that incorrectly demanded crossings in every
native-mixed category was corrected; structural/bijection and native goal checks
were preserved.

All-pools OoT original, deterministic regeneration and recorded-map replay match
as complete JSON. The map includes cross-game routes in every category:
Dungeon12, Boss6, Interior44 and Grotto32. Disabled candidate output equals the
baseline launcher output using the same native modules, without cross-map data.
The recovered historical v1 dungeon map also matches exactly.

Expanded v2 generation retains the existing 128 full-map retries, then may use a
bounded cycle/swap construction. Every accepted step and the final installed map
must preserve the full native structural footprint; identity-only repair is
rejected. This introduces a constrained distribution and can retain fixed
entries. It preserves reachability rather than weakening the oracle. V1
generation and exact recorded replay do not enter the fallback. Final native
item-fill and goal/playthrough validation still apply.

## Advanced configuration boundary

The continuing original POC recorded closed-Door-of-Time native mixed decoupled
fills exhausting existing retries, both with and without Ganon's Castle. These
are rejected attempts, not accepted seeds. Its native-only controls did not
establish that every rejection was a new cross-routing regression. An open Door
of Time with Ganon's Castle excluded produced a completable decoupled seed.
The diagnostic driver now uses that explicit acceptance preset and exposes the
closed-door/Ganon case separately through `--native-decoupled-stress`, retaining
nonzero failure results. The current coupled case rejected its first fill with Ganon unreachable, then
passed generation and playthrough on its second outer attempt. The stricter
closed-door decoupled/Ganon control returns exit1 after both outer attempts: a
Woodfall Compass placement dead end, then a final Ganon goal failure. It emits
no accepted spoiler. This rejection remains separate from the passing preset,
and does not establish that every closed-door configuration is broken.

## Required installed-game acceptance

These remain unchecked until exercised against installed game assets:

- [ ] All twelve bosses: first defeat, repeat warp, physical rewards/hearts/remains/
  Oath, logical source world effects and MM cycle reset.
- [ ] Unbeatable foreign boss: death/Continue, save/reload and cold load, including
  mixed decoupled source/reverse ownership; Farore/new ingress cancels recovery.
- [ ] Grotto/grave contents, exits, void returns and Farore recall, including shared
  MM rooms and spring/winter exterior returns.
- [ ] Each expanded dungeon, both Stone Tower orientations and simultaneous
  dungeon/boss completion with native entrance shuffle.
- [ ] Both starts, separate/copied slots and recorded seed reload.

A Windows build and installed rendering/gameplay session have not been run in
this Linux headless environment. Existing unrelated PR41 acceptance remains
separate.
