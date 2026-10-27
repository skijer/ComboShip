# Magic Mirror arrival and green ritual candidate

Baseline: `525b3160660225a06700470fb39d9d4948834414`, the source head of
PR #53. cor tested its Windows merge build `8303491aa4ffebdebb59984a3b9cb949374131e0`:
the prompt input fix and MM-local recall passed; recalling from OoT crashed.
Candidate: `fix/mirror-arrival-ritual-20261021`.

## Crash evidence and correction

The supplied `Fleet of Harkinian-crash(8).dmp` has SHA256
`fc19ea023aacb02fffd697eabc6d0809648902fb6740e9decf5b5a6a887925ac`.
It records exception `0xC0000005`, a write to `0x838`, with `R9 = 0` at
MM DLL RVA `0x62D851`. The matching `8303491` DLL instruction sequence
matches `CutsceneManager_StoreCamera`: copying the main camera into
`sCutsceneMgr.play->subCameras[2]`. The destination is Lost Woods, room 0.

The shipped private forest header omits command `0x1B`, which normally calls
`CutsceneManager_Init`. A first MM arrival from OoT therefore leaves the
manager's PlayState null; subsequent scene visits can inherit an old pointer.
In Combo builds, scene initialization now installs an empty manager before
executing the scene header. A native header's own list replaces that default.
Standalone MM builds retain their original startup path.

## Requested visual revision

Keep the exact Farore casting motion, timing and player action. The animation
implementation, Time Gate item logic, ownership, magic costs, existing input
fix, destination selection, stump placement and pickup GI are unchanged.

During recall, the shared magical seal, rising wisps and textured energy
replace the floating overhead mirror. They grow with the casting frame and
dissolve over 12 native update ticks after travel is accepted. Rendering is
sampled each frame and owns no particle actor or persistent allocation.

Every casting layer uses green hex colors: `18863C`, `44FF74`, `86FFB0`,
`2EEB5D`, `BCFFD0`, `70FF9C`, `32EB69` and `A4FFC0`. The existing white-alpha
medallion material supplies texture detail. Time Gate retains the default
temporal palette; the new mirror entrypoint selects the green palette.

## Verification and acceptance

- Camera regression reproduced the cold-arrival failure before the fix. Native
  scene initialization, cutscene manager, camera storage, stale-scene reset,
  normal list override, five-form shop entries and standalone MM control pass.
- Both real-engine mirror suites pass: one confirmation, unchanged Farore
  animation, no overhead model, aura growth/fade, movement ownership,
  cancellation, teardown, recall destination, magic/respawn preservation and
  grounded stump pickup.
- The actual mirror adapters run through both production presentation wrappers;
  every geometric and textured vertex must be green-dominant. The original
  temporal colors failed this check; the green candidate passes.
- Five complete native C++ objects compile: MM scene startup, both mirror
  adapters and both effect presentation adapters. Native clang-format14 checks
  pass. The forest suite passes 46 tests with two explicit original-donor skips.

This is a source candidate. Full platform linking and configured gameplay
acceptance are pending. Next runtime check: recall from OoT immediately after
launch, then recall locally in MM; confirm arrival beside the stump, the
unchanged motion, all-green energy and absence of an overhead mirror.
No scene archive or accepted runtime master is replaced.
