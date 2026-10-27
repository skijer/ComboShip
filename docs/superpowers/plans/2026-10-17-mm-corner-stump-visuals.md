# MM corner stump visuals implementation plan

> Execute inline with superpowers:executing-plans; preserve the isolated baseline.

**Goal:** Correct the atlas mapping and place one finished doorway at the corner stump.
**Architecture:** A hash-gated pack transform changes only scenery T, doorway
positions and four mesh bindings. Shared version-aware anchor helpers keep the
trigger, camera and rendering aligned. A small native draw helper adds the rim.
**Tech stack:** Python, XML/O2R, C++, existing MM display-list APIs.
**Spec:** `docs/poc/mm-corner-stump-visuals-20261017.md`.

## Global constraints

Preserve original inputs; never overwrite an existing archive or receipt.
Preserve collision, actors, textures, all non-mesh header commands and progression.
Only v3 and native fallback use the corner; v1/v2 retain their visible locations.
No full publication/build cycle or master promotion is part of this candidate.

## Review focus

Old packs must keep their own trigger and camera anchors.
All imported materials in both rooms must retain artist UV correspondence.
Exactly one doorway must be reachable from active room-zero roots.
The shimmer must not draw in solo mode, flashbacks, room one or older packs.
Outputs, receipts and source archives must not alias or overwrite each other.

## Task 1: Pack mapping and doorway placement

- [x] Add an asset regression against the original DAE's UVs and the exact old pack.
- [x] Run it with the unchanged candidate; observe UV and location failures.
- [x] Implement `tools/repair_mm_lost_woods_visuals.py`: hash-gated T correction,
  rigid doorway move, v3 wrapper and receipt, preserving all other bytes.
- [x] Verify DAE correspondence, exact changed-resource set and output safety.

## Task 2: Runtime anchor, camera and purple finish

- [x] Extend the actual native return test for v3, no old invisible trigger,
  v1/v2 compatibility, all forms/days and native fallback; observe failure.
- [x] Implement shared version-aware return anchor/yaw helpers in
  `ComboOpeningForest.h`; use them in the camera and fallback draw.
- [x] Add a tested deterministic rim/glint helper and native XLU submission.
- [x] Run the forest suite and production renderer compile/linkage check.
  Focused checks pass; the broader suite is blocked by the baseline's missing SDL2 headers.

## Task 3: Delivery

- [x] Render atlas/stump/grass and doorway comparisons from packed resources.
- [x] Review the complete change; correct material findings and rerun affected checks.
- [x] Record static versus runtime status, create a recoverable source package,
  and deliver the separately named candidate archive.
