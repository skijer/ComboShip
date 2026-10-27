# Cumulative debug integration, 2026-10-22

This candidate applies the reviewed revision-6 source patch to `4fff75754ef1578e586312fbf8c68e3adb99b4df` and publishes the three optional visual rebuild recipes. The accompanying JSON records the exact 87 payload paths and Git blob hashes. Two audit files accompany those payloads.

## Resulting behavior

- Shopkeeper and shelf camera framing use the native browsing flow. Purchases enter the get-item receipt and tutorial path, including the Magic Cape hide/show prompt, instead of stopping at a generic ownership message.
- MM mask transformation sounds in OoT use the native sequence clock formula. The former 3000 maximum tempo clamped authored sequences and advanced their scripts too fast; the native NTSC value is 10770. Authored animation cue timing remains intact. Normal completion preserves one-shot tails, while abort and reset paths retain cleanup.
- Gerudo form eyes receive the correct segment binding. Broader face appearance and exact replacement icon/GI remain separate acceptance items.
- Solo 2Ship and Combo MM expose the opt-in OoT-style location-card presenter. The optional native-font pack covers 61/67 English names, with complete native MM fallback for six unsupported names. The accepted Black Market wordmark is preserved.
- The shop backdrop regains its animated material; pots and glassware receive differentiated color/UV treatment. The mirror ramp uses native translucent routing with alpha 112 and retains collision, geometry and scroll behavior.
- Seasonal bindings include the supplied Meadow, Lon Lon Ranch and Zora's River materials and the Kakariko set, with grass on wall and brick trim covered by exact bindings/masks. Carried winter weather remains separate from a real Song of Storms event.

## Asset publication

All 33 new image-data parts use lossless zlib followed by ordered base64 text. Each part is at most 98,304 compressed bytes and 131,072 text bytes. Strict hashes, bounded decoding and exact roundtrips passed. The earlier 91 parts are inherited unchanged. No raw PNG, O2R, ZIP, ROM or input archive is uploaded.

The optional shop, translucent-ramp and native-title builders reproduce the reviewed three runtime archives with 59 disjoint resources. They remain optional recipes; this commit does not install or enable their runtime archives. Their readmes contain rebuild instructions. The saved visual ZIP has an older companion core-patch identity; its runtime payloads are unchanged. The current authoritative core identity is revision 6, 1,334,471 bytes, SHA256 `2a4f05a38f08056d2d8a9d866c0a83a8f295c089ca5f03c27cde3dfbe724335e`.

## Verification and remaining work

Publication checks apply all 108 source hunks with exact context, verify all existing base-file hashes, preserve modes, reject existing-path collisions, and verify optional files against their reviewed identities. Audio changed ranges are byte-stable under the workflow's clang-format 14.0.6. The existing equipment regression manifest reaches both new clock and lifecycle checks and propagates failures.

Recorded focused evidence includes 40 clock assertions for both hosts under ASan/UBSan, failure of the old clock controls, all eight native-reference sequence/PCM cue traces through the same fixed driver, and the normal/abort lifecycle cases. These are source and fixture checks. Current full-platform CI and configured gameplay/GPU acceptance remain pending.

The ice-death solid-white quad is still open; the included palette/pitch/SFX work is a bounded candidate. Wider Gerudo appearance and the exact updated icon/GI are not promoted. The fixed audio driver retains about a 1% cadence difference from native batched updates. The Biggoron casing report was resolved by the BGS Sword Unlocked checkbox, and adds no hand or casing change here.

Publication omits private input archive filenames, whole-archive fingerprints and upload/storage details from the native-title provenance and manifest. Derived-resource identities, attribution and rebuild validation remain intact; the reduced metadata rebuild reproduces the same runtime archive exactly. The separate visual delivery keeps its original private audit record.
