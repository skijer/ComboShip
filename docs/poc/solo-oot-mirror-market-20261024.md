# Solo OoT Mirror receipt and Black Market repair — October 24, 2026

| Field | Record |
|---|---|
| Baseline | Local `bb6f927eda710812daceb73f9124e0d536701918`, tree `292685496dd311b2b1b97c0847eea537988b6c5a`, identical to public repair candidate `602e430613ce3045f782f1ec9b76bd5878879784`. |
| Candidate | `fix/solo-oot-mirror-market-20261024`, isolated from the concurrent shop-camera and bridge work. |
| Reported build | Windows PR merge `9c3e41ee793250c0a8593361369ac8d64226bac9`, built from `e60ed69e`; supplied crash at 22:43:59 local log time. |
| Scope | OoT Mirror receipt control bytes, shop effects' raw segment restoration, and OoT shop A/B text colors. |
| Preservation | Grant/save timing, inventory, solo hosted MM forest/shop, engine selection, Mirror travel/arrival, shop stock and purchases, camera/layout/collision, assets, effects geometry/palette and MM text. |
| Status | Implemented; source behavior and affected native compilation verified. Configured gameplay, full executable and user acceptance are separate. No accepted master promotion. |

## Findings

The log records forest entrance `0x618`, receipt `0x3FEB`, a reset, another `0x618` load and market entrance `0x614`. The native OoT `soh.dll` remains the host. The donor MM Lost Woods scene is intentionally available inside Solo OoT; this evidence does not show the mode switching into the MM engine. The pickup already saves ownership and the forest location before its receipt finishes, explaining the owned Mirror on reset.

The receipt's literal C++ newlines are byte `0x0A`, OoT's persistent-message control. Native text refuses to close that end type, and the item action waits for closing forever. The fix uses the formatter's `&` line separators, which become native `0x01`. The actual formatter/message/item-closing regression fails before this correction and passes afterward. See `mirror-receipt-20261024.md`.

OoT's in-text A/B glyphs inherit the current message color; they do not receive the MM renderer's automatic button tint. The two shop pages now explicitly use the native blue/adjustable-green palette and restore white around their adjacent words. Item, price and message cosmetics remain native. See `tests/black_market/oot_button_glyph_recovery.md`.

## Crash evidence and correction

The supplied dump is SHA-256 `6860038cfba1ae19f3c38bdb0b1a3c09c3b0b20fba9048e0c771deea5647f258`; the supplied log is `da8a4f818cbb8df9815a7f2c0e25b26630c4f54b228395f9f1d49bf1d8a31eee`. Original attachments remain unchanged and are not included in public source.

The dump's own timestamp is 2026-10-25 03:43:59 UTC; its uploaded filename's later timestamp is not the exception time. Exception `0xC0000005` is a read from `0x2B7EB080C50` in `soh.dll` at RVA `0x93557A`. The log's nearest exported-symbol names have large offsets and are not reliable source function names.

The matching DLL was obtained from the already downloaded `ComboShip_Windows_Repair_20261024_e60ed69e.zip`. Its SHA-256 is `e955f114ccae782072331e60766c06271a4d7f66ec73949e413408ce10e394e3`; PE timestamp `1792894558` and image size `0x8B3E000` match the dump module. Captured code at RVA `0x935540` matches this DLL. At the exception instruction, `cmp byte ptr [r8], 0x5F` tests the first byte of the `__OTR__` signature. R8/RBX contain the invalid address; RDX is segment 8.

Disassembly at the caller return RVA `0x1147FE` shows the native setup restore, white primitive color, `0x80808080` environment color, a load of CPU `gSegments[8]`, and the call to `gSPSegment`. That sequence identifies the final segment restoration in `BlackMarketInteriorEffects::Draw`. This is instruction/source matching, not PDB symbolization. The caller instructions come from the exact matching DLL; those pages are not captured in the small dump.

The resource-aware `gSPSegment` wrapper probes its pointer argument. CPU segment 8 can retain the pause icon buffer after scene/reset teardown, so using that wrapper to copy an old raw address introduces a read from retired memory. The exact historical allocation cannot be reconstructed from this small dump, but the invalid read and offending call site are established. A native ASan fixture reproduces the same read with a freed icon-like buffer in both hosts.

The correction uses the native raw `__gSPSegment` packet emitter for this restoration only. It preserves the segment value without probing its contents. The renderer's `G_MW_SEGMENT` handler stores the value without dereferencing it; later consumers remain responsible for binding their own segment as before. Resource-aware handling of the live flame scroll lists and other resources is unchanged. The old test replaced the native wrapper with a raw-emitter stub and therefore missed this fault; it now runs each host's real wrapper and signature checker.

## Verification and recovery

- Native interior effects: before fix, ASan reports heap-use-after-free in `ResourceMgr_OTRSigCheck` through `gSPSegment` at the final effect restoration. After fix, both host fixtures pass, including freed-segment inputs, preserved packet values, effects geometry/palette, allocation bounds, pause determinism and inactive cases.
- Mirror receipt: red/green source reproduction plus native close/get-item gates under ASan/UBSan. Existing both-host Mirror runtime, form and integration checks pass.
- Glyphs: red/green native primitive-color packets, standard/custom message palette, item/price/choice/EVENT preservation; existing OoT keeper formatter and both native text codecs pass under ASan/UBSan.
- Full affected source units compile against real headers: OoT `MagicMirror.cpp`, OoT `ComboBlackMarketNative.cpp`, MM `BlackMarket.cpp`. This is object compilation, not a linked/booted executable.
- Production files pass clang-format 14.0.6 and whitespace checks. New receipt and glyph regressions are registered in CI; the existing entry/decoder/effects preflight already runs the enhanced effects regression.

Recovery: apply or revert only this candidate's three production edits and associated tests/docs. No binary asset generation or asset upload is necessary. The public baseline tree and isolated commit retain all previous repairs; do not overwrite the independent camera/bridge worktrees.

Decisive runtime checks remain: acquire an unowned Mirror in Solo OoT, advance both pages and regain control; pause/reset/reload then enter/exit/re-enter the shop; inspect A/B glyph colors and adjacent words in welcome/browse pages. Reboot success alone did not establish the reset path. Accepted masters stay unchanged pending configured testing and acceptance.
