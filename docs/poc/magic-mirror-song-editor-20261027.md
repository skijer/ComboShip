# Magic Mirror Saria song particles and save editor POC3

Baseline: POC2 `6d56917a27fcf3b4dc98204e501882def8b49b7d`. Scope: reuse Saria's Song's existing falling-leaf mesh, palette, pulse and animation fitted to the floating Mirror and both native pickup callbacks. Preserve the reflective glass highlights, reflection resources and cast ritual. Add a dedicated Magic Mirror grant button to each game's save editor; disable it when owned and use the established TravelTools grant API so the Time Gate's independent ownership, selected cell and equipped buttons remain intact. No new textures or companion archive are required.

Checks: compare fitted particles against the actual song sampler across frames/camera bases; run production editor button bodies against both native ownership stores; check existing forest particle rendering and editor/inventory behavior. Preserve prior POC2 source and archives. This remains a source/native-verified candidate, with configured gameplay/UI appearance and full executable build untested. Other October 27 source capsules are not integrated or overwritten.

## Additional authorized scope

- Restore the regular MM Lost Woods item-get close-up: the private exploration header supplied only an ocarina camera. Add the native unscripted ITEM_GET successor before player camera assignment, preserve it across Mirror recall, and leave authored scene chains untouched.
- Add default-OFF **Shuffle Magic Mirror** in both native randomizer interfaces. OFF retains the fixed Mirror pickup. ON contributes one Mirror and turns the stump into a standard shuffled check with its assigned reward/model, including foreign rewards. The stump remains collectible even if the Mirror was found elsewhere; collection must persist independently and must not grant another fixed Mirror.
- Stump logic requires access to our Lost Woods room, with only the existing entrance-route requirements; the Mirror itself is never required. Solo OoT uses its Lost Woods fairy-tunnel route; solo MM uses Woods of Mystery. The combined game has one physical MM check rather than two duplicate native locations. Combined generation treats either native checkbox request as ON and serializes the shared policy into both native seeds.
- The active combined fill and playthrough paths accept the native MM route or a native OoT Lost Woods region-access probe. The probe uses the same inventory and entrance search as the other checks and unlocks only the stump; reaching Clock Town or opening the main portal is not a substitute. It is never an item-pool entry or delivery location.
- Preserve append-only randomizer/check/option/Fleet identities, item-pool cardinality and one-copy behavior (including Plentiful), old seed defaults, independent Time Gate ownership/selection and equipment, all POC2 cast/reflection/audio behavior, and unrelated NEI items when the standalone Mirror option is enabled.

## Verified boundaries

Production editor bodies run against both native save stores, including legacy Time Gate state and selected/equipped cells. The private forest camera fixture runs native player camera assignment for both modes and all five MM forms. The existing forest particle/renderer suite and Mirror cast, prompt, arrival, ownership and inventory regressions pass. Shuffled receipts are cancelled whenever the Mirror runtime is reset so a queued acceptance cannot survive a scene, save or seed change, even if the next play state reuses the same memory address. Older MM check arrays are read with a bounded copy, and old seeds default the new option to OFF.

Configured gameplay, full application compilation and user acceptance remain unproven. Native logic/queue/camera/renderer checks are evidence for their specific boundaries, not substitutes for a runtime test.

UI follow-up: Shuffle Magic Mirror appears directly below Rod of Seasons and before Start with Rod of Seasons in both native shuffle menus. Both setting keys are registered as New accents; the MM immediate-mode control uses the established highlight scope and respects generation disabling.
