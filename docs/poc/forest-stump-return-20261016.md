# Opening Lost Woods stump return candidate

Baseline source: `1bbb94e692629c0d910f97b57dd7cd02240d3775` on
`bridge/audit-merge-20261008`. Candidate: `fix/shop-forest-entrances-20261016`.
The title-card and Moon's Tear corrections are separate commits on the candidate.
This is an implemented/static-verified candidate, not runtime acceptance.

The user clarified that Woods of Mystery's changing daily monkey routes explain
the separate arch. Its four native corridors, current arch and route remain
unchanged. This change concerns MM's opening Lost Woods, room zero only.

The return arch now sits against the west face of the large broken stump at
`(120, 0, 240)`, yaw `-16384`. The existing floor is walkable; the trigger begins
at X108, before the black face at X144..145 and the native stump wall. It does
not cut collision or consume the onward tunnel. A native-assets temporary arch
uses the same anchor. The active v1 mesh retains its original visible doorway
and trigger; the v2 mesh selects the stump and suppresses the temporary arch.
The scene/layer/room and current save guards remain active.

`tools/relocate_mm3d_lost_woods_return.py` consumes exactly the purple ExitSeasons
POC2 archive, SHA256
`f07a2e56c183b4600146434fd5f04b10cff7fd932227d33ab297e108a2bf78ac`.
The generated POC3 archive is SHA256
`6f120f52340da1874d5a650235bc57154cc62d5a8a44a5429a056ed8eb3f6284`.
Five doorway vertex resources change only X/Z through a rigid transform, four
room-zero mesh references change identity, and one wrapper is added. Every
other resource is byte-identical, including textures, scenery, all room-one
resources, collision, actors, UVs, Y, RGBA, seasons and the purple glow palette.
Replace the prior Lost Woods mod with POC3 when using this source.

The full forest suite and stump ASan/UBSan test pass. The actual passage renderer
compiles against native engine/dependency headers. Independent review verified
the archive delta, reachable native floor and material bindings. It found an
unsafe receipt filename; a failing real-file CLI regression now protects source,
candidate and previous evidence before transformation. Exception cleanup only
removes an output created by this process. The asset preview is rendered from
the serialized pack; it is not a game capture.

The later arrival-camera feature uses the user's reference framing and has its
own lifecycle evidence. Installed-game checks remain: enter and return with
native/Alt assets, each player form, all days/seasons, save/reload and both forest
routes; verify actual camera collision, arch placement and preserved progress.
