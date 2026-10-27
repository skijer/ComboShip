# October 8 audit merge bridge

This candidate recovers the saved Audit Merge Work handoff and its accepted follow-ups onto PR39 commit `7de5b7fac2e7b892f46eabcdcb21639e6f64c304`. Develop remains pinned at `a82852a3a02c8cabc541cce7c124da71c5755efb`. The current PR39 starting-Pendant migration repair is retained. The bridge is a draft candidate, with live game acceptance still required before promotion.

## Recovery route

Only incremental source deltas were applied, in the audited order: PR40 follow-up, the two wand follow-ups, Lantern, Rod cast POC6 from its original baseline, song/bottle POC2, summer visibility, late tunic, equipment recovery and the later elemental sheen candidate. Overlapping full PR exports were omitted. The original handoff is preserved in `audit-merge-handoff-20261008.txt`.

Shared equipment conflicts retain PR40's item/action carrier specificity and the later acquired-only, full-width equipment routing. Authored Four Sword held/shelf routing keeps its own pair mods and excludes Net; ordinary native/Din/custom hands retain their existing priority. Both hosts' inventory and resource selection paths are exercised by production fixtures.

## Accepted follow-ups

The bottle integration preserves the accepted mushroom, Princess and seahorse geometry, donor UVs/winding/texture pixels, the existing glass casing and procedural gold recipe. Bundled fairy scale doubles while selected replacement shells keep their own scale and native animation. Explicit profile-to-content mapping supplies Princess `80D846`, fairy `FFA0EB` with four faint motes, seahorse `FFE66D`, gold dust `FFD45A` and mushroom `DE64F5`. Native/foreign, Alt, replacement-resource, graphics-arena and matrix/material ownership checks remain active.

Rod modes are saved per seed: Individual seasons (legacy/default), one Rod with all four seasons, and a gated Rod with completion-based unlocks. Start with Rod is independent of the NEI pool switch. Rod ownership, season pickup bits and persistent completion gates are separate; gates merge by union across hosts and do not grant the Rod. `seasons-modes-audit-20261008.md` records the gate alternatives and the explicit recovery assumptions for OoT Fire/Forest/Ice mappings.

MM seasonal sky composition now provides rain-consistent Spring, clear Summer on all three days, intermittent Autumn cover and overcast Winter. Autumn preserves an actual native storm instead of clearing its sky above native rain. Story, interior, underwater and explicit weather-setting boundaries remain. The weather bridge uses `Seasons_GetSeason()`, sharing gated availability, Off and selection healing with the Rod wheel. A regression reproduces the old mask-only failure and verifies actual Great Bay/Snowhead completion adapters while the season-pickup mask remains zero.

Grace's idle/flight trail particles use the native pink fairy palette in both hosts. Forced-spell gold, particle counts and timing remain unchanged.

## Source verification

The formatted integrated production checkpoint is `5e618059427dded95adcd2ca22816d03d9cd3c56`. Independent whole-range review found no unresolved Critical, Important or Minor production findings and passed ten focused checks, including accepted bottle bytes, both native saves/Pendant migration, both generator policies, completion gates/shared union and the actual weather bridge.

The complete canonical NEI batch ran all 91 commands: 89 passed; the receipt-information and song-note extraction fixtures failed on new canonical header dependencies. Both complete commands passed after adding those headers, preserving their production bodies and assertions. The same bottle dependency in the separate MM item-visuals fixture was repaired and its complete command passed. Earlier fixture repairs likewise preserve actual production functions and substantive assertions.

Additional checks passed: asset collision scan (1873 shared paths, 181 previously accepted differing collisions, no new collisions), whole-tree clang-format-14, native weather with sanitizers, actual gated-weather/audio/sky bridge, shared-item integration, production cross-grants and sanitized reward delivery. Logs and independent review notes are retained with the bounded source recovery bundle.

The final publication's Git tree must match the local candidate exactly. Its existing Actions gate must then pass and produce matching Windows/Linux applications, both port archives and the Linux seed-generation smoke result. These build results are recorded in the final delivery manifest rather than inferred from fixture results.

## Remaining game acceptance

Check both hosts with Alt and representative replacement packs: bottle translucency/contents/fairy containment, sword receipt and shop-shelf poses, gated Rod selection through actual completion and host handoff, seasonal sky/weather and Off restoration, Grace flight particles, controller/textbox behavior and runtime performance. Static tests and linked packages do not establish framebuffer appearance or gameplay acceptance.
