# Equipment pause repair implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this bounded repair. The user has authorized the ongoing ComboShip fixes and checkpoint pushes.

**Goal:** Restore MM additional-equipment titles, imported OoT equipment icons/titles, and the correct passive Magic Cape icon.

**Architecture:** Keep the native pause draw and ownership/action paths. Publish full equipment IDs, select ambiguous names by grid/passive context, refresh cached titles on cell/context changes, and resolve OoT resources through their owning resource manager. Isolate the passive Cape icon from the legacy purple-tunic replacement path.

**Tech Stack:** Native C/C++ pause code, OTR texture paths, Python compiled production-function regression fixtures.

**Spec:** This request: every additional-equipment title is blank; Master Sword, Goron/Zora Tunics and Iron/Hover Boots are missing; Cape is a purple tunic with no title; Pendant has its icon but no title.

## Global constraints

- Parent: PR #34, `92b72bf29d15b80f3bfd88402319bae09b74c895`.
- Candidate: `poc/mm-equipment-pause-20261006`; do not advance PR #34 or master.
- Preserve native item grants, ownership, saves, equipment effects and unrelated GI/weather work.
- Preserve original native Cape artwork; no new GI/icon redesign.
- Source/build verification is distinct from GPU runtime acceptance.

## Review focus

- Cape/Champion and Pendant/Climb Boots reuse IDs across cells/pages.
- OoT assets may exist only in the donor manager; Alt modes differ between managers.
- Native MM Kokiri progression and Great Fairy Sword retain their own titles.
- Missing resources must not reuse a previous TMEM texture or read beyond the title table.
- Unowned imported equipment must remain unowned and unequippable.

### Task 1: Repair equipment presentation

**Files:** MM `z_kaleido_equipment.c`, `z_kaleido_scope_NES.c`, both hosts' `ext_equip_names.c` and `extended_equipment.c`, two copied native Cape PNGs; `tests/mm_equipment_pause/run_tests.py`.

**Interfaces:** Preserve `ExtEquip_GetNameTex(u16,u8)` and `ExtEquip_GetCapeIcon(void)`; add `KaleidoEquip_GetNameTex(void)` for the MM equipment cell title; use existing `NeiResource_Available/Route`.

- [x] Compile production cursor/name/resource selectors against fixtures; observe failures for the baseline's missing names and donor-only icons.
- [x] Apply only the equipment-relevant recovered name changes; add donor texture routing and safe cache refresh.
- [x] Copy the exact native red/gold Cape PNG to a distinct passive path and select it in both hosts.
- [x] Run the fixture normally and under ASan/UBSan; run native-header syntax, asset collision, formatting and relevant existing tests.
- [ ] Record exact source/asset checks and remaining runtime checks, commit and push the isolated checkpoint, and open a draft build candidate.

### Task 2: Preserve and hand off

- [ ] Make a small compatible Cape override from the preserved native upscale and save the recovery patch/checkpoint bundle.
- [ ] Verify the published tree and build status. Report source/static results separately from any completed application build.
- [ ] Runtime check: browse every extended cell, move between ambiguous passive/grid cells and pages, inspect the five imported equipment icons with independently toggled Alt modes, and confirm correct Cape/Pendant names. No master promotion.

## Source verification checkpoint

- 75 compiled production-function checks pass normally and with ASan/UBSan.
- The parent fails the same focused fixture; the candidate restores all tested titles and donor-only icons.
- Four changed pause translation units pass native-header C syntax checks; no new asset collisions or formatting errors.
- Existing held-equipment checks pass. The editor and broader regression runners stop on missing SDL2 and ImGui headers in this environment; full application linking remains a CI check.
- Independent review found a stationary Alt-toggle title cache problem. A new regression reproduced three failures before the fix; owner changes now refresh the title while unchanged owners preserve its display timer.
- Actual GPU appearance, archive packaging and independent local/donor Alt combinations still need game validation on the candidate build.
