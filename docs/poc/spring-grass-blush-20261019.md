# Spring grass blush correction — October 19, 2026

Parent: weekend source `520d7b6dc64ab5742226b9b2f998a34bcd6b4a9e`, tree
`35535ac14ed5c3324655f79dd09bcacdd4e2de36`, prepared on PR #44's
`43131342882972eeb4a257599ec8b061c7dc6d43` baseline.
Candidate branch: `poc/spring-grass-blush-20261019`.

Cor reports that the pink changes never reached spring grass and approves a
blush-pink grass treatment. This is approval of the palette direction, not
runtime acceptance of this new source candidate.

## Cause and bounded correction

Both hosts deliberately excluded general ground grass from Spring. The
existing full-strength blush applied to selected foliage/understory, so
strengthening that value again could not affect the excluded ground. MM's
randomizer grass wrappers were additionally limited to Road to Southern Swamp.

SoH now applies the existing `F6C1D9` full blush and 1.8 lift to ground grass
through its immutable texture-copy path. The existing green-pixel mask and
authored atlas bounds continue protecting other pixels. Canopy and understory
recipes retain their previous colors.

MM adds the reviewed grass/ground identities to Spring's selective image-copy
path. Ground does not gain a whole-triangle grayscale tint: mixed brown stone,
bark, alpha and source texture bytes remain intact. Already draw-scoped grass
trim is excluded from the new copy list to avoid double tinting. Private MM3D
and native-solo forest grass images receive the same treatment.

Seven registered native ground images use indexed colors. Their exact palette
bindings were read from the native donor display lists. Spring decodes CI4/CI8
into private RGBA copies, using 8x/4x byte scaling for the intact native load
commands. Shared palettes are neither modified nor substituted from a cold
asset namespace. Existing Road CI8 handling is retained.

MM's explicit replacement leaf/blade children now receive Spring blush in
eligible outdoor scenes beyond the Road. Flower/stem/reward children retain
their original material ownership. The other seasons keep their existing
replacement-grass scope. Season changes rebuild the eligible path list.

## Preservation contract

No scene archive, model, collision, geometry, UV, actor placement, reward logic,
save state, audio, weather/light controller, foliage canopy recipe or particle
asset changes. Summer/Native/Off restore the original resources. Autumn and
Winter retain their existing palettes and known source behavior. The accepted
masters and the existing weekend checkout remain unchanged.

The candidate retains originals and season copies through queued renderer
use, obeys native/Alt and generated scene aliases, and respects external cache
replacement and cleanup. It changes only registered greenery; an unrecognized
private pack identity is not automatically included.

## Verification and handoff

Focused tests first reproduced unchanged Spring ground in both hosts, missed
MM replacement grass outside the Road, and untreated native indexed grass.
Their corrections are checked through production pixel, binding, renderer and
raw-upload functions. The native donor is 36,251,870 bytes, SHA-256
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.

The recovery package records final commands, exit statuses and logs, including
the 567-material Autumn/Winter preservation suite, actual indexed-image
fixtures, both hosts' grass actor paths, sanitizer checks, real-header native
compilation for Combo and standalone modes, and clang-format 14.0.6. A missing
resource-init record in the new upload fixture was exposed by the first MM
sanitizer pass and corrected in the fixture before the final rerun.

Full application linking, Windows CI and configured gameplay appearance remain
separate checks. No installed build or GPU footage of this candidate has been
observed. The decisive visual comparison is Spring versus Summer at the same
location/time with the same packs and Alt setting, including ground, native
and randomized clumps, carried grass and reward markers. Check Hyrule Field,
Kokiri Forest, Termina, Road to Southern Swamp and the private forest routes
that are installed; then return to Summer/Native and re-enter the scene.

No new large texture archive is required. Restore the existing weekend source
kit before fetching the incremental spring bundle. Exact candidate commit/tree,
bundle hashes, restoration steps and verification results accompany that delta.
The change is prepared for source integration, not master promotion.
