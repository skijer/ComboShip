# Native custom-entrance title cards — 2026-10-22

| Field | Record |
|---|---|
| Baseline | Latest develop `c3b0c76b81766ce51378e573dcf9816ab3c59473` |
| Candidate | `poc/market-forest-followup-20261022`, title-card workstream |
| Scope | Missing fountain return and OoT→MM destination cards; analogous explicit same-game shop/solo-forest transitions; shuffled/boss/Farore arrival composition |
| Preservation | Final entrance composition, authored position/room/yaw, respawn slots, progress and cycle state, native destination art/text/language/layout, explicit hide setting, native cutscene ownership, normal room/respawn rules and existing opening-forest custom card |
| Assets | This entrance-policy change reuses existing OoT native/Alt title textures and MM native scene-title messages. The later custom market card is documented separately in `black-market-default-title-20261022.md`. |
| Evidence | Red/green native production-policy tests and affected source object compilation; full entrance regression and independent review recorded below |
| Status | Source candidate only. Configured gameplay, full executable link and runtime acceptance remain untested. |

## Causes and bounded changes

OoT already has a one-use cross-game title request, which preserves the native
destination selection despite a positioned respawn or unset entrance card bit.
The conditional Great Fairy return stays inside OoT, stages `respawnFlag = 2`
and previously never armed that request. The claimed custom fountain entry and
return now arm the same request, preserving all existing route and respawn data.

MM uses `Message_DisplaySceneTitleCard` with the loaded scene's `titleTextId`;
its place-name texture initializer is unused. Combo's direct boot bypasses
file select's `showTitleCard` initialization, and positioned/shuffled arrivals
can fail the native respawn/entrance-bit gates. A one-use request is now armed
after the final portal, shuffled/boss and explicit Farore composition. Ordinary
save resumes, including saved recovery, keep their native eligibility.

MM consumes the request before its opening-forest bypass, so an untitled,
hidden or story-controlled arrival cannot leak into a later visit. Only an
ordinary gameplay layer can use the request to bypass respawn/entrance-bit
gates. Native entrance cutscenes still execute first and may suppress the card;
the hide-card hook still vetoes it. Native MM text, message animation and layout
remain unchanged.

Both engines' explicit Black Market and solo-forest transition helpers now arm
the same request. This covers their positioned native-area returns without
forcing ordinary room changes. Their private untitled scenes continue through
the existing custom card implementations. No browsing, materials, placements,
model scales, progression flags, save formats or entrance maps are changed by
these title edits.

## Verification

The regression tests first failed on the missing requested behavior:

- Fountain: `a claimed custom fountain entrance must request its native destination card`.
- MM: `every direct crossing must display the native MM destination title`.
- Both games' custom transition helpers: `explicit same-game shop and forest returns must request the native destination [title/card]`.

Focused passing checks:

- `python3 -B tests/lost_woods_entrance/run_mm_title_card_tests.py --sanitize`:
  real composed boot, native entrance policy and native scene-title setup in
  Combo and solo builds; all paired slots, Woodfall/Woods of Mystery/Clock Town,
  positioned and unset-bit shuffled/boss/Farore destinations, native message
  mode/layout, hide/cutscene/story/untitled guards, opening-forest consumption,
  same-game shop/forest helpers and ordinary room/respawn parity.
  The follow-up also iterates every native `gSceneTable` row for portal, shop
  and forest requests with an unset incoming card bit and positioned respawn.
  Each titled final scene selects its own native `titleTextId`; every untitled
  scene stays untitled. Both consume the request once and preserve every save,
  respawn and progression byte. This covers arbitrary entrance-shuffle final
  destinations rather than a destination allowlist.
- `python3 -B tests/lost_woods_entrance/run_oot_title_card_tests.py --sanitize`:
  real boot, native player selection, art/language/fade lifecycle and suppression
  hook in Combo and solo builds; custom same-game requests and both transition
  helpers now included alongside the previously accepted crossing coverage.
- `python3 -B tests/lost_woods_entrance/run_fountain_return_tests.py --sanitize`:
  native collision exits and actual fountain route; both ages, OoT game modes,
  entrance shuffle, all six fountains, relocated/native room approaches,
  save/reload/recovery, preserved respawn/progress and one-use title requests.
- `python3 -B tests/black_market/run_native_tests.py`,
  `run_oot_native_tests.py`, `run_entry_access_tests.py` and
  `run_solo_forest_tests.py`: existing MM/OoT transaction, door, recovery and
  solo-forest behavior passed under their native sanitizer fixtures. These
  unrelated fixtures treat the title request as a boundary; the focused title
  suites execute its actual production implementation.
- Complete native object compilation passed in Combo and solo modes for both
  `title_setup.c` files, MM `z_demo.c`, both Black Market controllers, both
  `ComboSoloForest.cpp` files and `ComboLostWoodsFountain.cpp`. Native symbol
  inspection confirms C linkage for the request/consume helpers.

The test environment supplies normal external headers through
`test-dependencies/include_paths.env`. The first broad-suite attempt stopped at
missing `spdlog/async.h` after its vertical-scroll checks; this was an environment
dependency and was resolved before the broad-suite rerun.

Independent read-only review found no blocking issues and separately reran both
native title suites in Combo/solo modes plus the fountain collision suite under
ASan/UBSan; all passed.

The first full entrance rerun reached its older opening-forest cutscene fixture
before that fixture stopped on a missing new-consumer declaration. The fixture
now compiles the real consumer and checks that the opening-forest bypass
consumes the request. The final complete
`python3 -B tests/lost_woods_entrance/run_tests.py` run passed, including the
renderer-linkage objects, both native title suites, existing opening-forest
card, progression/save/arrival/camera guards, and portal/re-entry checks. Its
stdout is recorded in `title-card-verification/lost-woods-suite.log` in the
candidate workspace's parent directory.

## Configured runtime checks remaining

Check the fountain round trip, OoT→MM Woodfall, OoT→Woods of Mystery, a composed
cross-game shuffled destination, cross-game Farore recall, and native-area
returns from shop/solo forest. Each should show the final destination's card
once while retaining its authored arrival and progress. An ordinary later room
change/respawn must not replay the request; Hide Title Cards and native entry
cutscenes must retain control. Repeat affected OoT cards with the user's native
and Alt/Djipi configurations; MM keeps its own scene-title text and language.
Existing opening-forest texture-card behavior must remain single-presentation.

No configured gameplay observation or master promotion is claimed by compilation,
sanitizers, source publication or this handoff.
