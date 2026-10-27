# Forest and Black Market music checkpoint

This candidate adds two independent entries to **Audio Editor → World BGM** in
both hosts. It changes no room, actor, model, item, entrance, archive or camera.
The existing external Djipi O2R requirement for the optional Granny interior
is retained. No new audio or Djipi assets are bundled.

| Entry | Default | Scope |
| --- | --- | --- |
| Lost Woods (2Ship) | Saria's Song / Lost Woods (`0x3E`) | Combined MM forest exploration, solo 2Ship forest copy, solo SoH forest port |
| Skiejer's Black Market | Each host's regular Shop theme | Same entry for the magical and Granny appearances |

The entries support manual selection, custom BGM, preview/stop, shuffle,
reset and locks through the existing editor. Ordinary Shop and native Lost
Woods replacements remain independent. Each host retains its own selection:
native sequence catalogs and Shop IDs differ between the hosts.

The CVar suffixes under `gAudioEditor.ReplacedSequences` are
`COMBO_MM_LOST_WOODS`, `COMBO_MM_BLACK_MARKET`, `COMBO_OOT_LOST_WOODS` and
`COMBO_OOT_BLACK_MARKET`, each with `.value` and `.locked`. Defaults resolve
to physical native IDs; editor keys `0xFF00` and `0xFF01` are metadata only.
Invalid, missing, fanfare and virtual-key selections fall back safely. Virtual
keys cannot be used as ordinary sequence replacements or enter shuffle pools.

## Scene start and portal handoff

Scene configuration runs on the real shared `Play_Init` path after OTR scene
commands, immediately before native environment music selection. MM shuffles
before this point. The native environment still decides forced music/silence,
Final Hours, dawn and night ambience. Scene boundaries invalidate native
same-ID caches; only the main BGM player's ordinary starts receive the scene
selection. Existing resolved OoT preview/MM-sidechannel starts are preserved.

| Mode | Skull Kid end of the forest | Music handoff |
| --- | --- | --- |
| Combo | Existing opening-dungeon entrance normalizes to native Clock Tower interior | Destination's normal audio selection; forest entry no longer applies |
| Solo 2Ship | Retained forest port's `0xC000` exit leads to Clock Tower interior | Destination's normal audio selection |
| Solo SoH | Retained `0x61C` forest exit returns to the remembered OoT Lost Woods origin | Re-resolves native Lost Woods, including entrances that retain BGM |

The routing itself predates this candidate and remains unchanged. Audio tests
exercise the destination boundary and combined entrance normalization; the
existing solo forest tests exercise the actual return/normalization code.
Scene previews cancel their restore state at scene entry so that an old forest
or shop audition cannot suppress the destination selection. Intro/flashback,
title, credits, invalid-slot and wrong-mode visits do not acquire a scene row.

## Verification and remaining runtime check

Executed with real game headers under AddressSanitizer/UndefinedBehaviorSanitizer:

- Both production audio collection constructors, imported BGM/fanfare registration,
  replacement validation, independent selections and physical fallback IDs.
- Native `Environment_PlaySceneSequence` for both engines and MM's second scene
  request cache; same-tune arrival/return, forced silence/music, Final Hours,
  exact dawn boundary, night ambience and pending forced requests.
- Actual graph-thread start functions and worker-command encodings: full 16-bit
  custom BGM, unchanged native bookkeeping, main-player-only routing and OoT
  already-resolved/primed starts.
- MM preview cancellation/automatic stop and OoT exact-bank preview when native
  bookkeeping would otherwise suppress it; scene-row group-control comparisons.
- Combined/solo forest guards and Clock Tower/OoT Lost Woods destination handoffs.

Ten changed native translation units compile to objects with real dependencies.
Existing solo forest route tests and both streamed-audio runtime suites also pass.
These are source, native compilation and boundary-fixture results. No full game
link, audible in-game check or visual acceptance is claimed for this candidate.

After the cumulative build, listen to forest → shop → forest and forest → portal
in all three modes. Include differing custom tracks, the Granny checkbox, a
preview interrupted by a portal, a SoH remembered north entrance, night ambience
and MM Final Hours. These runtime checks remain pending.

Commands (native dependencies must be installed or supplied through `CPATH`):

```sh
python -B tests/black_market/run_scene_music_tests.py --native
python -B tests/black_market/compile_scene_music_units.py --out /tmp/music-objects
python -B tests/black_market/run_solo_forest_tests.py
python -B scripts/diagnostics/run_mm_audio_runtime_test.py /tmp/mm-audio-boundary
mkdir -p /tmp/oot-audio-boundary
python -B scripts/diagnostics/run_audio_runtime_test.py /tmp/oot-audio-boundary
```

## Merge and recovery

This work is isolated on `poc/forest-market-music-20261018`, based on the
recovered shelf-material source. The standalone base
`7a9b96d30423c0f4041502d3e4d8e5612ca56266` has exactly the same tree
(`80b359223ca77569d920b168f32dd59c6e5f9b4f`)
as the original source commit `3af965932120c31ab0d1024deb35ddba5d29f52a`.
Their histories differ. Apply this feature's patch or cherry-pick its bounded
commit onto the cumulative checkout; do not merge the standalone snapshot root.

The external recovery archive supplies the complete source bundle, exact refs,
commit/tree hashes, verification logs, dependency identities and restore steps.
The smaller merge kit supplies the feature patch, changed-file hashes and the
same handoff. The original shelf-material archive is retained unchanged:
`ComboShip-Shop-Shelf-Materials-Source-20261018T062734Z.zip`, SHA256
`fe29fb042a8da8619511975da708f884599d4fefcb854d4c978b2636497f3c20`.
Its retained scene packs and external Granny dependency still apply; this
source-only music change creates no replacement O2R.
