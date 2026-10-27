# MM song GI POC2 implementation plan

> **For agentic workers:** Execute this bounded revision inline using superpowers:executing-plans.

**Goal:** Refine the saved song effects according to cor's latest visual feedback.

**Architecture:** Keep the native clef display list and all item identities. Change only the shared particle policy, its existing expectations, and the offline preview labels.

**Tech stack:** C++ song meshes; Python/Mesa previews and diagnostic harnesses.

**Spec:** User feedback captured in `tools/song_bottle_20261008/README.md` under POC2.

## Constraints

- Native clef model, accepted song effects, and bottle/fairy code stay unchanged.
- Lullaby and its intro use varied musical notes; intro uses fewer particles.
- Bossa retains its bubbles and two existing ripples, adding a shorter third ripple above them.
- Elegy uses slow falling amber dust and Oath uses sparse violet glints, with shimmer in their existing icon hues. Cor approved these motifs to finish today.
- Previews explicitly label the gray guide and retain the offline/runtime distinction.

## Review focus

- Elegy dust must fall and Oath glints must twinkle, while retaining native clef and matching intrinsic shimmer.
- Bossa's complete three ripples must fit the 1,536-vertex particle arena.
- Musical note silhouettes must remain readable at GI scale and different camera angles.
- Other songs and the seven bubbles must retain their previous packed vertices.
- Preserve all saved bottle/fairy source and asset bytes.

## Task 1: refine shared effects and preview

Modify `NeiGiSongEffectPolicy.h`, the three existing song test expectations, and `render_preview.py`. Update the POC README.

- [x] Update tests for Elegy falling dust/Oath twinkling glints and observe their failure before changing policy.
- [x] Add camera-facing quarter/eighth/paired note silhouettes; replace the two Lullaby crystal profiles.
- [x] Append one shorter Bossa ripple; replace Elegy/Oath figures with dust and glints.
- [x] Run the song diagnostics and compare before/after production preview streams.
- [x] Render and inspect the revised overview and focused GIFs; save a separate POC2 checkpoint.
