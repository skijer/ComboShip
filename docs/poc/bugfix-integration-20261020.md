# October 20 bug-fix integration candidate

This is an isolated source candidate on `fix/bugfix-integration-20261020`.
`develop` and the previously accepted gameplay baseline are unchanged. cor
authorized implementation, source checkpoints and publication to
`marsh6487/ComboShip`; publication does not establish configured gameplay
acceptance.

## Source inputs

| Input | Exact local source | Integration |
| --- | --- | --- |
| Current cumulative/platform/Saria-placement baseline, PR45 | `613802447cf8a4fbc3e058ba084f9e3dd7b9f7ae` | Parent of all inputs |
| Geometry-only native forest/Black Market and real donor bundle | `9d5f550d98f46ff50523a8f6f6a011eddf56e1c8` | Initial candidate |
| Black Market structural access, manifest lookup and rejection diagnostics | `1d19e86cd` | Merged without conflicts |
| Native transformation camera, shield-light teardown and NPC resource/TLUT repairs, PR46/47 source | `a01321fe0` | Merged without conflicts |
| Completed elemental damage, selector/HUD and finish source revision | `412058d4c` | Merged without conflicts after its verification record arrived |
| NPC lossless batching and versioned draw-scale support | `75ee1848e2911001fdbec6ef0f0e912de54adb8b`, `ee83e2613ebcd0cb029869fc6dce90f7a09fdd75` | Combined with the existing GI particles and portal placement; both sets of lifecycle tests retained |

The first combined checkpoint is local commit
`a2b774ec0c0ef528fc35d23022e50376f9d5219c`, tree
`a6363ff62c95aaeae0c9440d2431b72c060b4ed6`. It contains both Black Market
checkouts and the camera/NPC/crash input, before the elemental and new
particle/dialogue/input changes. Published snapshots must reproduce their
recorded local source tree exactly; local merge history is retained separately.

## Preservation contract

- Keep native forest geometry, collision, moved mirror stump and existing
  portal/door routes, with Saria beside the portal. Native sources use stock
  textures; no new scene texture bundle is introduced.
- Keep the existing native/combined/solo game guards, actual door ownership and
  player-safety checks. Purchases retain seed/session validation. Empty stock
  and optional presentation packs do not bar physical shop entry.
- Preserve original model inputs, accepted audio, poses/props/blink, progression,
  save layout and source provenance. Repaired model archives remain separate
  installs; source publication alone cannot replace an installed old NPC pack.
- Woods of Mystery remains limited to its existing bridge/tunnel/seasonal
  integration. No Woods of Mystery scene archive is added.
- Elemental immunity must leave the ordinary sword strike's damage intact.
  The reported built-in Biggoron casing case remains an open configured runtime
  check; the elemental checkpoint does not claim that screenshot is resolved.

## Entry evidence and candidate builds

cor specifically requested entry tests before the build. The environment has
no runnable Linux game executable, Wine or Xvfb. The available Windows DLLs and
port archives belong to an older build and cannot prove this candidate's
gameplay. Production-function executable tests against actual generated scene
resources are recorded separately from live game entry/exit, drawing and
interaction.

The candidate's entry evidence must cover the native MM shop and solo OoT
private scene, authored and stock forest routes, valid structural resources,
optional pack independence, genuinely missing destination rejection, empty
stock, exit/re-entry and retained safety checks. A generated seed or native
object compilation alone is insufficient.

The first source publication used a CI-skip commit. cor subsequently requested
the combined test executable, paused advancement for the NPC source checkout,
then explicitly requested integration after that checkout was ready. Full
hosted builds now run through the existing regression gate. Live game entry
remains a runtime acceptance check, rather than a prerequisite for producing
the candidate needed to perform that check.

## NPC checkout integration and CI correction

The published bug-fix parent is `a0dcb15d5ae22b2e7ef4bf209b211bade07518c9`,
tree `1f25277b21a78e9a44afc183ef758b5b439354bd`. Its source tree is byte-identical
to the retained local checkpoint `d5b03c758`. The NPC donor is the exact public
`ee83e2613ebcd0cb029869fc6dce90f7a09fdd75` checkout, including its preceding
lossless archive-batching commit. Retain both published histories as merge
parents. No replacement model archive is regenerated or installed here.

Only the MM Saria lifecycle fixture conflicted. Preserve the existing portal
side-placement and older-portal-anchor assertions alongside the new V2 provider,
draw-scale, legacy fallback and caller-buffer checks. The production Saria
merge retains the shared green GI particle renderer and versioned scale support.
Apply clang-format 14.0.6 to the changed native source files.

The earlier hosted run `39516654774` failed in `renderer_linkage_test.py`:
solo MM no longer directly emits display-list interpolation macros after its
portal switched to the shared GI renderer. Its empty interpolation import list
is therefore expected. Reproduce the failure locally; update the fixture to
require the native `NeiGi_DrawWorldParticles` import for that translation unit
and compile the actual MM GI renderer as a sixth object, requiring both native
C interpolation symbols there. Other rendering objects retain their existing
requirements. No production rendering change is needed for this CI correction.

Fresh combined checks pass: 142 NPC Python tests (17 optional source-data skips),
native NPC routing/pose/draw-scale and actor compilation, forest/Saria lifecycle
and both 32/64-bit ABI layouts, MM provider ASan/UBSan, and all six renderer
linkage objects. The bug-fix preflight passed its first seven commands; its
native-forest preparation step initially lacked local CMake. Running that final
suite with the existing CMake binary passes all 31 tests (two optional skips).
Hosted full application/package results and configured runtime remain pending.

## Verification record

Production source is frozen at `39c52ee7d` before the final evidence-only
checkpoint. Additional verified source milestones are:

