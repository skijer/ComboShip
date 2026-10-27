# Combined helper/equipment candidate — 2026-10-10

Baseline: draft PR41 `d371fb26284b6b6e7d05ef0888fb90e09ca9fc3d`, tree `2994c8601669015bb7d283bb02242bf3871aa17d`. The preserved baseline and develop were not modified. The candidate combines localized helper presentation, MM equipment application and MM preview stream/state repairs.

## Local verification

- `scripts/diagnostics/run_combo_nei_regressions.sh` completed all **113 registered commands**, exit 0, with the diagnostic dependency toolchain. The equipment patch was combined before the equipment/helper stages. The earlier affected native-use, action, season rod lifecycle and Wand mode checks were then repeated on the final combined source, all exit 0; native-use also passed ASan/UBSan. This is a complete local gate plus final affected reruns, rather than a claim that the entire run used an immutable tree.
- Final helper tests: **429 checks** per normal/sanitized run, including actual MM header decoding and native input routing. Final equipment ownership: **426 checks** with ASan/UBSan. Final preview suite: **323 transition/composite checks plus 124 command stream/state checks** in normal and sanitized runs.
- The native-use driver passes **21 functional cases plus lifecycle integration** under ASan/UBSan. Its new controls reproduce same-action held identity, equipment dispatch and full Gust Jar cancellation failures before the respective fixes. Final Wand tests pass **2,224 parity observations**; season rod lifecycle and native action tests pass.
- Both changed MM owning C translation units pass actual-header GNU C17 syntax on the combined source. Both helper description units passed their real-header syntax checks. Existing compatibility/header warnings remain; no linked application build is inferred from syntax checks.
- Clang-format 14, `git diff --check`, asset verification and independent reviews pass. The preview review's fog mismatch and the equipment review's Gust Jar cancellation were resolved and given permanent failing-before/passing-after controls.
- Publishing will compare the remote candidate tree to the exact local Git tree. Fresh PR-head CI, Windows/Linux application builds and artifacts remain pending at publication; old-head build success is not a result for the new candidate.

## Acceptance still required

No actual installed archive stack, GPU rendering, controller or save/world transition was exercised. Verify the reported tool/rod/rune helpers, Byrna B selection and swings, Four/Trident/Byrna switching, C/D tunics while holding a tool, explicit Trident shields and the full MM doll/page turn with the new matching executable and archives. Repeat relevant loaded models in vanilla and Alt configurations. This candidate is not a develop/master promotion or runtime acceptance.

Details: [helper presentation](helper-dialogue-20261010.md), [equipment application](mm-equipment-application-20261010.md), [preview streams](equipment-preview-streams-20261010.md).
