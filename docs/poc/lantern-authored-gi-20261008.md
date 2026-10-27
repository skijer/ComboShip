# Authored lantern GI recovery POC

User request: restore the existing authored Lantern get-item model and trace its draw dependencies. Preserve its geometry, textures, glass, framing, held behavior, gameplay and unrelated replacement priorities.

Baseline: cumulative PR #40 head `d4e6c36d91801aee4707ad5649deb2d90fd80e55`; this is a source candidate, not a runtime-accepted master. Isolated branch: `poc/lantern-authored-gi-20261008`.

Trace: native OoT `GetItemEntry_Draw` → `NeiGi_DrawImpl`; native/imported MM Lantern → `MM_DescribeNeiGi` → `OOT_GetNeiGiDrawInfoForAssets` → `NeiGi_FillCrossGameInfo`. Both paths consult `HasLegacyGiMod`, which treats a generic Poe actor lantern replacement as an intentional replacement for the Lantern item and suppresses its complete authored model. MM can then choose that old Poe model via `GetSelectedOwnerGi` or its legacy `DrawOotNeiLantern` callback.

Hypothesis: remove only the generic Poe actor lantern from the legacy GI replacement table. Complete authored metal/glass resources should then win in both hosts. Intentional overrides at the authored lantern paths should retain priority; missing authored passes should retain the existing fallback.

Dependencies: `objects/nei_gi_redesign/lantern/gi_dl`, `gi_xlu_dl`, `scale_mtx`, `mesh_opa_vtx`, `mesh_xlu_vtx`, and brass/polished brass/dark bronze/roof patina/wick textures live in `soh/assets/custom`. `GenerateSohOtr` packs this tree into `soh.o2r`; MM routes these display lists and their dependencies through the OoT owner. No duplicate MM model is required.

Change: removed only the Lantern's generic Poe actor row from `HasLegacyGiMod`. Dedicated authored GI replacements and all other legacy replacement rules remain active. No asset, save, item-grant, gameplay, scale or held-renderer files changed.

Verification: the new regression fails against the baseline's suppression rule, then passes after the correction. `scripts/diagnostics/run_nei_gi_tests.py --combo --held` passes, exercising 32 generic-mod/Alt/effect combinations and 12 actual MM receipt configurations, including either missing authored pass. Existing authored-path overrides, shared ownership, shop fitting and held lantern checks pass. `tests/nei_lantern_grip/run_tests.py`, `tools/nei_gi/verify_assets.py`, asset collision, clang-format 14 and whitespace checks pass. XML reference audit resolves all eight dependencies; all ten Lantern resource files are byte-identical to baseline. External nlohmann-json headers are required by the diagnostics. The native C probes retain the baseline's compiler warnings.

The same renderer/held suite also passes under AddressSanitizer and UndefinedBehaviorSanitizer; the executor disables leak detection. The actual native left-grip fixture passes both ages, LODs and native/Alt/PAK selections without changing player state.

Independent read-only review of implementation `a6f6de42` found no critical, important or minor issues. It independently checked nested MM/OoT resource ownership, authored-path override priority, absent-pass fallback, resource bytes and sanitizer evidence. The final follow-up changes documentation only.

Verdict: implemented and compiler/static verified; full linked build and configuration-specific appearance remain untested. Runtime test: obtain the Lantern in MM with the actual pack order, Alt Assets ON → OFF → ON, then check one shop and the held lantern. Confirm the brass cage and transparent glass are the authored model. Use the matching executable/DLLs and `soh.o2r`/`2ship.o2r` build package; this source patch needs no additional lantern asset pack. No master promotion is implied.

Recovery: preserve baseline; deliver an isolated source patch and this POC record. Implementation commit: `a6f6de42e13eab12c8d42d9c99d238d0b8cab700`. Keep this candidate separate until integration and runtime acceptance.
