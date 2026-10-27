# Bridge surface, Snowhead mist and Summer firefly follow-up

User-requested scope: replace the Lake Hylia bridge tunnel's grass-bearing material with clean stone, put irregular sand on roughly half the dry floor around its existing stream, close the right-edge black-backdrop sliver, use the actual Winter mist appearance at the Snowhead approach, and make Summer-night fireflies brighter and more numerous in both games.

The tunnel currently uses one texture for every solid face. A plain texture replacement is archive-editable; spatially separating sand needs source. Keep the existing collision/water geometry untouched and partition only visual floor quads into separately replaceable stone and sand. Use existing clean Lake Hylia stone and sand as native fallbacks, with proper per-texture tile dimensions; retain the established stone override contract. Give the backdrop a small visual overlap without moving its collision or the portal.

Reuse Winter's untextured 15-vertex feathered mesh and material for Snowhead's local mist. Keep local clocks/anchors and weather independent. Snow sprites use intensity as opacity, bright primitive RGB, explicit translucent/no-fog state, and smooth camera-depth fade. Preserve geometry, progression, manual actors, arrival poses and NEI shimmer.

Increase both Summer swarms from 56 to 96, raise pulse visibility and widen their existing halo slightly. Keep daylight motes, warm RGB, textures, toggles, entry/edge/night fades, pause behavior and single draw pass. Keep preview motion in parity.

Alternatives considered: replacing a single tunnel texture cannot create an irregular spatial sand region; billboard dust does not match Winter's mist. Visual-only partitioning and reuse of the real Winter mesh are the direct implementations of the requested appearance.

Source/native evidence and configured gameplay acceptance remain separate. Preserve public baseline eb3c7cadd6419afd4a68015358880b8d27d0601e, keep develop/master untouched, and deliver a separately named recoverable source candidate for the next combine.

Later authorized extension: move the Snowhead trigger, full-valley shimmer and local cold effects deeper along the existing valley so the player can walk toward a future horizon mesh. Place the mouth/return at measured-floor X3600/3500, recompute shimmer bounds, and use shared deep-valley weather anchors while preserving the ascent and fitted valley geometry. Termina grass tint remains a separate read-only diagnosis recorded in its POC note.
