# Treasure Girl Dialogue Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development with explicit path ownership and a separate review. User already selected parallel workflow and approved the dialogue integration.

**Goal:** Give Treasure Girl an explicit A talk action, short character dialogue and optional shop help in both hosts.

**Architecture:** Share the content/presentation policy where useful, retain host-native codecs and message state. MM’s actual EnOssan actions retain navigation and camera ownership; a private dialogue callback handles only its optional conversation choice. OoT uses its existing keeper-view phase path.

**Tech Stack:** C/C++ existing shop adapter and message codecs; native actor/camera fixtures and native decoder tests.

**Spec:** `docs/superpowers/specs/2026-10-24-treasure-girl-dialogue-design.md`.

## Global Constraints

- Preserve all camera, stock, receipt, save, title, shimmer and spin fixes in source milestone be3d5660de7f88732cf8e5b6a5868c036d35043d.
- No forced tutorial on entry or on repeat visits; no purchase, grant, debit or save during dialogue.
- Keep MM native EnOssan action/cursor/camera ownership and ordinary shop/standalone behavior. Preserve OoT Data4.
- Keep native codecs, choice arrows, line/page bounds and message/camera cleanup on interruption.
- No public publication or hosted builds; bridge handoff and configured runtime acceptance remain open.

## Review Focus

Keeper A while facing her; pages/choices in both native fonts; Back/B from intro/help; interrupted dialogue/foreign camera/teardown; no transaction effects and ordinary native actor parity.

## Task 1: Both-host conversation integration

Files: a focused shared `combo/BlackMarketKeeperDialogue.h` if useful; `combo/BlackMarketText.h`, `combo/BlackMarketBrowsing.h`, `combo/BlackMarketBrowsingNative.h`; MM `BlackMarket.cpp`, `BlackMarketMmNativeShop.h`, `black_market_shop_adapter.h`, native EnOssan C; OoT `ComboBlackMarketNative.cpp` as required. Native dialogue, browser and text-codec tests.

- [x] Add a meaningful failing test: keeper A currently browses in OoT / native MM has only generic Talk EVENT, no optional help. Assert approved native dialogue flow and zero transaction effects.
- [x] Implement the smallest host-specific dialogue adaptation. Use a private MM callback only if required for the native Talk CHOICE; do not move input/browsing/camera logic into it.
- [x] Verify keeper A and L/R browsing, Help/Back/B, native page/choice decoding, camera retention, interruption and receipt continuation. Ordinary native C behavior must remain unchanged.
- [x] Report exact files, red/green evidence, full-build/runtime limits. Root handles staging/checkpoint.

## Task 2: Review and recovery

- [x] Independent review of the exact dialogue diff for spec and source quality; address concrete findings.
- [x] Record native proof and limits, save an independently restorable checkpoint, retain the pre-dialogue source milestone.

## Completion ledger

Task1 complete: source/native ASan/UBSan both-host browser, actual MM message decoder and native lifecycle, OoT pure native formatter/widths, null-callback fallback and original37-frame trace parity pass. Full OoT decoder remains environment-blocked by missing spdlog. Task2 independent review passes spec and quality with no open findings. Root preserves the verified pre-dialogue be3d5660 source milestone and adds this change to the final independently restored checkpoint. Full build/GPU/bridge-source/load-order/user-acceptance gates remain open; no publication.
