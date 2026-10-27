# October 25 cumulative bugfix — source validation

Baseline: public `83eed9fff57d7b5dfa1f27ecf5fbe6b65e411832`, tree
`54ca0f4860ea91affc0498d8f36c8d5b03872097`. The passing prior candidate,
its branches/PR and rollback package remain unchanged.

The six exact completed input checkpoints and ZIP/patch identities are recorded
in cumulative-bugfix-inputs-20261025.json. Each bounded patch independently
restored from the public baseline. Five original full trees matched exactly;
the Domain production baseline was byte-identical and its narrow patch restored
to `33d409b8b0c24f1f806a8c5ca264761fcf8b56f1`. All input production/test
snapshots remain byte-identical after integration and pinned formatting.
The older snow/mist correction is included by the latest Fountain source.

## Local verification

- All 14 inspected source ZIPs passed CRC; six selected inputs applied with no conflicts.
- The first cumulative checkpoint independently restored from a fresh public
  baseline object database to tree `0c44c2a67e21b6d33930fc3dc826765554476f56`.
- Silver hook ASan/UBSan: all nine cases plus 48 normal/MQ mappings pass.
- Native spin style ASan/UBSan and all four OoT/MM standalone/combined actor translation units pass.
- Complete existing Ikana ASan/UBSan suite and the three new native priority,
  owner/Alt availability and OoT pause-icon runners pass.
- All five real ImGui menu suites pass, including 12 color-only appearance accents.
- Full native regional sanitizer, presentation translation-unit and material suites pass.
- Native counter/collision/talk, close OoT camera donor parity, MM shop actor,
  resource readiness (128 cases), scene, Granny attachments, shelves, effects,
  room loader and draw-exit checks pass.
- Actual preflight assets, collisions, generated sword UI, shipped spin assets,
  native title assets and location runtime fixtures pass.
- Native market source preparation produces 200 OoT and 214 MM resources.
- Full project clang-format 14.0.6 makes no production change; whitespace passes.
- All 225 pre-existing CI task objects remain verbatim; two required entries
  add the Domain hook test and all three new Ikana runners: 227 total.
- CI-policy suite passes 25 tests. The new native-coverage policy first failed
  for the three missing Ikana registrations, then passed after registration.

## Assets and independent review

All 260 encoded source files and every tracked bundled payload match the public
baseline byte-for-byte. No texture is recompressed or retransmitted. The existing
27 larger legacy parts are reused by their exact Git identities; no new part
requires upload. New-upload limits remain 96 KiB decoded / 128 KiB encoded.

Independent whole-branch review found no Critical or Minor issue and one
Important missing-CI-coverage issue. The three Ikana tests are now mandatory;
the new policy guard demonstrated the gap before the correction. No production
fix was required by review, and no assertion or gate was weakened.

The companion NEI icon/title add-on is unchanged:
`NEI_Magic_Mirror_Gerudo_Icons_POC1_20261025.o2r`, SHA256
`a7013ca9ae232732ec74dc14200fcb4469038fb1a73139932010610de0ee408e`.
Its ten members pass CRC and include base/Alt native Ikana icon keys.
Use it above other relevant icon/title packs in both game mod folders.

## Evidence limits and remaining acceptance

Two initial local setup failures were resolved without production changes:
the real ImGui directory was not yet exported and extracted SDL headers needed
their multiarch include directory. The optional Granny donor-archive utility
requires external maps/StormLib and was not run; required native Granny fixtures
pass. An initial local package-manager installation was replaced by unprivileged
fixture-header extraction. These are environment checks, not gameplay evidence.

Full Windows/Linux builds, all six hosted groups and the final gate are still
required for the final published tree before any Windows package is delivered.
Configured gameplay, arbitrary third-party model/scene graphs, stock/seal
clearance and keeper interaction feel remain untested. Existing limitations
are not expanded into acceptance.

Focused gameplay checks: both shop banks/select/cancel/purchase/receipt/continue,
counter talk/climbing abilities and Granny on/off; room colors/What's New/Alt;
both spin levels per theme; native Ikana GI/icon in combined and solo modes;
Fountain particles/full-width shimmer and normal Snowhead bridge arrival versus
a genuine owl warp; adult Domain room 1 entry/re-entry with recorded thaw/Alt
settings and the unchanged caustics pack, then the child-room control.

Keep original game archives, saves and optional scene/model pack order separate.
Do not promote or merge develop/master. Source/native verification does not
establish game acceptance.

## Full CI fixture follow-up

The first public candidate `e90532fc57c05035c6f5e43df7dc3ff4dc720687`
(tree `930affcd36b1cf10b762e9b526feb72f23bc4252`, run 40211073847)
passed source/asset/format preflight and the entrance group. The separate
Windows compile preflight 40211073846 passed. Five required groups exposed
older fixture dependencies after the native Ikana integration; this run and
any uploaded Windows artifact from it are ineligible for delivery.

The failures were reproduced locally. Ten test/runner files now supply the
current native API/pose declarations, with tripwire assertions when a mask,
sword or unrelated editor path would incorrectly select the shield. All
original non-shield behavior assertions remain. The old Solo fixture's retired
worn-shield substring was replaced with the complete actual production
availability query and native two-pass draw body. It checks strict optional
archive loading, missing/incomplete/late pairs, registered combined ownership,
deferred paths, texture bindings and matrix balance, while retaining Farore and
Triforce receipt controls. Removing the real combined-mode guard makes that
fixture fail, demonstrating the preserved safety contract.

A broader native replay also caught a title fixture that omitted the new
module-owned Arrival state. It now includes the exact production state
declaration and retains all title request/consumption/hide/cutscene assertions:
222 titled and 117 untitled scene cases, combined and standalone, pass.

All ten focused fixture repairs pass; complete Solo (19 production-boundary
suites), real-header GI/held/Combo compilation and the remaining original native
entry/lifetime/forest commands pass. The existing native-forest suite still has
15 explicit optional external-archive skips. Those optional observations remain
unproved. All 25 CI-policy tests pass and all 227 required task commands are
unchanged. Production source and every asset remain byte-identical to the
first candidate; these corrections only repair integration fixtures.

## Shared native MM renderer linkage

Source816477c5/run40213437191 passed Linux, systems, entrances, equipment and
items, but the rendering group reached a deeper full-module fixture without
the newly required native-shield availability export. That missing link was
reproduced locally. The shared renderer fixture now supplies a controlled
native-pair availability boundary and preserves every existing authored-fallback
case when the native pair is absent. With the native pair available, all four
host/owner asset combinations decline authored geometry and retain Ikana's
identity/shimmer. Pure descriptor queries still supply native fallback palette;
the test checks identity and emitted graphics rather than forbidding those reads.

The complete MM native runtime/dispatch/interpolation/held/use/action suite and
all three renderer consumers (rewards, elemental arrows and air magic) pass.
All34 remaining rendering-group tasks after that native module pass as well.
This adds one shared fixture update; production, assets, registry commands and
the final gate remain unchanged. A fresh complete exact-tree CI run is required.
