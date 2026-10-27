# OoT ground tint polish — candidate

User authorized this narrow tint integration and publication on 2026-10-26.
Recorded parent: `9f23b84859ac545a283c155b6642aea302aad414` (PR64).
Input: `ComboShip_OoT_Ground_Tint_Polish_Source_POC1_20261026.zip`.
Input SHA256: `33761e311c32c3a8668320993213c5c29b3454794e19cc32006bfb3d9fd11cd0`.

Default Ground palettes gain smooth shadow lift: Autumn uses warm F2B45C,
320/256 light scale and 24/255 lift; Winter retains E5EDF3 and 461/256 scale
with 128/255 lift; Spring retains F6C1D9 and 461/256 scale with 24/255 lift.
Cache identities include lift. Atlas masks, protected pixels, alpha, material
table, actor tints, canopy/understory, explicit private-forest overrides and MM
remain unchanged. Summer/off retain original texture restoration.

The supplied POC passed standalone pixel/cache tests and actual Kokiri texture
processing. Integration additionally runs the repository's real Fast::Texture
tests. The preview is texture-space only: fog, lighting, mod order and configured
runtime appearance still require the user's test. No master/develop promotion.

Focused runtime check: Spring -> Autumn -> Winter -> Summer in Kokiri Forest
and Hyrule Field, compare ground/wall trim against nearby actors, verify stone
and bark remain original and Summer restores the original grass.

Integration validation: the native runner passes all four real Fast::Texture
tests, including 599 material cohorts and the new 65,536-color packed control.
The new dark-ground assertion fails on the parent and passes on the candidate.
Actual 2048x2048 Kokiri grass and 1024x1024 wall textures retain source pixels,
alpha, non-green pixels and metadata. Native grass/tree draw-scope tests pass;
CI-policy tests pass 25/25; changed C++ files pass clang-format 14.0.6. An
independent read-only review found no blocking or minor issue. Hosted CI and
matching Windows-package validation remain pending at this source checkpoint.

Rollback: retain PR64 and its matching complete Windows package. Install this
candidate separately with its matching executable, DLLs and both rebuilt port
archives; retain original game archives, saves and the accepted optional-pack
order. This tint patch requires no new scene archive or load-order change.
