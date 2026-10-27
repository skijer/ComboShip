# Native Black Market camera candidate, 2026-10-23
Historical checkpoint note (2026-10-24): the OoT implementation remains current. MM's custom browsing adapter described below is superseded by the actual native EnOssan/EnGirlA integration in mm-native-shop-integration-20261024.md. Its current scene metadata, UI framing and receipt continuation differ from this earlier POC; use the newer record for MM acceptance.

| Field | Record |
|---|---|
| Baseline | Merged `develop` commit `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`, tree `32e1dcb6a49e936bdab838c820052e206400ba9f`. The supplied 27-second recording demonstrates the previous row-tracking camera failure. |
| Candidate | `fix/black-market-level-camera-20261023`; native camera integration and bounded shelf framing. |
| Scope | OoT native shop viewpoint/Data4; MM linked actor cutscenes/Fixed1; one stable camera per four-offer bank; ownership-safe release; source-authored shelf alignment and clearance. |
| Preservation | Existing keeper and its animation, item models/scales/bob/seals, eight stock identities, pricing/unlocks/saves, native glyph dialogue and choices, receipt/tutorial/GI policy, title timing, doors, and unrelated scene assets. |
| Configuration | Private market room 0 in either Combo host, including the existing solo modes. Geometry proof uses the retained goth keeper's full 32-frame Sway and the authored shop-fit item/seal envelope. Arbitrary replacement models are not covered by that envelope. |
| Verdict | Implemented; native CPU integration, sanitizer, geometry and source checks pass. Full executable linking and configured GPU/gameplay acceptance are separate gates. No runtime master is promoted. |
| Recovery | Original source remains in the baseline commit. The candidate's exact staged tree and all changed-file identities are recorded in the recoverable source checkpoint; restore it onto the named baseline with the supplied verifier. Accepted input archives were read without modification. |

## Native camera ownership

The shelf actor `En_Tana` does not initiate shopping; its update is empty. Native shopkeeper interaction depends on valid scene camera data. The previous adapter used `FREE0` and wrote eye/at/FOV every update, with a 60-unit aim change between rows.

OoT now binds the player to its owned camera, wraps the private collision header with a valid shop BG-camera record at viewpoint index 1, and runs the real `CAM_SET_PIVOT_SHOP_BROWSING` / `Camera_Data4` path. Native yaw input turns between keeper and banks. The scene record fixes eye and pitch. The early native viewpoint guard releases ownership before a later actor/hook's active camera can be reconfigured by the shop viewpoint.

MM now registers persistent native camera records and the keeper/left/right/return actor-cutscene chain. It follows the real manager's queue, readiness, player-action, start and deferred-stop flow using `CAM_SET_FIXED1`. Row and column cursor changes do not restart bank cutscenes. Capacity is checked before talk and again after a queued wait; the manager's snapshot slot is reserved.

Native MM ownership protection is explicitly enabled only by the private market's scene initialization. Every normal native manager initialization defaults it off. The protected path validates camera pointer/UID identities at deferred End, preserves a surviving active owner's latest view, handles cleared/recycled owned and return slots, and avoids overwriting an occupied snapshot slot. Player/keeper cleanup still runs. Legacy protection-off EndCam0, EndCam1 and smooth-return behavior is tested. Unrelated scenes and standalone defaults retain their previous behavior.

## Framing and physical layout

Both rows use the same bank view, with one fixed customer eye `(0,88,253)`, fixed pitch `-13` degrees, and bank turns of `+/-30` degrees. The keeper and bank fields of view are 46 and 72 degrees. The bank camera is independent of the selected row and column.

The fixtures face those native bank angles. Their source-local back was extended by eight units so the full spinning item/seal envelope clears the rotated panel; model scales remain `.25/.32`. Banks are centered at `X +/-60`, `Z107.5`. Inner/outer anchors are `X +/-44.84456` / `+/-75.15544`, `Z98.75` / `116.25`, with the retained `Y58/110` rows. Render and collision vertices use the same rounded transform, with derived collision bounds and native integer-plane quantization.

The retained donor archive SHA256 is `3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`. Current prepared room geometry, rather than an assumption that it equals the historical donor, was checked against all 7,919 reachable room triangles. All eight complete selected item/seal envelopes clear the room and fixtures; all 32 keeper poses clear the fixtures and offers. The bottom planks intentionally contact the raised counter. Both source views frame every bank offer and cursor above the explicit native textbox at Y142 in 4:3. Actual configured rendering remains unproven.

## Verification

- `tests/black_market/run_browsing_native_tests.py`: actual OoT scene lookup/viewpoint/setting/Data4 and MM CutsceneManager/Fixed1 bodies, native camera allocation/copy, quantized trig, all eight slots, steady banks, mirroring, cancellation, receipt release, re-talk, ownership reuse, deferred return loss, foreign motion and pool exhaustion, under ASan/UBSan.
- Negative controls reject the old row-tracking behavior, a missing native pitch flag, FREE0 selection, and the unguarded OoT viewpoint dispatch. Real MM regressions reproduced foreign-camera overwrite/deletion and null dereferences before the ownership fix.
- Native shelf Dyna/camera/GBI fixtures pass in both hosts. The oriented geometry checker uses the actual rounded corners and retained native keeper poses. All offers have physical support and clear the complete reserved fit envelope.
- Existing native transaction/recovery, keeper and OoT purchase tests remain separate policy coverage. Camera/physics boundaries are not substituted for configured runtime proof.
- Native C syntax checks cover the MM manager with and without `COMBO_BUILD`; source formatting uses clang-format 14.0.6. The combined hosted build remains its own gate.

The native CPU harness does not execute the complete `Camera_Update`, GPU renderer, actual collision-ray environment, or full native GI delivery. Check the rebuilt game in the user's normal pack order and Alt setting: talk, both banks/all slots, confirm/decline, purchase/tutorial, return, leave and re-talk. Include the native keeper fallback and relevant custom keeper/model configurations before accepting their appearance.

The title-card sweep and market-only spacing correction are recorded separately in `mm-oot-title-artifact-sweep-20261023.md`.

Publication status: automatic approval review blocked the new public-source upload. Hosted Windows/Linux CI has not started; explicit authorization for this camera/title payload is pending.
