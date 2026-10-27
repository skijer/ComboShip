# Shield of Ikana editor and icon follow-up POC1

Baseline: `7cd3ac8d09dfb72009c2404ef10ac8da4a62c0c1`, tree
`e6e3270a8794eb9896d481b93dbbecabfa7a9308`. Candidate branch:
`poc/ikana-editor-icon-followup-20261025`. This is an implementation and native
fixture candidate, not in-game acceptance or master promotion.

The user reported pausing worked after removing Okayu, and the extended shield
appeared after manually granting its old editor cell. That establishes neither
a broken pickup handler nor ownership of the earlier save. The production trace
shows MM's native `RI_SHIELD_MIRROR` grant already calls `ExtEquip_GiveItem(1,3)`;
the legacy grid already calls that setter too. The newer custom-item catalog
filtered only `RI_OOT_NEI_` and `RI_OOT_EXT_`, excluding Ikana's native identity.
The native Equipped Shield dropdown controls appearance; it is not an ownership
control and is deliberately unchanged.

## Changes

- The MM custom-item catalog includes `RI_SHIELD_MIRROR` explicitly. Its ownership
  check uses the extended page's canonical bit, not the equipped nibble. Explicit
  editor grants use the existing extended-equipment setter without equipping,
  replaying a pickup, or adding randomizer acquisition counts.
- The current MM equipment grid has a named Shield of Ikana ownership checkbox,
  allowing grant and removal. Both hosts' setters update only its shared shield
  bit together with the existing extended ownership bit. MM removal releases an
  equivalent native MM Mirror value, preventing the existing sync publisher from
  inferring it again; imported OoT skins remain independent. Sync's existing
  monotonic merge protocol is unchanged; this adds no remote deletion protocol.
- Pause grids, receipt staging and catalog icons share the private resource
  `textures/icon_item_custom/gItemIconShieldOfIkanaTex`. Existing old MM path
  identities are still recognized when receiving donor metadata, then redirected
  to the private identity. Ordinary OoT Mirror Shield paths are untouched.
- Each port archive gains one new stock 32x32 PNG containing the exact original
  MM Mirror Shield pixels. This supplies a meaningful native fallback without
  an addon, including Solo OoT. No existing baseline asset is recompressed.
- The existing pause title already resolves to
  `textures/item_name_custom/gShieldOfIkanaNameTex`; the addon supplies its HD
  version without changing native name lookup.

No save migration or auto-grant is introduced. Native MM GI priority and the
user-selected authored fallback requirement remain unchanged. No shop, camera,
portal, scene, GI artwork or unrelated icon resource changes are included.

## Optional addon

`NEI_Shield_of_Ikana_Icon_Path_POC1_20261025.o2r`

SHA256: `dff5e42f6de209cfd9690cfee4b0ccd97d817646eb6bcc970ef6531803e842fd`

| Keys (base and `alt/`) | Type | Dimensions | Flags | HByteScale / VPixelScale |
|---|---|---|---|---|
| `textures/icon_item_custom/gItemIconShieldOfIkanaTex` | RGBA32, 1 | 512x512 | RAW, 1 | 16 / 16 |
| `textures/item_name_custom/gShieldOfIkanaNameTex` | IA4, 7, raw RGBA payload | 1024x128 | RAW, 1 | 64 / 8 |

Exactly four entries, deterministic ZIP timestamp/permissions/order/compression.
The title uses authored Century Gothic Bold, the same font as the native names
and recovered companion: centered 96px text with an 8px native dark outline.
The icon imports the approved generated master at 512px using Lanczos and retains
its alpha and shadow. No other icon or title is included or modified.

Install with a **new source build including this change**, including rebuilt
`soh.o2r` and `2ship.o2r` for their stock private resources. Put the same optional
addon in `mods/soh` and `mods/2ship` for combined play, `mods/soh` for Solo OoT,
or `mods/2ship` for Solo MM. Both Alt off/on are supplied. Keep the existing
companion and accepted icon packs. This addon does not change old executable
icon routing: an old build can use the pre-existing title key but will ignore
the new private icon. Do not replace the ordinary Mirror Shield keys as a
workaround. The logged old Slate Equipment pack and supplied companion remain
unchanged.

