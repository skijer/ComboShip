# Native NPC model expansion and archive-editable fitting

Support the two uploaded Saria alternatives, ordinary Kokiri girl/twins, adult
Malon and young Malon as separate optional O2R replacements for native and static
actors. Reuse Fado's paired private-rig pattern. Cover En_Ko types 1,5,6,9,10 and
static girl; preserve separate Fado. Keep both Saria choices on the same actor
contract, enabled one at a time. Preserve source GLBs, accepted game builds,
placements, dialogue, story gates, animations and current unrelated replacements.

Cor's Fado hands-behind-back sway and seated crossed-arms failures establish a
new requirement: after one adapter build, geometry/rig fitting and clip-specific
corrections must be editable in O2R without another game build. Use private,
model-scoped normal-animation and limb-mesh resources. Preserve native clip
identity, timing and root motion, plus native interpolation/morphing. Reject
invalid clip layouts and fall back safely. Keep the legacy Fado seated-5 alias
compatible when its earlier adapter is reconciled into ComboShip.

Use focused source compilation, real animation sampling, actual resource
readback and offline pose review. Produce source/asset checkpoints for the next
cumulative build. Do not run an expensive whole-game build for this checkpoint,
promote the baseline, or equate offline verification with runtime acceptance.
Document visible fitting limitations and archive-only iteration instructions.
