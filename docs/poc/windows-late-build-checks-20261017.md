# Windows late build failure and check audit

Baseline: PR #44, `integration/big-merge-20261016`, commit
`39ae1689b76ec48a5bdebbe3daaa9e358b1ea0fa`.
Failed run: https://github.com/marsh6487/ComboShip/actions/runs/38988813628
Windows job: https://github.com/marsh6487/ComboShip/actions/runs/38988813628/job/117275990018

## Evidence and correction

The gate passed in 42m58s. Windows then built libultraship, soh and 2ship
successfully, but failed at `Build comboui` after another 49m31s. Linux built
and packaged the complete candidate successfully, including both seed smoke tests.

The original bulk Windows log could not be downloaded through the connector.
The step API identifies the failing target; an independent reproduction identifies
a concrete source defect consistent with that Windows-only failure:
`ComboFilePicker.h` includes `windows.h` without `NOMINMAX`, and the new
`ComboUI_SelectGame` calls `std::max`/`std::min`. SDK macros rewrite those calls
into invalid C++. The existing selector fixture removed the file-dialog header,
so it could not expose this defect.

Define guarded `NOMINMAX` before the SDK include, matching `ComboResolve.h`.
Retain the production dialog header in the selector's new Windows modes. The
portable mode models only the SDK boundary; MSVC mode uses the real SDK.
The complete production screen and all five selection scenarios remain in use.
The new regression failed on the original header and passed after the correction.

## Other late checks

| Boundary | Finding and change |
| --- | --- |
| UI DLL | Depends only on libultraship. Move its complete compile/link before both game engines; expose compiler/linker diagnostics as check annotations. |
| Launcher and validator compilation | The independent Windows probe now compiles their actual generated projects without linking or building game references. It also builds the complete UI, while the long gate runs. Final normal linking remains in the full build. |
| Game picker SDK compatibility | Run before configure in both Windows workflows, and model the SDK macros in the first Linux gate step. |
| Both archive generators | Depend on their packers and collision validation, not game DLLs. Move both existing targets before the engines without duplicating them. The archive-copy destinations and final CPack steps remain unchanged. |
| Launcher icon/version resources | Existing native resource compile/link preflight already precedes configure; retain it. |
| Game DLL linkage and final package | Require the actual compiled outputs; retain normal final linking, CPack, staging and upload checks. Passing source checks alone does not prove these stages. |
| Long regression gate | Preserve every existing command unchanged. The expanded independent Windows probe provides earlier Windows compiler evidence without removing regression coverage. |

Only the shared file-dialog header, test boundaries and CI scheduling change.
Game source, integrated assets, upstream pins and accepted gameplay behavior are
preserved. Local reproduction and focused checks are distinct from a complete
Windows build; the corrected candidate still requires its new CI run.

## Local verification and publication status

- Selector: ordinary and modeled Windows-header modes pass all five scenarios.
  The Windows-header mode was observed failing before the production fix.
- ASan/UBSan: Windows-header selector and 13 ROM-setup cases pass. Leak detection
  is disabled because this host restricts sanitizer access to `/proc`.
- Complete solo behavior gate: 3 compiled mode/options probes and 18 production
  boundary suites pass, including the full launcher and native saved settings.
- Launcher CMake graph: one Windows icon/version resource and no Linux resource
  input. This local check does not claim an actual MSVC resource compile/link.
- Asset collision check: 1,893 shared paths, 182 known differing-content paths,
  no new collisions. Workflow YAML, formatting and diff whitespace checks pass.
- Every original regression command and Windows build/package/upload step is
  retained exactly once. Review identified that native compile-only preflight
  should use `_ClCompile`/`_ResourceCompile` wrappers; those wrappers are used.
- Recovery patch was applied to pristine baseline files and all resulting files
  were compared byte for byte with this candidate.

Initial GitHub publication attempts were rejected by automatic approval review,
including after retrieving the earlier authorization for this branch. The tested
correction was preserved as a recovery checkpoint. The user then explicitly
authorized publication in the current chat on October 16, 2026 at 22:08 CDT.
Native Windows compilation, linking and packaging remain pending the new CI run
at the time this correction is committed.
