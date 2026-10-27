# MM native OoT title artifact sweep, 2026-10-23

| Field | Record |
|---|---|
| Baseline | Merged develop `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`; canonical `tools/location_titles/assets/native-v3` and optional `hd-v1`. |
| Candidate | Title-only changes on `fix/black-market-level-camera-20261023`; separate from the camera work. |
| Scope | Audit all 67 English MM scene cards, the market, the original Lost Woods card, and all 49 native/HD glyphs. Repair demonstrated donor fragments, HD E tracing, and the subsequently requested market spacing. |
| Preservation | Native letter samples except explicitly identified neighbor pixels; all 49 advances, all 67 scene IDs/texts, default pair spacing, border profile, texture dimensions, ownership/override order, and native timing. Market-only spacing is the explicit user-authorized exception. |
| Evidence | Actual packed-byte previews, original donor-card context, exact baseline reproduction, pixel comparisons, red/green asset regressions, compiled presenter/resource fixtures, and deterministic optional-pack verification. |
| Verdict | Implemented and source/asset/fixture verified. Configured game/GPU rendering, vanilla/Alt pack parity, and user acceptance remain untested. No master promotion. |
| Recovery | Original assets remain in the baseline commit; the user-owned donor archive was read without modification. Restore only the title-owned paths to discard this candidate. |

## Confirmed source defects

The original curator reproduced the baseline native payload **and manifest byte-for-byte** from the supplied native OoT donor. Full donor cards with crop windows established that the following fragments belonged to adjacent letters, rather than authentic strokes of the isolated glyph:

| Glyph | Donor | Adjacent source letter | Additional removed face samples |
|---|---|---|---:|
| P | Fishing Pond, `g_pn_46` | following o | 10 |
| R | Royal Family's Tomb, `g_pn_44` | following o | 12 |
| V | Kakariko Village, `g_pn_28` | following i, including its dot | 28 |
| W | Lost Woods, `g_pn_36` | following o | 21 |
| r | Forest Temple, `g_pn_01` | following e | 6 |
| v | Ice Cavern, `g_pn_10` | following e | 6 |
| x | Treasure Box Shop, `g_pn_51` | preceding o | 7 |
| y | Back Alley, `g_pn_18` | preceding e | 12 |
| **Total** | | | **102** |

`assets/native-v3/glyph-isolation.json` records each absolute donor coordinate, original fractional face value, exact source resource hash, pre-sweep crop hash, and resulting crop hash. The curator validates those identities before removal. Every unlisted native face sample is preserved, including faint antialiasing. The existing halo algorithm then recomputes the same border profile around the corrected shapes. This is an explicit isolation addendum, not a threshold or largest-component cleanup.

The optional HD E had a separate cause: half-coverage tracing disconnected the tip of its partially covered native top bar and rendered it as an opaque dot. E now uses smooth bicubic interpolation of its actual fractional native mask. Its built-in native glyph is unchanged. The other 48 HD glyphs retain the existing native-contour method. Real i/j dots are retained.

The 102 removals change eight native glyphs; the other 41 native glyph layouts are identical to the baseline. HD changes nine glyph face/halo pairs including E; the other 40 and the original whole-word forest planes remain byte-identical. The earlier provenance limitation remains: E/N/O/&/j use native OoT NES-font resamples because the English place wordmarks do not supply them.

## Market spacing correction

The authoritative text remains **Skiejer's Black Market**. The reported visible gap was measured in its actual packed lettering. The first i/e boundary already had a one-pixel core gap; the conspicuous gap before the following dotted j was three native pixels. A shop-only `ej: -2` adjustment closes that gap to one pixel. The HD gap changes from 11 to three physical pixels, an eight-pixel reduction at 4x scale.

The market layout width changes from 182 to 180 logical pixels and is recentered within its existing 240x24 texture. Native first-e/j core extents change from `[56,64)` / `[67,71)` to `[57,65)` / `[66,70)`. HD extents change from `[224,255)` / `[266,284)` to `[228,259)` / `[262,280)`. A regression measures those actual output-pixel extents.

