# GI runtime regression candidate — October 6

Baseline: PR #34, `92b72bf29d15b80f3bfd88402319bae09b74c895`.
Candidate: `poc/gi-runtime-regressions-20261006`, stacked onto that baseline.
This is an isolated draft POC. No baseline or master promotion is authorized by build success.

## Findings and bounded changes

| Report | Confirmed source behavior | Candidate behavior |
| --- | --- | --- |
| MM dungeon items, boss souls and bottles become blue rupees | MM asks the OoT English-name producer for OoT-owned MM aliases that have no exported draw recipe. | Dispatch 29 exact aliases through existing MM draw handlers: 16 dungeon items, five boss souls, two bottle identities and six Tingle maps. Preserve original foreign placement, grant owner and name. |
| Custom tunic bodies absent while effects render | OoT's per-module ExtensionCache can be cold when MM starts first, despite resident owner archives. The availability gate silently declines the body while effects draw separately. | Query the registered OoT archive directly, including base and owner-selected Alt alias-only `.meta` entries. Keep local overrides, tint and shimmer. |
| Compass displays unknown reward on a third line | The supplied all-items plando has no OoT boss reward placements. The old receipt inserts textual fallback when the reward sprite is unavailable; MM can omit the information entirely. | Always keep enabled compass receipts to title plus assigned boss. Draw the actual seed reward icon on that boss row when available; omit textual reward fallback. Never invent a vanilla reward for an incomplete shuffled seed. |
| Map entrance line omitted | The supplied plando disables dungeon entrance shuffle. OoT maps add the physical entrance hint only for shuffled entrances. | Preserve this condition. This case is expected for the supplied settings. |
| Held GIs now a little low | The accepted receipt origin is fixed per player form. | Raise that origin by two world units; preserve all accepted item scales and overhead bobbing. |
| Summer sun jerks while the item spins | Camera-facing counter-rotation is baked into tick-dependent vertices while the parent transform interpolates between ticks. Texture scroll also advances one full texel every four ticks. | Use native recorded matrix billboarding for both sun passes, stable local vertices and quarter-texel scroll every tick at the same average speed. Other seasons keep their existing behavior. |
| Custom GI still clips after several POCs | Recovered real-pack and native receipt framing fixtures already pass on the baseline. They do not establish the live archive selection, actual receipt route or camera in the reported session. | Preserve scales. Add automatic bounded `[ComboGI]` logs for native item identity, selected draw recipe roots and fitter context/outcome. Runtime clipping remains unresolved. |

The current native randomizer catalogs and supplied plando contain no separate bottled Seahorse item identity. Gold Dust uses MM's existing bottle handler (whose decomp draw enum is named `GID_SEAHORSE`). A separate Seahorse award is not covered by the alias count above.

## Evidence and limits

Affected production-boundary controls failed before their fixes for missing MM native aliases, cold owner tunic lookup, alias-only resource availability, absent compass reward sprites and tick-dependent sun vertices. Sanitized tests exercise production exporters, resolver/dispatcher bodies and resource-fit boundaries, with game rendering and the full scene loop outside that boundary.

The held framing fixture projects all 61 serialized authored GI models through the actual extracted MM receipt transforms, all five form cameras and a full spin. Recovered Din progressive sword resources contain six selected meshes; the two accompanying GI/icon archives add no parsed meshes. These checks support preservation and code correctness, not the user's screen clearance.

Supported runtime acceptance remains the user's MM-first all-items run with the actual complete mod stack, matching game modules and port archives from one candidate build. Verification results and exact build references are recorded in the downloadable recovery checkpoint.

## Runtime evidence to retain

1. Reproduce one clipping award with the same loaded archives and Alt Assets selection. Retain the application's log, the award name, player form and screenshot.
2. Find `[ComboGI] receipt` lines: `nativeMM`, `kind`, requested roots, pickup form, fitter context, `measured`, `fitScale` and `lift`. Native/legacy identity is logged when the authored renderer declines a model. A false `measured` or unexpected owner/root establishes a different investigation from a successful fit that still clips. The roots are requested resource names, not a complete record of deferred alias/Alt child selection.
3. Check each imported MM item family, both tunic body and effects, map/compass receipts with the information option on, actual shuffled boss rewards, and the summer sun through a complete receipt spin.
4. Accept or reject framing and smoothness from that session. Until then, this branch remains a candidate.

Logs are capped at 512 distinct receipt/fit records and require no manual CVar setup. They do not substitute another model when a graph cannot be measured.
