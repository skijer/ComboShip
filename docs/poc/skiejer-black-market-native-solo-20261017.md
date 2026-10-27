# Skiejer's Black Market: native ComboShip solo candidate

The native market began at baseline `9a4c1aabe63e54d8d3874611ac57aaf79d89e76c`; the shared-forest approach continues its `8ccbd47d` checkpoint in `comboship-solo-forest`. This extends the committed combined-mode shop. It does not establish support for separate, non-`COMBO_BUILD` SoH or 2ship executables.

Status: implemented and checked with compiled production fixtures and native translation-unit compilation. No game runtime or user acceptance is claimed. The paired archive must satisfy the generated `BlackMarketResources.h` contract; the main POC record identifies the final archive and visual evidence.

## Mode routing

| ComboShip mode | Native exterior | Native destination | Return and persistence |
| --- | --- | --- | --- |
| Combined | MM opening Lost Woods, room 0, existing shop door at `(410, floor, -300)` | Existing MM scene `0x02`, entrance `0x1200` | Existing local opening-forest return; combined container stock/clears and MM purchase snapshot |
| MM only | Woods of Mystery room-1 passage → private native forest scene `0x03`, entrance `0x1600` → the same shop doorway at `(410, floor, -300)` | Same private MM shop scene `0x02`, entrance `0x1200` | Captured grounded private-forest return, room 0; native MM save stores frozen stock, purchased bits, unique clear mask and return point |
| OoT only | Existing pink Fairy tunnel in native Lost Woods room 1 → private native forest scene `0x6F`, entrances `0x618`–`0x61B` → the same shop doorway | Dedicated shop scene `0x6E`; entrances `0x614`–`0x617`; `scenes/shared/skiejer_market_oot/skiejer_market_oot` | Captured grounded private-forest return, room 0; dedicated native SoH save section |

These native routes do not enter the entrance-shuffle pools or call the cross-game launcher. Shared opening-forest/prologue, cross-dungeon and cross-game solo guards remain unchanged. The private forest stump returns to the captured native origin. Its Clock Tower portal also returns to native Lost Woods in OoT-only mode; MM-only mode retains the native Clock Tower destination. See [native-solo-forest-candidate-20261017.md](native-solo-forest-candidate-20261017.md) for exact forest packs, load order and donor hashes.

For native room/position restoration, each solo controller stages the engine's disposable RETURN respawn for one load and restores its previous contents afterward. TOP/Farore's Wind is not edited. Saved-inside markers are bound to the current file and frozen seed. Final recovery runs at `Play_Init`, after unordered native load hooks, so entrance randomizer savewarps and Remember Save Location respawn loading cannot overwrite the shop recovery route.

The market returns into private forest room 0. The separate forest normalizer preserves that legitimate RETURN, starts forest reloads/voids at its safe native spawn, and redirects a missing-addon recovery to the captured native origin. Its saved-inside marker is applied after native load hooks. OoT file-init callbacks do not wait under the native save mutex; ordinary age/warp notifications cannot replay a stale file-load marker.

OoT movement updates an unsaved grounded candidate. Entry waits for native saves, then publishes the serialized return point. Save serialization derives `savedInside` from the captured native entrance, rather than live play state.

## Stock, grants and keeper

- The existing independent deterministic stock/price streams and fixed eight offers, six NEI/two wildcard and 2/4/2 tier quotas are preserved. Existing combined stock JSON stays unchanged.
- Native MM uses the complete MM-supported catalog and tiered native prices. Native OoT excludes MM masks, refills and other MM-only wildcards; its eligible wildcard set is Mario Mask, Poke Ball, Magic Meter Upgrade and Double Defense. Native item prerequisite gates remain in force.
- Solo stock freezes into native saves. Purchased bits and real boss-defeat clear bits persist with those saves. Repeat native load notifications do not reset an active OoT market.
- A successful purchase grants the exact identity, debits the actual saved balance and writes the purchased bit in one native snapshot. Failed prerequisites or unsuccessful grants do not debit or commit a shelf.
- Native OoT grants exact cane skills, rune/season/wand bundles and the NEI page-two feather. Shared Shovel/Dominion ownership is latched from physical native slots before and after grants, preserving both same-visit purchases and legacy Dominion ownership when Seasons replaces its former slot.
- Both engines use the same private Goth Treasure Chest Shop Girl skeleton and blink resources. The presentation continuation selects her retained crossed-arm Sway lean and shared `(0,1,130)` placement behind the counter. MM replaces the actor init function before native dispatch, avoiding minigame init/destructor effects. OoT registers a dedicated ActorDB NPC with gameplay keep and no vanilla shop/minigame controller.
- OoT uses the established `NeiGi_DrawShop` fitting path, with the native fallback only when declined. Both controllers retain `.25` display / `.32` selected scale and the symmetric display grid at `z=48`. Locked offers use translucent scrolling X ribbons fitted to the actual selected native model bounds; unlocked selection adds no seal. See `market-final-presentation-20261017.md` for POC12 and final evidence.
- The private title initializes each engine's native title-card context. OoT's standard scene-name initializer rejects new scene IDs, so the controller initializes that context directly and retains native update/draw/cosmetic hooks.

## Verification and remaining runtime proof

`tests/black_market/run_stock_tests.py`, `run_price_parity_tests.py`, `run_cane_tests.py`, `run_native_tests.py`, `run_oot_items_tests.py` and `run_oot_native_tests.py` pass. Native fixtures compile and execute actual production bodies with real engine save/play/actor types and controlled engine-boundary dependencies. They cover routing, guards, captured return, native serialization, failed/successful transactions, slot/seed separation, load-hook ordering, exact grants, shared tool-wheel ownership, dedicated actor profile, native title context and shop draw/fallback dispatch. These checks do not run the game or load its renderer.

`tests/solo_games/run_tests.py` covers the three compiled mode/options probes and existing production-boundary suites. Its save fixture declares both market and private-forest save callback dependencies and checks native slot/backup/copy dispatch plus combined exclusion without weakening IO isolation assertions. Current forest evidence and remaining native/runtime checks are recorded in [solo-forest-wip-recovery-20261017.md](solo-forest-wip-recovery-20261017.md); native compilation remains separate from behavioral fixtures.

Smallest decisive runtime checks still needed:

1. Enter/exit each mode's doorway with the final pack, verify Goth idle/blink, all eight goods, native title and readable dialogue; verify vanilla and Alt paths.
2. Buy both shared tools in each order before opening pause; verify both ownerships, rupees and purchased bits after reload.
3. Verify a legitimate unique boss clear unlocks a tier, and duplicate/non-arena defeats do not add clears.
4. Reload, void and die after saving inside. Exercise OoT entrance shuffle with Remember Save Location off and MM Remember Save Location on; verify local position, room, prior RETURN and unchanged TOP.
5. Verify no cross-game transfer in either solo mode and repeat entry stability. Keep the combined opening-forest route as the control.

Candidate only: no push, merge, full build, master promotion or runtime acceptance is implied.
