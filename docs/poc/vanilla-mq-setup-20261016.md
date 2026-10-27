# Concurrent vanilla/MQ dungeon support

Candidate branch: `feat/vanilla-mq-setup-20261016`.
Exact base: `1bbb94e692629c0d910f97b57dd7cd02240d3775` (merged PR43).
This is an isolated source candidate; the accepted baseline is unchanged.

## Intended behavior

The shared setup screen now asks for an OoT ROM, then optionally the other
OoT quest, then MM. Either vanilla or MQ may come first. The second OoT step
accepts only the complementary quest and can be skipped. Installed archives
are retained. Existing installations can add the missing quest by launching
`ComboShip --rom-setup`. The option is consumed by the launcher before game
initialization. If both OoT archives are installed, setup skips that step.

Both archives retain their native names, `oot.o2r` and `oot-mq.o2r`. Native SoH
already mounts them together and exposes its MQ controls when both are present.
The Combo menu therefore keeps native None, Set Number, Random Number and
Selection modes, including individual Vanilla/MQ/Random dungeon settings.
These layouts feed the existing mixed Combo entrance pools. The shared setup
screen also supports `mmNeeded = 0`, so a future solo OoT launcher can use both
quest pools without requesting MM. A full solo launcher is outside this change.

## Why preparation and replay changed

Combo preparation now seeds native settings finalization from the master seed.
Oracle initialization reuses that prepared context rather than rolling MQ
layouts a second time after building the check/item pools.

New consolidated seeds record `oot.masterQuestDungeons` as native dungeon names
and `oot.dungeonLayoutAssets` as the original quest availability mask (vanilla 1,
MQ 2, both 3). The latter preserves native RNG consumption when an installation
adds an archive: an MQ-only seed can have a disabled menu preference of None,
yet native finalization selected all MQ and consumed a dungeon shuffle before
selecting keyrings and trials. Replaying with both archives must reproduce that
normalization. The override exists only during Combo's native finalization;
resource loading and settings UI still inspect physically installed archives.

Layout metadata is validated before reload changes accepted native or pending
seed state. Validation rejects unknown/duplicate dungeon identities, malformed
availability masks, inconsistent snapshots and unavailable required assets.
Both launcher and headless paths restore the snapshot after setting the seed
and before rebuilding pools. Native save files retain SoH's exact MQ dungeon
IDs. Legacy consolidated files without these new fields retain their seeded
settings reconstruction; they cannot recover unrecorded historical archive
availability.

The extraction worker publishes completion only once its future is ready, so
the same worker can immediately accept the complementary OoT ROM. Header
classification preserves the existing boolean export and adds a separate quest
classifier. A missing vanilla PAL debug revision in `IsMasterQuest` is corrected.

## Verification boundaries

| Check | Coverage |
| --- | --- |
| Setup behavior, ASan/UBSan | 11 screen scenarios: ordering, either quest first, skip, installed archives, wrong quest/game, auto-scan, reselection and an OoT-only callback configuration |
| Launcher behavior, ASan/UBSan | Setup flag is consumed while other arguments are preserved; existing installs can reopen setup; actual archive availability reaches the screen; normal startup retains its fast path |
| Native classification, ASan/UBSan | 27 header/byte-order fixtures plus invalid, truncated and missing paths; sparse synthetic fixtures contain no game assets |
| Extraction progress, ASan/UBSan | A completed atomic flag cannot start the next job until the worker future is ready |
| Native MQ/settings probe, ASan/UBSan | Exact-count endpoints/mixed counts, random count, individual Random, mixed small-key counts, either starting game, oracle reuse, native save/load, missing assets and deterministic layout/trial/keyring replay after adding vanilla |
| Production reload callback | Invalid layout metadata preserves accepted seed A with cross-dungeon shuffle enabled or disabled |
| Existing cross-dungeon suite, ASan/UBSan | Catalogue, composition, pools, routing, host adapters and recovery regressions |
| Existing native seed/settings probe, ASan/UBSan | Existing settings and save-context behavior |
| Real-header compilation | Launcher, headless tool, setup screen, extractor, OoT globals and native settings with the project's `COMBO_BUILD` configuration |

The native settings probes compile production function bodies with real
Context/Settings/Dungeons/Options/RNG/save traversal. External graph construction,
GUI services and inventory construction are fixture boundaries. The UI probes
compile the production screen with a scripted renderer/file-picker boundary.
These checks establish source behavior, not an installed-game runtime proof.
Leak detection is disabled because LeakSanitizer cannot inspect this runtime's
process task directory; address and undefined-behavior instrumentation remain on.

Commands (provide the normal project's third-party headers via the build
environment, or `CPLUS_INCLUDE_PATH`):

```sh
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/rom_setup/run_tests.py --sanitize
python3 -B tests/rom_setup/run_syntax_checks.py
python3 -B scripts/diagnostics/run_cross_dungeon_reload_test.py
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/cross_dungeon/run_tests.py --sanitize
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/item_receipts/run_seed_settings_tests.py --sanitizers
```

The negative control below deliberately ignores recorded archive availability;
it must fail the archive-upgrade trial/keyring replay assertion:

```sh
python3 -B tests/item_receipts/run_seed_settings_tests.py --mq-layouts --ignore-recorded-quest-assets
```

An additional `run_syntax_checks.py --standalone` diagnostic checks files without
`COMBO_BUILD`. Full globals compilation in that configuration is already blocked
on the exact base by unguarded `GetTrickNamesEnglish` and
`SetComboRandoGenerating` references. The same failures occur in this candidate;
they are unrelated to the upcoming solo mode, which can use the existing Combo
build. `--source-rev 1bbb94e692629c0d910f97b57dd7cd02240d3775` reproduces the
baseline diagnostic. No standalone-SoH build success is claimed.

## Installed-game acceptance still needed

Build/package the candidate, complete setup with both quest orders using the
player's supported ROMs, generate mixed-layout Combo seeds from either starting
game, enter representative vanilla and MQ dungeons and verify their keys, then
save/reload and reopen a recorded seed. Also add a complementary ROM to an
existing one-quest install with `--rom-setup`. A full native application link,
Windows package and these runtime checks were not performed in this environment.

Rollback is the supplied patch in reverse (or remove the candidate checkout).
Existing archives and the accepted baseline require no migration or replacement.
