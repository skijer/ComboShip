# Cross-game entrance expansion

Approved scope: cor's October 15 instruction to integrate the three reviewed recovery cases, cross-game bosses, expanded dungeons, and cross-game interiors/grottos. All other proposed mechanics are excluded.

## Baseline and preservation

- Current source candidate: PR41 `913bc3298bfcdd57226f87e326eaf4bf72ccd33a` (9.3, Farore, weather, Field and Zora work).
- Recovered routing: `f6c4eeccc113b5830868a951859ca8a32a295fad`, originally based on accepted develop `a82852a3`.
- Work on `poc/cross-entrance-expansion-20261015`. No develop/master promotion or gameplay acceptance is implied.
- Keep native OoT entrance shuffles, mixed pools, decoupled returns, native reward identities, shared items, game-owned age/time and existing portal behavior.
- Every new pool is opt-in and defaults off. Existing dungeon-only seeds retain their codec and deterministic routes. Legacy seeds without cross data remain unchanged.

## Pools and routes

Retain the shared game-tagged endpoint model. Add explicit Dungeon, Boss, Interior and Grotto categories; shuffle each enabled category independently after native OoT resolution. Native mixing remains effective before this overlay. Expanded v2 layouts that exhaust the ordinary retry budget may use a bounded fallback: validate the identity, then accept random cycles or adjacent swaps only when the full native structural probe remains valid. Refuse an identity-only repair; keep v1 generation and exact recorded replay unchanged. Expanded dungeon selection adds MM's two Spider Houses, Ikana Castle, Secret Shrine, both Beneath-the-Well entrances, and Pirates' Fortress.

Add the eight ordinary OoT and four MM boss arenas; exclude final bosses. Distinguish reverse-door returns, completion warps and recovery destinations. Precompose a boss completion warp through its recorded native mapping and the active dungeon route exactly once. A boss completion alias has one final owner even when its parent dungeon also lists that alias.

MM shared grottos require explicit logical identities, reserved virtual return endpoints, and native load/return/void materialization. Derive canonical spawn data from the existing native BetterMapSelect tables. Never dispatch a logical-only grotto ID to the native scene table. Multi-exit interiors retain all native access conditions.

## Boss progression and item ownership

Physical boss defeat, logical source-dungeon completion, and physical reward collection are separate states. Preserve native heart/reward/Giant/Oath checks and once-only grants. Logical completion updates the source dungeon's world effects and dungeon-count logic without collecting its physical boss reward.

The joint oracle propagates logical clear bits as well as foreign arrival roots. Boss escapes require the native defeat predicate, including native MM edges currently marked unconditional. Temporary oracle state must not mutate gameplay or survive the next inventory query.

Persist logical completion in the bound combo slot. Apply MM world effects on the actual completion event; loading a permanent completion mask must not resurrect cycle-reset effects after Song of Time.

## Recovery and time

Farore points capture an optional dungeon checkpoint on new casts and restore it on same-host or foreign recall. Missing legacy metadata means no checkpoint; migrating an old point cannot attach the current room's checkpoint. Explicit recall wins over ordinary save recovery. Boss death/reload resolves inverse ingress to the source parent; a foreign source resumes its frozen native save through a separately validated arena-identity request.

Normalize Stone Tower inversion switch 0x14 to the actual exterior orientation before initial room actors run, only while cross routing is enabled. Preserve native scene behavior otherwise.

Preserve runtime MM day/time and native clock behavior. Conservatively require playable Song of Time for ordinary foreign MM arrival roots and a genuinely owned clock slice when clock shuffle is enabled. A boss-only exemption requires verified frozen-time behavior; no global clock freeze or forced reset is introduced.

## Evidence and delivery

Use failing behavioral regressions before fixes, source/native fixtures, sanitizer checks where supported, exact-map replay, disabled parity, both starts, combined pools, first/repeat boss warps, save/death/void/Farore recovery and invalid seed rejection. Separate static/build evidence from installed gameplay acceptance. Preserve source checkpoints at task boundaries and approximately every ten minutes of extended work.
