# MM equipment preview command streams — 2026-10-10

- Baseline: draft PR41 `d371fb26284b6b6e7d05ef0888fb90e09ca9fc3d`. Candidate only; runtime acceptance remains pending.
- Report: equipping the Trident selects the Divine Shield, then reopening MM equipment shows an oversized shield across the page and a black player preview. Shield selection is addressed by the adjacent equipment policy patch; this patch addresses the preview renderer.
- Source cause: `KaleidoEquip_RenderDollFB` captures only OPA commands. Player callbacks also emit translucent equipment/effects on XLU, which then execute in the main screen pass with preview limb matrices. Additionally, framebuffer clearing installs NOOP rendering; the direct `Player_DrawImpl` call expects its caller to restore player render/combine state.
- Fix: capture OPA and XLU, replay both inside the isolated preview framebuffer, terminate/skip both in the main passes, then restore the main framebuffer. Both streams receive the preview camera, viewport, lighting and the native no-fog `SETUPDL_26`. Its blender matches the preview's geometry; gameplay `SETUPDL_25` with fog geometry disabled can instead interpret opaque vertex alpha as full fog.
- Preservation: existing page-space composite, framebuffer dimensions, transition framing, per-form transforms, loaded model callbacks and Mario preview delegation.

## Verification

- Production command-graph fixture reproduces escaped XLU commands before capture repair. A second failing check reproduces NOOP rendering after the clear. Review found and a third failing check reproduced the fog-blender/geometry mismatch before selecting matched no-fog state.
- Final `run_doll_stream_tests.py`: **124 checks**, zero failures in normal and ASan/UBSan runs. Executes the extracted production renderer plus native setup display lists/wrappers; replaces only GPU encoding, allocation and the loaded skeleton boundary. Covers all five forms, opaque/translucent/both channels, missing actor/skeleton, balanced matrices, single framebuffer creation and Mario delegation.
- Existing `run_preview_transition_tests.py`: **323 checks**, zero failures with ASan/UBSan. The registered transition suite now also runs the stream fixture, so the existing canonical gate covers both.
- Actual-header `z_kaleido_equipment.c` syntax passes with 15 existing header warnings. Independent source review and independent normal/sanitizer stream runs have no unresolved findings.
- The fixture does not load ROM resources or execute an actual GPU. No linked-game visual proof is claimed. The combined candidate's broad gate is recorded with the equipment patch.

## Runtime check

With the matching executable and archives, select Trident from each shield state and reopen equipment. Confirm the explicit compatible shield stays selected, an incompatible one clears, and an unselected owned Divine Shield is not auto-selected. Confirm the entire doll and shield stay inside the preview without a black block or foreground geometry, then turn the equipment page in both directions and cycle its L subpages. Repeat in each MM form, with vanilla and Alt/custom models, and check Mario delegation separately.
