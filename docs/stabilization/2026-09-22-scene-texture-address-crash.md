# Scene-load texture address crash candidate

Baseline: published cumulative PR #8 head `116d42672fadc0fb7b68ec9116b383c4b62edd68`.
The supplied log used the earlier incomplete CI build `dae84c8`. Both builds
have the same renderer source at the failing path. This candidate retains the
corrected integration, including the MM scabbard and OoT-sky controls, by changing
only six production lines on top of the complete published candidate.

## Evidence and cause

The three latest recorded crashes are MM access violations at 04:29:04,
04:34:26 and 04:38:42. Their stack is `strncmp` -> `OtrSignatureCheck` ->
`gfx_check_image_signature` -> `gfx_set_timg_handler_rdp`. The string pointer
is respectively `0x08000000`, `0x09000000`, and `0x08000000`.

These untagged N64 segment addresses pass through `SegAddr` unchanged. SETTIMG
was probing the OTR signature before calling its existing address validator,
so the intended rejection happened too late to prevent the invalid read.
The supplied log does not identify the originating display list or prove
which individual mod emitted the address. Older crash records and missing
Zora model display lists are separate observations, not resolved by this claim.

Log SHA-256: `779a3f44f5bb7e57dc05a01f5200962f7e95ff266a6f52746964190b95489e7c`.

## Change and preservation

Validate the resolved input before the signature probe. Retain the second
validation after resource loading, since that checks the texture pixel pointer.
Use the existing module-address exception so valid low-address static textures
remain accepted. Rejected input preserves the prior texture state and leaves
normal command advancement intact.

No game logic, feature controls, archives, scene geometry, resource ownership,
weather, audio or actor behavior changes. The shared renderer serves both games.

## Verification and remaining runtime check

The production-function fixture reproduced SIGSEGV with segment 8 on the
baseline; AddressSanitizer reproduced the exact signature-check stack with
segment 9. After the fix, AddressSanitizer/UndefinedBehaviorSanitizer and a
non-PIE low-module-address run passed. Coverage includes both crash addresses,
null/low/unbound tagged addresses, valid raw/module textures, bound segment
offsets, OTR resource paths and HD texture metadata. The existing MM graphics
patch suite also passed all 13 cases. Local LeakSanitizer is unavailable under
the container's tracing environment, so only its leak pass was disabled locally.
CI runs the sanitizer fixture with its normal settings.

Run `python3 scripts/diagnostics/run_gfx_texture_address_tests.py`; add
`--low-module` for the low mapped-module variant. The CI gate runs both.

Full Windows/Linux builds and in-game acceptance are separate checks. With the
same packs and Alt Assets setting, repeat a previously failing debug warp and a
normal transition, then re-enter each destination several times. Confirm both
stability and texture appearance. This is a crash-fix candidate, not promotion
of an accepted master or proof that every historical crash is resolved.
