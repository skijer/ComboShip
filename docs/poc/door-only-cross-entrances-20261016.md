# Door-only cross-game shops and interiors

This extends the reviewed forest source `4245e544` and PR43's combined
`86582f5a` baseline. User authorization on October 16 covers merging this forest
work, the existing generation/reset and other earlier corrections, and narrower
entrance settings. The separate Demise visual candidate is outside this change.

| Control | Selection | Permutation pool |
| --- | --- | --- |
| Cross-Game Interiors | Existing complete 82-entry interior catalogue | Original Interior (2) |
| Cross-Game Shops (Doors Only) | Retail shop ingress with an opening door | Shop (4) |
| Cross-Game Door Interiors | Other ingress with an opening door | DoorInterior (5) |

Narrow controls default off and work without the dungeon option. Broad Interiors
takes precedence; its original catalogue order, categories and RNG remain intact.
The menu disables the redundant narrow controls while retaining their preferences.
Native entrance shuffle settings remain independent. New menu choices affect
new generation; an existing slot continues using its recorded map.

## Physical ingress classification

The OoT native reversible table supplies each forward/reverse pair. The narrow
classification explicitly admits real doors; unknown ingress defaults to Passage.
Kokiri Shop, every Kokiri house, Goron/Zora shops, tents, fairy caves and the Temple
of Time are excluded as a category. The original Mask Shop portal stays excluded.
Bazaar and potion-shop doors, including distinct Kakariko front/back doors, remain
separate entries. Granny's potion retail building counts as a shop.

Native OoT destinations are classified by their incoming physical source after
composing the native entrance map. An open passage mapped natively to a Bazaar
therefore stays outside the cross-game overlay; a Bazaar door mapped to Kokiri
Shop retains its door classification and native paired return. A target with
conflicting incoming kinds is conservatively excluded from narrow selection.
Broad interiors bypass this filtering and retain the original catalogue.

MM's existing 42 descriptors now record a kind per ingress. The native archive's
En_Door transitions corroborate Zora Hall doors, the Marine Lab, Tourist Information,
Romani doors, town retail/services and the Observatory's exterior door. The town
Observatory tunnel remains Passage. Fisherman's and Ghost huts, Goron Shop/Shrine,
Goron Graveyard, fairy caves and the Deku Playground hole remain Passage. Curiosity
Shop's storefront is a shop; its separate back hideout is another door interior.
Milk Bar retail purchases put its door in Shops. Minigames, Lottery and the smithy
remain other door interiors. Native schedule/access conditions are untouched.

## Seed, return and compatibility contract

Narrow groups have distinct pool identities, so they never exchange with each
other, dungeons, bosses, grottos or broad interiors. New narrow maps use entrance
seed schema v3. Versions 1 and 2 retain their recorded category identities, routes
and replay; they cannot masquerade as new narrow maps. Missing new flags default
false. Non-boolean flags, disabled categories and cross-category permutations are
rejected. There is no game-save version bump, migration or progress reset.

OoT checkpoint lifetime and boss reverse composition recognize both narrow pools
as interiors. They retain the same paired reverse, Farore and death behavior.
The forest save/session and cutscene guards remain as reviewed in the earlier
POC records. The matching modules and modified MM3D archive remain required for
the custom authored return and private foliage material targets.

## Verification record

The first option test failed because narrow-only generation stayed disabled; both
native catalogue tests failed because no requested ingress was selected. Production
menu, interior checkpoint and boss reverse-checkpoint tests fail when paired with
their original bodies. These controls establish that the tests exercise the change.

Current targeted checks pass: new v3 round-trip, isolated permutations over 50
seeds, v1/v2 rejection of narrow category tags, strict option types, exact native
membership and broad precedence, independent menu toggles and retained preferences,
and both new checkpoint cases. BenPort.cpp, OTRGlobals.cpp, ComboMenu.cpp and the
headless generator pass full translation-unit compilation against actual engine
and dependency headers. The headless executable builds, and both new flags reject
invalid values correctly. Existing compiler warnings are retained; no full local
platform link is claimed.

The first complete cross-dungeon ASan/UBSan run passed. Independent review found
an interaction with native entrance composition: an open source mapped to a
selected door could inherit its cross-game leg. A production catalogue/oracle/
runtime regression reproduced it. Physical-source filtering now passes the
coupled open/door swap, both paired returns and ambiguous-source controls.
The final full suite is rerun after that correction. CI adds a focused narrow-pool gate and a
full Linux generation/validation smoke using both narrow choices with all broad
cross-game categories off. Platform packaging and configured in-game traversal
remain separate evidence; preview approval establishes visual preference only.


## Hosted fixture correction

The first hosted run passed the new entrance/forest checks and stopped in the
older complete file-select panel fixture. Its ImGui observer incremented a scalar
only for BeginDisabled(true), then decremented it for every EndDisabled. The new
valid BeginDisabled(false) group exposed that mismatch. The pinned ImGui
implementation pushes and pops an item-flags scope for both true and false.
The fixture now uses a stack with inherited disabled state. The actual full
production panels reproduce the old failure and pass after the fixture correction
under ASan/UBSan, including MM-first parking, return to file select, busy generation
and missing exports. The dedicated entrance menu fixture also remains green.
No production menu or generation policy changed in this correction.
