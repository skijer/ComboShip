# Elemental Spin Styles Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add independently selectable spin appearances and optional sword-imbue
polish to eligible powered spins in both games, including inherited elemental hits.

**Architecture:** A shared pure resolver separates base style, native elemental
materials, and accent element. Each native spin actor owns its snapshotted
presentation and original damage flags. Private resource namespaces avoid pack
collisions; existing audited sword damage handling admits exact owned spin hits.

**Tech Stack:** C/C++, existing ImGui widget/CVar menus, native GBI display lists,
existing binary texture/array loaders, Python asset and native-fixture scripts.

**Spec:** `docs/superpowers/specs/2026-10-23-elemental-spin-styles-design.md`.

## Global Constraints

- Dropdown order/default: Native (0), Fire (1), Ice (2), Light (3), Lightning (4), Arcane (5), Astral (6), Dark Energy (7).
- `gEnhancements.Graphics.SpinStyle` defaults to 0; `gEnhancements.Graphics.ElementalSpinEffects` defaults to 1.
- Preserve native spin damage, charge thresholds, size/range, timing, magic requirements/costs, and Great Spin progression. Add no elemental surcharge.
- Ordinary sword swings remain free and retain their existing imbues, damage, cutting, HUD badge, equipment selector, and per-sword saved choices.
- Custom variants must work with Alt off and on.
- Add at most 12 accent particles for a regular burst and 24 for a great burst.
- Keep all unrelated source edits and accepted input archives untouched; commit only owned paths.

## Review Focus

- A style change during charge/release must affect the next actor, with no dangling resources or mixed active effect.
- An empty meter, denied magic request, or missing Great Spin upgrade must retain native behavior.
- The same element bits on a real arrow, custom weapon, or foreign collider must not grant spin ownership.
- Missing private resources and Alt changes must preserve a native fallback and leave following draws intact.
- Frozen/stunned repeat hits and Baba/grass cuts must retain native spin damage and reactions.

---

## Execution boundary

The user approved the expanded plan, direct execution and publication of candidate
checkpoints on October 23, 2026. Execute all four tasks in this isolated worktree,
then obtain one independent whole-branch review. Runtime acceptance is separate.

### Task 1: Independent style policy and menu controls

**Files:** Create `combo/render/SpinAttackStyle.h`,
`tests/spin_style/policy_test.cpp`, and `scripts/diagnostics/run_spin_style_tests.py`.
Modify `soh/soh/SohGui/SohMenuEnhancements.cpp` and
`mm/2s2h/BenGui/BenMenu.cpp` beside the existing sword visual controls.

**Interfaces:** Produce `CwSpinStyle` with the eight values in Global Constraints;
`CwSpinVisualSelection { CwSpinStyle style; uint8_t nativeMaterialElement;
uint8_t accentElement; }`; and
`CwSpinVisualSelection CwSpinStyle_Resolve(int requestedStyle, int effectsEnabled,
uint8_t swordElement, int eligiblePoweredSpin)` in the shared header.
Its element IDs match `sword_element.h` (None, Fire, Ice, Light).

- [x] Write `policy_test.cpp` assertions: invalid indices resolve Native;
  Native+Fire+enabled resolves Fire material and Fire accents; Arcane+Ice+enabled
  resolves Arcane, no native material, and Ice accents; effects off clears both
  elemental presentation fields; no imbue retains every requested style;
  Astral and Dark Energy keep their styles with any optional accent element;
  an ineligible spin receives no new presentation.
- [x] Run `python3 -B scripts/diagnostics/run_spin_style_tests.py --policy`;
  verify the new test fails because the production policy is absent.
- [x] Implement the pure resolver and both always-visible controls using the
  existing `WIDGET_CVAR_COMBOBOX`, `ComboboxOptions` and checkbox APIs. Use
  "Spin Style" and "Elemental Spin Effects" as labels. Tooltips explain next-spin
  application, separate combat ownership, and the full Native elemental treatment.
- [x] Run the policy test normally and with `--sanitize`; inspect both native
  menu registrations to verify identical CVars, order and defaults with no
  arrow/imbue visibility gate.