Rebuild:

```bash
python3 scripts/assets/build_ikana_icon_addon.py \
  --icon /path/to/approved-ikana-master.png \
  --stock-mm /path/to/original-mm.o2r \
  --companion /path/to/NEI_Magic_Mirror_Gerudo_Icons_POC1_20261025.o2r \
  --output /path/to/NEI_Shield_of_Ikana_Icon_Path_POC1_20261025.o2r \
  --evidence /path/to/evidence
```

Add `--write-stock-sources` only when recreating the two new fallback PNGs. The
recipe verifies their exact native pixels even without that flag. All inputs
are read-only except that explicit source recreation option and output paths.

Provenance:

| Input | SHA256 |
|---|---|
| Original `mm.o2r` | `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59` |
| Stock MM icon resource | `f145ac21320f65594a2383db430c65c6570c0509bdf47be4b69581c56f607524` |
| Exact recovered companion | `a7013ca9ae232732ec74dc14200fcb4469038fb1a73139932010610de0ee408e` |
| Approved icon master | `283b51782c4ae8f15ccebaee203157445e079c108d9db12809c4db8d2fbeca74` |
| Authored font | `90cb613b492874a560c0ff18a3402b1d24fb7e846dff11295d5c4644d6c75e83` |

## Verification and runtime boundary

RED tests captured the catalog exclusion, shared-mask grant discrepancy,
competing Mirror icon selection, missing real ImGui ownership control, and MM
native-nibble resurrection after explicit removal. Their production routes now
pass. Real Dear ImGui mouse events grant and remove the actual current editor
cell; ordinary drawing does not write ownership. Both production icon getters,
MM native export, foreign receipts, CPU pause preflight and renderer owner
selection use the private path, including independent Alt states and Solo modes.

Commands used (fixture dependencies through `CPATH`, pinned `IMGUI_DIR`):

```bash
python3 tests/ikana_editor_icon/run_ownership_tests.py
python3 tests/ikana_editor_icon/run_ui_tests.py
python3 tests/ikana_editor_icon/run_native_syntax_tests.py
python3 tests/mm_editor/run_tests.py
python3 tests/oot_editor/run_tests.py
python3 tests/mm_equipment_pause/run_tests.py
python3 tests/mm_equipment_pause/run_ownership_tests.py
python3 tests/oot_equipment_pause/run_ikana_icon_tests.py
python3 tests/ikana_gi/run_tests.py --sanitize
python3 tests/ikana_gi/run_native_priority_tests.py
python3 tests/ikana_gi/run_native_availability_tests.py
python3 scripts/diagnostics/run_mm_item_receipt_tests.py
python3 scripts/diagnostics/run_mm_item_presentation_tests.py
python3 scripts/diagnostics/run_receipt_syntax_tests.py soh/soh/Enhancements/debugger/debugSaveEditor.cpp
```

`run_native_syntax_tests.py` compiles the edited functions against actual game
headers; the editor tests compile whole current MM editor/GiveItem translation
units. A direct whole extended-equipment unity compile hit its model
include dependency (`equipment/objects/ikaxe_DL/model.inc.c`) in the bare
syntax runner, so it is not reported as a successful full game build. The
optional private Fados mesh regression is skipped without that separate mesh.

The addon is byte-identical across two independent builds and verifies resource
keys, typed headers, dimensions, raw data size, scaling, deterministic metadata,
base/Alt parity and baseline stock pixels. Runtime GPU rendering, save/reload,
configured mod precedence, game switching and receipt appearance remain untested
in the actual user's game. No in-game shield claim follows from the Okayu pause
report. Next decisive check: explicitly grant/remove Ikana in the new editor,
open the extended page with its HD icon/title in Alt off/on, receive it normally,
save/reload, and verify the user's ordinary OoT Mirror mod remains distinct.
