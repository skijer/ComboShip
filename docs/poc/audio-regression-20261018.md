# Rain thunder and scene music regression candidate

## Baseline and scope

Candidate branch: `poc/audio-regression-20261018`, parent
`8d9500eea71cd3991164d4aae39f2aec657bcafa` (native arrival fades), with music
checkpoint `df56c2c1` already present. These are source candidates, not accepted
in-game masters. The baseline thunder decoder and night updater are byte-identical
to the corresponding files at PR 44's current big-merge commit
`43131342882972eeb4a257599ec8b061c7dc6d43`; the recovery kit retains the comparison.

Preserve native weather scheduling, lightning styles/frequency/volume, rain loop
ownership, ordinary SFX contention, BGM/fanfare/cutscene priority, dawn restoration,
the scene/portal routes, title cards, room assets and both solo-mode guards.
No new O2R, sample, geometry, music bank or external dependency is required.

## Confirmed defects and changes

1. **Thunder clip truncation.** `WeatherSamplePlayer` treated the engine's
   `sampleEnd` field at loop offset `0x0C` as a decoded length. The actual SOH
   resource ABI defines those bytes as unused padding; both resource importers
   populate the real end at offset `4`. Padding equal to `1` reduced a 16-frame
   thunder fixture to one audible frame. Decode now uses `loopEnd`, as MM's
   weather adapter already does. Rain's genuine loop bounds and both transient
   thunder voices are preserved.
2. **Night track omitted from shuffle.** The legacy Hyrule Field night selector
   was outside the World BGM registry, so an imported selection stayed fixed.
   `Hyrule Field (Night)` is now a normal independently replaceable World BGM row.
   Its metadata key `0xFF02` never enters playback or a replacement pool. It uses
   the existing `gAudioEditor.HyruleFieldNightSequence` setting, preserving saved
   choices. Reset and lock controls use the same normal editor machinery; its
   lock is `gAudioEditor.ReplacedSequences.HYRULE_FIELD_NIGHT.locked`.
3. **Stale selection after rerolls/edits.** Editor replay requests refresh from
   the night owner instead of replaying a truncated native bookkeeping ID.
   A row edit is remembered while MAIN yields to an event, fanfare or preview.
   The next safe update resolves the exact selected bank once; ordinary fanfare
   recovery retains the cached bank. Native dawn and scene-exit ownership remain.
4. **Imported night preview restoration.** Native preview saves the exact night
   bank only when full physical identity confirms that night is actually playing.
   Stop restores that bank through the resolved queue, provided the same night
   registration still owns it. Cancel/reset and other native previous-sequence
   captures clear this preview state. A same-low-byte event bank cannot acquire
   the captured identity. Actual OoT Play destruction also clears transient
   audition state, including standalone SoH; dormant-host null updates do not
   clear the shared editor setting.

The new Black Market and Forest entries were already registered for shuffle.
Both engines' actual scene and generation callbacks now have direct coverage
with imported-only eligible pools, exclusions, category matching, disabled
options, locks, resets and empty pools. Hyrule Field day and night are both
covered on the OoT side. There is no duplicate Black Market music implementation.

## Verification and limitations

The retained logs distinguish source checks from a running game:

- Regression tests were observed failing before each correction: resource
  padding truncation, absent night row, cached replay, preview restoration,
  deferred row edit, and stale preview state after Play destruction.
- Both real collection constructors and both real native scene/sequence-start
  boundary suites pass under ASan/UBSan, including custom 16-bit IDs, ordinary
  preview behavior, solo/combined guards and destination music handoffs.
- Both real shuffle algorithms and actual scene/generation callbacks pass under
  ASan/UBSan. The night updater, editor replay, native preview start/stop and
  production sequence queue pass the dedicated runtime-boundary fixture.
- Weather decoder/mixer tests cover Low Thunder and Lightning, codecs `0`/`3`,
  genuine short ends and arbitrary padding. The ordinary-SFX allocator fixture,
  native weather actors, seasonal weather and streamed-audio boundaries pass.
- Changed native translation units compile to objects with real headers, with
  and without `COMBO_BUILD`. This is not a full executable link or Windows CI.
- The broader stabilization script passes its audio/weather checks, then fails
  linking the unrelated Time Gate chest fixture: undefined `GameInteractor_Should`
  in `EnBox_Init` / `EnBox_WaitOpen`. That fixture is unchanged by this candidate.
  Later checks from that script are unexecuted, not implicitly passing.

Thunder's trigger path remains present. Default added weather audio is Low
Thunder, frequency `50`, volume `70`. Disabled thunder, frequency zero, native
lightning ownership and occupied/unsupported samples retain their existing
gates. Seasonal-only rain intentionally does not request added thunder; explicit
outdoor rain and authored/native thunder requests remain separate.

There is no audible baseline/candidate comparison for cor's exact current pack
and configuration. The decoder defect is proven; it is not proof that it was the
only cause of the reported build's missing thunder. Status: implemented and
source/native-object verified, awaiting configured in-game listening and acceptance.

## Decisive runtime check

Use the cumulative build with the same packs/order and weather settings as the
reported build. In explicit/authored rain with thunder enabled and nonzero
frequency/volume, listen to Low, Layered and Lightning styles while BGM plays.
Check scene re-entry and an indoor/outdoor handoff. Enable weather diagnostics
only if the result still differs, to distinguish trigger/admission from decoding.

For scene shuffle and generation shuffle, use a pool containing clearly distinct
imported BGM. Check Hyrule Field at night, Black Market's two appearances and the
forest/portal return in combined and solo modes. Include dawn, one fanfare, a
night row edit during an event, and preview/stop followed by leaving/re-entering
the field. Confirm the night enable checkbox and row lock still control behavior.

## Recovery

Apply the bounded audio patch/cherry-pick onto the cumulative source that already
contains `df56c2c1` and `8d9500ee`; never merge the standalone snapshot root.
The source recovery kit records exact refs/trees, changed source hashes, an
incremental git bundle, ordered patches and independently verified restore steps.
Its prerequisite is the retained full source archive
`ComboShip-Forest-Market-Music-Source-20261018T173206Z.zip`; the kit also carries
the intervening arrival-fade commit. Existing external scene packs, optional
Granny appearance dependency and asset load order are unchanged.
