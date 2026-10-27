# Granny compatibility and built-in magical materials

Baseline: local magical-material fix `a366ea1491f4403aa9b5f9ddd271f6db4bd5e6b6`,
parented by public cumulative PR66 `0cd35e4d25f853dc68995dd88e8b1c177840e301`.
Candidate branch: `poc/black-market-granny-availability-20261027`.
The recovery archive records exact candidate refs and restoration evidence.

## Report and cause

The supplied screenshot directly shows **“Incompatible Granny rear pot
components”**, with Regular Granny Interior selected and Magical Room Materials
unchecked. Dependencies had reached room validation; this is not the generic
missing-pack condition. The same error was reproduced using the original
unchanged Djipi files through the actual native XML display-list/vertex factories.

The rear-pot filter recognized native packed triangle opcodes `0x05/0x06`, but
the XML factory emits `Triangle1` as extended opcode `0x26`, with raw vertex
indices split across both words. Ignoring those faces made strict component
validation reject the compatible donor. The filter now decodes that format,
checks bounds and loaded cache entries, removes only selected complete faces,
and leaves surviving commands byte-identical. The existing six-piece geometry
checks remain strict. The older donor fixture now uses the actual single-face
codec; a new real-factory test prevents a repeat of that inaccurate assumption.

Two separately reproduced false-unavailable cases are also corrected: a partial
active-host Djipi cohort no longer masks a complete registered peer cohort,
and a scene-only override no longer masks a complete mounted native archive.
Complete active-host maps retain priority; native data stays pinned to one
complete archive. These selection defects were not the screenshot's exact cause.
Preparation failures now record their full existing reason in the log.

## No external archive for magical materials

Magical Room Materials uses the graph, original neutral texture and geometry
prepared into the ordinary `soh.o2r` and `2ship.o2r` assets. It does not require
a Djipi pack, Granny pack, or supplemental Black Market archive. Both Alt-off
and Alt-on may resolve the built-in base resources without an external override.
The previous source correction preserves active Alt geometry when overrides
are installed; it is included in this cumulative checkpoint.

The added asset regression traverses the complete magical graph, all 28 material
roles, vertices and texture dependencies using only normal port preparation
inputs, for both hosts and both Alt states. The native preparation fixture
retains built-in-only normal/Alt coverage, and the MM attachment probe confirms
that a failed optional Granny request does not disable magical materials.
Regular Granny Interior remains the separate optional mode that reads the
user's installed Djipi and native OoT resources.

## Preservation and verification

Retain the accepted shop camera, collision, stock, actors, purchases, palette
controls, alpha/UVs and source resources. No generated asset, texture part or
donor archive is changed or included in recovery. Rear-pot removal and counter
seams remain the existing authored recipes; this correction makes their runtime
decoder agree with their validated inputs.

Evidence in the recovery archive covers:

- Failing controls for extended single triangles, the exact screenshot error
  through the real native factory, and both donor-selection defects.
- Native shared/UI header checks, strict filter controls and owner-selection
  cases, with ASan/UBSan where supported by the existing runners.
- Real XML factories for both hosts; exact 342-face removal and four counter-seam
  faces added, with original display/vertex buffers unchanged.
- Corrected external-donor refinement and native vapor/metadata preservation;
  native scene metadata retains four lights and two flame placements.
- Native attachment and next-entry behavior in both hosts, seven built-in magic
  asset tests, and production MM material preparation/trace/fallback checks.
- A syntax check of the complete shared resource header with real project
  dependencies; pinned clang-format 14 and whitespace checks.

External input SHA256 identities (read-only, never packaged):

| Input | SHA256 |
| --- | --- |
| Djipi 20 Scenes Map | `0f79d188684b1459b722437999a44101df48a1aae44fbfc5674ba51f9e9ad2d4` |
| Djipi 21 Scenes Map 3DS Textures | `cc2d44fc66b4a46e22db75207d5b8a518f02514a4452f443a02f482668aac81d` |
| Native OoT archive | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |

Initial standalone header setup lacked SDL2 headers; the completed check uses
the project-pinned SDL 2.32.10, thread-pool 4.1.0 and real JSON/spdlog headers.
No full executable build, Windows CI, configured GPU rendering or runtime
acceptance is claimed. Keep this a source candidate with develop/master unmerged.

## Next decisive runtime checks

With a matching rebuilt candidate and no external scene mods, toggle Magical
Room Materials on/off with Alt Assets off/on; check colors, restoration,
pause/unpause and re-entry. Then, with the original Djipi/native prerequisites
loaded, enable Regular Granny Interior and re-enter: its room should load
without the rear-pot compatibility error, retaining the approved counter,
entry props, keeper, stock, lighting and flames. Preserve the old candidate and
the current accepted mod order until these configurations are accepted.
