# Runtime regression corrections

> For agentic workers: use systematic-debugging and test-driven-development for each independent correction; root integrates and reviews the combined diff.

**Goal:** Correct the reported monkey voice/conversation crash, OOT seasonal rain sound, Black Market camera and potion-floor clipping, and Blast Mask tutorial/action prompt, plus the subsequently requested ice textures and impact/death sound.

**Baseline:** Published cumulative PR #57 head `3c6a17627b2f214507057726eb2e7515aa6c66fa`; tested Windows merge `cb0de089ff5e74a9295820f2a78becc41cb42c02`. Fresh isolated clone; candidate branch `poc/runtime-shop-audio-20261023`.

**Spec:** cor's October 23 report and supplied log, dump, two PNGs and two MP4 clips. Added scope: Blast Mask garbled tutorial ending and OOT A-button Explode action. Native camera handoff received and checked against the current checkout; fixed-eye framing and actual FREE0/Unique6 tests are in progress.

**Architecture:** Fix the owning boundary for each regression without replacing the cumulative baseline. Use real native message/actor/audio/asset paths in focused regression fixtures.

**Tech Stack:** C/C++, Python deterministic resource builders, native engine fixtures, GitHub Actions.

## Global constraints

- Preserve accepted Lost Woods and Black Market texture appearances and all unrelated cumulative work.
- Keep Combo/standalone guards, MM rain, existing receipt/purchase flows, and actor progression intact.
- Baseline source and exact assets are durably recoverable from the published Git tree; do not add recovery ZIPs or duplicate repository assets to Library.
- Every bounded fix requires a reproduced regression, minimal implementation, relevant verification, and an isolated source checkpoint.
- Build/static verification is separate from configured gameplay acceptance; do not promote develop/master.

## Review focus

- Camera ownership across keeper, shelves, purchase, cancel, re-talk and cutscene interruption.
- Monkey text registration and native table fallback in every supported language.
- Rain ownership when native nature ambience was already tracked; thunder and MM behavior preserved.
- Potion floor decal state restoration with all geometry, UVs, textures and collision unchanged.
- Blast A prompt and actual A input agree; competing interactions and cooldown retain priority.

## Tasks

### 1. Monkey conversation and sound

- [x] Reproduce missing 0x8F27 OnOpenText interception and native Woods guide cue selection.
- [x] Correct catalog registration and custom actor WALK cue; keep lifetime semantics.
- [x] Run native dialogue open/decode/reopen and monkey audio/actor regressions.

Owned files: StaticStoryDialogue.cpp; En_Viewer/static_story_dialogue.{c,h}, z_en_viewer.c; mm_sfx_ids.h and relevant monkey/custom-message fixtures.

### 2. OOT seasonal rain

- [x] Reproduce seasonal PCM suppression while native nature-rain ownership is tracked.
- [x] Apply native-to-private ownership handoff in seasonal resolver while preserving live Song of Storms target authority.
- [x] Run real PCM/worker, native season/weather and MM preservation checks.

Owned files: GlobalOutdoorRain.cpp and global_outdoor_rain_audio_test.cpp; native weather tests as necessary.

### 3. Black Market potion floor

- [x] Identify the overlapping panel/platform faces from native scene geometry and supplied clip.
- [x] Apply opaque decal depth mode only to the coplanar overlay faces, restore draw state.
- [x] Compare both generated asset trees; prove all other resources/textures/geometry identical.

Owned files: tools/black_market/market_interior.py; tests/native_market/test_interior.py and test_platform_overlay.py.

### 4. Black Market camera

- [ ] Compare production camera targets/adapters with native shop flow and user's incoming findings.
- [ ] Reproduce identified boundary in real native camera fixture before modifying code.
- [ ] Verify keeper/shelf framing and native purchase/cancel/interrupt behavior on both hosts.

Candidate files: combo/BlackMarketBrowsing{,Native}.h; tests/black_market/browsing_native_test.cpp and runner. Received comparison confirms the manual FREE0 adapter and deliberate pullback. Preserve that bounded controller and correct its camera target contract; do not wholesale replace keeper or stock actors.

### 5. Blast Mask tutorial and A action

- [ ] Trace malformed tutorial tail through actual formatter/codec.
- [ ] Add actionable OOT Explode label and A detonation only when input is available; preserve current B behavior.
- [ ] Verify encoding/termination, HUD label bounds, cooldown and interaction priority.

Owned files: OOT mask-wear/extended-player/tutorial/HUD implementation and focused fixtures, exact list recorded by worker.

### 6. Integration and handoff

- [ ] Review all diffs and add changed native tests to existing CI groups if not already reached.
- [ ] Run appropriate native checks, formatting and compile preflight; record unavailable runtime checks.
- [ ] Save bounded candidate source checkpoints remotely, confirm final tree and PR build association.
- [ ] Supply concise audit and focused configured gameplay test list; keep master/develop unchanged.

## Recovery

Restore the baseline with `git fetch origin poc/market-forest-followup-20261022` and `git checkout 3c6a17627b2f214507057726eb2e7515aa6c66fa`. Each subsequent correction commit names its owning subsystem and can be reverted independently. The published exact commit's source archive is available through GitHub, including tree-owned asset parts and deterministic rebuild recipes. Source inputs uploaded for diagnosis remain unchanged.

## Verified milestone ledger

- Monkey: catalog-derived hook registration, native opening/decoding in Solo/Combo and four languages, native Woods cue 0x2900 and real queued SFX lifecycle. Independent review found no production defect; optional catalog-extreme-index coverage remains deferred.
- Rain: stale nature-channel private-loop suppression reproduced; live Song of Storms counterexample also reproduced before its guard. Real PCM runner and independent native-header handoff check pass; full MM weather suite passes.
- Potion floor: independent review confirms exactly one of140 resources differs; eight overlapping triangles use opaque decal depth, all textures/vertices/UVs/collision intact. 36 available native-market tests and actual CMake preparation pass; six immutable-donor tests require unavailable original POC14 input.
- Camera: user comparison received; fixed-eye focused-offer framing and actual engine update fixture underway. Full-bank-above-bottom-textbox static bounds do not justify pulling the camera behind Link.
- Blast: native codec reaches END/DONE correctly; visible internal CVar wording is the reproduced tutorial defect. A detonation must agree with its action prompt and retain competing-interaction priority.
- Configured gameplay/audible appearance acceptance is pending for every new candidate.

## Added ice scope and publication status

- User supplied two additional October23 combat clips: preserve the improved native ice mesh, improve only its surface/material texture, replace elemental ice sword Deku-nut-like impact cue, and continue the native glacial death sound even for dismembering enemies. Audio and material are separate bounded changes.
- A public checkpoint branch exists at the baseline. Automatic approval review rejected uploading the first source checkpoint twice, including after current repository push/admin permissions and prior explicit user authorization were verified. No modified source has been published. The user requested precise authorization wording and received it; local verification continues while a trusted explicit authorization is pending.
- Native camera regression now fails with the former moving eye and passes with fixed eye/eyeNext and actual Init/FREE0/Unique6 updates. Focused rows and explicit merchandise-bottom text replace the over-constrained whole-bank projected bound.
