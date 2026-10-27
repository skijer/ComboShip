# Private MM shop presentation v2 — source-verified POC

Baseline: public `5f8966fae4b0810ce86021b3f88814ffd77e0224`, tree
`545292a667083fb835ccd86b078e3aac7a484cd1`. Local recovery parent
`86387591cb6fba0adb2528c258c8dadd624136c1` has that exact tree. This candidate
is `MM_PRIVATE_PRESENTATION_V2_20261025`; it is not a master promotion.

Current verdict: implemented and verified by native-source/ASan/UBSan,
serialized-resource, physical-sweep and projection checks. Configured game
rendering, optional-pack appearance and user acceptance remain unproven. No
complete executable was built or linked in this workstream.

The previous failed experiment is retained separately at
`poc/mm-shop-framing-failed-20261024`, commit
`326ef14acc6e8102c7473d143cccffd04d6a614e`, tree
`12dac41800e89d8ff2cc366d9456dfaf6cea1808`, and
`deliverables/ComboShip_Shop_Shelf_Diagnosis_FAILED_POC_20261024.zip`.
Do not install that failed candidate as this v2 implementation.

## Evidence and bounded scope

Directly inspected recording: `MedalTVScreenRecording20261024185713184.mp4`,
SHA256 `1ad6dbe8548acb2177c563b542cf8e70c0f2b92b384e42e2f4a1b550f72cf422`.
Its native blue cursor, bank changes and Bunny Hood selection/return motion are
already present. Merchandise remains small in the wide shot. The clip does not
identify its executable hash, optional-pack order or Alt Assets setting.

A fixed camera-only experiment could not achieve the confirmation-readability
target while also fitting all four native 48px cursor rectangles. The original
.25-scale straight selection path also crosses shelf boards under the complete
conditional model/seal envelope. The earlier .32/t⁴ experiment added a hanging
basket obstruction and still brushed a board during return interpolation.
Those failures are recorded as findings, not inherited as proof.

The current POC coordinates only private MM shop presentation:

- Bank eyes advance from `(±57,53,310)` to `(±50,58,298)`; keeper shots stay
  unchanged. Native FOV60, .75 interpolation parameter, four-cutscene chain,
  32px-per-edge letterbox, cursor/actions and transaction ownership remain.
- Real private EnGirlA actors use .32 display scale after their native Update2;
  initialization alone is insufficient because Update2 restores .25 each frame.
- Selection ends at `(±8,67,198)` in the customer aisle. Horizontal motion and
  native .15 take/return timing stay native; vertical displacement uses t⁶ to
  leave the cabinet before crossing rows.
- The selected stock actor refreshes only its owned native Fixed1 target height
  after every native stock update, including CannotBuy/error holds. It
  compensates the native one-step vertical lag during motion and restores the
  original bank target at t=0. The camera allocator, cutscene manager, native
  rotation/FOV handling and unrelated cameras remain native.
- A private MM graph relocates both obstructing connected wicker baskets and
  all five associated hanging foliage planes outward: left assembly −50X,
  right assembly +60X. All 558 moved vertices retain Y/Z, UVs, colors and face
  indices; materials and textures are reused unchanged.

The six new XML owners live under
`custom/skiejer_market/mm_shop_presentation_v1/`: room root, main draw root,
tri15/vtx15 foliage, and tri17/vtx17 baskets. Only the MM room mesh points to
this graph. Every original shared resource and every OoT output stays
byte-identical. Shared shelf geometry/materials/collision, eight shelf anchors,
Treasure Gal, native actions, prices/eligibility/receipts and ordinary shops
remain protected.

The original blocking owner is
`custom/skiejer_market/geometry/mahouya_room_0DL_006548_tri_17`, including the
right basket component at `[[34,129,144],[77,181,186]]` and left component at
`[[-85,107,156],[-31,175,210]]`. Whole connected components, not a broad spatial
region, are matched by exact bounds, vertex counts and triangle counts. The
immutable bundle and default shared preparation remain the source inputs.

## Fresh verification

