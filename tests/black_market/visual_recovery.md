# Shop visual recovery milestones

The original POC11 archive remains unchanged. This source is being recovered
after the transient checkout was lost; each completed piece is checkpointed.

The native X geometry and offline adapter are restored. Fresh
`python3 tests/black_market/check_lock_mesh.py` passes asymmetric-center,
front/back enclosure, triangle/model clearance, UV animation, selected alpha
and invalid-bound checks, including a fast-math compilation.

At that geometry milestone the native controllers still used their old ring.
The continuation now wires the shared fitted X in both native controllers;
real-GBI blend/depth/texture and locked-only capture fixtures pass. Keeper lean
selection/placement and refreshed previews are now implemented and verified
against their native source/asset contracts. See
`docs/poc/market-final-presentation-20261017.md` for current evidence.
Full native linking and gameplay remain untested.
