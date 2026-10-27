# Foreign Master Sword shop receipt freeze

Candidate parent: `43131342882972eeb4a257599ec8b061c7dc6d43`
(`integration/big-merge-20261016`). The reported build identifies its merge
commit as `4d002ee117956bc3e9b5d7f7bb6fbfcb2a76ef40`.

## Evidence and cause

- `Fleet of Harkinian(20261017-073506).log` ends at 01:54:52.378 while opening
  custom item text `0xF8`; it contains no crash stack at that point.
- The previously supplied `file1(5).combosav` has the identical master seed,
  `2031844488`. Its foreign map assigns `GC Shop Item 6` to an MM-owned
  `Progressive Master Sword`. The log records its purchase flag `0x48`
  at 01:54:35.380, before the shop interaction ends and the receipt starts.
- `BuildComboForeignMessage` builds the equipment lesson and calls
  `msg.Replace(receiptName.c_str(), name)`. `name` is an lvalue string such as
  `Master Sword (MM)`, so overload resolution converts it to a `CustomMessage`.
- The localized `Replace` overload searched again from the beginning after
  every replacement. Its replacement contains `Master Sword`, so it matches
  its own output indefinitely. An unchanged foreign label also loops.

## Correction and preservation

Both replacement overloads advance past the inserted text; empty searches
leave the message unchanged. Advancing by replacement length also preserves
adjacent deletions, which the plain overload previously skipped.

No shop price, placement, item grant, sword model, animation, archive, save,
wallet, or other gameplay code is changed. The candidate remains separate from
the integration baseline and is not an accepted in-game build.

## Verification

- The new focused test executes both production method bodies with their
  production declarations. The foreign Master Sword lvalue call timed out
  on the original code, then passed after the correction.
- Normal and address/undefined-behavior sanitizer modes pass. Leak detection
  is disabled only for the local sanitizer invocation because this container
  cannot inspect process threads for LeakSanitizer.
- Coverage includes the real localized Master Sword lesson, unchanged labels,
  replacements containing the search text, translated fallback, adjacent
  deletion, and empty searches. A fast CI step runs both test modes.
- The subsequent broader audit supplied the pinned dependency headers and
  completed the full native/foreign receipt runner in normal and sanitizer
  modes, including the real donor bridge and seed/settings fixtures. See
  [the pickup-text audit](pickup-text-audit-20261017.md) for additional findings,
  coverage, and remaining build/runtime limits.
- A complete game build and the actual shop transaction have not been run.
  Decisive runtime check: buy the MM-owned Master Sword at GC Shop Item 6,
  finish its receipt, confirm control returns and the award is granted once.

## Evidence hashes (SHA-256)

Log: `4c72d543895c95d4b7ce0f55d1858c7954ef169cdf7d43c8aa4661d026f8bf4b`

Save: `960ca990c63277a171a4917fe21b5e31c5a4b52e09b1122a54754d50c48e477c`
