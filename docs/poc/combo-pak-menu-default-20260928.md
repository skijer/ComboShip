# ComboShip missing PAK controls

Baseline: PR #22, `43d8443c7d96e1ee4dbfd5209b8bc5ea7577bfc2`.
Candidate: `poc/combo-pak-menu-default-20260928`.

The supplied screenshot shows Bow, Hookshot, Hammer and Deku Stick but no
Slingshot/Boomerang; Fairy Ocarina is absent and Child Masks is empty.
The native slot registry contains all 27 entries. ComboShip's delegated SoH
widget bridge checks unset CVars with fallback 0 before drawing. Slot dropdowns
use -1 for inheritance. If model 0 lacks a slot, the bridge returns without
drawing that slot. The production-function fixture reproduces this failure.

Use each combobox's configured default in that guard, matching CVarCombobox.
Keep explicit selections and the invalid-selection guard. Standalone SoH's
integration branch has no DrawWidgetByIndex bridge, so this defect is not
present there. This diagnosis does not depend on the renderer optimizations.

Also carry the SoH voice scanner fix into ComboShip: recurse under the resolved
mods directory, accept mixed-case .pak extensions, and exclude the sibling
2ship subtree. The initial voice fixture failed on nested and mixed-case files;
it passes after the change. Archive parsing/audio decoding are fixture boundaries,
not real-pack runtime proof. The root resolver itself is unchanged: this test
covers mods/soh beneath the selected mods root, not an independently located
soh/mods directory. The user's exact on-disk layout and packs remain unverified.

Preserve model/slot registration, archive parsing, asset contents, selections,
renderer optimizations, MM behavior and audio playback. No master promotion.

Verification: production bridge fixture failed before the correction and passed
afterward (unset/inherit/explicit/invalid/nonzero-default/empty/null cases).
Voice filesystem fixture failed before and passed afterward with UBSan.
Changed C++ files pass clang-format 14; git diff --check passes. Independent
read-only review found no blockers. Both tests are added to the existing CI gate.
Full platform builds and in-game behavior remain untested for this candidate.

Runtime check: with the same packs/configuration, open Pak Loader and confirm
all 27 slots, especially Slingshot, Boomerang, Fairy Ocarina and the eight masks.
Confirm existing selected overrides remain selected. Check voice discovery and
playback with the user's actual pack location. Recovery is unchanged PR #22.
