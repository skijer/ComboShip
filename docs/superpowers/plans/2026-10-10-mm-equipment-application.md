# MM equipment application regression plan

**Goal:** Make owned MM extended equipment activate consistently through A and C, retain its held sword identity on B, and avoid selecting a shield implicitly for Trident.

**Architecture:** Keep the existing equipment slot setter and ownership records authoritative. Route C/D presses to its toggle entry point, put Byrna on B using its own item ID and the native one-hand sword carrier, and let same-action sword changes update the held ID through the native item transition. Preserve Great Fairy combat perks, existing render/resource ownership, age/form guards and native input gates.

**Tech stack:** Native MM C, Python production-path regression drivers, actual MM headers and graphics ABI.

**Scope:** Isolated POC from `d371fb26284b6b6e7d05ef0888fb90e09ca9fc3d`; no push, merge or runtime acceptance. The parent task owns pause framebuffer stream isolation separately.

## Task 1: Extended sword carrier and held identity

Files: `mm/mods/extended_equipment.c`, `mm/mods/extended_player.c`, `mm/src/overlays/actors/ovl_player_actor/z_player.c`, `tests/mm_equipment_pause/run_ownership_tests.py`, `tests/mm_nei/run_use_tests.py`.

- [x] Add production native button/use cases for Trident-to-Four, Four-to-Byrna, native-to-extended and extended-to-native identity changes; execute the real item-action resolver.
- [x] Add owned Byrna A-selection checks for B identity, removal and no fabricated native ownership.
- [x] Run the tests against the baseline and confirm the identity/B failures.
- [x] Put all three extended sword IDs on B and alias Byrna to the existing one-hand sword action; extend the existing accepted Net identity transition to extended swords.
- [x] Run both regression drivers and held visibility/hand checks with real headers.

## Task 2: C/D toggle dispatch and explicit Trident shield policy

Files: the same native button source, equipment setter and regressions.

- [x] Add native C/D press interception assertions, B swing assertions and blocked-input controls; execute the real equipment toggle for owned tunics with the legacy toggle off.
- [x] Add Trident acquisition checks for incompatible native/extended shields, compatible explicit choices, earned ownership and page reentry.
- [x] Run the baseline tests and confirm missing toggle and auto-pick failures.
- [x] Intercept extended items only on C/D press in the existing accepted button dispatch; let the owned toggle entry point opt in through `ExtEquip_Equip`.
- [x] Preserve compatible explicit shields and clear incompatible shields without auto-selecting owned Divine/Ikana.
- [x] Run ownership/use/action/held suites, actual-header syntax, formatting and `git diff --check`; record runtime limitations and commit isolated delta.

## Review focus

- C/D equipment presses must not replace a currently held tool with action NONE.
- B presses must swing the extended sword rather than toggle it off.
- Same-action extended sword switches must refresh held identity without bypassing native acceptance gates.
- Trident must retain an explicitly compatible shield and preserve genuine ownership when clearing an incompatible shield.
- Equipment rendering still requires installed-archive/GPU acceptance; no model, particle or GI resource is changed here.
