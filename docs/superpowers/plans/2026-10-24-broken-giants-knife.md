# Giant's Knife held model and casing implementation plan

**Goal:** Keep the authored Giant's Knife appearance, with a visibly broken blade and a matching elemental casing when its native durability is depleted.

**Baseline:** `25e99033`, isolated branch `poc/broken-giants-knife-20261024`. Existing held selector and geometry tests pass; engine-header macro redefinition warnings predate this change. Runtime acceptance remains separate.

**Design:** Use the existing reviewed `giants_knife` GI for the intact held model. Derive a separate closed broken mesh from that same serialized mesh, retaining its iron guard, brown grip, textures, grip scale and socket. Append held and aura identities rather than renumbering existing identities. Generate the two new elemental fits from their actual serialized blade surfaces. The native health/flag state must select the same visual variant regardless of whether the held item is `ITEM_SWORD_KNIFE` or `ITEM_SWORD_BGS`.

**Alternatives:** Vanilla fallback is already authorized but would discard the preferred authored appearance. Reusing the full Biggoron mesh preserves the reported mismatch. The authored Knife variants follow cor's preferred solution.

**Preservation:** No damage, reach, durability, progression, save layout, animation, scene or audio edits. Preserve Great Fairy/Four Sword priority, selected custom body/equipment ownership, live vanilla-held checkbox, complete-resource fallback, and all existing authored/GI/aura resources.

## Task 1: Reproduce and correct the held identity

- [x] Extend the real held-selector fixture with literal intact/broken Knife resource expectations for both native item IDs and both OoT ages; watch the old source fail.
- [x] Generate a capped fracture from the serialized authored Knife without touching its source; create intact/broken wrappers with the existing sockets and uniform full-sword scale.
- [x] Append new held identities, route by native durability, preserve overrides and vanilla fallback, and register explicit appended aura identities.
- [x] Run `python3 -B tests/nei_held/run_held_sword_tests.py` and focused serialized mesh checks.

## Task 2: Match elemental geometry and verify recovery

- [x] Extend the production held-selector/renderer fixture to assert short broken and full intact casing bounds, exact hand identity, all three elements and both ages; reproduce the failure first.
- [x] Append generated casing fits from the actual Knife variants. Keep all prior casing data unchanged.
- [x] Run held, elemental-aura and Biggoron suites; run actual native source compile checks and deterministic generators.
- [ ] Review the bounded diff, commit owned paths, create a durable source/recovery handoff, and verify an independent restore.

**Review focus:** broken-to-repaired state switches; complete-resource failure; selected custom longsword/broken hand priority; vanilla-held toggle; Great Fairy/Four Sword supersession.

**Runtime check:** In OoT with Alt Assets off, equip an intact Giant's Knife, break it, and replace it; compare Fire/Ice/Light on adult and child, then repeat with the selected equipment pack/Alt setting. A broken blade and all casings end at the same short tip. Build/static proof is not in-game acceptance.
