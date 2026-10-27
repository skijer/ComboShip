# MM Wolf reconstruction evidence

Baseline: f8cf82928db4a31a23405b6e397a68275a192316; candidate branch poc/reconstruct-wolf-20261004. Task 1 only. No game-runtime acceptance or master promotion is claimed.

## Shared skin checkpoint

- Added opt-in authored root motion, fractional TRS interpolation and definition-only scale. Zero defaults preserve MM Pikachu root stripping, frame snapping and shared SkinScale override.
- Pose computation is available independently of graphics submission. Bone queries require the instance that most recently computed the pose and reject invalid bone indices, stale instance queries and destroyed skin poses.
- Production skin and native MM z_skin_matrix.c compiled against real MM headers. Allocation, CVar and graphics/matrix submission boundaries are stubbed; skin calculation and packed vertices are production code.
- RED: python3 -B tests/mm_wolf/run_skin_tests.py failed at expected authored/interpolated packed X=23, actual X=2 on unmodified production renderer.
- GREEN: same command passed Pikachu defaults, root translation/scale interpolation, shortest angle arc, owner-scoped bone queries and destroy invalidation.
- git diff --check passed.

## Acceptance boundary

This report records new source/compiler evidence. Full platform builds and in-game asset/render/action acceptance are separate gates; the old historical gate is not proof of this reconstructed source.

## Loader and donor scaffold checkpoint

- Copied the accepted OoT Wolf implementation into an MM-owned standalone TU, translating native movement fields, cylinder damage fields, audio/input entry points and MM skeleton/collider constants. This is reconstruction, not byte-for-byte recovery of the unavailable historical MM source.
- Added 64 MiB file allocation cap, full nonoverlapping header/data chunk bounds, wide count products, alignment, one ordered skeleton root, influence index/order/sum checks, bounded names/frames/frame rates and float payload validation.
- Float validation reads IEEE-754 integer bits; it remains effective under fast-math. It validates vertices, inverse binds, bone positions, all animation TRS floats and frame rates.
- RED donor-loader fixture accepted NaN vertex and failed the required rejection assertion.
- GREEN: python3 -B tests/mm_wolf/run_core_tests.py passed both undefined-behavior sanitizer and -O2 -ffast-math builds with valid format plus NaN/Inf/max-float payloads, count overflow, header overlap, alignment, bone weights, self/cyclic/disconnected parents, unterminated names, empty animations, frame-chunk escape/overflow and truncation. Native MM structures/headers are used.
- No host activation is attached in this checkpoint. Combat state is the translated donor scaffold and will receive native damage/freeze/lifecycle fixtures before it is activated.

## Pikachu metadata review correction

- Review found the first skin checkpoint imposed equality between skeleton and weighted-bone counts. Actual generated Pikachu has 48 skeleton nodes, 47 skin/inverse-bind bones and one trailing rigid limb. The original 3/3 synthetic metadata fixture did not cover this.
- Added production generated Pikachu mesh and skeleton to the fixture and used the actual registration header. RED: no display list submitted for actual 48/47 layout. GREEN: actual Pikachu layout draws with its existing scale override; extra/unweighted bone queries are rejected.
- Pose validation now accepts bounded trailing rigid limbs while requiring enough animation data for every weighted bone.

## Native ownership checkpoint 6c0adc44

- The core accepts the captured raw pad and an explicit native-action ownership signal. Generic damage plus MM's live action pointers remain authoritative even when invincibility is zero. Wolf clears its own pause/attack state, shows a damage pose, and never resets actionFunc, changes health, or reapplies damage.
- Live cleanup restores the original shadow/cylinder, removes the owned AT collider from current submissions, and frees mesh/joint allocations. Stale scene cleanup does not dereference a prior actor allocation.
- Production core fixtures passed UBSan and -O2 -ffast-math for bites/active windows, native MM DMG_SLASH tiers 2/4, shield bounce, moving A dash, wall rebound, native damage/freeze release, and cleanup restoration.

## Graphics fixture correction 2307f56d

- Review exposed an uninitialized graphics-arena end pointer in the fixture. MM GRAPH_ALLOC is inline and did not call the fixture's Graph_Alloc stub. RED reproduced a UBSan null-offset failure at ssbb_skin.c:361.
- Every synthetic and actual Pikachu draw now initializes both .p/.d pointers and uses the genuine arena allocator. GREEN ASan/UBSan/bounds passed. LeakSanitizer is disabled because the managed executor blocks /proc task inspection; memory/bounds/undefined-behavior checks remain enabled.

