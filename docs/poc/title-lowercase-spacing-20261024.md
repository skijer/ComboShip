# Native title lowercase spacing POC — 2026-10-24

| Field | Record |
|---|---|
| Published baseline | `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`; its accepted runtime state is separate from these editable assets. |
| Candidate parent | Uncommitted title glyph-isolation, HD E interpolation, and Market `ej:-2` candidate. Preserved independently before this layout change. |
| Scope | Shared pair kerning: `fa:-2`; `ay/ey/gy/hy/ny/ry/ty:-3`, in native logical pixels. Existing `ar:-1` and `et:-1` stay unchanged. |
| Preservation | All 49 native face/halo planes and advances; all HD glyph planes; exact Lost Woods donor and current Market resources. No glyph generation or replacement font. |
| Evidence source | User Woodfall screenshot plus immutable native OoT `oot.o2r`, SHA-256 `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b`. |
| Configuration | Offline IA8 native card rendering and optional RGBA HD card rendering at the same native logical dimensions. Runtime configuration is untested. |
| Expected result | `fa` no longer looks detached at its stems; lowercase `y` no longer carries its donor crop's large preceding gap. |
| Status | Implemented and statically verified; native/HD regeneration reproduces byte-for-byte. Runtime/visual acceptance remains untested. No master promotion or publication. |

The supplied Woodfall title has native white-face gaps `oo=1`, `od=1`,
`df=2`, `al=1`, and `ll=1` pixels. Those pairs remain unchanged. Its `fa`
bounding boxes hide the visible five-pixel gap between the lower stems:
the f hook extends above the a. Moving the a and following letters left
two pixels leaves a three-pixel stem gap without changing the hook.

The lowercase y crop has three pixels of leading bearing. In the exact
Back Alley donor, e's white face ends at x103 and y starts at x105, leaving
one clear pixel. The composed `ey` pair leaves four. The same three-pixel
placement difference appears in the exact Hyrule Field `Hy` reference.
Adjusting pairs before y removes that crop-bearing discrepancy, with no
change to the glyph's face, descender, following spacing, or advance.

Private comparison images use the actual native bitmaps and current HD
planes. They are technical offline previews, not runtime proof.

## Packed-pixel measurements

The gaps below count clear columns between faces at coverage >=128. Native
measurements use logical pixels; HD measurements use physical pixels at
4x resolution. `fa` uses logical rows 12–16; the remaining pairs use rows
8–17. HD antialiasing can place the half-coverage edge at a fractional
native coordinate, so its measured gaps need not be exact multiples of four.

| Pair | Native before → after | HD before → after |
|---|---:|---:|
| `fa` | 5 → 3 | 20 → 12 |
| `ay` | 4 → 1 | 18 → 6 |
| `ey` | 4 → 1 | 17 → 5 |
| `gy` | 5 → 2 | 20 → 8 |
| `hy` | 5 → 2 | 22 → 10 |
| `ny` | 5 → 2 | 20 → 8 |
| `ry` | 4 → 1 | 16 → 4 |
| `ty` | 5 → 2 | 20 → 8 |

The shared table is authored once in the native curator, stored as
`pair_kerning` in the native manifest, and consumed by the HD card composer.
The Market-only `ej:-2` adjustment is combined with that table separately.

## Scope and preservation checks

Exactly these 22 native resources and the corresponding 22 HD resources
change. All other 47 native and 47 HD records remain byte-identical:

| ID | Title |
|---|---|
| `0105` | Laundry Pool |
| `0106` | Ikana Graveyard |
| `0107` | Town Shooting Gallery |
| `0108` | Honey & Darling's Shop |
| `010b` | The Mayor's Residence |
| `010e` | Curiosity Shop |
| `0112` | Lottery Shop |
| `0113` | Beneath the Graveyard |
| `0114` | Astral Observatory |
| `0117` | Woodfall |
| `011b` | Swamp Shooting Gallery |
| `011d` | Woods of Mystery |
| `011f` | Woodfall Temple |
| `0127` | Mountain Smithy |
| `012a` | Goron Graveyard |
| `0130` | Doggy Racetrack |
| `0134` | Great Bay Coast |
| `0137` | Waterfall Rapids |
| `0139` | Great Bay |
| `013d` | Great Bay Temple |
| `013e` | Fairy's Fountain |
| `0141` | Ikana Canyon |

- All 49 native glyph face planes, halo planes, advances, bounding boxes,
  and provenance match the pre-spacing candidate exactly. Canonical
  `glyph_layout` SHA-256:
  `59ac2f7ae9f6dd8ce48ba821156e5e3695e0265407219d3b4c4a85a519004dab`.
- Both encoded HD glyph parts remain byte-identical. Decoded HD stream
  SHA-256:
  `91b4af84b003df1ed2d0a07b13c5ddb2e75c01fe68b2701bc328983aa07f082e`.
- Native and HD Lost Woods and Market records remain byte-identical.
- Every changed native card retains exactly the same total white-face
  coverage. None of the eight adjusted native pairs intersects at any
  nonzero face coverage, including faint antialiasing.
- HD face planes are unchanged. The only intersecting HD samples are
  three `ry` antialias tails at coverage 1/255, merged by the existing max
  compositor. No samples at coverage >=2 intersect. Halo merging follows
  the unchanged max compositor and unchanged source halo planes.
- Fresh native and HD authoring runs reproduce every generated file
  byte-for-byte. Native/HD inventories, framing, and texture dimensions
  remain unchanged.

## Verification and recovery

`python tests/location_titles/test_native_assets.py -v` passes all 9
tests, including native glyph provenance, HD contours, detached-stroke
checks, Market placement, and all 16 native/HD lowercase pair checks.
The new spacing test was first run against the old assets and failed all
16 cases for the measured excess gaps.

`python tests/location_titles/run_tests.py` passes the compiled presenter
and resource harnesses for MM and Combo. `git diff --check` passes.

The protected input archive is unchanged. The independent pre-spacing
source snapshot and final JSON evidence are retained with the private
audit outputs. Reversing only this POC restores that snapshot's native
cards/manifests, HD manifest, and curator/composer changes while retaining
the prior glyph-isolation and Market work. Nothing was staged, committed,
published, or promoted by this workstream. The next decisive runtime check
is Woodfall and a name containing `y`, with native and optional HD titles.
