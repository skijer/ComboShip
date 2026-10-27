# Final NPC source reconciliation — 2026-10-19

The cumulative source retains live actor routing, pose sampling, blink/expression
selection, tracking and Malon stance work. Model-archive refinements are separate
from engine behavior and require the matching external replacement pack.

| Actor | Selected asset | Disposition |
| --- | --- | --- |
| Anime Saria | POC12 | Accepted demonstrated model; configured runtime remains pending |
| Traditional Saria | POC11 | Retained accepted alternative; no Anime-only waiting correction |
| Kokiri twins | POC9 | Final model accepted; credits/posture guards retained |
| Young Malon | POC12 iris | Candidate; final iris acceptance and gameplay checks pending |
| Adult Malon | POC5 | Retained with blink, tracking, basket stance and native hand routing |
| Young Fado | NPC POC2 | Retained existing adapter and external pack |

Exact archive identities are in `final-npc-assets-20261019.json`. Load one complete
pack for each actor and only one Saria alternative. Full Saria packs include the
matching HD ocarina; do not stack the older standalone ocarina overlay on them.

## Recovered tooling

- Twins POC9's cumulative patch was applied over the full weekend/Malon tree.
  Shared test conflicts retain both twins credits/posture and Malon tracking/stance
  checks. Native skin, expression and pose hooks remain cumulative.
- Anime Saria's saved smile/waiting source was reconciled with the newer Malon arm
  and stance code. The final POC12 `Refinement.json` supplies the waiting profile:
  left hand offset `[0, -60, -250]`, rotation `[30, 0, 0]`, elbow swivel `-110`.
  The WIP `-220` profile is superseded. The source/model SHA gate protects other
  models and Traditional Saria.
- Young Malon's POC10 face/idle module supersedes its POC9 implementation. POC9's
  registered iris artwork remains required. Exact POC11 eye/hair source, registered
  root/scalp artwork, and the POC12 iris script/test are restored from their saved
  review packages. These are asset-only refinement recipes. The iris script
  requires the exact accepted POC11 parent hash and does not promote its candidate.

## Verification

Fresh Python discovery ran **118 tests: 103 passed, 15 skipped**. Skips explicitly
require external native/model review inputs; they are not passes. Native routing,
pose bridge/sampler, four Malon tracking modes, credits routing and all eight
real actor objects plus static Kokiri compiled and ran successfully. Those checks
exercise production slices; they do not link or run the complete game.

The isolated render/save/menu fixtures were updated only at their external observer
boundaries after reproducing missing declarations. Their focused reruns passed.
The repository's complete clang-format selector processed 1,995 source files with
clang-format 14.0.6. No generated asset headers were reformatted.

The canonical cumulative gate is recorded separately in the final cumulative audit.
Matching platform executables/port archives, installed model rendering, pose/blink/
tracking, and all three game modes still require evidence for this exact source.
No accepted master or model archive is replaced by this source checkpoint alone.
