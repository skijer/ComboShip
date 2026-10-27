# Native title lowercase spacing implementation plan

> **For agentic workers:** Use superpowers:executing-plans for this bounded task. Do not commit, stage, or publish; the coordinating worker owns integration.

**Goal:** Correct the approved `fa` and seven lowercase pairs before `y` while retaining every source glyph and both protected wordmarks.

**Architecture:** Store the shared native logical pair adjustments in the native manifest. The native authoring compositor and optional HD compositor use the same table. Regenerate native cards and the HD manifest deterministically; the HD glyph stream must remain byte-identical.

**Tech stack:** Existing Python asset tools, Pillow/potracer authoring tools, unittest.

**Spec:** Approved bitmap proposal in `docs/poc/title-lowercase-spacing-20261024.md`; coordinating worker approved `fa:-2`, and `ay/ey/gy/hy/ny/ry/ty:-3` on 2026-10-24.

## Global constraints

- Retain all 49 native glyph face/halo planes and advances.
- Preserve exact native Lost Woods and current Market resources.
- Preserve prior glyph isolation, HD E interpolation, and Market `ej:-2` fixes.
- Change exactly the 22 MM cards containing the approved pairs; no runtime acceptance or promotion is implied.

## Review focus

- Measure packed white faces, excluding the black halo.
- Measure the lower `fa` stems on shared rows; its upper hook overlaps the `a` horizontally without colliding.
- Check every approved `*y` pair in native and HD resource bytes.
- Pin unchanged source glyphs, HD glyph stream, and protected cards.
- Reproduce generated files and compare byte-for-byte.

### Task 1: Shared layout correction

**Files:** `tools/location_titles/curate_native_titles.py`, `tools/location_titles/hd_assets.py`, native/HD manifests and encoded assets, `tests/location_titles/test_native_assets.py`.

**Interface:** Native manifest adds `pair_kerning: dict[str, int]`; `hd_assets._card(text, width, font, extra_kerning)` consumes the complete table supplied by `resources()`.

- [x] Add packed-pixel tests for `fa` and `ay/ey/gy/hy/ny/ry/ty`, plus protected glyph/card identity checks in the independent proof.
- [x] Run the new tests; all 16 native/HD spacing cases failed against the previous assets for the expected excess gaps.
- [x] Add the approved shared pair table, retaining `ar:-1` and `et:-1`; combine the Market-specific `ej:-2` entry.
- [x] Regenerate native and HD assets with the unchanged immutable donor archive.
- [x] Run the complete title test file (9 pass), compiled presenter/resource harnesses (pass), and deterministic reproduction checks (byte-identical).
- [x] Record the 22 affected cards, measured gaps, preservation hashes, and untested runtime status in the POC document.

Task 1 complete: fresh title asset tests and presenter/resource harnesses pass;
all 22 intended resource pairs change and all other 47 pairs are identical.
The coordinating worker explicitly prohibited staging, commits, publication,
and further delegation, so integration/review remains with that worker.
