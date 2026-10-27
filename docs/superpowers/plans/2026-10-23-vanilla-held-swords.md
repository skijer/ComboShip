# Vanilla Held Swords Implementation Plan

> For agentic workers: execute the bounded render change and its regression tests together; the two menu bindings can be implemented independently. Review the whole contribution before publication.

**Goal:** Give players an optional cosmetic checkbox for original held sword appearances while retaining authored swords as the default.

**Architecture:** One shared, persistent CVar, `gEnhancements.SkijerNEI.UseVanillaHeldSwords`, is read during held-model selection. The checkbox appears in both games' Link & Items cosmetics tabs. Returning no authored model lets existing native/progressive fallbacks and custom-model priority resolve the draw. Four Sword must also skip its secondary authored blade/hilt fallback.

**Tech stack:** Existing C/C++ rendering, libultraship CVars, UIWidgets, native graphics-ABI regression fixtures and Python runners.

**Spec:** The October 23 user request: preserve authored defaults, custom equipment/body/hand priority, Alt Assets, missing-resource behavior, elemental effects, and GI/shop/pickup independence; cover OoT/MM sword families, progressive tiers, Four Sword, and live switching. Keep this contribution isolated and do not merge develop.

## Baseline and preservation

- Supplied source reference: PR #57 at `2244ebd16602a22aedec067c716cec21f6e474c6`.
- Verified current PR #57 head and candidate parent: `523e71e949dd1e9d18db1fb47cf17ecbbc24b710`, tree `b5f48b5547dd8ce3129afa35a402b755e5234e9a`.
- Isolated candidate: `poc/vanilla-held-swords-20261023`. The cumulative branch is `poc/market-forest-followup-20261022`.
- The existing held-sword selection/geometry tests pass before changes. Existing MM controller macro redefinition warnings are present in that baseline.
- No model resources, gameplay progression, player state, save layout, GI presenters, shop renderers, damage, collision or elemental-effect policies are changed.
- Original Four Sword means the existing legacy Four Sword blade/hilt, since the retail games do not contain a Four Sword.
- Other authored held items remain unchanged: their fallbacks use separate actor, hand/back or multi-pass systems. A general authored-resource gate would also affect new items and shared GI assets.

## Approach

Use a live gate in `combo/NeiHeldSword.h` plus the two secondary Four Sword branches. A gate at each player draw call would duplicate policy and risk skipping required native/progressive fallbacks. Gating generic resource availability or GI presentation would affect unrelated items and pickups. The shared held selector is the narrow boundary matching the request.

## Global constraints

- UI label: `Use Vanilla Held Swords`; default false in both editors.
- Shared CVar is literal; the games' cosmetic CVar macros have different prefixes.
- Do not add this boolean to palette randomization or color-reset tables.
- Read the option on each draw; do not clear resource caches or require a restart.
- Keep selected native/Alt body, hand, equipment and Four Sword pair resources authoritative under the existing rules.
- Preserve resource checks and fallback behavior when the option is unchecked.
- Persist the candidate separately and leave cumulative/develop/master refs unchanged.

## Review focus

- A populated secondary Four Sword authored pair must not bypass the checkbox.
- Live off/on/off switching must work with already-populated wrappers and native piece caches.
- Progressive native blade/hilt fallbacks must retain their native elemental aura profile.
- The same saved option must be visible in both editors, including its absent/unset default.
- Custom models and partial resources must preserve the pre-existing fallback behavior in both Alt states.

## Task 1: Held-model policy and native regression coverage

Files: `combo/NeiHeldSword.h`, both `mods/equipment/behaviors/equip_foursword.c` files, `tests/nei_held/held_sword_runtime_test.cpp`, `tests/nei_held/four_sword_visibility_test.cpp`, and their existing runners as needed.

- [x] Add behavior tests for default/off/on, all shared sword/frame selections, native and progressive fallbacks, live switching and custom equipment priority.
- [x] Add a real Four Sword getter case with both authored graphs present and distinct legacy blade/hilt resources; prove live fallback and selected-pair priority.
- [x] Run the new cases against the baseline and record the expected failures.
- [x] Add `NeiHeldSword_UseVanilla()` and gate only authored selection, including the secondary Four Sword pair.
- [x] Run held selection, Four Sword visibility and OoT native limb regressions; retain elemental aura checks.

