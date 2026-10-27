# Cross-entrance generation and reset candidate

Baseline: `2069d37e2a9338494acff3a70ca56ddaf2e035fa`, PR #43's entrance
candidate. The supplied Windows dump matches the SoH module in Build Artifacts
run `38826860624` (PR merge `c38da2bae5684bfe82c7cdd355a20a9be33ec104`):
its CodeView GUID and age match the distributed DLL. The accepted base branch
remains unchanged.

## Evidence and limits

The dump records exception `0xc0000005`, a write to address `0x38`, at
`soh.dll+0x5c94ff`. `SetAllEntrancesData` dereferenced a null return from
`Region::GetExit` while assigning Bottom of the Well's reverse entrance
`0x2a6` (678), from region 244 to region 163. The caller is `IndexRegions`,
through `SearchScope` and `Combo_SOH_Rando_GetReachableChecks`. The scope's
saved `searching` flag was false. This proves the unchecked missing-return
failure; it does **not** prove how that edge became unavailable.

The reported control generates with the new options off. A later report shows
generation advancing with cross dungeons, expanded dungeons and grottos/graves
on, and cross bosses/interiors off. Boss shuffle is a useful next comparison,
not an established root cause. The saved config also enables native overworld
spawn shuffle; the other native entrance categories are off.

Fresh-process Linux probes against the matching distributed modules passed
structural validation with the reported config: 27 layouts with bosses on and
10 with bosses off for the same test seed, `cross-crash-repro`. Disabling native
spawn shuffle also passed structural validation with bosses on. The full-fill
probe reached item placement but was stopped at its 80-second investigation
limit; it does not establish complete generation or beatability. Original
config/log/dump files are private investigation inputs and are not committed.

## Bounded correction

- Bind native entrance metadata by the original edge identity, using the
  existing `GetEntrance` helper. Live native/cross destinations remain intact.
  Resolve both halves before publishing a pair; report a missing edge with its
  index and regions instead of writing through a null pointer.
- Route Reset through a launcher cancellation callback. Set cancellation,
  join the generation worker through its scope cleanup, discard pending apply,
  clear seed readiness, then release the generating flag and run native reset.
  An older launcher without the callback cannot reset under an active worker.
- Poll cancellation around structural oracle queries, including baseline,
  whole-map retries and constrained-cycle probes. Preserve rollback and the
  noncancelled layout/RNG behavior.
- Join the worker before normal success **or failure** returns control to
  file select. A published pending result can precede the final scope cleanup.
- Run the metadata and worker-lifetime checks immediately after CI checkout,
  alongside the existing hook/linkage preflight, before dependencies and long
  build stages.

No entrance option is disabled or merged with another option. No archive,
scene, asset, save format, native permutation or accepted baseline is changed.

## Verification and remaining runtime proof

The metadata test initially aborted in the production `Region::GetExit` for
a redirected edge. It now checks original identity, preservation of live
routing, coupled/decoupled metadata, and rejection of a missing Well return.
The reset test initially showed the worker still alive after Reset. It now
executes the production callback setter, reset handler, cancellation and poll
bodies with real threads and cleanup destructors after completion publication,
including pending results, failed generation, synchronous startup generation
and the mixed-version fallback. The structural test initially ignored cancellation
raised during a native query; it now verifies cancellation and map rollback.

The complete cross-dungeon runner passes normally and with ASan/UBSan. The full
launcher and native `entrance.cpp` pass
syntax compilation against real dependency headers; `comborando` builds with
the modified generation header. Full `OTRGlobals.cpp` compilation is left to
the configured platform build; the local standalone attempt lacks the SDK's
`nlohmann/json_fwd.hpp`. These are code/build checks, not a replacement for the
Windows game test.

Required follow-up: full Windows/Linux packaging; complete generation with
the original and boss-disabled settings; Reset during entrance preparation
and item placement; file-select input and a second generation afterward.
The original missing-edge trigger remains unconfirmed. This is a failure-
handling candidate, not accepted runtime proof or master promotion.