| Check | Result and boundary |
|---|---|
| Actual native EnOssan C + EnGirlA update + production adapter + actual CutsceneManager/Fixed1, ASan/UBSan | PASS: lifecycle, cursor/bank chain, receipts, cancellation, foreign-camera/UID ownership, pool exhaustion and teardown |
| All eight reference-item confirmation views | PASS: 23 native pixels selected versus 12–13 shelved; ≥20px and ≥1.75×; B restores all original anchors |
| Immediate A on first frame reaching t=1 | RED reproduced stale extrapolated camera drift; GREEN after per-stock-frame refresh, all eight CannotBuy holds and return targets; independent reviewer measured zero drift over20 frames |
| Continuous physical take/return sweep | PASS: complete .32 conditional FrameFit + bob + seal envelopes clear all7,919 actual private-room and144 actual shelf triangles; conservative interval hulls include native frame interpolation |
| Actual native camera projection during take/return | PASS: native frame poses and1/32-frame subdivisions keep the full item/seal envelopes inside side margins, full32px letterbox boundary and Y142 textbox |
| Steady native bank views | PASS: all eight item/seal envelopes and actual native48px cursors fit; row navigation retains one bank view |
| Private resource identity/closure | PASS5 tests: six owners, exact whole-component moves, closed graph rooted at the actual MM room mesh, unchanged original resources, required runtime inventory |
| Actual serialized scene command factories | PASS5 tests: camera/cutscene bytes decode through production factories; deterministic per-host preparation; OoT byte preservation |
| Actual selected native/Alt scene and room readiness | PASS123 cases through production guards and real factories: typed/count/metadata/pointer/lifetime checks plus private room mesh and actual loaded pointer |
| Required-resource and legacy-cache controls | PASS both hosts; each of six missing private MM owners rejects readiness; stale MM legacy packs require the private companion layout |
| MM/OoT bundled resource-entry controls | PASS: production door/entry/spawn/return/re-entry structural checks; full base owner/keeper absent in this fixture, so full configured readiness fails closed as expected |
| Whitespace/diff checks | PASS; no shared accepted fixture or OoT consumer edits |

Native update order is checked against production: EnOssan NPC category4 runs
before EnGirlA PROP category6; Actor_UpdateAll walks ascending categories before
Camera_Update. The fixture uses that same order.

Evidence in `/workspace/scratch/e8c5cc0ab01b/analysis/`:
`shop-retry-immediate-confirm-red.log`,
`shop-retry-immediate-confirm-green.log`,
`shop-retry-layout-native-final2.log`,
`shop-retry-selected-room-readiness-final.log`,
`shop-retry-layout-resources-final.log`,
`shop-retry-native-scene.log`, `shop-retry-readiness.log`,
`shop-retry-resource-entry2.log`, `shop-retry-steady-banks.json`.
Earlier `shop-retry-*` failure logs are retained diagnosis, not final verdicts.

## Build/load and remaining runtime test

Run `scripts/assets/prepare_native_market.py` for this source and rebuild the MM
resource archive with the executable. It deterministically emits the six new
private XML resources and redirects only the MM mesh; preparing twice writes
zero additional files. Missing private owners reject shop availability. A
legacy/custom/Alt MM room that retains the old mesh root is rejected by the
actual selected-resource/path/loaded-pointer guard even if its camera records
match. Visual packs overriding MM scene/room headers must be rebuilt or merged
with both `mm_native_shop.apply` and `mm_shop_presentation.apply` over the
immutable shared inputs. All retained visual-only materials can remain.

The physical envelope is conditional on the authored FrameFit model contract;
arbitrary oversized native/Alt replacements and extra GI effects are not
bounded. Full actor animation appearance, relocated basket/foliage appearance,
custom-pack priority, exact rendering interpolation, performance and configured
runtime remain unproven. The static checks do not establish price/text legibility
on the user's display; the existing native textbox/text bytes and placement are
unchanged.

The decisive runtime check is both banks, every shelf slot, selection and return
with Alt off/on, immediate A and rejected purchase, B cancellation, successful
receipt and continue-shopping, exit and re-entry. Record executable/resource
hashes and pack order, inspect the two relocated hanging assemblies, and confirm
ordinary MM shops and OoT remain unchanged. User acceptance is still required
before calling this runtime-proven or promoting it to an accepted master.

Controller integration additionally compiled the complete changed MM `BlackMarket.cpp` translation unit with the native headers and MM precompiled-header inputs (16 existing compiler warnings). The integrated native camera/action sweep and all five resource isolation tests passed. This does not establish a linked executable or configured gameplay.

CI portability followup: Ubuntu's external fmt9 reached the scene fixture through
`SetMeshFactory -> Context.h -> spdlog/async.h`, outside its destructor-log stub,
and failed at link before any scene assertions ran. The runner now explicitly
uses fmt's header-only mode for its C++ compile. The unchanged ASan/UBSan fixture
reproduced the link failure with external fmt11.2, then passed all 123 cases in
both external and bundled configurations after the flag. Exact Ubuntu fmt9
verification remains a CI check. Production code and fixture assertions are
unchanged by this followup.
