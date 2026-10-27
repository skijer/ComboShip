# Approved GI integration

The `slate-element-preview` branch includes the approved sword revisions,
elemental colors, sword particles, rotating model-local Slate rings, foreign
draw routing and shelf poses, together with the previously committed GI work.
The [preview gallery](PREVIEWS/README.md) retains the review history; the
[approval manifest](SOURCE/sword_visual_approval.json) records the exact
accepted Master, Gilded and True Master exports.

## Verification — 2026-10-04

The complete 19-suite regression gate passed:

```sh
bash scripts/diagnostics/run_combo_nei_regressions.sh
```

This includes native MM runtime, foreign presentation, GI effects, held items,
sanitizer checks and resource parity. Standalone fixtures now use the same MM
audio compatibility include and GNU C resource-pointer flag as production.
The earlier preview notes' MM held-fixture failure is resolved.

Used-effect preservation checks require the exact historical Shipwright
objects fetched by `.github/workflows/build-artifacts.yml`; those baselines
were restored without changing the comparison fixtures. Python needs NumPy;
preview generation also needs Pillow.

Serialized asset verification and focused sword proportion checks passed for
all 60 GI models. The generated archive contains 1,128 resources with valid
CRCs, including the separate MM Kokiri model. Asset collision checks report
no new collisions.

The complete production-tree clang-format 14 gate passed. GI, item, mask and
foreign-scale bridge checks also passed after the final header formatting.

A full game build and in-game visual/performance verification remain pending.
Visual approval applies to the exported mesh previews.
