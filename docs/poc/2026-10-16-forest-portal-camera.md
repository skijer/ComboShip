# Forest portal arrival camera

| Field | Record |
|---|---|
| Baseline | Isolated `fix/shop-forest-entrances-20261016`, including stump candidate `9e177bcf`; title-card candidate `00c893ad` retained. |
| Reference | Inspected user screenshot `image(20261016-093258).png`: full Link low in the middle, camera in front looking back toward the centered doorway. |
| Scope | Queue a single presentation in `MM_ApplyComboPortalArrival` for Opening Forest and Woods of Mystery. Ordinary save resumes, scene re-entry, flashbacks and other arrivals do not queue it. |
| Placement | Mystery retains `(0,0,0)`. Opening native/v2 uses stump `(120,0,240)`, yaw `-16384`, and front depth 226, placing Link at `(-106,0,240)` after native floor validation. V1 resolves the active legacy spawn/doorway instead. Parent task confirmed native ground at the v2 candidate: polygon 285, surface 6, Y=0. |
| Framing | Existing main camera, FOV 35, outward arm `max(190,4.3*height)`, target height `clamp(1.05*height,46,80)` and eye above target `max(20,.25*height)`. All five native form heights fit the geometric frame. |
| Preservation | No subcamera, cutscene, story, inventory, saved form, control-lock or scene-resource changes. Native E/D entry target is carried with relocated position/yaw; its existing automatic walk remains intact. Floor and eye clearance must both succeed before moving player/camera. |
| Collision | Native `Camera_BgCheckInfo` checks the presentation eye each frame; a hit shortens the eye with clearance. An unusably close eye or missing/steep floor rejects preparation and leaves native state intact. |
| Release | Track the original automatic entrance walk through the fade, then hold at most 60 ready-control camera updates. Input, pause, mode/setting changes, room/slot/seed/lifetime changes and another camera's pose changes permanently release ownership. Native handler resumes from the current pose; no reset behind Link. |
| Evidence | Failing regressions preceded implementation, Queue integration, native entry-target/walk fixes and later-hook takeover fix. Updated ASan/UBSan fixture passes, including exact native `Camera_Normal1` initial update with the native NORMAL0 table, trig/atan math and flat/no-obstacle collision boundaries. It checks both routes, native/v1/v2, every day/form, projection, native first-release eye/target/FOV bounds, save bytes, rollback and lifetime. Production C++ object compiles against real headers. Independent review has no remaining blockers. |
| Limits | In-game screenshot matching, active archive collision, and actual user camera enhancements remain untested. Native handler tests cover the stated flat-ground boundary, not every camera AI branch. |
| Recovery | Remove the camera enhancement/header and two Queue calls/header include from BenPort; scene/archive geometry and the title-card candidate are independent. No master promotion is implied. |

Decisive runtime check: cross each forest portal with native assets and the current v2 pack, let the entry walk/fade settle without input, then move, use the camera, pause/unpause and return. Confirm full Link plus doorway framing, smooth control release, no reacquisition, collision clearance, and unchanged native onward routes.
