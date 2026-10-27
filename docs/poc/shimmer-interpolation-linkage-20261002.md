# Shimmer frame-interpolation linkage repair

Baseline: PR #33, `poc/mm-native-oot-soul-flame-20261002`, head
`644ef5a05d2a8c4be983b0dcbf4f9c1321957186`.

Build Artifacts run [37074006869](https://github.com/marsh6487/ComboShip/actions/runs/37074006869)
passed the regression gate but failed both full application builds. Linux job
111061581040 rejected conflicting C/C++ linkage for
`FrameInterpolation_RecordOpenChild` and `FrameInterpolation_RecordCloseChild`.
Windows job 111061581036 failed to link the C++-mangled versions of those symbols
from `draw.obj`.

`ComboMaskShimmer.h` expands OoT's `OPEN_DISPS`/`CLOSE_DISPS` before
`ComboForeignAnim.h` supplies its namespace-scope C declarations. Their
block-scope declarations therefore initially acquire C++ linkage. Include the
native `soh/frame_interpolation.h` in the draw translation unit before shimmer.
Only this native-header include changes production code. Retain all baseline
item grants, notifications, icons, Chateau handling, flame/shimmer effects,
models, owner routing, colors, and prior stacked changes.

Strengthen the existing shimmer fixture: derive the early interpolation include
from production draw.cpp; replay the later foreign renderer declarations;
define interpolation functions in a separate translation unit using the native
header. Same-TU early stub definitions previously concealed this include-order
and linkage defect. The existing CI shimmer step runs this stronger check.

Verification: removing the new include reproduces the two exact Linux linkage
errors. Candidate shimmer compile/link/presentation checks pass in both engines
normally and under ASan/UBSan. Foreign soul and Great Spin checks also pass in
both engines normally and under ASan/UBSan. Whitespace checks pass. Local leak
detection is disabled for the existing container limitation. Full Windows/Linux
application builds are pending CI at publication; in-game acceptance is pending.
No merge or master promotion. Rollback is removal of this isolated include and
its fixture update; baseline commit above remains available.
