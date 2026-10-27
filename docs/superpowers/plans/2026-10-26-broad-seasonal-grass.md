# Broader seasonal grass implementation plan

**Goal:** Include the Termina parser correction and Spring-equivalent reviewed grass queries, preserving all accepted tint formulas.

**Spec:** docs/superpowers/specs/2026-10-26-broad-seasonal-grass-design.md

**Base:** 53f21d5c0d1f1bb12b6afd249c8ba60b8f2065eb. Earlier bridge, valley, mist and firefly changes remain protected.

## Preservation contract

Keep OoT's existing foliage runtime, table, masks and palettes exact. Keep MM's SeasonPalette, SeasonTintMode, indexed conversion and BuildSurfaceTexture arithmetic exact. Broader queries are additive; accepted draw treatments stay authoritative, and image fallbacks must not double-tint a shared image. Preserve originals, alpha, metadata, scene/Alt gates and queued allocation lifetimes.

## Tasks

- [x] Audit OoT selection and run the existing native/sanitized foliage suites.
- [x] Add cohort characterization covering every eligible Spring material in Autumn/Winter, native/Alt, repeat transitions, metadata/alpha/source preservation, Summer and Off. Observed599 cases pass; no runtime change was needed.
- [x] Record exact source-block/hash comparison proving formula preservation.
- [x] MM: demonstrate the exact merged floor rejection, preserve its native 0xEF packet, and prove nonempty correct seasonal variants with original command preservation.
- [x] MM: broaden reviewed ground and eligible foliage-only queries, preserving working draw treatments and adding selective existing-formula copies for missing coverage.
- [x] MM: test no double tint, shared identities, source reload/cold states, native/Alt aliases and queued lifetimes. Cache stable coverage; all four actual grass roots passed 120 steady updates without parser rescans or buffer replacements.
- [x] Run focused native/sanitized foliage suites and review the new source delta. Both reviewer findings have permanent controls observed failing before their corrections and passing afterward.

## Delivery record

The follow-up source ZIP records exact candidate/base refs, trees, ordered commit objects, cumulative/history patches and independently checked restoration results in `SOURCE_REFS.json`. It includes the earlier bridge, valley, mist and Summer changes. This is a candidate source checkpoint; the earlier separate source bundles have not been cumulatively integrated or publicly merged.

Windows and configured in-game appearance remain separate from source/native results. No public merge or texture archive rewrite is part of this change.
