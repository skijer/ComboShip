# Master Sword hilt symmetry review

- [Four close inspection views](master_hilt_review_views.png)
- [Mirrored front-outline comparison](master_hilt_review_mirrored_outline.png)

The accepted Master Sword's two guard wings have exactly mirrored vertex
positions in both the authored geometry and exported GLB. Both wings measure
10.8061 model units wide and 10.2286 units high. The central hilt mount also
matches its reflection exactly. The leather wrap spirals around the grip,
so that small surface detail is intentionally directional.

The collection preview uses an orthographic camera at 15 degrees of yaw and
12 degrees of elevation, with light from the left. The combined angles put
equal-height wing tips about 1.16 model units apart vertically on screen;
lighting and reflective highlights also differ between the sides. The distant
view makes those differences harder to interpret.

Close inspection views show the same accepted mesh with effects disabled.
The flat front view removes lighting and reflections so its shape can be
compared directly. The angled view reproduces the collection preview camera.
All lighting remains an offline approximation.

The straight-on guard silhouette has zero differing pixels when mirrored.
Tiny surface differences from triangulating quantized quads are below one
native vertex step and do not alter that outline.

No model, game resource, or runtime rendering code was changed for this review.
