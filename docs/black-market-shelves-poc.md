# Shared Black Market shelves POC

Parent: `c95dad1ab54a613d8ce87c66cf5af774a612f710`, preserved on
`poc/black-market-native-browsing-20261018`. Candidate:
`poc/black-market-shelves-20261018`. The parent has an independently restorable
source checkpoint. This is a source candidate, not a master promotion or
configured executable acceptance.

cor requested physical shelves in both the magical alcove and the optional
regular Granny room. The same two wooden, two-row display racks now flank the
keeper in both engines and both appearances. The regular-room checkbox chooses
the room; the Animated Item Seals checkbox still controls only the seals.
Items retain their floating/spinning presentation, flat depth, stock and
purchase behavior. The crossed-arm leaning keeper and raised counter remain.

`combo/BlackMarketShelves.h` authors twelve cuboids, per-face colors and a
deterministic 32x32 I8 wood texture in source. No shelf resource archive or
vanilla shop actor, inventory, rando or entrance routing is required. The
regular room still requires the user's installed Djipi maps/textures and
native OoT resources through the prior external-resource adapter. No original
Djipi room, texture or donor archive is added to source, a new O2R, a build or
the source checkpoint. The accepted POC14 archive is unchanged.

The racks occupy X 14..92 and -92..-14, Z 84..133. Bottom boards seat on the
raised counter; posts/back panels start above the original book station and
behind the retained room props. Upper offer anchors rise from Y102 to Y110 to
clear the middle boards and the complete authored selected-item/seal envelope.
Both rows retain Z110. Bank camera target Y rises from 48 to 52 and FOV from
60 to 64 to retain the crowns and native bottom-message area at 4:3. The
keeper-facing camera is unchanged.

`BlackMarketShelvesNative.h` owns persistent native vertices, two solid rack
collision proxies, outward collision planes, one default surface, one valid
CAM_SET_NONE camera entry and a collision-only DynaPolyActor. Registration
uses each engine's native DynaPoly_SetBgActor, unit scale and a non-null update
callback. The owner is never borrowed from Link or the keeper. Scene/play
reset clears ownership without reading a destroyed MM collision context; the
native context releases its registration. This proxy prevents walking through
the fixtures; it does not simulate physical individual floating items.

Review found that native floor-camera lookup can dereference a dynamic
collision header's camera list. An ASan/UBSan regression reproduced the null
lookup before the fix. Each host now gets an owned no-camera record (plus the
OoT list length), and tests exercise actual native surface-index/camera lookup.
The reviewer inspected the correction and found the reported issue resolved.
Another regression caught use of tile 1 in the second combiner cycle; the
opaque shelf draw now uses MODULATERGB followed by PASS2.

Verification completed:

- Complete MM and OoT controller object compilation after the final source.
- Both native shelf fixtures under ASan/UBSan: actual registration, retained
  actor/header/vertices, outward planes, floor-camera metadata, re-entry,
  exhausted context and emitted opaque textured GBI commands.
- Every offer has a board beneath it; all eight complete selected item/seal
  envelopes and all 32 keeper poses clear the shelf mesh. Retained opaque room
  geometry clears the additions except intentional bottom-board seating.
- Production camera framing fits the crowns, all full item/seal envelopes,
  native selection cursor and native textbox clearance at 4:3.
- Native browsing, transactions/recovery, keeper, seals, Granny dependency and
  original-flame metadata, plus 4096 deterministic full-catalog stocks pass.

Evidence is under `lock-visual-verification/shelves-*` in the recovery artifact.
The shelf source bridge drives both the geometry checks and six offline
renders under `shop-shelves-preview`; source/input/image hashes are recorded.
`tools/black_market/shelves_room_preview.py` renders both appearances, views
with seals hidden/visible, and production bank cameras. It draws no mock
textbox. The prior browsing clip's textbox was a preview overlay; game dialogue
uses the native message system.

Remaining proof: a linked executable in OoT-only, MM-only and combined modes,
with both room appearances, normal entry, all offers, purchase/cancel/leave,
pause/re-entry and Alt Assets off/on. Offline room effects, filtering and
lighting remain approximations. Native/Alt item replacements beyond the
authored FrameFit envelope remain outside the geometry guarantee. No full
game build, remote push, or runtime acceptance is claimed.
