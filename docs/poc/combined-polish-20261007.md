# ComboShip daily polish integration — 2026-10-07

Baseline: remote `develop` at `13901c677d0084e0305eec4672c0557f4434aea7`, the previous cumulative merge. Candidate: `integration/todays-polish-20261007`. cor authorized integration into the latest ComboShip branch and remote publication. The original baseline is retained as `checkpoint/pre-polish-20261007`.

| Input | Included work |
|---|---|
| Final Polish POC4 | Reusable empty/potion bottle recipes, live palette slots, gold Trident/Roc's Boots shimmer, selected native MM boss rigs and matrix type safety |
| Sword/Autumn bridge | Separate sword model/effect fit, active-host Alt selection and retry; 16/32/48 world-space leaf layers, showers and reversible scene foliage |
| Tunic/Ikana recovery | Missing tunic fallback, complete replacement priority, horizontal shield correction, MM-owned Ikana receipt/equipment icons |
| Final key-dialogue bridge | Morning single-box map fit, junk/song receipts and song GIs; all 34 supported dungeon key identities/colors and shuffled Chest Game dispatch |
| Cape visibility QoL | Per-game saved Show/Hide choice in native and foreign cape receipt flows |
| Sand/Storm behavior POC | OoT Sand placement cadence/cost parity and MM Storm collision centering |
| Shadow Crystal POC1 | Original black/orange geometry with charcoal/copper shimmer and particles |
| Elemental Arrow POC1 | Fire/Ice/Light receipt auras and one shared shimmer in native and foreign draws |
| Complete equipment review seed | All equipment/progressive tiers plus retained dungeon-item, soul, Tingle and bottle cases |
| Older cape texture and dungeon/bottle routing checkpoints | Already retained from the previous merge; confirmed against their production deltas and covered again by the combined fixtures |

Shared files were merged by delta. The sword matrix changes retain the appended Gold effect bounds. Elemental arrows suppress the wrapper's duplicate shimmer while retaining sword effect fitting. Key text uses the newer complete identity/color policy with POC4's localized wording templates. The final bridge's dungeon colors and Chest Game receipt supersede POC4's older green-only/native-tutorial expectations. Native and foreign grant behavior and counters remain protected.

The `tools/polish_20261007` review kit carries the exact owner-specific bottle archives, repaired dungeon-key archive, editable keys, rollback, original bottle input and equipment test files. It does not install historical scene fixtures. Generated arrow/Shadow Crystal assets are embedded by the normal custom-resource build.

Verification is recorded for the combined candidate, rather than inferred from individual POC logs. In-game visual acceptance is pending: use the review-kit instructions and the subsystem POC records to compare the intended packs and independent owner Alt settings. Application compilation and publication do not establish GPU/gameplay acceptance.
