# Optional Black Market room materials implementation plan

> **For agentic workers:** Use the subagent-driven-development workflow with explicit path ownership. Preserve existing authorization and do not ask for repeated approvals.

**Goal:** An optional original magical room appearance with independent colors and compressed source assets.
**Architecture:** Generate private room display-list variants from the final per-host prepared graph, referencing unchanged current vertices. Runtime chooses the variant and reads four host-specific colors. Original Granny mode has precedence.
**Tech Stack:** Python source preparation, C++20 native graphics and existing ImGui/cosmetic controls.
**Spec:** `docs/poc/black-market-room-magic-20261025.md`.

## Global Constraints

- Baseline `2d4e08a72eb8f9ddbc8d87ba871b05deefdf2add`; original camera/geometry/collision/shelf texture content must remain exact.
- Checkbox `gCombo.BlackMarket.MagicalRoomMaterials`, label `Magical Room Materials`, default false.
- Four cosmetic keys `World.BlackMarketRoomViolet`, `World.BlackMarketRoomTeal`, `World.BlackMarketRoomRose`, `World.BlackMarketRoomLavender`, defaults RGB (165,148,230), (135,194,210), (204,170,215), (175,160,216). Alpha stays 255.
- Source variants live under `custom/skiejer_market/room_magic_v1/`; room root suffix `room`, material suffix `geometry/mat_mahouya_room_0DL_006548_GranmaShopNN` (00..27), texture suffix `texture/arcane_stone`.
- On MM, clone the final `mm_shop_presentation_v1/room` graph, preserving those vertex paths; on OoT clone the current `room` graph.
- Default/static resources and shelves are untouched. Only private variant materials scroll with the original per-index directions.
- Source asset parts remain at most 128 KiB and verify hashes before decoding. No publication/CI or develop/master merge; recoverable source archive required.

## Review Focus

- Toggle off restores the exact static room without stale pointers or repeated room draws.
- Cosmetic reset/rainbow updates affect only room palette, never shelves.
- Active Granny mode, non-market scenes and absent variant resources retain native rendering.
- Scene transitions/resource teardown cannot retain invalid room or display-list pointers.
- Final MM prop relocation, current UVs and all unrelated resources remain exact.

### Task 1: compressed source texture and generated material variants

Owned by root: `tools/black_market/room_magic.py`, `tools/black_market/assets/room_magic/*`, `scripts/assets/prepare_native_market.py`, `tests/black_market/test_room_magic_assets.py`.

- [x] Write failing behavior tests for private variant generation, baseline-byte preservation and malformed compressed input rejection.
- [x] Preserve source identity; reuse byte-identical original pixels in the existing standard-library zlib/base64 parts and hash manifests, with a separate room resource path.
- [x] Generate private variant display-list graphs after final host preparation. Preserve all geometry/UV/triangles and renderer state except explicit material texture/scroll/primitive colors. Preserve original M/L/alpha.
- [x] Test exact asset reconstruction, closed references and native/MM geometry invariants; capture evidence.

### Task 2: checkbox, cosmetic colors and dual-host runtime selection

Owned by implementer: `combo/BlackMarketRoomMagic*.h`, `combo/gui/BlackMarketAppearance.h`, the two existing Black Market host controllers, the two host cosmetic editor files, and new focused native tests. Do not edit asset preparation or root-owned documentation.

- [x] Read the spec and global constraints. Build tests from actual production behavior for toggle restoration, independent four-color palette, Granny precedence, unavailable-resource fallback, transitions and teardown.
- [x] Implement minimal dual-host private room selection; avoid mutation of shared cached static room data. Honor host resource ownership and normal/Alt paths. Runtime color changes must be visible without re-entering.
- [x] Register four independent built-in World cosmetic controls in each host; follow each host's actual Changed/Color/Value conventions and keep shelf controls untouched.
- [x] Add checkbox in shared Black Market appearance UI with concise tooltip and state save; correct old tooltip that calls the static default magical.
- [x] Run native meaningful tests and compile changed host translation units with existing local compile tooling; commit only owned paths.

### Task 3: review, recovery and handoff

- [x] Independent review of combined bounded changes and evidence; address material findings.
- [x] Run affected existing preparation/native tests, verify protected files and resources.
- [x] Commit source, create self-contained recovery patch/archive with compressed asset parts, refs/hashes and restoration proof; save durably.
- [x] Report menu locations, checks and configured runtime limitation; maintain source publication hold.