## Real MM host checkpoint 7a2d9ec6

- Added a standalone Wolf host discovered by the existing CMake mods glob. Captures Shadow Crystal via full-u16 C/D-pad accessors before custom-equipment/tool listeners; D-pad activation respects its existing option. Native mask/custom-form presses release Wolf and reach their existing native paths.
- z_player.c now runs the host after MM has resolved incoming damage but before native action dispatch, reapplies the low Wolf cylinder after native body fitting, supplies its speed multiplier, replaces only the selected/ready local-player mesh, and releases on destroy.
- Mario, native transformations/masks, other custom forms, PAK/O2R models and active Pendant/Beetle/Kite/Trident/custom tool ownership take priority. First-person/interaction/carried/item/cutscene/swim/horse contexts yield actions; contextual A remains available.
- Missing/invalid assets leave normal draw/input/cylinder intact. Scene changes, transition starts, death, disabled Wolf and destroy clear ownership. Native freeze shell and native item/effect draws remain in the Wolf draw path, with the actor matrix restored so Wolf's render scale cannot rescale the native shell. Existing normal held/equipment/attachment paths are unchanged.
- RED initial production fixture failed full-width C Shadow Crystal activation while the host was missing. GREEN runs actual host/core/ext-button accessors with real MM headers/native math. Native Player_Action_82 and func_8082DE88 bodies are extracted unchanged and compiled as C: periodic freeze damage happens once, A/B mash reaches the native helper, thaw animation completes with the native action retained throughout. Knockback/electricity function bodies, allocator/graphics/audio/collider submission boundaries and native animation/damage completion dependencies are fixtures. This is source behavior evidence, not a full game run.
- Syntax command: source test-env.sh; python3 -B tests/mm_wolf/run_syntax_tests.py. All four changed TUs passed with genuine project/upstream headers; 14 existing button-header macro warnings for standalone files and 1246 native unity/OTR/resource warnings for z_player.c. No fabricated BenPort/player headers.
- Core, skin, host suites and whitespace checks passed immediately before this checkpoint. Full platform links/packages and in-game Wolf asset/render/camera/interaction/attachment acceptance remain separate.

## Final malformed-transform and input-policy checks

- RED production loader accepted finite scales that overflow when chained. Validation now bounds every clip's parent-composed operator norm and translation norm, including fractional TRS interpolation, before allocating or registering the asset. Scale bound is 100000 and translation bound is 32767 in authored model units. Input floats remain validated by integer IEEE-754 bits under fast-math.
- Wolf scale and speed CVars accept 0.05..10; NaN/Inf/extreme/out-of-range values fall back to their existing defaults (0.3 and 1). Pikachu's shared scale option is retained.
- RED float-cast-overflow sanitizer caught packed vertex X=60030 and attack-cylinder Z=32790 overflowing s16. Production skin coordinates/normals and Wolf attack-cylinder positions now saturate before casts, and reject nonfinite values by integer bits. Invalid animation-frame/matrix poses invalidate cached bone queries. Positive/negative packed bounds and NaN frames are covered.
- RED fractional angle fixture with authored 720/-270 degrees chose the long path (packed X=17 instead of 22). The modulo correction preserves the shortest arc for negative multi-turn angle differences; Pikachu's existing non-interpolated mode is unchanged.
- Independent review reproduced Wolf accepting raw B while MM's effective pad was zero for PLAYER_STATE1_20/RemoteBombchu, actor override and textbox cooldown. It also activated Shadow Crystal with disabled input.
- Host now reserves raw buttons before equipment/tool listeners, restores only its own reservations into the native physical-pad branch, lets MM's override/disabled/cooldown and OnPassPlayerInputs policy run, then captures the effective pad. Core consumes that capture. C/D-pad Shadow Crystal toggles resolve after effective-input policy. Native remote/override/transition control releases the form without consuming those owners' raw buttons; textbox cooldown yields actions.
- First-frame freeze can recover only permitted buttons from Wolf's own reservation. Actual native freeze/mash bodies verify ordinary mash advances by 6 and hook-suppressed mash advances only by the native automatic 1; Wolf never reintroduces suppressed A/B.
- The production host fixture now compiles the exact native input-selection block in addition to the exact freeze action/mash helper, and checks remote/override/cooldown, input-hook suppression, C/D toggle suppression and full-width item IDs.
- No-op host seam boundaries were added to the existing extracted native use-item/input/action harnesses. Production item/grant handlers were unchanged.

