# MM runtime review candidate — October 5

Baseline: PR 34 head `c10fd9a5cfd679ddff0c90defb15a06fc88f651a`.
The original checkout and supplied source archives remain the recovery controls.
This candidate stays on the existing draft PR; no merge or master promotion.

| Report | Bounded correction | Proof to retain |
| --- | --- | --- |
| Champion/Sage tunic missing, Sage medallions still visible | Load the tunic mesh through its OoT resource owner while MM is active. | Actual foreign descriptor and native draw callbacks; missing-resource recovery and active mod choice. |
| Pendant/Bomb Arrows rupee sentinel | Cover the actual MM Pendant selected by the exported duplicate-name catalog and the Bomb Arrows custom recipe. | Actual catalog ordering, exported name boundary and production MM submission. |
| Ice Rod, swords and Lantern clip overhead | Apply receipt-specific fits, including Goron's steep camera; measure selected resource matrices and effect layers. Keep OoT and shop envelopes. | All 61 authored meshes and the six selected meshes from the supplied Din pack, spun through all five steady native MM form cameras. |
| Bolero fire and Serenade circle | Filled orange flame tongues with yellow cores; remove only Serenade's ring. | Production particle geometry and offline production-triangle preview; native note/shimmer preserved. |
| Heart Piece quarter icon, Pegasus boots icon | Route the existing full Heart Piece and stock anklet artwork in receipts/inventory. | Actual native loader/catalog paths; HUD progress slots and unrelated boots preserved. |
| Map spacing and wording | Fixed native 75% text, measured wrapping and extra pages. Use the native ordinary/masterful hint table and colors verbatim, then “It seems the entrance is at [source].” | Real font widths, long Ice Cavern source, inverse source quantities, native page/icon handling. |
| Missing compass hints | Follow forward mixed-pool dungeon/boss-door mappings; identify reward from the actual destination boss check. Bound cycles and state unavailable destinations explicitly. | Real Entrance objects through CreateEntranceOverrides and SOH_DumpEntranceOverrides, donor and MM receipt builders. |
| Rod of Seasons MM behavior | Autumn warms EnWood02 canopy/leaf colors and adds bounded native falling leaves while preserving native weather. Spring adds rain on Days 1/3 and preserves native Day 2 rain scheduling; Spring clears snow on all days. Summer clears Day 2 rain/sky/storm ambience. Winter presents snow. | Native environment, weather actors, day schedule, selector/held-item lifecycle, restored native state and actual audio command boundaries. |

Autumn uses existing tree/leaf artwork and native falling-leaf actors. Ambient leaves
have their own presentation RNG and bounded spawn budget; trunk, bush, collision,
drop and roll-burst behavior remain native. Selected Alt foliage keeps its authored
materials. Other tree actor families are outside this patch.

## Crash evidence and remaining limit

The supplied Windows log records exception `0xc0000005` after the Champion's Tunic
receipt. Disassembly of the matching 2Ship binary places its return address at the
native `Actor_UpdateActor` indirect `actor->update(actor, play)` call. The callback
address is outside the reported loaded modules. The log does not establish that
focus loss caused the invalid pointer or identify the actor that owned it.

The shared crash handler still belonged to OoT after entering MM, so the report
omitted the active MM scene and actors. Both resume paths now restore their own
reporter before restarting audio/game work. MM copies scene, room, actor identity,
parameters and callback address before dispatch; a failed update reports those
facts without walking possibly damaged actor memory. Nested/successful dispatch
restores the prior context.

This improves the evidence for the unresolved crash. It does **not** establish a
fix for the invalid actor callback or prove Alt-Tab safe.

## Runtime acceptance

Component/sanitizer tests and platform builds are separate from live acceptance.
Retest the reported pickups across player forms with the active mod stack and Alt Assets on/off, long
map/compass receipts, all four MM seasons including native Day 2 storm taper and
Snowhead region changes, and Alt-Tab after the Champion's Tunic award. Install the
matching game modules and port archives from one candidate build together.
