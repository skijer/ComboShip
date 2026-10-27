# Young Malon face and idle repair plan

> **For agentic workers:** Use native execution for this bounded asset repair.

**Goal:** Repair the diagonal eyes and sharp facial shading, and restore vanilla's relaxed idle arm and hand character.

**Architecture:** Keep Young Malon POC8 immutable. Trace its face against the original GLB and vanilla skeleton, then patch only private face resources and the idle arm channels into POC9. Preserve the fitted body, scarf clasp, raised-hands and singing clips, blink routing, tracking and fallback.

**Tech Stack:** Python, NumPy, SciPy, existing O2R readers/writers and offline renderer; imagegen for bounded iris artwork cleanup.

**Spec:** User's 2026-10-19 request: diagonal eyes and facial shading need repairs; refer to vanilla for idle arms/hands; other poses look acceptable.

## Global constraints

- Parent archive SHA256: da1f2ae86efdaef03897f73b06621a4aa9bf451c80c2f71e19372e2fba74f21f.
- No full game build, merge, push or change to the accepted Twins.
- Face shape and silhouette retained outside the eye interiors; source diffuse retained as recovery.
- Native timing, root, head, torso and legs preserved; connected wrists and shared shoulder skin retained.
- Candidate remains unaccepted and runtime untested until demonstrated.

## Review focus

- Both diagonal directions: continuous irises without inward dents or angular fragments.
- Shared face boundaries: normals agree across UV and expression splits.
- Blink states: matching eye shape and continuous surrounding skin at open, half and closed.
- Idle: vanilla shoulder/elbow/hand orientation measured over the full loop; no garment penetration.
- Preservation: raised-hands/singing resources and all unrelated records byte-identical to POC8.

### Task 1: Evidence and narrow regression checks

- [x] Compare lit, unlit, clay and original GLB eye surface; measure native/custom idle directions.
- [x] Write focused failing checks for a bounded smooth eye depth field, continuous face normals and native-relative idle arm/hand orientation.

### Task 2: Child-only asset patcher

**Files:** Create `scripts/npc_models/refine_young_malon_face_idle.py` and its focused test; keep shared exporters unchanged.

- [x] Reconstruct smooth eye interiors with the existing eyelid boundary retained; use one continuous skin normal field.
- [x] Bake bounded iris artwork onto the unchanged frontal registration, keeping all surrounding artwork and blink states.
- [x] Fit the connected idle arm chain to vanilla directions and hand rotation, adjusting only for demonstrated dress clearance.
- [x] Export separately named POC9 with provenance, permitted resource set and exact preservation comparisons.

### Task 3: Review and delivery

- [x] Run focused regression and canonical model verification.
- [x] Render both diagonal face views, front, open/half/closed eyes, vanilla/custom idle front/side, and the preserved other poses.
- [x] Inspect the actual archive renders; save previews and recovery/source patch together.

Validation: final POC9 SHA256 8d3aada73d8a34470aaebe6eaa526c3883ae95a918aa297e50d265ce692dd465; 98 model Python tests pass, canonical archive checks pass and independent review found no blocking/important issue. 293 protected records remain identical; all 91 shared face positions have identical normals. Actual archive previews inspected at front, both diagonals, three blink states and all three poses. Runtime untested; candidate unaccepted and unmerged.
