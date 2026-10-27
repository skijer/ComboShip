# Task 4 — native MM sword-damage diagnosis

Status: DONE_WITH_CONCERNS (bounded diagnosis; no proven native damage regression and no production fix).
Base: `26915d4f709674d5479a3ac5bfb277c85b49c0d9`.

## Finding and evidence

The uploaded configuration contains `gRando.Options.RO_SHUFFLE_ENEMY_SOULS = 1`. Native `mm/2s2h/Rando/ActorBehavior/Souls.cpp` registers `VB_PERFORM_AC_COLLISION` when the active save is randomizer and its saved Enemy Souls option is enabled. Its callback returns `HaveEnemySoul(ac->actor->id)`. The native map includes Chuchu and Deku Baba, while grass is absent and therefore admitted. `CollisionCheck_SetATvsAC` returns before setting AC_HIT when that callback rejects a collision. This happens before numeric damage resolution and independently of either Din checkbox. Menu.cpp explicitly describes enemy immunity until the corresponding soul has been obtained.

The user files supplied during diagnosis were read directly from the upload paths, not changed or committed:
- `/workspace/scratch/fa90949b48fc/upload/comboship(2).json`: DinFireSword=1, DinFireSwordDamage=0, ForceForm=0, gSm64Mario=0, Enemy Souls generation option=1.
- `/workspace/scratch/fa90949b48fc/upload/Fleet of Harkinian(1).log`: MM randomizer initialization applies 899 placements at 12:49:55.903. Repeated subsequent native UseItem entries show Kokiri item 0x4D mapping to action 0x03, held/current 0x03, human transformation 4. This does not support an unknown/zero-damage item-action mapping for those logged swings.
- Controller independently identified a successful Real Bombchu enemy drop at 12:54:41.720; Souls.cpp deliberately excludes Real Bombchu/Flying Pot from soul gating. This is corroborating evidence, not an observed per-hit trace.

This is a config-consistent explanation, not confirmation of active-save soul state: the JSON is generation/menu configuration, and neither it nor the log supplies the active save's Enemy Souls setting and owned Chuchu/Deku Baba soul flags at impact. No damage semantics should be changed on this evidence.

## Source investigation

Traced native strike setters, Din flag ownership/refresh, adult override/PostLimb delegation, custom-form dispatch, quad stamping/registration, collision admission/resolution, and numeric damage modifiers. Din replaces only flags, preserving numeric native sword damage. Adult PostLimb delegates native held-weapon collider handling; custom forms do not dispatch when no form is active. Native sword setters use positive human sword damage; the no-weapon row is zero, but supplied Kokiri log mappings do not select it. RPG Power only retains/increases damage. Ivan multiplier initializes to 1; its ownership gates and other numeric writers do not establish the supplied failure. IK Axe's inactive overwrite can reduce an upgraded sword to 1, but cannot explain complete immunity and was left outside this task.

The existing fixture directly supplied receiver AC_HIT/acHitElem and returned true at every GameInteractor boundary. It therefore skipped the precise admission gate implicated by the uploaded configuration. Its earlier passing result was not treated as reproduction of the report.

## Bounded native proof

Expanded the existing diagnostic with the unchanged production Souls.cpp map, HaveEnemySoul, registration condition, and collision callback, extracted at build time. Only hook dispatch and saved ownership bits are fixture boundaries. Calls execute production `CollisionCheck_SetATvsAC` followed by production `CollisionCheck_ApplyDamage`; receiver hits are no longer fabricated for these cases.

64 cases cover Chuchu, ordinary Deku Baba, Kusa, Obj_Grass; Enemy Souls off/on; souls unowned/owned; Din visual off/on; Din damage off/on. The fixture uses the logged Kokiri action, adult ownership, production strike setters, Din update/refresh, real enemy damage tables and grass masks. Results:
- Souls enabled and unowned: both enemies receive no AC_HIT and zero damage.
- Souls disabled or owned: both enemies receive AC_HIT and 1 Kokiri damage.
- Grass receives AC_HIT and numeric damage 1 for all combinations (its cut signal is preserved).
- Both Din checkboxes leave these outcomes unchanged.

## Commands and results

`python3 -B scripts/diagnostics/run_mm_din_fire_damage_tests.py` — exit 0. New 64-case admission proof passes; the five existing damage/reaction/ownership/drop/ice/mine groups pass in the same affected command. Compiles the real Din production C module, native extracted collision/strike bodies, full actor translation units and the new C++ hook fixture against production headers. Existing controller-button macro redefinition warnings remain. No full native game build or runtime launch is claimed.

Red/green: there is no failing bug regression or code-fix red/green sequence. The new diagnostic reproduces immunity as expected native behavior with an enabled/unowned soul gate and demonstrates admitted damage with that condition disabled or owned. No speculative production patch was written. Per controller instruction, no additional unchanged-suite checks were run after this sufficient bounded proof.

`git diff --check` — clean before commit. Formatting applied to the touched C/C++ fixtures. Self-review confirmed only test/diagnostic/report files plus controller-authored Task4 plan/spec additions are committed; no accepted Tasks1–3 production changes, artwork, native transforms, assets, shared ownership, SoH, PAK/voice, remote mutations, or save/config edits.

## Changed files

- `mm/tests/din_fire_damage_test.c`: native collision-admission cases and hook dispatch boundary.
- `mm/tests/din_fire_soul_gate.cpp`: test adapter executing extracted native soul registration and ownership logic.
- `scripts/diagnostics/run_mm_din_fire_damage_tests.py`: Chuchu table and unchanged native admission/hook extraction/compilation.
- `docs/superpowers/plans/2026-09-29-mm-nei-parity.md` and `docs/superpowers/specs/2026-09-29-mm-nei-parity.md`: retained controller-authored Task4 additions.
- This report.

## Runtime limit / precise next observation

Confirm the loaded slot's saved `RO_SHUFFLE_ENEMY_SOULS` and `RANDO_INF_OBTAINED_SOUL_OF_ENEMY_CHUCHUS` (and one ordinary enemy soul) at the failing encounter, or observe whether obtaining that soul immediately restores damage. Do not infer live ownership solely from the generation CVar. The diagnostic does not render the Din/adult skeleton, measure a swept quad against the actual enemy in Termina Field, or execute full player update/draw/collision registration. If the saved gate is off or the corresponding soul is already owned, capture that failing encounter's melee numeric damage/flags, registered AT/AC, and the `VB_PERFORM_AC_COLLISION` result; a separate native failure would remain to diagnose. No local runtime assets were available.