- [x] Commit the owned policy/menu/test changes.

### Task 2: Private variant resources and elemental presentation

**Files:** Create `scripts/assets/build_spin_style_resources.py`,
`combo/render/SpinAttackStyleResources.h`, `combo/render/SpinAttackAccents.h`,
`tests/spin_style/resources_test.py`, and `tests/spin_style/render_test.cpp`.
Generate private resources under `soh/assets/custom/objects/combo_spin/` and
`mm/assets/custom/objects/combo_spin/`; extend the Task 1 runner.

**Interfaces:** Consume `CwSpinVisualSelection`. Produce a shared resource recipe
`CwSpinResourceRecipe { const char* charge; const char* disk; const char* cylinder;
}` and `CwSpinResourceRecipe CwSpinStyle_Resources(CwSpinStyle style,
uint8_t nativeMaterialElement, int greatSpin)`. Produce TU glue
`CwSpinAccents_Draw(PlayState* play, uint8_t element, int greatSpin,
float phase, float alpha)` for both native GBI environments. Its caller supplies
the world-space spin frame and native effect lifetime.

- [x] Resolve/materialize the five exact archives named in the spec. Record
  SHA-256 and complete entry hashes. Inspect actual root DLs, nested DL calls,
  vertex/texture references, format/dimensions, and both charge levels before
  adapting them. Locate corresponding final OoT archives if needed; do not
  reinterpret an MM binary against OoT without verifying its resource commands.
- [x] Write resource tests requiring complete private graphs for all seven
  custom styles and three Native elemental materials, regular/great coverage,
  native/Alt availability, valid vertex indices and unchanged original resources.
  Write draw tests asserting particle caps, deterministic motion, native alpha,
  and matrix/segment/color/resource-manager cleanup.
- [x] Run `python3 -B scripts/diagnostics/run_spin_style_tests.py --resources --render`
  and confirm the missing recipes/renderer cause the intended failures.
- [x] Implement the deterministic generator and private resource recipes. Reuse
  the located Fire/Lightning/Arcane/Astral artwork and geometry, retargeting all private
  references; use compatible existing elemental materials for native-shaped
  Ice/Light. Emit full-image texture metadata compatible with current loaders.
  Preserve authored dimensions and sampling domains; record generator inputs
  and output hashes. Generate no global replacement of native resource names.
- [x] Add the proposed Dark Energy style with dark violet wisps, a crimson
  inner glow and sparse red sparks. Reuse compatible existing intensity
  resources and native rendering primitives; retain bright edges and the native
  fade so it stays visible beside dark player models. It grants no new element.
- [x] Implement a bounded accent pass: Fire embers, Ice crystals, Light sparkles;
  normal/great caps 12/24, no world actors or gameplay RNG. Preserve each selected
  custom style's original material and motion beneath this pass.
- [x] Run resource/render tests normally and under ASan/UBSan; run
  `run_spin_effect_texture_tests.py` and `run_mm_spin_effect_texture_tests.py`.
  Verify fallback on absent/malformed resources without a partial draw.
- [x] Commit only the generator, private resources, recipe, accent and tests.

### Task 3: Native actor presentation and inherited elemental hits

**Files:** Modify both `soh/src/overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.{c,h}`
and `mm/src/overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.{c,h}`;
both `src/code/din_fire_sword.c` files; and
`combo/gameplay/elemental_sword_finish_impl.h` only where exact attack ownership
requires it. Create `soh/include/spin_attack_style.h`,
`mm/include/spin_attack_style.h`, and `tests/spin_style/native_actor_test.cpp`.

**Interfaces:** Consume Tasks 1/2. Each spin actor stores style/toggle snapshots,
the release element and its original/applied attack flags. Produce native
`uint32_t SpinAttack_OriginalDamageFlags(PlayState* play, const ColliderInfo* hitInfo)`
in OoT and the equivalent `const ColliderElement*` signature in MM. It returns
unchanged flags unless the pointer belongs to an exact live eligible local spin
cylinder in that scene. Implement against live actor ownership without a
long-lived collider-pointer cache; declare it in the respective new header.

