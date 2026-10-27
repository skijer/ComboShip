# Foreign boss-soul replacement skeletons — 2026-10-01

cor reports Volvagia's soul using the TP boss model in MM's Trading Post, with part of the lower jaw stretched across the scene. This correction is requested for the pending shop-stream/Romani mask candidate.

Baseline: PR #31 head `d196518008067debf922372ab60bc9649d440c8b`. The reported installed TP archive, exact build and clip are not available in this task; the visual root cause remains a strong source-level diagnosis rather than a pack-specific runtime proof.

## Source evidence

- `OOT_FillBossSoulAnim(4)` describes vanilla Volvagia as a rigid skeleton (`nonFlexSkeleton = 1`).
- `ComboForeignAnim_Draw` previously used that recipe flag for both initialization and drawing, even if the resource manager selected a replacement flex skeleton.
- SoH's own `DrawVolvagia` calls `SkelAnime_DrawSkeletonOpa`, which instead examines the loaded skeleton's type and supplies the flex matrix array when required.
- Both games' resource factories expose `skeletonType` in their loaded header. MM's public native header omits that field, so the shared consumer uses an explicit factory-layout mirror with size checks.
- Native flex drawing writes the limb matrix array and binds segment 13. Rigid drawing does neither. A replacement jaw display list that references other limbs' matrices can consequently read an unrelated scene matrix array on the old path.

## Candidate

Select rigid/flex initialization and drawing from the loaded resource type, retaining its actual flex display-list count. Reject unsupported curve resources before emitting drawing commands. Preserve vanilla recipe scales, poses, colors, scrolls, soul flame and item ownership/grants.

Include owning game and owning manager's Alt selection in the skeleton/animation cache key; an Alt change must not keep the previous selection's limb pointers/type while drawing the new display lists. This respects the existing per-game Alt settings; it does not synchronize their independent controls.

After the OPA flex model draw, restore segment 13 using existing foreign-render segment hygiene, preventing the model's matrix array from being inherited by later foreign draws. The existing XLU-only fairy path is otherwise unchanged.

No model archive, geometry, scene or save is edited. No new checkbox is required.

## Checks and limitations

`run_foreign_soul_skeleton_tests.py` compiles the complete production foreign-animation header against the actual native rigid/flex limb drawers from BOTH engines. Resource I/O, animation initialization and matrix/GBI primitives are modeled. The command stream is replayed after matrix generation, checking the jaw's root and local matrix entries, owner-routed DLs, segment-13 cleanup, vanilla/replacement selection, repeated Alt switches, separate owner caches, one update per frame and unsupported/missing-resource fallback.

- Candidate: both engine fixtures pass normally and with ASan/UBSan.
- Baseline: loading the flex replacement first fails the missing segment-13 jaw-matrix assertion; this isolates dispatch from the separate cache bug.
- Existing MM item presentation and foreign-scale/texture/save lifecycle probes pass.
- clang-format 14 for changed C++ lines/new fixture and `git diff --check` pass.
- Local LeakSanitizer is disabled because the container cannot inspect process threads. CI retains its default sanitizer environment.
- Full application compilation and actual TP model appearance are not yet verified. The fixture demonstrates the draw-path defect, not the exact TP archive's matrix indices or geometry.

The CI gate includes the normal and sanitizer fixture runs. This is an implemented, offline-verified candidate only; no merge or master promotion is implied.

## Smallest decisive runtime check

Use the combined pending shop/mask package, the same TP boss pack/load order, and the same Trading Post shelf item. Observe the Volvagia soul from several angles during idle and browsing, leave/re-enter, and compare the owning game's Alt selection off/on. Confirm the jaw remains attached and adjacent shelf items/scene geometry stay intact. Check the acquisition GI if the item can be bought in this seed. Other rigid-to-flex replacement boss souls share this correction but still need their own visual acceptance.

Recovery: revert this isolated correction; the model archive and prior shop/Romani work remain unchanged.
