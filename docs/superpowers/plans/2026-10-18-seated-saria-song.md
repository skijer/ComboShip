# Seated Saria song gift implementation plan

> **For agentic workers:** Use superpowers:executing-plans inline, with a final independent review.

**Goal:** Add seated Saria in Termina Field who always grants the existing MM Saria's Song once per save.

**Architecture:** Extend the existing OoT resource snapshot with pose 3, retaining private model blinking. Keep the Lost Woods waiting NPC. A separate reserved reward placement queues the existing MM custom item presentation after dialogue closes; an appended, serialized MM save flag records actual delivery.

**Tech Stack:** C/C++, native game actors, GameInteractor item events, existing o2r resources, Python diagnostics.

**Spec:** User selected a fixed Saria's Song reward, triggered by talking. Preserve the existing monkey and Lost Woods Saria. No accepted scene/model archive changes or master promotion.

## Constraints and review focus

- Original and combined game guards remain intact; reward only in combined MM Termina Field.
- Preserve blinking in native and both custom models, including seated pose.
- Claim only when the queued item actually arrives; no ownership-based claim test or repeat grants after hint use, room re-entry, reload or cycle reset.
- Queued work must not retain an NPC pointer or grant into a different save.
- Old saves default to unclaimed; normal gameplay and unrelated randomizer pools remain intact.
- Automatic position is provisional and floor checked; authored reward placements suppress it.

## Task 1: Seated resources

- [x] Add a failing production snapshot test that pose 3 selects `gSariaSitting3Anim` and preserves three custom blink heads.
- [x] Extend `PlacedSariaResources.h` with pose 3, retaining fallback for unsupported pose 2.
- [x] Check the native seated clip and both existing fitted clips against the archive contract.

## Task 2: Fixed gift and placement

- [x] Add failing lifecycle and real JSON tests for talk/close, deferred delivery, duplicate suppression, file/mode guards and old saves.
- [x] Append `comboSariaSongClaimed` to MM `NeiSaveData` and its JSON conversion, without syncing the flag to OoT.
- [x] Add seated scenery params `0x7F33` and separate seated reward params `0x7E33`; retain existing standing params.
- [x] Spawn the reward NPC once in combined Termina Field; authored reward NPC wins. Normalize the seated cutscene root to ground seating and use seated focus/collider height.
- [x] Queue a pointer-free `GIEventGiveItem` after teaching dialogue closes. Use `Rando::GiveItem(RI_SONG_SARIA)`, existing receipt and draw; persist the claim with the reward. Enable existing hint handling when taught even if its shuffle option is off.

## Task 3: Verification and recovery

- [x] Run focused production tests, affected real-header compilation and read-only real-archive verification.
- [x] Independently review the bounded diff and fix material findings.
- [x] Update the actor handoff, commit isolated source, build a deterministic recovery archive and verify independent restore before saving it.

## Rulings / evidence

- 2026-10-18: Fixed gift chosen explicitly in user input; it does not add or move a randomized seed check.
- Native seated loop and both private fitted clips exist. Native clip has 24 frames; its first root is `(0,7903,-350)`. Use a constant 7003 model-unit Y subtraction (scale 0.01) so the seated hips start at 9 world units above the authored floor, retaining the loop's breathing motion.
- Prior candidate `3f14ae38` is clean and independently recoverable. Work stays on `poc/forest-actors-20261018`.
- Focused suite: eight PASS families, including actual JSON serialization. Native compilation: eight affected object configurations PASS; command/source hashes retained. Native archive check confirms floor Y=-77 and all private seated roots/blink wrappers.
- Independent review: no blocking findings. Live native/private seated fit, placement, receipt, reload and Song of Time remain runtime acceptance work.
- Standalone hint compilation required explicit JSON and narrower Context/Config includes instead of the umbrella header; no runtime behavior changed by that include cleanup. Diagnostic dependencies include a real `stb_image.h` (the in-tree `torch/lib/n64graphics` copy suffices for object checks).
