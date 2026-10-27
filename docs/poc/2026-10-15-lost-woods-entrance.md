# Lost Woods / Woods of Mystery entrance POC

| Field | Record |
|---|---|
| Baseline | ComboShip `develop` at `a82852a3a02c8cabc541cce7c124da71c5755efb`; source baseline, not newly runtime accepted |
| Candidate | `poc/lost-woods-2ship-entrance-20261015`, isolated checkout |
| Scope | OoT first Lost Woods clearing's straight-ahead dead-end tunnel ↔ dedicated tree passage in MM's first Woods of Mystery clearing |
| Preservation | Existing Mask Shop/Clock Tower crossing; paired save slots; MM cycle/time; OoT age; Farore TOP respawn; native maze corridors and swamp exit; original scene resources/collision |
| Assets | No archive replacement. MM passage uses the native RGBA16 32×32 bark resource `Z2_26SARUNOMORITex_00BAC0`, allowing the active asset resolver to select its override |
| Recovery | Keep the parent `develop` commit; all feature changes are on the candidate branch |

The active single-process ComboShip handoff is used. Legacy FleetWarpArrival's separate launcher pipeline is untouched. Each game's destination is applied after its native save-load hooks, with RETURN respawn placement outside the trigger and facing into the clearing. TOP, used for Farore's Wind, is preserved. A brief black fade precedes the handoff. Reset/destruction cancels the armed OoT crossing, and its captured slot is validated before saving.

The MM passage is a drawn tree arch immediately in front of the existing north-east wall, between the clearing's north/east corridors. The trigger fires before native wall collision. It does not replace a corridor or require a collision/archive edit. The original randomizer generator remains conservative about access through the existing portal; it does not assume this new shortcut when placing progression.

## Static coordinate evidence

- OoT scene `0x5B`, room `0`: trigger in the north tunnel at `-560 < z < -360`, `|x| < 65`. Native exit floor starts at `z=-400`; the trigger begins before that floor. Return position `(0, 0, -280)`, yaw `0`.
- MM scene `0x64`, room `1`: passage centre `(160, 0, -160)`, facing south-west. Native diagonal wall at player waist height intersects `(178, -178)`; the gate starts in front of it. Arrival `(90, 0, -90)`, yaw `-8192`.
- The native MM loader accepts filename slots 1–3 but exposes native `fileNum` 0–2; both crossing guards use native slots.
- Vanilla source scene SHA-256: OoT `5b39a3fcda9c00d0c07cabff57802da46ee8dbc841445e9089fbc8c6b87bb2b0`; MM `ed89de65d52d8593fe8902e0de1c85daac5da1e3001a0001e4b4bb93826379cd`.

## Verification and status

`python -B tests/lost_woods_entrance/run_tests.py` executes both real title-setup/arrival paths with native engine types. It checks saved state, original portal arrivals, route consumption, Remember Save Location, native slot zero, safe return positions, and corridor exclusions. It also executes the actual OoT trigger/fade/handoff registrations against event, disk and DLL boundaries. The reset regression was reproduced before cancellation was added; the test now covers reset, Play destruction, attract-demo reuse, source-slot changes, successful crossing and subsequent Market entry.

The existing crash-owner/resume regression also passes. A focused independent code review found the reset issue; its fix and regression were re-reviewed with no remaining finding.

Affected native translation units were compiled to objects using the available dependency/generated-asset cache: both `title_setup.c` files, `OTRGlobals.cpp`, `BenPort.cpp`, `ComboShip.cpp`, and `ComboLostWoodsPassage.cpp`. This is object-compilation verification, not a fresh full executable link or an in-game test.

**Candidate status: implemented; focused tests and native object compilation verified. Runtime, Alt Assets parity and user acceptance remain untested. No promotion or push.**

## Decisive runtime check

1. From Kokiri Forest, enter Lost Woods and walk straight through the opposite tunnel. Confirm arrival beside the tree passage in Woods of Mystery, without an immediate return.
2. Enter that same passage. Confirm return walking out of the same OoT tunnel. Repeat the round trip and check save state.
3. Check the normal maze/swamp paths on each MM day and the original Mask Shop/Clock Tower crossing.
4. Check child/adult OoT entries, the active Djipi/Prelude configuration and native assets. The current custom scene archive/load order was not available for runtime verification.
