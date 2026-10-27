# Snowhead portal exterior arrival candidate — 2026-10-25

Baseline: `7cd3ac8d09dfb72009c2404ef10ac8da4a62c0c1`, tree
`e6e3270a8794eb9896d481b93dbbecabfa7a9308` (accepted public/CI source).
This bounded candidate changes only the queued Snowhead arrival presentation.
It is source/native verified; configured game runtime and user acceptance are pending.

A successful Snowhead regional handoff still chooses native seasonal entrance 8
and ordinary start F in a module-owned copy. The cached native owl spawn stays
`(-649,8,-196), yaw20388, OWL`. Only that matched queued spawn requests a reveal.
At scene-init the existing camera helper checks scene, entrance, spawn, room,
slot, seed, combined mode, active main camera and ordinary gameplay state. It
consumes requests on success or mismatch. Normal owl warps and resumes do not
queue this presentation.

The successful reveal places Link 140 units along the authored mouth's outward
normal: approximately `(-414.989624,8,-183.208908), yaw-1362`, outside the 110-unit
return arming radius. It carries the native F walk target with the new pose.
The camera stands 160 units in front of Link, looking back at Link and the
portal with native 60-degree FOV. Native collision still bounds the view;
invalid floor or insufficient camera clearance leaves player and camera at the
native landing. Forest presentation distances, 35-degree FOV, input cancellation,
walk tracking, bounded release and native-handler handoff are preserved.

Read-only native MM archive SHA256:
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
Winter collision is `Z2_10YUKIYAMANOMURACollisionHeader_007004`; spring is
`Z2_10YUKIYAMANOMURA2CollisionHeader_0058E8`. Exact production camera outputs
using native five-form heights pass dry floor/27-unit footprint, 137-unit
headroom, native owl/snowball clearance, a usual 40-unit F walk, eye rays and
full 136-high/136-wide portal visibility against both native triangle meshes.
The eye ray check adds a one-unit safety margin. An initially considered longer
camera arm collided with the native cliff; the final short native-FOV framing
avoids that geometry. No native collision or props are changed.

Regression evidence:

- Before production edits, the new real queued-spawn/camera regression failed
  with `queued Snowhead portal must begin a front-facing reveal`.
- Full portal camera fixture executes production regional spawn code and the
  complete camera enhancement, native F/E start, native camera collision math,
  form heights and NORMAL1 release. Winter/spring, three slots, five forms,
  cached owl/save preservation, mismatched/consumed requests and transactional
  floor/camera fallbacks pass. Existing forest native/v1–v8/day/form cases pass.
- Regional suite, sanitized fixed/native-start suite, sanitized portal camera
  and native forest-camera fixtures, MM native fade fixture pass.
- Both affected production translation units compile with real headers in
  combined and standalone modes. Pinned clang-format 14.0.6 check passes.

Reproduce with dependencies available on the normal fixture include path:

```sh
python tests/regional_passages/run_tests.py
python tests/regional_passages/run_mm_fixed_tests.py --sanitize
python tests/lost_woods_entrance/run_portal_camera_tests.py --sanitize
python tests/lost_woods_entrance/run_mm_camera_arrival_test.py --sanitize
python tests/lost_woods_entrance/run_arrival_fade_tests.py --game mm
# Use a fresh output file: the fixture appends exact source/native poses.
COMBO_SNOWHEAD_POSES=/tmp/snowhead-camera-poses.txt python tests/lost_woods_entrance/run_portal_camera_tests.py
python tests/regional_passages/mm_collision_test.py /path/to/native/mm.o2r /tmp/snowhead-camera-poses.txt
```

Runtime limits: triangle sampling and native fixtures do not run the game/GPU.
Verify the configured vanilla/Alt asset load order, incoming fade/ordinary F
animation, whole-portal framing, walk/input camera release, return/re-entry and
ordinary owl warp in both seasons. Unusual carried entrance speeds, actor
motion, native camera terrain behavior and replacement scene collision need
runtime checks. Accepted shop cameras/framing, stock/GI scale, counters,
materials, Fountain effects and level 2 Ice/Light spin paths are untouched.
Shield testing remains blocked by pausing; this candidate does not diagnose it.

Recovery: apply/revert only this bounded source/test/doc commit on its baseline;
no game/archive assets, external branches, saves or accepted masters are modified.
