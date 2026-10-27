# Fire and Ice Rod cast POC6

Cor provisionally accepted the POC6 generated artwork and requested a recovery
package for combining this work with other ComboShip changes.

Base source: `a9f16e2a514f1d29c3b278594f6a5417157d1cd1`.
The candidate combines the preceding narrow rod geometry/presentation changes
with the accepted natural Fire and fractured Ice release materials.

## Scope and preservation

Fire charge/focus uses rising embers and flame detail. Fire/Ice release has a
softer tinted front, separate crest/flow sampling and moving Ice facets.
POC6 holds POC5 geometry, timing and vertex tint/alpha fixed for the material
comparison. Only the final artwork/resource bindings differ from POC5.

Ice charge/focus, including falling icicles and rotating shards, is protected.
Gameplay, collision, charge thresholds, RNG, projectiles, projectile wakes,
trails, impact effects and Light effects are unchanged by this candidate.
Both game wrappers use their existing resource-owner and matrix/render paths.

The two private native RGBA32 resources are:

- `objects/nei_rod_cast_poc6/fire_natural`
- `objects/nei_rod_cast_poc6/ice_fracture_release`

Each is 512x512 pixels with a 32x32 logical tile. Generated RGB and transparency
are retained, with resolution conversion only. Exact source/output identities
are in `tools/nei_rod_cast_poc6/material_manifest.json`; tests pin them directly.
Regenerate `soh.o2r` when building so both resources reach the OoT owner archive
used by both games.

## Evidence and acceptance

The preceding POC5 full effect suite and native SoH/MM presentation compile
checks passed before the POC6 material path edits. Those logs remain historical
evidence, not a fresh complete POC6 game build.

POC6's offline sampler compiles and renders. The Ice charge preview matches
the reconstructed POC5 control pixel for pixel. POC6 wrapper copies originally
changed only four material paths per game. The merge package updates resource
identity assertions and material expectations to those final paths and checks
the cumulative patch can reconstruct all candidate bytes from its baseline.

The offline movie uses production meshes at 20 updates per second, standard
alpha and no added bloom. User acceptance is provisional acceptance of this
artwork/preview direction. A complete game build, loaded pack configuration,
in-game blend ordering and visual/performance acceptance remain untested.

After combining changes, run `python3 -B tests/nei_used_fx/run_tests.py` in the
configured full repository, regenerate port archives and inspect both games
with the user's current packs and Alt Assets on/off. Confirm the protected Ice
charge plus small/large Fire and Ice releases indoors and outdoors.
