# Young Malon POC10: preserve orbital volume

User feedback: POC9 improved the eyes and shading but gave the face the flattened/mask-like effect seen in the earlier Twins iteration. POC9 is not an accepted face baseline.

Evidence: the broad POC9 orbital ellipse moved 101 original source points, including 68 outside the visible eye aperture, by up to .0206913 source units outside the aperture. An original-depth control restored the eyelid shape but reintroduced oblique iris breakup.

Bounded correction: derive a shallow convex eye interior from the actual visible iris/white samples. Apply it only inside the registered painted aperture, with an inward distance taper. Keep every surrounding skin/rim point at the original source XYZ. Preserve the POC9 eye artwork, all blink textures, idle arm rotations, other poses, body, weighted shoulder skin, scarf clasp, skeleton, native fallback and tracking.

Candidate: Young_Malon_Native_NPC_POC10.o2r, SHA256 40eef326945f6d3d8181c1ad1e77e4f96bcd099ad928ba533d500e6bebb7072c. Rebuilt from immutable POC8 body/source plus the POC9 artwork and idle improvements.

Validation: 99 model Python tests pass, including a red/green guard that forbids changing source depth outside the painted aperture. Canonical archive verification passes. 1,047 original quantized head records outside the aperture retain their XYZ; 309 POC9 resources are byte-identical, including all textures, poses and weighted skin resources. Inspect actual open/half/closed archive renders at front, both diagonals and near-profile. The full candidate can be reproduced byte-for-byte by applying the saved head-only overlay to the exact POC9 archive.

This remains an unaccepted offline review candidate. No game build, publication or merge. The accepted Twins pass is untouched.