- [x] Write production-branch fixtures for regular/great release, successful and
  denied magic, no magic acquisition, empty meter, preserved damage/radius/timers,
  style changes midcharge, paused animation, and safe scene teardown. Assert
  excluded beam/cone/custom-form paths retain their native calls and flags.
  Test Fire charge preview, pause/equip Ice, resumed Ice preview, Ice release,
  then equipment change during fade: both release appearance and combat stay Ice.
- [x] Write collision assertions for None/Fire/Ice/Light at both spin levels;
  preserve native numeric rows and spin multipliers on sword-admitted enemies,
  add only already-audited elemental reactions, retain Baba/grass cutting and
  frozen/stunned repeat-hit behavior. Real arrows, foreign and stale pointers
  must fail the ownership check.
- [x] Run `python3 -B scripts/diagnostics/run_spin_style_tests.py --native --combat`
  and verify failures demonstrate the absent actor/collider integration.
- [x] Capture presentation preferences in native initialization; update the
  charge element preview from the current sword only on gameplay updates.
  Snapshot the actual element after the native powered release succeeds. Keep
  the original magic calls and native state transitions. Apply only the matching
  element bit to this cylinder; preserve native original flags in actor state.
- [x] Extend `DinFireSword_OriginalDamageFlags` through the exact owned-spin
  helper, then retain the existing audited `DinFireSword_DamageEntry` rules.
  Verify MM frozen-admission and finish observers use recovered original flags
  for the new owned cylinder without broadening genuine arrow behavior.
- [x] Select the complete private recipe in both native draws, retaining the
  current fallback for missing resources. Submit elemental accents only for
  the eligible powered effect; Native/None follows the current DLs and cosmetics.
- [x] Run native/combat tests normally and with sanitizers, plus the existing
  elemental sword state/damage/aura/finish tests. Compile affected native units
  in standalone and Combo modes. Commit the bounded actor/combat integration.

### Task 4: Integration evidence and recoverable delivery

**Files:** Extend `.github/workflows/build-artifacts.yml`'s Fast source checks
and its helper `scripts/ci/route_changes.py` only as needed to register the gate.
Add `docs/poc/elemental-spin-styles-20261023.md` and the final
verification record alongside it. Update this plan's checkboxes.

**Interfaces:** Consume the completed policy/resources/native integration and
their `scripts/diagnostics/run_spin_style_tests.py` command-line gates.

- [x] Run the full new gate and existing `run_spin_gi_tests.py`, both spin texture
  scripts, and `run_elemental_sword_regressions.sh`; inspect every result. Register
  the new fast gate beside the existing gates after confirming normal and
  sanitizer execution requirements. Do not replace older checks.
- [x] Run clang-format 14 over changed C/C++ files, `git diff --check`, and inspect
  the diff for unrelated source/resource changes. Run appropriate native build
  checks once; report source fixtures, compilation and runtime separately.
- [x] Record exact parent/head/tree IDs, required resource hashes and generation
  recipe, actual command results, unresolved failures and a small gameplay matrix.
  Preserve a durable source checkpoint and verify restoration independently
  before any long build, publication or context handoff, following `AGENTS.md`.
- [x] Obtain an independent whole-branch review using the execution method
  selected by cor; resolve material findings before delivery.
- [x] Deliver the candidate and concise runtime checks for both hosts: no magic,
  both charge levels, Native Fire/Ice/Light, Arcane+Ice, Astral, Dark Energy,
  effects off, a live style
  change, Alt off/on, pause/scene teardown and representative enemy/cutting cases.
  Do not label it runtime-proven, accepted or promoted before that evidence exists.

## Plan self-review

Policy independence and defaults map to Task 1; complete looks and safe resource
ownership to Task 2; inherited elemental combat and native magic/progression to
Task 3; integration, recovery and acceptance evidence to Task 4. All five Review
Focus cases have explicit owning tests. Native actor types stay host-specific;
the shared resolver and recipe use no host structs. No feature implementation
or in-game verification is claimed by this planning checkpoint.
