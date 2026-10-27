# Windows launcher resource correction — 2026-10-13

Status: focused checks passed; hosted MSVC and full application packaging remain
pending. This correction stays on draft ComboShip PR #41.

## Failure and cause

On parent `a16476a6ebcdc43444b80c777d59c79689a4d791`, run
[38344032786](https://github.com/marsh6487/ComboShip/actions/runs/38344032786)
passed the regression gate, Linux packaging and Windows SoH, MM, libultraship
and shared UI builds. The Windows launcher link then reported:

```text
CVTRES : fatal error CVT1100: duplicate resource. type:ICON, name:1, language:0x0409
LINK : fatal error LNK1123: failure during conversion to COFF: file invalid or corrupt
```

The 9.3 integration merge `1d3d430de1b753bdfdaaa267ab8a13f5ed8b9703`
added generated `ComboShip.rc` to the launcher while retaining legacy `Fleet.rc`.
Both files define an application icon. Linux excludes both Windows resources,
so its successful build did not expose the conflict.

## Correction and preservation

The generated `ComboShip.rc` is now the only resource input. It uses the
established `FLEET.ico` and retains its existing version information. The legacy
resource file remains in the source tree but is no longer compiled into the
launcher. Output name, save gates, stack reserve, game code, asset deployment
and scene archives are unchanged.

Unchanged source SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `combo/FLEET.ico` | `978c77da07180b4f42321bdc3cd00fe6be88bf654480a4630a996db49d6adeaf` |
| `combo/windows/ComboShip.rc.in` | `7b849834308a01acf991dbd5d52258acfc4bba62adc6fab79c4657e64b51d800` |

## Verification

`run_windows_launcher_resource_tests.py` evaluates the complete production
launcher CMake file with only dependency boundaries stubbed. Before the fix,
its Windows source check fails on the two resource inputs. After the fix, it
finds one resource, the Fleet icon and the expected file/product versions.
The separate Linux condition selects no Windows resources.

The generated resource also compiles and links into a minimal Windows PE using
Zig on the local Linux host. The compiled bundle contains one icon group and
one version record, and every icon image matches the original Fleet ICO bytes.
Independent read-only review repeated both checks and found no blocking or
important issue. Whitespace verification passes.

The Windows workflow runs this same regression before its dependency restore
and full build. On Windows it uses `rc.exe`, `cl` and `link`; consuming the `.res`
at that link exercises the MSVC resource-conversion path. Local Zig proof is
not hosted MSVC or full application proof. The synthetic Linux CMake graph
checks source selection, not a Linux crossbuild. In-game actor/texture
acceptance remains governed by the separate actor-tint record.

Reproduction from the repository root:

```text
python -B scripts/diagnostics/run_windows_launcher_resource_tests.py
```

On a Linux host, add `--zig <zig-executable>` to compile/link the Windows
resource bundle; without it the runner checks only the production source graph.
An alternate CMake executable can be supplied with `--cmake <cmake-executable>`.
