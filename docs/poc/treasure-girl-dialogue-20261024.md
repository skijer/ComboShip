# Treasure Girl dialogue implementation — 2026-10-24

Base milestone: `be3d5660de7f88732cf8e5b6a5868c036d35043d`.
Approved scope: short character response and optional help, reached through A / Talk to shopkeeper; native browsing and cameras retained; no forced help on entry.

## Implemented behavior

- OoT keeper-view A opens “No guessing games here. Pick the treasure you want!” without selecting merchandise. A then opens native “How does this shop work?” / “Back” choices. Help has three native pages covering displayed prices and purchase confirmation, unique dungeon-clear seals, and one-time offers versus empty-bottle refills.
- MM retains the native Hello EVENT with A Continue, followed by native FaceShopkeeper CHOICE “Talk to shopkeeper” / “Leave”. Only native TalkToShopkeeper invokes the appended optional `keeperDialogue` adapter callback. No browser, input navigation, cursor, tween or camera controller was added in MM.
- Help completion, Back or B from any conversation phase returns to the keeper view. Native page advancement remains engine-owned. Repeating talk remains optional. Left/right browsing, confirmations, purchase receipt continuation and all previous source camera/ownership fixes remain exercised.
- Dialogue has no reservation, purchase, grant, debit or save call. Native message-loss/closing, transition and death interrupts release owned conversation state while preserving foreign text; OoT dialogue teardown is covered as well.

## Red evidence

With only new native conversation assertions added, the native ASan/UBSan browser fixtures failed:

1. OoT `run_browsing_native_tests.py`: keeper A failed `sSelected == -1 && lastText == 0x8F15 && purchases == 0 && sTalking`; the old path entered shelf 0.
2. MM `run_shop_camera_mm_tests.py`: intro advancement failed `actionFunc == EnOssan_TalkToShopkeeper && currentTextId == 0x8F17 && messageState == TEXT_STATE_CHOICE`; the old path immediately returned to the keeper menu.

The first OoT test invocation omitted the existing JSON include path and failed compilation; rerunning with the required include produced the meaningful assertion failure above.

## Green evidence

All commands used `CPLUS_INCLUDE_PATH=/workspace/scratch/89f832daa772/deps/json/single_include`. Native C/C++ runners enable ASan and UBSan.

| Check | Result |
| --- | --- |
| `python tests/black_market/run_browsing_native_tests.py` | PASS OoT actual Data4/CalcAtDefault, native trig/viewpoint dispatch, both banks/all rows, native glyph/cursor GBI, mirrored navigation, purchase/receipt, ownership and new dialogue flow/interrupts. Also runs MM full integration below. |
| `python tests/black_market/run_shop_camera_mm_tests.py` | PASS actual native EnOssan/EnGirlA, CutsceneManager and Fixed1 camera integration, intro/help/back/B with retained camera and zero purchases/receipts; new dialogue interrupts plus all existing native shelf/receipt/camera ownership fixtures. |
| `python tests/black_market/native_mm_shop_adapter_tests.py` | PASS full actual native actor private/default parity, repeat/input/diagonal/inversion, all forms, stock/selection, failure/success paths and receipts. An adapter with null appended callback retains original native Talk behavior. |
| `python tests/black_market/run_text_codec_tests.py --game mm` | PASS actual MarketText, native formatter/decoder/font, choices on rows 2/3, three-line page limits, EVENT termination, native A/B glyphs, native Start/Continue/Close and repeat-talk behavior; includes intro/help/choice pages and MM Hello label distinction. |
| `python tests/black_market/run_keeper_dialogue_oot_formatter_tests.py` | PASS actual pure AutoFormatString and native pixel-width tables, 216px line bounds, three help pages, choices on rows 3/4, EVENT/A glyph controls. This dedicated new test uses exact production formatter functions/tables and an inert debug-logging boundary; it does not replace or weaken the full codec gate. |
| `native_mm_shop_adapter_tests.py --vanilla-trace` compared with `--source /tmp/native-mm-shop-en-ossan-baseline.c --vanilla-trace` | PASS byte-for-byte equality of all 37 real non-adapter native frames. Both compiled/running under sanitizers. |
| `git diff --check` | PASS. |

## Exact paths changed

Production:

- `combo/BlackMarketBrowsing.h`
- `combo/BlackMarketBrowsingNative.h`
- `combo/BlackMarketText.h`
- `mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp`
- `mm/2s2h/Enhancements/Gameplay/BlackMarketMmNativeShop.h`
- `mm/include/black_market_shop_adapter.h`
- `mm/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c`
- `soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp`

Tests:

- `tests/black_market/browsing_native_test.cpp`
- `tests/black_market/market_mm_text_codec_test.cpp`
- `tests/black_market/market_oot_text_codec_test.cpp`
- `tests/black_market/native_mm_shop_actor_test.cpp`
- `tests/black_market/native_mm_shop_support.h`
- `tests/black_market/shop_camera_mm_test.cpp`
- `tests/black_market/run_keeper_dialogue_oot_formatter_tests.py` (new)

Record: `docs/poc/treasure-girl-dialogue-20261024.md` (new).

## Limits and handoff

`python tests/black_market/run_text_codec_tests.py --game oot` remains blocked compiling the unchanged production `CustomMessageManager.cpp`: missing external `spdlog/spdlog.h`. No dependencies were fetched and no existing gate was weakened. Its native decoder test source now contains the new dialogue assertions for use when dependencies are available; full OoT decoder/metadata proof is not claimed.

No full app build, GPU frame capture, configured in-game runtime test or user acceptance was obtained. Native fixtures establish source/engine integration and font-format contracts only. Publication remains held for bridge-source integration and configured runtime acceptance. No staging, commit, push, publication or agent spawning performed by this worker. Root owns independent review and a recoverable checkpoint.
