# Native MM runtime feedback

Baseline: PR 24 head `1d542f5318a5e5ca0625c1f4313988f1f87b1456`, tree `8a3dccd531d38cee6d101a71d0b9a567af563219`. The supplied log identifies tested merge `de7c8d6f65c20e7edb9966c71e462be4a19139ab`, which has the same tree. Original checkout and uploaded evidence remain unchanged.

Required corrections from the September 29 follow-up:

- Remove remaining vanilla Ice Rod projectile overlay while retaining the accepted custom projectile/trail and all working Fire/Light rod effects.
- Restore the accepted ice/light arrow iterations in native MM where a missing port or resource binding is proven. Ice uses the existing Henriko snowflake artwork. No replacement artwork is authorized.
- Keep Mogma Mitts equipped when native wall climbing starts, preserving magic use and ordinary cancellation/stow.
- Resolve the four-square foreign Progressive Scale message icon. Trace reward progression and save/reload behavior before changing their semantics: the log records an earlier scale grant, so the reported first-chest Golden Scale is not by itself proof of a tier bug.
- Remove Leaf/Shovel's incidental cutscene letterboxing during ordinary use while preserving real camera/cutscene/targeting behavior.
- Leave Din's Fire casting behavior alone.

Preserve all accepted models, Alt fallback, both games' native state layouts, custom GI and Cane progression, PAK/voice, weather, performance and previous integration work. No master promotion. Deliver a reviewed candidate through the existing PR/build workflow under the prior port authorization; distinguish component tests and platform builds from user runtime acceptance.

Evidence: the supplied six-second clip shows a chest pickup labeled Progressive Scale and the four-square message icon. Other symptoms are user-reported. The log spans multiple sessions; earlier save-slot and missing-texture errors must not be attributed to the latest build without matching timestamps.
