# Progressive sword GI vanilla toggle implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this bounded fix inline.

**Goal:** Keep an awarded sword tier fixed while allowing its MM pickup GI to follow Alt Assets changes.

**Architecture:** Preserve the concrete sword receipt name before the cross-game grant in a sword-only cache. Refresh its cosmetic recipe using that name, rather than the original progressive placeholder. Mark sword receipts as appearance dependent even when the producer classified the original request as progression dependent. Other live receipt recipes retain their previous behavior.

**Tech Stack:** C++20, existing cross-game draw ABI, Python diagnostic runners, ASan/UBSan.

**Spec:** The bounded POC contract below, based on cor's October 9 report, uploaded seed and clip.

## Bounded POC contract

- Source baseline: `bridge/audit-merge-20261008` at `56c83a878562052ca873b7d414f7d0e1175b6325`.
- Runtime evidence: cor reports tunic GIs working and sword orientation/effect sizing accepted; the supplied 9.539-second clip shows a Din Kokiri GI persisting while the player appearance changes. Its executable commit and installed archive stack are not identified. The seed's `_review.build` is older metadata, not an executable fingerprint.
- Defect: `OOT_DrawDependency` classifies progressive requests as `1`; the MM grant latch then freezes the entire recipe, including the selected custom mesh.
- Scope: MM foreign sword pickup recipe caching only. Preserve authored resources, held weapons, native GI drawing, upright matrices, particle sizing, shop recipes, grants, and non-sword priority.
- Expected: receive with Alt on, advance donor progression, then toggle vanilla/Alt repeatedly; vanilla uses the shipped authored GI for the awarded tier and Alt uses the custom sword for that same tier.
- Status: candidate only; source checks do not prove game/GPU behavior or authorize master promotion.

## Review focus

- Warm caches and repeated appearance switches must preserve the awarded tier and shimmer.
- Temporary missing shipped resources must retry the frozen tier without borrowing a mod.
- New save slots and foreign-map generations must clear receipt identities.
- Uncollected progressive previews must remain live.
- Non-sword receipts and shop rendering must retain existing behavior.

### Task 1: Separate progression latching from sword appearance

**Files:** Modify `combo/menu/ComboForeignDrawMM.h`, `tests/sword_fallback/asset_toggle_test.cpp`, `scripts/diagnostics/run_sword_asset_toggle_tests.py`, and the existing receipt-cache fixture `tests/item_receipts/latch_test.cpp`.

**Interfaces:** Retain the existing `ComboFillForeignDrawInfoOOT(..., const char* namedItem = nullptr)` and receipt-name map. Add a sword-only name cache with the same slot/generation lifetime; no ABI change.

- [x] Extend the runner to execute production dependency classification and the actual grant latch/cache. Add tests for all seven progressive sword tiers, advance donor state after latching, and toggle Alt repeatedly. Assert concrete tier, shipped/custom resource owner, shimmer and unchanged donor mode.
- [x] Run `python scripts/diagnostics/run_sword_asset_toggle_tests.py --sanitize`; expect the post-latch vanilla transition to fail against the baseline.
- [x] Refresh sword receipts through their stored concrete name and keep sword appearance live at the latch. Leave other rendering and gameplay code unchanged. Keep the shared receipt fixture's dependency fields and optional named-query signature current, while retaining its original non-sword assertions.
- [x] Run the extended toggle gate, existing sword rendering/pose gates, and relevant cache/receipt bridge gates. Inspect results and the complete diff.
- [x] Commit the isolated candidate locally and package a forward/reverse patch with verification evidence. Publish the patch as a GitHub issue handoff; do not push a Git ref or start CI. Keep the accepted branch unchanged and leave runtime acceptance open.

## Verification and review record

- Baseline direct selection gate passed; the new actual-latch reproduction failed with `latched progressive sword kept Din's mesh after switching to vanilla` before the production change.
- ASan/UBSan selector gate passes all seven awarded sword tiers, grants starting in either asset mode, repeated vanilla/Din transitions, donor-state advancement, frozen shimmer identity, live uncollected previews, slot/generation reset, and transient missing shipped data.
- Sword fallback dispatch passes in both hosts, including owner scopes, transforms, blade/flame independence, shimmer and segment restoration.
- Sword pose gates pass with unchanged Four Sword shelf pose and actual MM DrawItem/NeiGiPresentation include order.
- Sword receipt/camera gates pass through all five MM receipt cameras; they report no new or worsened clipping. Existing Goron Kokiri/MM Kokiri shimmer clipping remains a recorded baseline defect.
- Actual MM `DrawItem.cpp` real-header syntax passes (16 compiler warnings; a full executable build is not claimed).
- Existing MM receipt suite, key-cosmetic cache suite, and 30-alias/mask/remains bridge gates pass.
- Independent code review identified the shared latch fixture's old two-argument boundary and missing shimmer field. Those seams were updated to match production.
- Ruling: freeze the cosmetic lookup name only for sword receipts. A control using the existing non-sword Magic Meter assertions failed under the first generalized refresh; the narrowed cache preserves those assertions and passes the full receipt gate. This avoids changing unrelated live receipt behavior.
- User's publication constraint: publish the ready patch without pushing commits, changing remote branch refs, or triggering a build.
- Candidate is source/fixture verified. No game/GPU test or runtime acceptance is claimed. The decisive next test is the original Debug Check Give sequence: start Alt, receive a progressive sword, then Tab vanilla/Alt during its receipt and confirm the same awarded tier changes mesh.
