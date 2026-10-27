# Map, junk, song and key presentation POC — 2026-10-07

Base: `13901c677d0084e0305eec4672c0557f4434aea7`.
Branch: `poc/mm-receipt-key-song-polish-20261007`.
Status: implemented and verified with compiled regression fixtures and offline geometry checks. Full application build and configuration-specific runtime acceptance remain pending. Do not promote this POC to the master baseline from these checks alone.

The supplied 30.3-second clip shows a map entrance sentence wrapping after “Shadow”, despite only a few words remaining. Both Latin receipt renderers now measure their actual per-glyph pen advances and try sizes from 75% to 55%, selecting the largest size that keeps the map body in three lines. Extremely long entrance lists can join the title's short layout row. Explicit attribution pages remain intact, and the font/space size returns to normal on later pages and ordinary messages. Compass boss/reward sprite receipts retain their established layout. Bodies that cannot fit the bounded minimum still paginate safely rather than dropping words.

Native MM junk rewards now prefer their short native receipt. Imported consumables can use the unresolved OoT receipt export instead of the generic queue name/icon fallback. Traps remain excluded. Five story-cutscene song grant handlers (Soaring, Healing, Time, Storms and Epona) now use the existing descriptive, localized receipt builder and the correct song icon; the native Item_Give calls and cutscene selection remain in place.

Bolero retains its mandatory red shimmer with no flame mesh. Prelude retains its mandatory yellow shimmer and emits small rising light motes instead of its ring/disc. Soaring again selects the existing feather mesh through both native and portable song dispatch. Storms rain, plain regular notes, Serenade, and accepted Zelda/Saria particle samples remain covered by regression checks.

The key repair is a separate Alt Assets archive using the existing `cor_mm_keys_poc2` resource namespace. Woodfall's coplanar side petals and the Great Bay small key fin receive small closed-part depth separation; one duplicate Great Bay boss-key eye triangle is removed. Stone Tower's overlapping cap junctions receive bounded relief, with all copies of a shared point moving together. Offline checks certify no remaining positive-area coplanar overlaps, no increase in open/nonmanifold edges, unchanged bounds, unchanged eight donor bodies/teeth/normals, valid triangle batches/resource dependencies, and ordered Stone Tower cap depths over each previously overlapping interior.

Key input SHA256: `2b4948ff7d784dbce5943f264ed8049b1e7e2a57b8744071fcf30b39c06b83c5`.
Key candidate SHA256: `965b797c97d06b2717b46f92a5f8e2b81790753d294dbd470484690a6159b7a5`.
Reproduce with `scripts/assets/repair_mm_dungeon_key_surfaces.py SOURCE.o2r TARGET.o2r --report REPORT.json`. The script uses the Python standard library and rejects an unexpected input archive.

Verified checks:

- `run_mm_item_receipt_tests.py`: real catalogs and native glyph widths, map/compass pages and sprites, receipt resets, native junk bodies, actual story-song grant lambdas in ENG/FRE/GER with cutscenes on/off, donor Soaring/native junk export, queue grants/traps, frozen progression, cache resets, seed/save settings, and pause information.
- `run_song_gi_tests.py`: native notes/colors, overlay selection, shimmer-only Bolero, ring-free Prelude motes, Soaring feather rachis, bounded animated geometry, and accepted Zelda/Saria samples.
- `run_nei_gi_tests.py --held --combo`: native OoT and both real foreign GPU submission paths, one mandatory shimmer, Soaring feather submission, toggle/Alt/mod paths, and existing renderer bounds/arena guards.
- `run_mm_nei_tests.py`: real native MM song submission and donor/Alt independence plus existing dispatch/lifecycle fixtures.
- `run_receipt_syntax_tests.py`: all changed production translation units, including the five song-grant handlers, with real game headers.
- `run_mm_dungeon_key_tests.py --sanitize`: native draw/export/cache/latch/OPS replay and independent palettes. Leak scanning is disabled because this execution environment runs under ptrace; address and undefined-behavior sanitizers remain active.
- Key archive repair certification and clang-format-14 on all changed production files.

For runtime acceptance, replay the clip's Spirit/Shadow map and working compass, inspect all eight rotating keys with current cosmetic settings, collect native/foreign junk rewards, and grant Soaring/Bolero/Prelude through the queue and available story handlers. Confirm each item is granted once and the expected text, icon and particles appear with cutscenes on/off. These observations have not been performed for this POC.
