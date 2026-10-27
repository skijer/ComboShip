# Native customer-side shop orientation

| Field | Record |
|---|---|
| Source baseline | `e3cb39c7`; flat stock layout retained, earlier room orientation rejected. |
| Asset baseline | Exact POC13, SHA256 `bca46926d8474b3503653bdc243ee53ff0ab598c3db2b094827ad902a5ada4bf`; unchanged rollback input. |
| Candidate | `poc/black-market-entrance-orientation-20261018`, POC14. |
| Fault | Player entry, exit region, keeper facing and preview used the staff side as the customer side of Granny's counter. |
| Correction | Original player spawn/heading in both engines, local exit at the original doorway, keeper behind the counter facing customers, preview from that same customer side. |
| Protected | All room/model vertices, collision, textures/UVs/materials/scroll, cosmetics, exterior, original skeleton/animations, raised counter, flat offer grid, stock/purchase/save/forest logic. |

The supplied native OoT archive contains Granny at `(-14,-22,84)`, yaw0,
the player spawn at `(-11,0,381)`, yaw-32768, and the door at `(-10,0,420)`.
These are decoded actor/start/transition records, not inferred from a camera
image. The baseline facts and input/resource hashes are saved in
`market-orientation-candidate/native-room-orientation-baseline.json`.

Both custom scene headers now use that original player position and heading,
retaining their existing native player modes. The shared local return region
is at Z>400, between the safe spawn and physical door. The staff area no
longer acts as the exit. Existing solo/combined return transactions remain.

The Goth keeper keeps her native crossed-arm Sway, Y1 and scale .01. Z86,
yaw0 puts her on Granny's side of the counter, leaning toward customers.
The 44-unit position shift and half-turn reflect the earlier lean across
the counter's Z108 midpoint; the existing Y41 top and hand clearance remain.
All eight offers retain their flat Z110 layout and native slot identities.

POC14 changes exactly four of 192 payloads: MM scene spawn, MM keeper actor
list, OoT scene spawn and manifest. All 188 others are byte-identical, including
the entire room geometry and collision. POC14 SHA256 is
`3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`.
Reproduce it with `tools/black_market/orientation.py` from the exact POC13.
The keeper/counter audit reads the actual packed placement so historical
counter reproduction cannot silently substitute the new source placement.

Regression evidence first rejects the old native exit and old packed spawn.
The corrected fixtures cover local entry/exit/reload/void/TOP isolation in
both engines. The pack check independently compares native Granny records,
all 32 actual lean poses, countertop clearance, supported soles, customer
floor approach and exact payload preservation. The existing full-spin/bob
model/X reserve and viewport checks pass with the flat offers unchanged.
The keeper pose check now measures forward lean relative to yaw in both
directions; it no longer treats negative Z as inherently forward.

The overview, entrance diagnostic and keeper/item details all view the room
from the customer side. Preview hashes guard source and pack identities.
Named native fixture/object logs accompany the checkpoint. Earlier scratch
dependency headers became unavailable; the recovered official header tags,
hashes and compilation environment are recorded separately. This is object
compilation and offline evidence, not a complete linked game build.

Install POC14 **in place of POC13**, together with the updated source/build.
The previous source/archive remain the rollback baseline. Native normal
entry/browsing/camera/return playtesting, Alt/external resource parity and
user acceptance remain untested. No master was promoted.
