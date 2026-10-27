# MM music-pack stress crash

The September 21 stress-test log contains two identical access violations and a separate C++ archive exception. The logged build is `a77259f6acde6689ca419301e5d0e540c23f481a`, whose tree matches the streamed-ID repair at `cc1346ce`. It predates the combined weather/item candidate, so this evidence does not implicate the added rain or item colors.

## Confirmed repeated failure

The exact Windows artifact from workflow `35647288150` was inspected, rather than relying on the crash handler's nearest exported symbol. At `2ship.dll + 0xCC9363`, `ogg_page_serialno` reads the serial-number bytes through a null Ogg page header. Its caller at `+0x45BBF8` is MM's `GetOggType`, immediately after an unchecked `ogg_sync_pageout` failure. The log does not identify which sample supplied the input.

The classifier previously ignored the page and packet parser results, advertised 4096 input bytes even for short files, and compared signatures without checking packet length. Malformed, empty, truncated and checksum-invalid inputs reproduce the null-header crash in the actual production functions linked with libogg/libvorbis.

## Bounded repair

- Classify only a successfully parsed page and packet, using the actual input length and bounded signature comparisons. Handle zero-size callback reads without dividing by zero.
- Check Vorbis open, dimensions, read results and decoded length. Own temporary allocations until decoding finishes; rejected samples remain unpublished. Log the resource path on importer failure instead of throwing from the detached decoder thread.
- Keep the already-cleared streamed playback buffer silent while a sample is pending or rejected. This also prevents selecting a rejected sample from causing a later null-pointer access.
- Check Opus decoder open and seek results and stop on any nonpositive read. Review found these related unchecked failure paths; they are not claimed to be the logged crash address.

Other codecs, archive locking and decoder thread scheduling are outside this patch.

## Follow-up: archive corruption after the Ogg repair

The last session in `Fleet of Harkinian(2).log` starts at 18:05:57 with PR merge commit `2c1b5733868d1dc3ad4cc992850d79f437d3c2ec`. Its tree matches head `1071265310da03b33475a4fd0f8c32db06d59249`, so this is a crash with the Ogg repair present. The Windows artifact from workflow `35662374455` was inspected (archive SHA-256 `57ba5618de202c83635f06c06a0d2c0bd79ee59433b35ea6c1c485cee03f37c2`).

At 18:06:01 the access violation is in `_zip_hash_lookup`, `libultraship.dll + 0x1D6581`, called by `O2rArchive::LoadFile`. The MM caller at `2ship.dll + 0x1271EB` is the custom-cosmetics scan loading `CosmeticEntries`. The filename hash-table pointer in `R10` is `0x7FFA63E1D660`, which resolves to `2ship.dll + 0x12AD660`. That address contains the three-command Goron tunic color display list, not a ZIP hash table. Interpreting its `PipeSync` opcode as a bucket-array pointer produces the logged invalid `RAX = 0xE7000000`. Binary references to this array identify the Goron waist/hat patch sites at indices 16 and 17.

The native cosmetic patch bridge indexed `DisplayList::Instructions` without checking its length, resource type, or null return. `IsCustom` was its only model-layout guard. A replacement binary can retain `IsCustom=false` with fewer commands. In addition, the editor called the native cosmetic callbacks before the first custom-model ownership scan. Together these allow a saved Goron cosmetic to patch a shorter replacement before its model is recognized.

The actual production patch function, real `Fast::DisplayList` and GBI types reproduce an AddressSanitizer heap-buffer-overflow when the index-16 Goron patch targets a 16-command unmarked list. This establishes an unsafe operation and a mechanism consistent with the corrupted pointer; it does **not** identify the exact resource or archive that supplied the user's shorter list. The log also mounts an unmarked model pack, but a filename is not proof of its contents.

The follow-up repair:

- Validates display-list type, metadata, and both source/destination indices before patching. Rejected path/patch pairs are logged once.
- Saves a weak reference to the actual patched resource. Unpatch/reset restore that instance only, with bounds checked again, instead of applying saved instructions to a replacement now found at the same path. Expired resources need no restoration.
- Scans custom-model ownership before startup native-color callbacks run. Existing custom-model suppression therefore applies on their first invocation.

Archive locking and music-pack conversion are unchanged. The earlier mutex exceptions might be another effect of corruption, but that connection has not been demonstrated.

The user reports the trigger was Prelude music packs made using `oot.o2r`, and that removing those archives lets this setup boot. They will rebuild the same tracks using `mm.o2r`. This establishes a user-reported workaround, not acceptance of the decoder/cosmetic repairs or identification of the exact corrupting resource. Keep that comparison on the same executable with other mods and settings unchanged, replacing the old packs rather than loading both sets. Retain the old packs for reproduction. A successful comparison would implicate pack contents or their interaction with this setup; it would not by itself prove which write caused the corruption.

## Verification

`python3 scripts/diagnostics/run_mm_audio_decoder_tests.py` runs 21 cases. It extracts the production Ogg functions and streamed playback branch, uses real libogg/libvorbis with generated tone data, and injects documented Vorbis/Opus error returns at library boundaries. Opus importer tests cover header classification and encoded-byte preservation; mixer tests cover open/seek/read failures, successful partial reads and EOF, not an end-to-end Opus listening test.

The original implementation failed the malformed-header, Vorbis-error, zero-size-read and null-playback regressions. The original Opus mixer failed open/seek/read-error cases. The repaired implementation passes all 21 cases with AddressSanitizer and UndefinedBehaviorSanitizer. LeakSanitizer was disabled because the local executor's tracing environment prevents it from running; no leak-check result is claimed.

The existing real-header MM audio compilation/runtime checks, weather suite and 62 item-color cases also pass locally. CI installs the decoder test dependencies and runs this suite before full Windows/Linux builds. Full build success and the user's pack-specific runtime stress acceptance remain separate gates.

For the cosmetic follow-up, `python3 scripts/diagnostics/run_mm_gfx_patch_tests.py` runs 13 cases against the production bridge with real engine resource implementations. Cases cover short/unmarked and marked-custom lists, missing/wrong resources, negative and oversized copy/patch indices, valid patch/unpatch, replacement and expired resources, and lists resized before restoration. All 13 pass with AddressSanitizer and UndefinedBehaviorSanitizer (`MM_GFX_PATCH_CXXFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'`, with local LeakSanitizer disabled as above). Before repair, the short-list case overran the heap and the same-size replacement case restored an unrelated instruction into the new model. The scene-randomization fixture also failed the new assertion that custom-model ownership must be ready before the first native callback; it passes after the startup reorder. The 62 item-color cases and their real-header C/C++ checks remain passing. CI runs the display-list suite before full builds.

## Remaining evidence and runtime check

The middle log exception (`0xE06D7363`) reaches `libultraship.dll + 0x1068D2` through `O2rArchive::LoadFile`. Disassembly maps it to a failed pool-mutex lock followed by `_Throw_Cpp_error(5)` (`resource_deadlock_would_occur`). Its root cause is unresolved. It is not evidence that a decoder thread limit was reached, and this patch does not claim to fix it.

Retest the same music packs with the combined candidate. Confirm startup, track changes, scene randomization, fanfares and OoT/MM switching. If an importer rejects a sample, retain the new resource-path diagnostic to identify the supplying pack. Preserve any new crash log, especially if the archive exception repeats. Do not promote the candidate to the sacred baseline without runtime acceptance.
