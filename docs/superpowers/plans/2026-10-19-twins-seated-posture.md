# Twins seated posture correction — POC9

Goal: address the full replacement's hunched pose3 silhouette while retaining its native local head nod. Continue inline within the existing approved asset-repair scope; no game build, commit or merge.

Parent: POC8 `c57501ff2848a33579fe125d73ba8d662a8e4d96b6b57da4afa4e7108a26015f`. The previous pass checked head rotation but left the excessive full-body hunch unresolved. Keep the face/blink/shoulder geometry, POC8 pose2 skirt and pose5 arm repairs, rig, palette, all other poses and runtime code untouched.

- [x] Measure the full rendered head-over-seat line against actual native geometry. Record the POC8 failing posture check.
- [x] Add `refine_twins_seated_posture.py`: compare head-area centroids over the seated hip anchor, rotate only torso joint8, and retain native local head joint15 plus every other channel. The resulting rotation is approximately11.9 degrees back and moves the complete connected upper-body chain.
- [x] Validate the new private clip in the generic asset path. Add a single-clip, variant/parent-bounded verifier exception that rederives the exact fit rather than permitting arbitrary torso/head edits.
- [x] Verify the entire upper-body chain stays rigidly connected, root/legs remain unchanged and malformed root/leg/head/arbitrary torso mutations are rejected.
- [x] Render native/before/after front/65/90 at the same frame, camera, world scale and bounds; inspect the full silhouette and joints.
- [x] Run canonical archive verification, all93 exporter tests and independent source/preview review. Package the candidate and cumulative source recovery patch without treating this as runtime proof or user acceptance.

Production files: `scripts/npc_models/refine_twins_seated_posture.py`, `scripts/npc_models/verify_native_npc_models.py`. Diagnostic scripts: `twins-poc9-diagnostics/check_posture.py`, `verify_posture_guards.py`, `render_matching_posture.py`. They exercise actual exported geometry and native animation resources.

Preservation: only the manifest changes in existing parent resources; add the private pose3 animation and its report. All342 other parent resources are byte-identical. The native head nod remains relative to the corrected torso, rather than preserving the old hunched world orientation. Runtime and user acceptance remain pending.
