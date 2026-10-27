# ComboShip item ownership provenance probe

## Candidate and preservation

The trigger is unexplained ownership of Din's Fire in MM and Light Arrows after
returning to OoT, with no remembered get-item notification. Absence of a
notification does not identify the writer: dormant delivery and reconciliation
can legitimately bypass the visible pickup animation.

This candidate includes passive diagnostics and the narrow item repairs described below. Parent: the published initial probe
`e5d16f21d31ff396b9658fa099bb2852aedebbf1`, stacked on the transformation-collision
candidate `6ba22e0d1a079464220837111ac487dc06be24e8` (PR #30). The parents' runtime
limitations remain unresolved. No existing-save rewrite, reseed, merge or promotion
is part of this PR. The accompanying audio
correction is opt-in and recorded below. Existing renderer, audio,
reward-pool and transformation work is retained.

The native `Item_Give` bodies are preserved byte for byte and called by small
before/after wrappers. C++ scopes survive early returns and unwinds. The observer
reads raw save fields and the pointer-only `Nei_Save` accessor. It never asks
logic, extraction, inventory getters, conversion or reconciliation to calculate
ownership. Exceptions at the diagnostic boundary are swallowed. No PlayState
pointer is dereferenced by the capture functions. Non-Combo builds use a no-op
bridge; observer symbols remain internal to each game DLL under the existing
hidden-visibility build configuration.

## Runtime evidence and item repairs (October 1 follow-up)

The latest session in `Fleet of Harkinian(20261001-154019).log` reports seed
668268527. At 10:32:03.292, the Water Temple small key grant changes
`nei.ootSpellsOwned` from 0 to 1. Delivery identifies the Bank Adult's Wallet
check. The Forest Temple key from Astral Observatory Passage Pot 03 follows at
10:32:05.978. All 46 OoT dungeon-item cases in MM's `Rando::GiveItem` fell through
to Din's Fire because their no-effect branch lacked a break. The repair ends that
branch; FC identity recording before the switch is preserved. Moon's Tear instead
sets `RANDO_INF_OBTAINED_MOONS_TEAR` and gives native item 0x28, with its own break.
It does not use `ootSpellsOwned`. Existing accidental ownership is not removed.

At 10:37:24.747, MM's save reset leaves zero-filled item slots and saveType 0;
the same timestamp reports an OoT Item_Give of 0x12 (Light Arrows). At
10:39:54.244, after the matching MM save reload, `MM_RaiseSharedTier` gives Light
Arrows to MM. The launcher invalidates its MM resident-slot cache only after the
MM loop returns, while the shared tick can still run during reset. Previously
`MM_GetSharedTier` considered any value other than 0xFF owned, including reset
zeros. The repair rejects non-randomizer save state in both MM tier reading and
raising, and requires the actual arrow item ID for each elemental arrow tier.
Valid dormant saves remain eligible; this is not a gameplay-only gate. This path
matches the reported quit/reload behavior, but the log contains no OoT audit
records to attribute its initial grant beyond the native Item_Give line.

The new extracted-production regression reproduces the dungeon fallthrough and
reset-save tier failure before correction. After correction, ASan/UBSan checks
cover all 46 dungeon IDs with all spell combinations and Moon's Tear states,
legitimate spell/Moon grants, reset-save source/destination rejection, valid
dormant eligibility, and exact arrow ownership. These narrow fixtures model host
services and do not replace platform builds or runtime acceptance.

The audio log confirms both checkbox states executed the gain path (384 requested
bytes: 384 processed on, 768 off). The user's audible A/B test reports continued
shop crunch in both modes. The gain span defect remains corrected behind the
opt-in setting; it has not resolved the reported shop audio problem.

## Writer coverage and criteria

| Path / criterion | Probe boundary and evidence |
| --- | --- |
| Save creation and explicit starting items | Both save initializers; OoT `StartingItemGive` includes check and resolved item IDs; MM `GrantStartingItems` logs raw configured/computed item followed by converted native grant. Creation changes are distinguishable from later pickups. |
| Existing saves and NEI migration | Both actual save-load functions and OoT NEI deserialization have scopes. A first/changed-slot snapshot is `state-present`, not proof of acquisition. Same-slot load deltas have a save-load origin. |
| Native and randomizer pickups | Both C `Item_Give` wrappers, OoT `Randomizer_Item_Give`, and recursive MM `Rando::GiveItem`; scopes carry actual item IDs, MM check ID, and OoT starting-check IDs. Entry is an attempted call; the after snapshot shows what actually changed. |
| Foreign delivery while active or dormant | `SOH_GrantCrossItem`, `MM_GrantCrossItem`, `Combo_GrantResolvedOOT`, MM dormant-resolved delivery, and native dispatch; existing suppression/dormant logs remain. Empty-bottle rejection and early returns still produce coverage without an ownership delta. |
| Shared tier convergence | Both `RaiseSharedTier` seams and their nested native grants; existing OoT family/current/target/resolved-item log retained. |
| Direct shared JSON/registry writes | Both `ApplyShared` and `ExtractShared` scopes, including native folding, leaked-cell healing and registry materialization inside these functions. Full obtained/applied FC counts are observed, not just flags for the reported items. |
| Native/custom/virtual slot setters | Both extended slot setters and NEI owned-slot setters; MM virtual OoT slot writes are directly attributed. In particular, `ExtInv_SetOotSlotItem(VSLOT_DINS, ITEM_DINS_FIRE)` writes the Din ownership bit without calling Item_Give. This is a possible writer, not an established cause. |
| Give-all and editor paths | MM NEI give-all, OoT SW97 full inventory, both save editor draws; direct uninstrumented console/debug/network/actor writes to covered ownership fields are detected at the next checkpoint or before the next scope. |
| Item model/icon previews | Both exported static/animated draw-info queries and OoT icon-info query have quiet before/after scopes. A preview call that changes ownership is labeled with its preview origin; unchanged per-draw calls do not log. |
| Seed-generation oracle scratch ownership | MM Reset captures the live baseline and suspends observations until Restore copies the live context back. Restored state is compared to the live baseline, never to scratch grants. A failed restoration delta remains visible. |
| Game transitions and saves | Both prepare/notify/resume boundaries; MM combo load/init and both save-write entry points; ordinary game-state start and draw-end checkpoints continue coverage during normal play. |
| Bypass writes and a non-reproducing run | Every covered persistent field is sampled at frame/draw boundaries. Changes outside named scopes are explicitly `unscoped-interval`; earlier changes are flushed before attributing the incoming scope. Every 600 checkpoints a coverage record reports samples and cumulative changed-field count even with no bug. |

The writer audit searched the native give functions, randomizer gives and
starting kits, SaveManagers, FleetSync, cross-delivery/tier APIs, preview
resolvers, NEI accessors, extended-inventory setters, save editors, console/debug,
network and native inventory writes. Some paths have exact scopes; the frame
fallback covers persistent mutations by the remaining paths. It does **not**
pretend that a sampled interval identifies an exact source line.

## Ownership schema

Both games include all native item slots (MM's 24 mask slots too), upgrades,
quest bits, dungeon items/keys, health capacity, magic/defense ownership, native
randomizer ownership bits, custom NEI slots, bottle contents, all 128 legacy
counts, all 512 FC obtained counts and all 512 applied counts. Captured custom
flags cover equipment, weapon upgrades, trades/masks, wands, slate, seasons,
shovel/dominion/pokeball, keg, cape/pendant, cane skills, Quartz, bomb arrows,
shield ownership, Triforce and captured lantern types. OoT adds BGS ownership,
Ultrashot, pictobox, learned echoes, Rito flags, MM quest mirror and RPG counts.
MM adds native equipped sword/shield state, fairies/tokens, OoT spell/upgrade/
quest/weapon mirrors, clawshot/Mario ownership, RPG levels and every check's
obtained, cycle-obtained, eligible and shuffled bits.

MM check flags are packed in blocks of 16. `rando.checkFlags[start]` holds four
bits per check: bit 0 obtained, bit 1 cycleObtained, bit 2 eligible, bit 3 shuffled.
For check `start+j`, shift the word by `4*j` and mask `0xf`. The index is the
first check in the block, not an item ID.

Current rupees, ammo, health, active spell timers and button selections are not
classified as ownership: normal use would make those per-frame traces noisy.
Grant entry IDs still record consumable attempts; this probe cannot demonstrate
that a rupee or ammo amount was applied. Bottle contents and key counts are
included because they are relevant to item replacement and possession.

## Reading the records

Records retain `[ItemGrantAudit]`, game, thread ID, per-thread sequence, phase,
slot, mode, save type, field count, sample count, cumulative changed-field count,
active nested origin chain, and native-width hexadecimal before/after values.
MM records its persisted numeric seed. OoT has no equivalent raw numeric seed
in SaveContext, so its snapshot seed field is 0; use the startup/seed log and
slot context rather than interpreting that zero as a seed identity.

- `kind=state-present baseline=1`: raw ownership already present at first
  observation, changed slot, changed save type or changed MM seed. The displayed
  empty-to-value notation is an inventory summary, not a grant accusation.
- `kind=ownership-delta`: raw fields changed since the previous observation.
  A save-load origin means restored state; a preview origin identifies a preview
  mutation; a give origin identifies the call interval containing the mutation.
- `kind=coverage`: entry/exit, restored oracle state or periodic checkpoint with
  no changed ownership. It is useful in a clean run and for rejected/no-op gives.
- `origins=unscoped-interval`: mutation happened between observations without a
  named writer. Preserve the surrounding records; do not attribute it to the
  next unrelated pickup merely because it is nearby in the log.
- `overflow=1` or `schemaChanged=1`: field coverage changed or exceeded capacity;
  no claim of complete field coverage is valid for that segment.

An existing save already containing an erroneous item cannot reveal its
historical first writer. Its first snapshot proves state present only. Missing
heartbeat/restoration records indicate incomplete coverage, not proof that no
writes occurred. Thread-local histories do not establish a total order across
threads. Sampling cannot catch an uninstrumented write that is undone before
an observation, and this is not a memory watchpoint or data-race detector.
Quiet preview scopes do catch temporary changes that survive to their boundary;
changes wholly internal to the preview and restored before return can be missed.

## Verification

- `python3 tests/item_grant_audit/run_tests.py`: tracker passes ASan/UBSan for
  a clean run, prior bypass writes, nested grant/preview attribution, no unchanged
  preview spam, live oracle restoration and failed-restore detection, changed
  save/seed baselines, removals, nested suspension, overflow and scope balancing.
- The actual production capture functions compile against extracted production
  ownership structures: OoT 1,498 fields; MM 1,516 fields; both below the 2,048
  capacity. Capturing leaves fixture saves and NEI state byte-identical. Other
  host SaveContext services are modeled; this is not a complete application build.
- Native grant body hashes match the initial probe's original bodies. C ABI
  wrappers preserve all 256 return values and balance begin/end calls. Stock C
  and C++ bridge fixtures compile/link without the observer.
- Existing shared-item fill/plando/launcher and cross-delivery regressions pass.
- Native dungeon-reward pool checks pass all 32 configurations (16 ComboShip,
  16 standalone). Asset collision and diff checks pass.
- The provenance suite is wired into the existing CI gate. Full platform
  compilation and actual in-game attribution remain unverified at publication.
  No fresh runtime test or user-save modification was performed.

The next evidence is the provenance log from an ordinary session on this exact
candidate, including startup identity and first ownership snapshots. A clean
session is useful; deliberate reproduction is not required to establish that
sampling is active. The diagnostic does not remove existing items or claim to
have fixed their source.

## Opt-in MM audio gain correction

Cor reports persistent crackling in MM shops after the earlier audio worker and
SSE2 candidates. A confirmed additional defect exists in `aHiLoGainImpl`: each
iteration changes eight signed 16-bit samples (16 bytes), but subtracts only
8 from the remaining byte count. A 416-byte command therefore changes 832
bytes of synthesis workspace. Native MM synthesis calls this after resampling
whenever a note has nonzero gain. The overrun can alter adjacent note/Haas
workspace; it is not proof that this explains the supplied shop report.

MM Enhancements → Fixes → **Fix MM Audio Gain Buffer** controls
`gEnhancements.Fixes.MMAudioGainBuffer`, default **false**, as explicitly
requested by cor. Enabling counts the bytes correctly; disabling retains the
existing candidate's gain-loop behavior. Changes take effect on the next gain
command. The prior worker, streamed-PCM and SSE2 candidates are preserved;
this checkbox controls this new correction only. Samples, instruments,
sequence tempo, volumes and reverb coefficients are unchanged.

`[MMAudioGain]` reports first gain use and mode changes, including enabled,
gain, rounded requested bytes and actual processed bytes. It does not log each
note. A missing record cannot establish that this particular path was involved
in the distortion. A present record proves use of the gain path, not audible
resolution. Natural low sample fidelity cannot be corrected by repairing this
buffer span, and the user's quoted Google explanation is not established as the
cause of the observed crackling.

Verification: the actual complete production MM mixer failed the new span
regression before repair (179,515 differing samples in the first run). The final
regression checks all 256 gains, nine byte lengths, both checkbox modes, native
DMEM_TEMP placement and every adjacent workspace sample. Off matches the
legacy arithmetic and extent; on preserves all bytes outside the rounded
requested span. First-use/mode-change reporting is bounded in the fixture.
137,137 MM and 132,529 OoT cases pass with zero differences, including ASan/UBSan.
Existing production audio worker and 32/44.1/48 kHz streamed PCM checks pass.
Shared fill/plando/launcher/delivery and all 32 dungeon-pool configurations pass.
Full game compilation, menu visibility and audible resolution remain unverified
until CI and ordinary runtime evidence. No user reproduction session is required
as part of this implementation handoff.
