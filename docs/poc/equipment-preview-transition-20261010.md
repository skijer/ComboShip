# Equipment preview page transition — 2026-10-10

| Field | Record |
|---|---|
| Baseline | Cumulative PR41 candidate on parent `142e82537c9838f56cb73458cda35d2b21098791`, including the reconstructed C-Up sweep. |
| Scope | MM equipment framebuffer composite only. The preview follows its equipment page while leaving, approaching and returning to that page. |
| Preservation | Existing 192×336 framebuffer, Link/transformation/Mario pose and asset paths, alpha, approximately 64×112 settled footprint, equipment ownership/actions, L subpage cycling and C-Up tutorials. No promotion to develop; root publishes the cumulative PR41 candidate separately. |
| Evidence | Failing production transition/render fixture; failing production texture-normalization fixture; normal and ASan/UBSan runs; production translation-unit syntax; adjacent equipment/inventory/tutorial regressions. |
| Verdict | Implemented and build/static verified. Installed-stack visual/controller acceptance remains untested. |

The MM preview previously used an identity-modelview `OVERLAY` image rectangle at fixed screen coordinates `(86,68)..(150,180)`. The page transition moves the pause camera over eight frames while retaining the departing `pageIndex` until completion. Consequently the equipment page rotated under a pinned preview, which then vanished when `pageIndex` changed. Incoming equipment pages also lacked their cached preview. OoT already uses a framebuffer quadrangle under its equipment-page modelview, including incoming/off-axis pages; its production path is unchanged.

The MM composite now uses the caller's equipment-page modelview on `POLY_OPA`. Its integer page coordinates project to the former settled rectangle within one pixel under the existing pause camera. It samples the same framebuffer, retains pause alpha and submits the cached image whenever the normal dispatcher draws the equipment page. No modelview, viewport, projection or scissor override is emitted by the composite. The existing WORK framebuffer rendering and form poses remain unchanged.

The first quadrangle exposed a second issue during review: framebuffer binding selects the GPU texture but leaves loaded texture dimensions from the preceding icon. Production triangle submission uses those dimensions to normalize UVs. The bounded correction follows OoT's dimension-seeding pattern: an aligned, zeroed 16-byte raw placeholder establishes the full 192×336 `LoadTile` metadata immediately before the framebuffer bind. Production `LoadTile` does not read pixels; the bind clears texture-dirty flags before either triangle, preventing a CPU import of the placeholder. No renderer code changes are needed.

`tests/mm_equipment_pause/run_preview_transition_tests.py` extracts the production equipment draw/cursor, both equipment-page draw dispatch blocks, page-input handler and transition update. It records GPU commands and projects the emitted vertices under the production page matrix and camera positions. It also executes the production texture-image setter, tile setter, load-tile function, framebuffer-bind handler and the production triangle UV-normalization regions against five preceding icon widths. The original composite produced 77 failing transition/ownership-of-render-state assertions. The first quadrangle then failed all five seeded icon-normalization cases before the metadata correction.

Final verification, with `/workspace/scratch/6f91fd2cf849/diagnostic-env.sh` loaded:

```sh
python3 tests/mm_equipment_pause/run_preview_transition_tests.py
python3 tests/mm_equipment_pause/run_preview_transition_tests.py --sanitize
python3 scripts/diagnostics/run_receipt_syntax_tests.py \
  mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c
python3 tests/mm_equipment_pause/run_ownership_tests.py
python3 tests/mm_pause_inventory/run_tests.py
python3 tests/pause_tutorials/run_tests.py
```

Both preview runs passed **323 checks**. Coverage includes every real Z/R departure and return frame, adjacent cached/incoming previews, unavailable framebuffer and nonadjacent-page suppression, all three L subpages, settled placement, full-frame UVs, production normalization to `0..1`, alpha and unchanged modelview/scissor/save/equipment state. Address and undefined-behavior sanitizers passed; leak detection is disabled for the host's ptrace limitation. The changed production translation unit passed real-header syntax with 15 existing header warnings. Adjacent equipment ownership passed 373 checks, the MM inventory suite passed, and all 205 C-Up checks passed.

The decisive remaining in-game check is to leave and return to MM equipment in both directions, watch the entire turn with Link and Mario/transformation forms, then cycle all L subpages and open/close C-Up before equipping. Confirm the preview stays attached to the page and retains its previous settled framing. This fixture does not claim a rendered GPU screenshot, linked-game runtime proof or user acceptance.