| Change | Local commit | Result |
| --- | --- | --- |
| GI particles for OoT/MM Saria poses and both bridge routes | `498a6e5c7` | Small green filled hexes with the actual GI Glow/Star primitives and grayscale-safe renderer; active ring callers removed; purple tunnel material preserved |
| MM custom-dialogue decoder and Mirror input ownership | `4fabc710e` | Normal high-ID custom dialogue avoids the credits decoder; bounded font writes; human/Wolf input remains owned during prompts and B/No/Yes handoff frames |
| Real-resource Black Market controller entry tests and receipts | `ee8898aca` | 66 full-input controller cycles plus 18 bundled-only structural cycles; negative resource/safety cases pass |
| CI formatting | `bc278c274`, `39c52ee7d` | Required files formatted; non-whitespace source bytes unchanged |

| Verification | Result and boundary |
| --- | --- |
| `bash scripts/diagnostics/run_elemental_sword_regressions.sh` | All 16 commands pass on the combined candidate; all 95 elemental input paths still match the completed checkpoint exactly |
| Optional native MM ice-arrow resource bridge, explicit dependency roots | Solo and Combo sanitizer variants pass; the default gate's optional skip is covered separately |
| `bash scripts/diagnostics/run_oct20_bugfix_regressions.sh` | All eight commands pass: real message decoders, GI renderers, structural entry/safety, legacy cache, shield lifetime, TLUT and native forest preparation; 31 forest tests include two explicit optional donor-test skips |
| Final Mirror native and integration checks | Both layouts pass ASan/UBSan, including same-frame closing suppression, raw menu input, identity, cleanup, cast and arrival fixtures |
| Real-input entry executables | Both final binaries independently replayed successfully; exercised production hashes match the final source |
| Inherited camera, shield lifetime, Saria TLUT and reward asset-packaging checks | Pass; the earlier missing-CMake environment issue was resolved for packaging |
| Native compilation supplied by focused workstreams | Ten GI/actor/portal configurations and 16 dialogue/player/Wolf/adapter configurations pass; the final four Mirror adapters were recompiled after the handoff fix |
| Formatting, whitespace and custom asset collisions | Pass using the repository's exact clang-format-14 selection |
| Full hosted application/Windows builds | Requested for the combined NPC/bug-fix candidate; completion pending |
| Configured game entry, mount/load order, rendered particles and user acceptance | Unproven; no runnable candidate in this environment |

`tests/bugfix_integration/evidence/20261020/verification.json` records exact
commands, source identities, log hashes and the evidence archive. Entry-specific
receipts and logs are in `tests/black_market/evidence/20261020/`. Receipts may
record an earlier HEAD while another isolated workstream had uncommitted edits;
their per-unit SHA-256 values were independently compared with the completed
source and match. These hashes identify the actually exercised code.

Independent review found and resolved the Mirror cancel-frame leak, the stale
missing-donor paragraph and three CI formatting omissions. Runtime limits above
remain separate from source verification. The new regression preflight is wired
into `build-artifacts.yml` before full platform builds. The combined NPC follow-up
does not skip CI; source/build checks remain separate from runtime acceptance.

Repaired NPC model packs remain separate installs, identified in
`docs/poc/npc-bundle-hash-repair-20261020.md`. Source publication does not replace
old installed packs. Existing optional full-scene FINAL presentation compatibility
and the reported built-in Biggoron casing case remain open as recorded in their
focused reports.

## Recovery

Restore from the published candidate source commit/tree or the original
checkpoint refs in the table. Original input archives remain unchanged. Discard
only a bounded new change when a regression is found; retain the independently
verified inputs. No accepted runtime master is promoted by this integration.

## Combined gate follow-up

The published cumulative head `4a91c33ba88d78dbe17a29bfbbdaf2f430818094`
contains NPC source checkpoint `ee83e261` and the completed bug-fix history.
Its exact tree is `164adb4ef53e166580c13645c7f126630b3cb1ac`, also held by
local merge `b76e7475`. Windows compile preflight `39521052893` passed.
Build Artifacts `39521052872` stopped before platform builds because the older
Goron audio fixture did not declare the new elemental finish cleanup observer
called by the actual `Actor_Destroy` body.

Reproduce that failure, then retain the production actor/collision bodies and
their normal observer calls. Add production-header-compatible observer
boundaries to the Goron audio/collision, native MM water/weather, MM wand damage
and OoT stat fixtures. The Goron and native MM lifecycle fixtures assert the
observer handoff alongside their existing gameplay checks. The actual elemental
finish implementation remains covered by its independent native sanitizer suite.
No production renderer, model recipe, actor, scene or elemental implementation
changes are part of this follow-up.

The subsequent archive-authoring checkpoint `9633262f` explicitly freezes the
game rebuild source at `ee83e261`; its texture/normal/iris/ocarina recipes do
not require further executable changes. Those separate archive candidates do
not replace installed model packs through this source push.

Fresh checks pass: the 28 Goron audio scenario/backend runs, nine native Goron
collision cases, the complete sanitized wand suite (including 2,224 Sand parity
observations), native water/weather lifecycle, native transformation and Somaria
GI checks, and stat/Anchor snapshot checks. The full eight-command October 20
preflight passes, including 31 forest tests with two optional-data skips. The
NPC suite passes 142 tests with 17 optional-data skips; native NPC routing,
pose, draw-scale and actor compilation also pass. Asset collisions, whitespace
and changed-source clang-format 14.0.6 checks pass. Optional detailed local
test logs are excluded from this publication.

Independent review of all six changed fixtures found no blocking issues.
Recover the preceding cumulative candidate by checking out
`4a91c33b`; recover its unmodified parent source from its two retained public
histories. Configured runtime acceptance and full platform builds remain
separate from this test-fixture correction.
