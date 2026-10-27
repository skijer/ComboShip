# MM runtime feedback candidate — September 29

Baseline: PR 24 head `1d542f5318a5e5ca0625c1f4313988f1f87b1456`; tested merge `de7c8d6f65c20e7edb9966c71e462be4a19139ab`. Both have tree `8a3dccd531d38cee6d101a71d0b9a567af563219`. The original checkout, supplied log/clip and accepted source archives are unchanged.

| Report | Diagnosis and bounded correction | Evidence |
| --- | --- | --- |
| Vanilla ice over the custom Ice Rod | Native update still spawned visible EnIce clumps. Carry accepted OoT zero-scale setting; retain allocation, RNG, lifetime, freeze hit and collider behavior. | Production update test failed before correction, passes afterward; native rod and snowflake suites pass. |
| Mitts disappear at wall entry | Native climb setup called the newly connected custom stow path. Retain active held Mitts only for the no-cutscene climb continuation. | Real climb/put-away path regression fails before and passes after; drain, no magic, damage, re-equip, explicit and cutscene stow covered. |
| Leaf/Shovel black bars | OoT input-disabled alias is MM camera/HUD suppression bit 0x20. Use custom tool activity in native input gating and stop writing/clearing that camera bit. | Native camera/input and actual tool lifecycle regressions fail before and pass after; actual suppression flags survive interruption. |
| Four-square foreign scale icon | Direct SETTIMG texture paths did not resolve the existing `__OTR__@oot:` owner marker, although display lists did. Add scoped owner lookup to both texture commands. | Actual handler regression fails before and passes after; native/custom formats, ordinary texture isolation, invalid owners and raw/segment address checks pass. |
| Golden Scale before the expected Silver | The log already records native MM Swim sharing a scale at 12:49:55, followed by OoT Item Give 0x53 (Silver). The later chest records 0x54 (Golden). | Production progression test gives Silver then Gold; native sharing path accounts for the prior upgrade. No progression patch. |
| Scale retained after unsaved reload | Cross-game delivery eagerly saves target and source obtained state. Reopening an already obtained check does not deliver/broadcast again; the ephemeral presentation latch is gone, explaining generic text. | Production MM source queue/save/load plus progressive resolver exercised. Target save service is a fixture boundary; crash atomicity is not established. No save-semantics patch. |
| Missing native Ice/Light arrow iterations | Snowflake pack appears only in `mods/soh` in supplied log. Final Light archive uses OoT `s1Tex/s2Tex`; MM consumes `gLightArrowTex/gLightArrowMaskTex`. | Three-resource asset-only remap preserves every source byte; actual MM importer and native snowflake actor tests pass. |

Din's Fire casting behavior remains unchanged as requested. All accepted rod artwork, remaining models, animation poses, PAK/voice, performance, weather and other-engine behavior are outside these corrections.

## Arrow companion

Deliverable: `MM_Ice_Light_Arrow_Parity_POC1.zip`, containing O2R, README and resource manifest. Place its O2R in `mods/2ship`, enable Alt Assets, move it above MM Reloaded/other effects in MM's Mod Menu, save the order and restart.

O2R SHA-256: `5e1751402f8f80b6a250734180031ef6a8e20e165524554bec856394612f7d3f`.

The exact source hashes and reproducible mapping are in `tests/mm_nei/build_arrow_parity_pack.py`: POC2 snowflake from `Henriko_Ice_Arrow_Snowflake_POC2.zip`, and final Light pair from `zzz_Medallion_Magic_POC1_HD.o2r`. No artwork was generated or altered. Original packs remain installed.

## Verification and limits

- Baseline native MM suite and the cumulative `run_combo_nei_regressions.sh` suite passed. New focused probes are wired into that cumulative gate.
- Full modified native MM player unity translation unit compiled to an object with production headers/C17. Existing port/header warnings remain; no full local platform link is claimed.
- Texture address/segment tests, low-address module variant, ASan/UBSan texture and scale lifecycle probes passed. Local LeakSanitizer cannot inspect this host's traced `/proc` tasks, so leak detection was disabled while address/undefined behavior checks remained enabled.
- Existing icon/Cane presentation, shared-item integration, cross-grant boundaries and asset-collision checks passed. Asset scan retains 1,733 shared paths and 181 known differing-content collisions, with no new ones.
- Independent read-only review found no publication blocker. Live appearance, archive precedence, targeting feel, multiplayer and crash-interrupted save atomicity remain outside component proof.

Candidate only: no merge or master promotion. Runtime retest: Ice Rod flight; native Ice/Light charge, release and impact with Alt on/off; Mitts entering/climbing/releasing a wall with and without magic; normal Leaf/Shovel use and genuine cutscene interruption; a new foreign pickup's icon. Install both matching game binaries and port archives from the candidate build together.