## Final verification

The following checks were run using /workspace/scratch/68d44ffc4c98/test-env.sh and genuine project/upstream headers:

- python3 -B tests/mm_wolf/run_syntax_tests.py: PASS for Wolf core, host, MM skin and complete native z_player.c. Standalone TUs show 14 button-header warnings; native unity TU shows 1246 resource/compatibility warnings. Syntax checks are not full platform links and do not promote these warnings to errors.
- python3 -B tests/mm_wolf/run_core_tests.py: PASS UBSan/float-cast-overflow and -O2 -ffast-math; malformed files, aggregate transforms, CVar guards, combat, native ownership and cleanup.
- python3 -B tests/mm_wolf/run_skin_tests.py: PASS ASan/UBSan/bounds/float-cast-overflow, actual Pikachu metadata, interpolation/root/scale, coordinate saturation and pose ownership.
- python3 -B tests/mm_wolf/run_skin_tests.py --fast-math: PASS with the same sanitizer suite plus -O2 -ffast-math.
- python3 -B tests/mm_wolf/run_host_tests.py: PASS after final first-frame native-freeze permitted/suppressed mash and D-pad hook-policy assertions.
- python3 -B scripts/diagnostics/run_mm_nei_tests.py: PASS after restoring the two new host seam boundary stubs in existing extracted use-item/action fixtures.
- git diff --check: PASS.

## Checkpoints and limits

Reconstruction commits before the final safety checkpoint: 12d05550 (skin options), 7de4d22c (loader/donor), 02ff213f (real Pikachu layout), 6c0adc44 (native damage ownership), 2307f56d (graphics arena fixture), 7a2d9ec6 (MM host seams). No push was made from the isolated reconstruction branch; root owns review/integration/publication.

Actual Wolf binary/game assets and game runtime were not used for acceptance. The valid binary fixture is a complete donor-format synthetic rig (37 bones, all 26 required clips). Renderer/actor matrix submission, allocator, sound/collision submission, several model/tool queries and native animation/damage completion dependencies are test boundaries. Native freeze/mash/input-selection bodies and production core/skin/ext-button code execute unchanged.

The aggregate clip bounds are conservative; an unusually large/canceling authored rig could be rejected even if its displayed pose fits. The real donor binary must be checked in game. Activation requires no live held item/tool action, so the native held item must be stowed first. Actual model fitting, camera/focus, enemy damage tables, freeze visuals, interactions and passive equipment/anklet attachment on Wolf remain runtime checks. Existing normal-player held/equipment/attachment paths and save layouts are unchanged. Full Windows/Linux builds/packages, the root combined regression gate and user game acceptance remain separate integration gates.


Independent review separately reran core/host/skin/fast-math fixtures after the effective-input repair. A proposed extra state1_20000000 defect was withdrawn after checking its existing IN_CUTSCENE alias and a native dispatch reproduction; the fixture now records that existing yield behavior.

## Shared graphics-arena capacity checkpoint

- Independent review reproduced valid skin drawing with an opaque arena 16 bytes short: the native command head crossed its matrix tail. Per-draw geometry is persistent; the dynamic cost is 9 opaque Gfx (10 with material) plus ALIGN16(sizeof(Mtx)), with two temporary Gfx slots in each xlu/overlay arena for nested native markers. On this 64-bit fixture ABI those bounds are 208/224 opaque bytes and 32 bytes per side arena.
- The fixture now compiles the exact Graph_OpenDisps/Graph_CloseDisps and Gfx_SetupDL25_Opa bodies from native MM source, instead of replacing them. RED: a required-minus-one-byte arena submitted a draw. GREEN: preflight skips null, reversed, short opaque and one-slot xlu/overlay arenas without changing any command bytes, pointers, packed vertex buffers, buffer index or prior pose owner; exact capacity and one additional byte render without crossing head/tail. Actual production Pikachu still renders.
- SSBBSkin_Draw reports whether it submitted a draw; existing callers may continue ignoring the result. Rejection happens before pose or vertex mutations and before native graphics helpers.
- Fresh normal and -O2 -ffast-math skin fixtures pass ASan/UBSan/bounds/float-cast-overflow. Native matrix conversion and resource display-list submission remain test boundaries. Full platform/runtime acceptance is unchanged.
- The new Wolf Player_Draw continuation needs a matching capacity-skipped result and its own marker/freeze overhead preflight; that follow-up is tracked separately from this shared-skin checkpoint.
