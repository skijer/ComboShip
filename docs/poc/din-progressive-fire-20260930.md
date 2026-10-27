# Din progressive sword fire candidate

User request: preserve the existing Din fire sword across every NEI progressive
sword upgrade in ComboShip. Cane of Byrna, Trident and Four Sword are explicitly
excluded; the user's clarification retains the NEI progressive sword work.

## Baseline and scope

- Code parent: `a6ae6e1b84108de857dbf75d65b3a734346c5cb4`, including the MM Time Gate SFX fix.
- Isolated branch: `poc/din-progressive-fire-20260930`.
- Asset parent: `Din_Fire_Sword_Shield_Combo_POC1.o2r`.
- Asset SHA256: `4c6765488e1669ff94a7a504e2b2c507c893f3b673fd7143f93c551db2b17895`.
- Candidate: `Din_Fire_Sword_Shield_Progressive_Combo_POC1.o2r`.
- Candidate SHA256: `5f7972b03e494aa3c309860dc2e91d85ab3bbe18c61244c06d063f8205e27306`.

Actual NEI progression is Kokiri -> Razor -> Gilded, Master -> True Master,
and Biggoron -> Great Fairy. True Master continues the existing Master path.
Native MM Razor/Gilded/Great Fairy IDs are covered alongside the NEI flags.

OoT's upgrade compositor changed its effective hand type to OPEN, bypassing
fire capture. MM explicitly excluded upgrades and native MM upgraded sword IDs.
An O2R cannot change either executable guard. The matching engine fix is required.

The new archive preserves every one of the baseline's 259 entries byte-for-byte.
It adds three private held-sword aliases (adult, child, full BGS) plus candidate
notes. Existing meshes, fire textures, shield, cosmetic bindings, stowed equipment,
body, hair, animations and progression are untouched. The enabled fire option
selects the matching existing Din blade for upgrades after equipment hooks;
fire off, Alt off or absent private resources restores the usual renderer.
Explicitly hidden PAK hands remain hidden. Four Sword, transformed forms and
other weapon owners remain excluded.

The full Great Fairy model wins over a leftover broken Giant's Knife inventory
bit. This is verified at the final OoT hand-composition stage, after equipment
and PAK overrides, to avoid a full flame shell over a broken mesh.

## Evidence

Passed:

- `run_din_fire_sword_tests.py`: real OoT renderer, child/adult/BGS/broken,
  new progressive meshes, cosmetic colors, lifetime and option/asset fallbacks.
- `run_mm_din_fire_tests.py`: real MM renderer, both ages, native and NEI
  upgrades, mesh/profile selection, other owners, option and old-pack fallbacks.
- `run_din_progressive_hand_tests.py`: production final OoT hand stage;
  fitted GFS survives late equipment/PAK replacements, hidden/off paths retained.
- `run_din_fire_sword_damage_tests.py`: production collision, sword power,
  enemy reactions, drops, fire interactions and toggles. Test dependency:
  official nlohmann/json v3.11.3 single header supplied through CPLUS_INCLUDE_PATH.
- `run_mm_din_fire_damage_tests.py`: native sword powers, 13 enemy tables,
  grass masks, reactions, drops, soul gating, ice blocks and mines; native
  Razor/Gilded damage remains unchanged with the fire flag enabled.
- Complete modified OoT `z_player_lib.c` compiled to an object with production
  headers, `COMBO_BUILD` and `OOT_BUILD_DLL`.
- ZIP CRC, exact baseline preservation, three aliases and their complete
  66-entry resource dependency closure.
- `git diff --check`.

The MM progressive renderer case failed at `DrawSword() > 0` before the fix.
Pre-existing MM controller macro warnings and OoT pointer-type warnings remain.
The sparse checkout initially lacked CMake/assets headers; those were restored
from the same commit. No production changes were made for test dependencies.

Not claimed: full platform build/link, CI pass, game execution, visual acceptance,
or master promotion. Module fixtures simulate engine resource/allocation boundaries;
they are not an in-game session. An independent code review found the late-equipment
overwrite, which is fixed and covered by the final-stage regression.

## Install and runtime check

Install a build containing this patch. Replace the old Combo fire O2R with this
candidate in both `mods/soh` and `mods/2ship`, then restart. Keep the player body/hair
pack. Enable Alternate Assets and Din Fire Sword. Fire Damage remains optional.
Shield settings are unchanged; no item is granted.

Check every progressive stage, both MM ages, draw/swing/stow, cosmetic colors,
pause/re-entry, fire/Alt off, ordinary grass/enemy hits and Fire Damage off/on.
Retain the existing mod pack as rollback. This remains a candidate until the user
accepts its in-game result. Reverting the archive removes progressive aliases and
causes the engine to fall back to ordinary upgrade handling.
