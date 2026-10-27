# OoT Four Sword / Cane of Byrna recovery

| Field | Record |
| --- | --- |
| Baseline | PR #43 source `2069d37e2a9338494acff3a70ca56ddaf2e035fa`; both defects remain in that source. The user's installed binary/archive identities are not established by this inspection. |
| Candidate | `fix/oot-four-sword-byrna-recovery-20261016`, isolated from the entrance-generation checkout. |
| Reported failure | OoT Four Sword remains invisible with native/Alt assets; Cane of Byrna equips but cannot swing. |
| Scope | Correct generated held-sword XML calls and the missing OoT Byrna button-action alias. |
| Preservation | All authored GI meshes, materials, textures, grip matrices, held reach, item IDs, saves, sword ownership/selection, native combat hooks, MM one-handed Byrna mapping, forest routing, weather, and Field repairs remain unchanged. |
| Status | Implemented candidate; local test results below. No linked executable/package or installed-game acceptance is claimed. |
| Publication | Retain the user's equipment-specific no-push instruction. No remote branch/PR update or master promotion. |
| Recovery | Remove this candidate's diff to return to `2069d37e`; the earlier isolated `38cc6fc0` object is absent from this checkout, so its commit is not claimed recovered. |

## Confirmed causes

The shared held-sword generator emitted `<DisplayList Path="..."/>` as a child command. The production XML display-list factory recognizes `CallDisplayList`, while `DisplayList` is only the document's root element. Its unknown-command path prints `DisplayListXML: Unknown node DisplayList` and substitutes `PipeSync`. The held wrapper therefore pushes the correct grip matrix, never calls the GI mesh, then pops. All 27 wrappers from that generator had this defect, including Four Sword's OoT child/adult and MM human frames.

Existing geometry tests incorrectly interpreted `DisplayList` as a call and explicitly expected that invalid tag. The old Four Sword selection fixture and old geometry fixture passed on the broken baseline; they did not prove that the engine submitted mesh geometry. The corrected geometry interpreter accepts the engine's supported call command. A new fixture compiles and invokes the complete production XML factory, its resource classes and bundled tinyxml2, replacing only logging.

OoT `ExtEquip_SetSlot` stores Byrna's own `ITEM_EXT_SWORD_1` ID on B. `ExtPlayer_GetItemAction` contained aliases for Four Sword and Trident but none for Byrna, so that item fell through the registry and vanilla table to `PLAYER_IA_NONE` (0). The correction maps Byrna to `PLAYER_IA_SWORD_MASTER`, following the existing Net/Cane melee relationship. The existing `ExtEquip_ByrnaIsTwoHanded` hook independently supplies OoT's stance; the Byrna combo hooks already accept an ordinary melee action. MM's existing one-handed mapping remains unchanged.

Review rejected an initial local Biggoron-action alias: it also selected real Giant's Knife durability/break handling and blocked charging with zero native sword health. The amended test reproduces that failure through the actual native durability and charge handlers. The final Master-action alias avoids those side effects without changing native sword handling or save values.

## Changes

- Correct `tools/nei_held/build_swords.py` and regenerate all 27 wrappers. Each wrapper changes only its mesh-call tag; generated manifests, transforms and source assets remain byte-identical.
- Add the missing OoT Byrna alias without changing the stored item ID or the other sword aliases.
- Exercise the actual XML loader and actual OoT action function through `tests/nei_held/run_equipment_regression_tests.py`. The existing canonical equipment runner invokes these checks before its selection/geometry tests.
- Correct the existing geometry interpreter to follow only the supported nested display-list command.

## Verification

Before production changes, the new XML test reported zero mesh calls and the unknown-node warning; the action test reported Byrna action 0. The corrected geometry expectation failed all 27 wrappers. These failures were inspected before implementing the correction.

The ordinary regression runner passes after the change: the real factory emits exactly one call to the expected authored GI for each of the 27 wrappers, and the actual OoT item-action function resolves Byrna to the existing melee action while retaining the other sword aliases. Geometry/source-hash checks pass for all nine sword bodies and all three frames, including balanced grip/GI matrix pushes and unchanged authored vertices/materials.

The existing `python -B tests/nei_held/run_equipment_tests.py` suite passes, covering both-host equipment selection, missing-resource fallbacks, native/Alt priority, held sword graphs, and OoT limb/clone handling. The older Four Sword visibility runner also passed on the baseline, as expected; it is evidence of retained selection behavior rather than a proof of the corrected mesh call.

The final real-loader and native-action/charge/durability fixtures pass with AddressSanitizer and UndefinedBehaviorSanitizer. Local LeakSanitizer scanning is disabled (`ASAN_OPTIONS=detect_leaks=0`) because this environment cannot inspect `/proc`; no leak-check result is claimed. Changed-source clang-format 14, Python syntax, whitespace and asset-collision checks pass. Read-only review corrected the unsafe initial Biggoron alias and the XML fixture's call/branch bit assertion; no remaining code blocker was identified. The full cumulative unrelated-feature gate and complete platform build are outside this local verification claim.

## Decisive installed-stack check

Build the candidate and use its executable with its newly generated `soh.o2r` (and matching packaged game modules). Reusing the old `soh.o2r` retains the malformed wrappers. Record installed packs/load order and the exact package revision.

In OoT, hold and swing Four Sword as child/adult with Alt off/on, including the installed player replacement packs and clones. Equip Byrna and verify that a button tap starts melee and its held model follows the hand. Verify a normal sword still draws/swings. The shared XML correction also requires an MM human/adult Four Sword visibility check; MM Byrna must retain its existing one-handed behavior. Runtime/GPU acceptance remains pending those checks.
