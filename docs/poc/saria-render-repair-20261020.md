# Saria render repair POC — 2026-10-20

Parent source: `4af924940` (native camera fix atop cumulative integration).
Candidate branch: `fix/goron-transition-saria-render-20261020`.
The shared branch also contains the separately owned Goron effect-light repair;
this record covers only Saria assets, their verifier, and the TLUT uploader.
Inputs remain unchanged. No actor behavior, placement, progression or scene
source was edited by this workstream. The reported input appearance was broken;
no accepted visual baseline or native GPU run is asserted.

## Confirmed defects and correction

1. POC12 has 533 unresolved private hash commands in 149 display lists (42 unique
   missing targets). Face/head wrappers, fitted hands, ocarina wrappers and two
   private eyelid textures hash canonical paths, while those payloads exist only
   as `alt/` entries. `Archive::IndexFile` indexes literal names only; resource Alt
   selection cannot repair a hash that has no indexed name. The offline model
   verifier incorrectly invented canonical aliases for all Alt resources and
   therefore hid this defect. The candidate changes each unresolved private hash
   to its actual `alt/` resource. The verifier now uses the real literal index.
2. `Interpreter::GfxDpLoadTlut` copied both palette banks only for exactly 256
   colors. Other uploads beginning in bank 0 silently dropped bytes beyond 256.
   Native extraction metadata declares Saria eye palettes as CI8 with a
   63-by-4 RGBA16 TLUT (252 colors) and mouth palettes as CI8 with a 29-by-8 TLUT
   (232 colors). The bounded correction copies the spill into bank 1 and retains
   its source pointer. The production-method regression demonstrates both old
   failures and the corrected upload, with exact source sizes under ASan/UBSan.

The native donor display list is not locally available: its exact LOADTLUT
count/offset was not inspected. The uploader defect and native palette sizes
are established; this remains a causal match to the reported MM face garbage,
not proof that its runtime draw used those counts. Exact shipped ComboSaria.cpp
was compared with the source parent and already has the same eye 8/9, mouth A,
segment C and opaque alpha setup. No speculative actor-binding change was made.

## Artifact identity

| Artifact | SHA-256 |
| --- | --- |
| Saria_Anime_Native_NPC_POC12_Review.zip | `8b180269d331d5c642bd9ee130656713529e26f1a60e202d001a25ea810034e1` |
| Saria_Anime_Native_NPC_POC12.o2r | `94a6235b27c2329feac19b5297aa517a67f425e667edf918f723718e4b6e070f` |
| Saria_Fairy_Ocarina_HD_POC11_Overlay.o2r | `49cb2da9246caea8d0871bda713a901f3d034748e72628d5ff53f7eaf2570143` |
| Saria_Anime_Native_NPC_POC12_LiteralHash_POC1.o2r | `bbcf3551ab4a9eb5184f28d7403795dbab2240d7a62f9d6f1ff459ac63e92562` |
| Saria_Fairy_Ocarina_HD_POC11_Overlay_LiteralHash_POC1.o2r | `54762875b548fd972eff0cf67855d897479c1b35c300a28ce4a0bbac1cd225e4` |

The Anime POC12 candidate has exactly the same 351 resource names. Only 533
8-byte hash payload fields in 149 display lists change; the other 202 resources
are byte-identical. Every non-hash byte, including display-list commands,
geometry, vertex buffers, textures/UVs/material state, skeletons, skin palette,
animations, face states and manifests, is preserved. All blink/expression
wrappers remain present. The optional HD overlay preserves its four archive
entries and texture pixels; only four hash payloads in its two prop display lists
change. The original POC12 already contains its HD prop texture; this corrected
overlay is optional for users retaining that separate override.

Anime POC11 and Render POC11 repairs were generated only as intermediate controls,
with the same hash-only preservation checks. They are not additional requested
deliverables or accepted replacements.

## Verification