`market_pair_kerning` is recorded in the native manifest and read by the HD composer. Comparing against the glyph-repaired checkpoint before this spacing correction shows **all 67 MM scene resources and Lost Woods are byte-identical in both native and HD**. No global advance or scene kerning changed.

## Audit and verification

All 69 native/HD pairs and the complete 49-glyph atlas were rendered from actual resource records and visually inspected before and after the sweep. The market was inspected again after its spacing correction. Newly composed cards retain a transparent perimeter; the original Lost Woods donor retains its intentional original last-row alpha.

The original asset suite passed despite the defects. The new detached-stroke regression first failed on all nine damaged HD letters, while retaining the i/j dots. The native preservation regression first failed on all eight damaged native letters. It now compares every pixel of all 44 original place-card glyphs against the immutable earlier native-source archive, allowing only the explicit 102 removals. The market extent regression separately failed for both native and HD before its spacing correction.

Verification commands:

```sh
python tests/location_titles/test_native_assets.py
python tests/location_titles/run_tests.py
git diff --check -- tools/location_titles tests/location_titles
python tools/location_titles/preview_native_hd.py --output <preview-directory>
python tools/location_titles/native_assets.py --hd --output <optional-pack.o2r>
```

The eight asset tests cover all 69 native/HD resource inventories and dimensions, native contour agreement, smooth alpha, transparent composed frames, corrupt input rejection, both-host preparation/repair, exact native sample preservation, detached-stroke/dot behavior, and the market spacing. The C presenter/resource fixtures cover the real scene dispatch for all 67 IDs, native fades, interruption, geometry, override precedence, and MM/Combo resource ownership with native/meta/Alt availability.

Native and HD authoring were reproduced into separate directories and compared byte-for-byte. Native authoring needs the user-owned OoT donor and the unchanged `manifest.titles` list exported as its coverage JSON. HD authoring used the recorded Pillow 12.3.0, NumPy 2.5.4, and potracer 0.0.4 dependencies; ordinary builds still need only the bundled data and standard-library loader.

Final resource differences from the baseline: 58 native cards and 59 HD cards changed; respectively 11 and 10 remain exact, including Lost Woods. Both inventories still contain 67 scene cards plus market and forest. The optional archive has 69 exact resources, valid CRCs, deterministic timestamps, and identical bytes on a repeat build. It remains a separately optional higher-resolution replacement.

| Candidate identity | SHA256 |
|---|---|
| Native manifest | `8badf572966d7c3e794bee3e7eb3ae7d621315890866e1ea7bb33080566a1432` |
| Native encoded payload | `e35e8c459235a61211a40ff354621f3d30e1407fbbadb12fd5ab5cfcb2e04d39` |
| Isolation addendum | `e3d4c216988ae9f45ed722ea78b5bfc72f16586b8168c79a01545a6d7275318c` |
| HD manifest | `2fc8c9d133000b9441ce622f1a5b35d1f11e093509d635491d19db6eb1e3ce6a` |
| Optional 69-card `.o2r` | `2375870002d0c5074fef89f51ffaaf9c608ff5f4821638cb78b83ae488596aa3` |

Review artifacts: `Title-Artifact-Sweep-Before-After.png`, `Market-Title-Spacing-Before-After.png`, `All-69-Native-Titles.png`, `All-69-Smooth-HD-Titles.png`, and `Native-and-Smooth-HD.png`.

## Remaining runtime proof

These are offline source/pixel and native fixture results, not gameplay acceptance. Inspect the rebuilt ordinary assets and optional HD replacement under the user's real pack order and Alt setting. Useful controls are East Clock Town (HD E), Laundry Pool (P/r/y), Mountain Village (V and the real i dot), Music Box House (x), Stone Tower (terminal r), and the market spelling/spacing. Confirm arrival fade and re-entry in the same configurations. Existing whole-card override precedence is deliberately preserved, so an older installed override can still supersede this candidate.
