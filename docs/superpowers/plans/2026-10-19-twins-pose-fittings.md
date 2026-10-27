# Twins pose fitting repair plan

> **For agentic workers:** Execute inline with the existing authorized POC workflow. Do not commit, build or merge.

**Goal:** Correct the reported pose 2 cloth breakup and pose 5 whole-arm placement, and compare poses 3/4 to native head/hand orientation.

**Architecture:** Derive new pose-specific skin lists from POC7 and keep accepted shared resources intact. Reconstruct pose 5 arm rotations from native wrist targets scaled by the replacement arm length; keep wrists attached through the existing IK solver. Preserve native timing, root and all non-arm channels.

**Tech Stack:** Existing O2R exporter/decoder, NumPy/SciPy and offline native animation preview.

**Spec:** User feedback on skirt in 2, downward head in 3, native hand orientation in 4 and hand/arm placement in 5.

## Global constraints

- Parent POC7 SHA256: 85ea0720d30cdc20eb70277383ade3c1af033a500a7fcb87e94d144aa1e58868.
- Accepted POC6 head, faces, blink art and shoulder surfaces remain unchanged.
- Original GLB, native archive, POC6, POC7, other model candidates, actor routing and runtime code stay unchanged.
- Produce a separate POC8 archive and actual archive renders; do not imply runtime/user acceptance.

## Review focus

- Pose 2 lower tunic remains a continuous surface across original leg-host boundaries.
- Pose 5 wrists stay above their own elbows as in the native pose, with native palm direction and connected forearms.
- Pose 3 downward tilt and pose 4 palms are compared to actual native controls, not guessed.
- Existing pose 0/1 resources and all face/blink/palette resources are byte-identical.
- All six static pose routes remain usable, including static4 donor legs/head correction.

### Task 1: Reproduce and diagnose

- [x] Inspect native and candidate controls at matching frames.
- [x] Trace lower tunic ownership and native/custom elbow/wrist positions.
- [x] Record failing integration checks for cloth coherence and raised crossed wrists.

### Task 2: Bounded asset repairs

**Files:** Create `scripts/npc_models/refine_twins_pose_fits.py`; diagnostic integration check in `twins-poc8-diagnostics/verify_pose_repairs.py`.

- [x] Re-emit only pose 2 skin hosts 0/1/4/7 with coherent chest-attached lower tunic vertices and original triangle/UV/material data.
- [x] Retarget pose 5 whole arms from native shoulder-relative wrist positions, preserving native hand rotation and non-arm channels.
- [x] Confirm integration checks turn green and run canonical exporter verification and focused regression suite.

### Task 3: Review and preserve

- [x] Render changed poses at front/65/side views and native controls for 3/4/5.
- [x] Inspect actual images, adjust only measured fitting issues, and preserve all accepted surfaces.
- [x] Package new candidate, cumulative source patch, plan, checks and preview comparisons; save deliverables.