| Check | Result |
| --- | --- |
| Real POC12 literal dependency regression, before repair | Expected FAIL: 533 unresolved dependencies |
| Repaired full POC12, POC11 Anime and POC11 Render references | PASS; literal private names plus declared native references |
| Repaired POC12 with corrected overlay mounted last | PASS |
| Resource set and non-hash byte preservation | PASS |
| Deterministic regeneration of all four candidate archives | PASS; identical SHA-256 |
| Python model suite | PASS: 121 tests, 16 optional missing-source skips |
| Production TLUT method before fix | Expected FAIL: 252/232-color upper bank and crossing upload |
| Production TLUT method after fix, ASan/UBSan | PASS: 252/232, full 256, both CI4 banks, crossing offset, end clamp and raw fallback |
| Existing autumn scene foliage/renderer regression suite | PASS, including real renderer/material and CI4 paths |
| Native MM ComboSaria object compilation | PASS; existing macro/deprecation warnings |
| Whitespace diff check | PASS |
| Linked executable, GPU runtime and user acceptance | Untested |

Initial native object compilation lacked the nlohmann include directory; it was
rerun successfully with the existing dependency headers. The TLUT method is real
production C++ compiled in an isolated sanitized fixture; no full renderer or
whole-executable build is claimed. Tests referencing unavailable source/donor
archives remain explicitly skipped; the original full visual verifier was not
run without those inputs. A declared native reference name establishes the
binding contract, not the presence/quality of a user's mounted donor payload.

Reproduce the focused checks from the source root:

```sh
python -B scripts/diagnostics/run_saria_palette_tests.py --baseline-ref 4af924940 # expected failure
python -B scripts/diagnostics/run_saria_palette_tests.py
python -B -m unittest discover -s scripts/npc_models -p 'test_*.py'
SARIA_PACK=/path/to/Saria_Anime_Native_NPC_POC12_LiteralHash_POC1.o2r \
  python -B -m unittest discover -s scripts/npc_models -p test_saria_resource_hashes.py
python -B scripts/npc_models/repair_saria_resource_hashes.py \
  /path/to/Saria_Anime_Native_NPC_POC12.o2r \
  /path/to/Saria_Anime_Native_NPC_POC12_LiteralHash_POC1.o2r
python -B scripts/npc_models/repair_saria_resource_hashes.py \
  /path/to/Saria_Fairy_Ocarina_HD_POC11_Overlay.o2r \
  /path/to/Saria_Fairy_Ocarina_HD_POC11_Overlay_LiteralHash_POC1.o2r \
  --reference /path/to/Saria_Anime_Native_NPC_POC12.o2r
```

## Configuration and smallest decisive runtime check

| Launch mode | Model/optional overlay location | Toggle |
| --- | --- | --- |
| Combined ComboShip, OoT and placed Saria while visiting MM | `mods/soh/` | OoT Alt Assets enabled |
| MM-only ComboShip/2Ship launch | `mods/2ship/` | MM Alt Assets enabled |

Use one selected full Saria style. Replace the old POC12 pack with the corrected
candidate; do not retain a competing old copy in the same mod directory. If
using the separate HD overlay, mount the corrected overlay after the full model
pack. The original overlay also contains unresolved hashes and must not shadow
the repaired props. Keep the existing native companion archives required by the
launch mode. Alt off remains the native control.

With a build containing the TLUT fix, compare native/Alt off and repaired
Anime/Alt on under the same placement, scene/setup, actor parameters and save.
In OoT check native Saria and the Meadow playing route for head, both hands and
ocarina; check open/half/closed eyes and the relevant animation. In MM check the
same placed Saria's native face and repaired Anime head for waiting, standing,
playing and seated routes, and confirm all blink heads. Check a nearby actor
with CI4 textures for palette contamination. Keep interaction and one-time song
progression, re-entry and existing placements as regression controls.

Verdict: implemented and static/compiled-fixture verified POC. Runtime appearance,
all configured routes and user acceptance remain unproven. No sacred/master
promotion. Recovery is the untouched input archives plus the isolated source
change at the recorded parent; discard this candidate independently if runtime
checks reveal a regression.
