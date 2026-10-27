# MM boss soul selection and material audit — 2026-10-07

Baseline: verified develop `13901c677d0084e0305eec4672c0557f4434aea7`.
The user reports corrupted boss soul textures, especially Goht and Odolwa.
No screenshot, exact installed boss replacement archive, or Alt configuration
was supplied for this report. The following defects are reproduced from source;
they do not prove that every reported visual symptom has the same cause.

## Confirmed native renderer defects

`DrawGoht`, `DrawGyorg`, and `DrawOdolwa` initialized a static `SkelAnime`
once using `SkelAnime_InitFlex`, then always used `SkelAnime_DrawFlexOpa`.
`ResourceMgr_LoadSkeletonByName` selects MM's Alt resource at initialization.
These drawers therefore retained the first selected rig and clip after an Alt
toggle, while later display-list/texture resource resolution used current
selection. The extended executable fixture fails when it expects the replacement
limb paths after the first toggle.

The native drawers also ignored the loaded skeleton's type. A selected normal
replacement has no flex matrix count. Selecting one first in the fixture exposes
a 64-byte heap write through the production `SkelAnime_DrawFlexOpaImpl` into a
zero-size matrix allocation. ASan identifies `DrawGoht` as the caller. The same
initialization/draw pattern occurs in Gyorg and Odolwa; all three candidate
entry points are exercised with a selected normal replacement.

Native MM now queries the existing boss animation recipe and shared renderer,
as native Twinmold already did. That renderer keys rigs/clips by owning game and
owning Alt state, validates limb counts, and chooses the rigid/flex drawer from
the loaded resource type. It also contains the model/flame transform and restores
the flex matrix segment. Twinmold's public native helper remains a wrapper for
its existing route.

## Native material evidence

The read-only audit used the user's native `mm.o2r`, SHA-256
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
It walks each boss's actual limb table and mesh command stream, resolves hashed
texture dependencies, and records material/LUT/primitive state. All referenced
texture resources are present in this archive.

| Soul | Native rig | Limbs / flex matrices | Texture / TLUT resources | Dynamic body texture binding |
| --- | --- | --- | --- | --- |
| Goht | `gGohtSkel` flex | 32 / 18 | 6 / 0 | Segment 8, circle-pattern chin texture |
| Odolwa | `gOdolwaSkel` flex | 51 / 30 | 15 / 6 | None; CI4 palettes load inside meshes |
| Gyorg | `gGyorgSkel` flex | 14 / 9 | 10 / 4 | None |
| Twinmold | `gTwinmoldHeadSkel` normal | 12 / 0 | 12 / 5 | Segment 8, blue skin |

Goht's jaw uses raw `G_SETTIMG` references at `0x08000001`, so its segment-8
texture binding is required. The actor, former native soul drawer, and exported
`MM_FillBossSoulAnim` all use the same correct texture. Odolwa's actor body has
no additional texture segments; its post-limb eye quads are a separate effect.
Native Goht/Odolwa mesh materials configure their combiner, LUT mode and
primitive colors themselves. No missing native palette or material binding was
established by the audit, so no speculative color reset or texture rewrite was
introduced.

Models, clips, scales, native soul flame palettes/phase, MM-owned Alt selection,
and all exported boss recipe values are preserved. No gameplay or archive
resource is changed. Majora and enemy soul behavior remain outside this change.

## Executed checks

```sh
python3 scripts/diagnostics/run_foreign_soul_skeleton_tests.py --baseline-mm-bosses
python3 scripts/diagnostics/run_foreign_soul_skeleton_tests.py --baseline-mm-bosses-type --sanitize
```

Expected baseline failures: stale native limb paths after Alt changes; ASan
heap-buffer-overflow through native flex traversal with a selected normal rig.

Observed red signatures:

```text
--baseline-mm-bosses: expected mm_soul_custom_ limb path; retained mm_soul_native_
--baseline-mm-bosses-type --sanitize: AddressSanitizer: heap-buffer-overflow
WRITE of size 64: Matrix_ToMtx -> SkelAnime_DrawFlexOpaImpl -> DrawGoht
```

```sh
python3 scripts/diagnostics/run_foreign_soul_skeleton_tests.py
python3 scripts/diagnostics/run_foreign_soul_skeleton_tests.py --sanitize
python3 scripts/diagnostics/run_foreign_soul_skeleton_tests.py --sanitize --native-mm-bosses-alt-first
python3 scripts/diagnostics/run_boss_soul_model_tests.py --sanitize
python3 scripts/diagnostics/run_barinade_syntax_tests.py
python3 scripts/diagnostics/audit_mm_boss_soul_materials.py --native-mm /path/to/mm.o2r
git diff --check
```

Candidate checks pass. The skeleton fixture executes complete shared rendering,
native entry-point bodies/static macros, both engines' native limb traversal,
rigid/flex selected resources, repeated owner Alt switches, native model/flame
transforms and colors, and matrix/segment cleanup. Resource loading and animation
initialization remain fixture boundaries. Leak detection is disabled under the
existing fixture's ASan environment; no leak-proof claim is made.

The header runner additionally compiles all four native MM boss entry points
with the actual MM engine declarations. Both full changed native C++ source
files also pass syntax checks with MM's normal PCH prerequisites and supplied
build dependencies; existing controller macro-redefinition warnings remain.
There is no full application build or runtime appearance acceptance here.

## Runtime acceptance still required

Use the user's installed boss pack/load order. Compare native Goht/Odolwa/Gyorg
GIs with MM Alt off, on, off again, including shelf/re-entry and acquisition.
Confirm selected materials and weighted meshes remain intact, animation and
native colored flames match, and neighboring items retain their own state.
Check the foreign MM souls in OoT under MM's owner Alt selection and preserve
the accepted Twinmold native/foreign head presentation. Arbitrary incompatible
replacement rigs and unsupplied pack textures are not remapped or visually
validated by these tests. The legacy native fallback remains for unavailable
shared recipes/resources, matching the existing Twinmold fallback policy.