## Task 2: In-game cosmetic controls

Files: `soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp`, `mm/2s2h/BenGui/CosmeticEditor.cpp`, `tests/nei_held/run_vanilla_sword_menu_tests.py`, and the existing CI regression inventory if needed.

- [x] Exercise the real menu checkbox binding and production CVarCheckbox behavior with controlled UI input; prove unset default and shared cross-game live changes.
- [x] Add the checkbox and explanatory tooltip in each Link & Items tab.
- [x] Run the menu test and register it with the equipment regression suite.

## Verification, recovery and handoff

- [x] Review the combined diff independently and format only changed C/C++ lines/files as appropriate.
- [x] Run the focused selection, real Four Sword, native limb, menu and elemental-render regressions; record actual results and limitations below.
- Publication contract: commit only the owned contribution paths to the separate candidate branch; verify the actual remote commit/tree and restored contents before reporting publication.

Configured in-game visual acceptance remains a separate step: toggle with a sword drawn in both games, both Alt modes and a selected custom equipment pack. Native harness success does not establish gameplay acceptance.

## Execution record

The implementation uses five production files: the shared selector, both Four Sword getters and both cosmetic editors. The remaining changes are regression coverage, CI registration and this source record. No assets changed.

Observed red/green evidence:

- The new held preference test failed against the baseline because authored selection ignored the checked CVar; it passes after adding the shared gate.
- The real Four Sword legacy-selection test failed both before the gate and with only the shared gate implemented, proving it catches the secondary authored fallback. Both real getters pass after the additional guards.
- Both production menu fixtures failed against the exact baseline with the checkbox absent, then passed after the controls were added.
- The existing equipment fixture needed a default-returning CVar service stub for the newly consumed dependency. Its first link failed with the missing symbol; the completed fixture and suite pass.

Verified commands:

| Check | Result |
|---|---|
| `python3 -B tests/nei_held/run_equipment_tests.py` | PASS; includes native XML loading, equipment actions/geometry/bindings, OoT/MM held selection, and OoT limb lifetime/ownership |
| `python3 -B tests/nei_held/run_four_sword_visibility_tests.py` | PASS; GI ownership plus 12 real held-getter scenarios across the two hosts, with both Alt states and child/adult frames |
| `python3 -B tests/nei_held/run_vanilla_sword_menu_tests.py` | PASS; actual menu and checkbox code, unset/saved defaults, repeated cross-editor switching, save/apply calls, inactive tabs and unrelated settings |
| `ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_elemental_sword_aura_tests.py --sanitize` | PASS; real aura GBI/state integration and full native aura source compilation, OoT/MM in solo and Combo modes |
| `python3 -B -m unittest discover -s tests/ci -v` | PASS; 21 checks |
| clang-format 14.0.6 `--dry-run --Werror` on all five changed production files | PASS |
| `git diff --check` | PASS |

Independent source review found no actionable Critical, Important or Minor findings. The review confirmed the held-only boundary, secondary Four Sword gate, live cache behavior, shared UI value, C/C++ linkage and preserved custom priorities.

The baseline MM controller macro redefinition warnings remain. Leak detection is disabled for sanitizer runs because the execution environment cannot inspect process tasks; AddressSanitizer and UndefinedBehaviorSanitizer remain active. No full Windows/Linux executable or configured gameplay/visual acceptance is claimed by these local checks.

### Recovery and integration

The candidate has exactly the current cumulative checkpoint as its parent and carries only this bounded contribution. Fetch `poc/vanilla-held-swords-20261023` from `marsh6487/ComboShip`, verify the immutable commit and tree reported with the contribution, and cherry-pick that commit onto the cumulative integration when ready. Reverting that single contribution restores the previous authored-only behavior. All model resources remain the baseline resources; no archive rebuild or optional pack is introduced.

The original supplied commit and the newer baseline above remain independently recoverable on GitHub. The separate draft contribution targets the cumulative branch, with develop/master promotion left to the user's authorized integration workflow.
