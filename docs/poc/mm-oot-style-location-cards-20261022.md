# Optional MM location-card presentation — 2026-10-22

Baseline: `4fff75754ef1578e586312fbf8c68e3adb99b4df`.

The off-by-default **OoT-style Location Cards** checkbox is under MM
Enhancements > Graphics. It applies to the next native location message.
It centers the existing decoded MM lettering near (160,120), draws a black
one-pixel outline and white face, removes the gradient and runs the native
TitleCard_Update fade (delay 20, duration 80). The native title's HUD policy
and eligibility remain owned by its existing caller. No scene, entrance,
showTitleCard, travel-request or save-history policy is changed.

The setting dispatches only when the current native scene table's nonzero
title ID equals the decoded message's ID. Day/dawn/night IDs 1BB2–1BB6 are
explicitly excluded. Actor-context boss names, the custom Black Market card
and the opening Lost Woods donor presenter are unchanged. The new fade uses
a private context and leaves an occupied native title context intact.
Opening another message or initializing the message system resets its owner;
a different ID, scene or message mode cannot draw or update the old title.

## Native lettering and limits

The default uses MM's already-loaded glyphs and decoded title, without
inventing or substituting a font. Printable NES glyphs, spaces, color controls,
quicktext markers and up to three lines are supported. Unsupported controls,
lines wider than 300 logical pixels, invalid font-buffer indices, empty titles
and buffer-size violations retain the complete native message presentation.
The native terminal slot at decodedTextLen must exist and contain MESSAGE_END;
excluded event/page/persistent terminators retain the native path as well.
No translated name is hardcoded. The baseline message loader currently forces
English; this change does not change that existing language policy.

This is an OoT-like layout/fade using original MM lettering, not OoT's wordmark
font and not a Djipi reproduction. No original location-card bitmap set or HD
art set is included. Actual ROM-decoded title coverage and visual resemblance
still require a configured game.

## Optional texture contract

A pack may provide a single 192×24 logical IA8 card (white lettering, black
keyline, transparent perimeter) at:

`custom/mm_location_titles/<numeric-language>/<four-digit-lowercase-text-id>`

For example, the native Southern Swamp title ID 0116 uses
`custom/mm_location_titles/1/0116` with the baseline English language value.
Use a packed native texture record or the normal replacement texture plus
metadata. The availability bridge checks both the resource and its .meta
entry in MM's own archive manager. Alt-only art is eligible only with Alt
Assets enabled. Normal load order, replacement metadata and native texture
dimensions control HD replacement; the renderer keeps the logical 192×24
rectangle. Missing art uses native glyphs immediately. Native availability
does not rely on the extension-only cache, which misses extensionless packed
records. In Combo, a missing MM owner never falls through to OoT's manager.

## Verification

`python3 -B tests/location_titles/run_tests.py` compiles the production C
presenter under ASan/UBSan and compares 120 frames to the actual native
TitleCard_Update body extracted from z_actor.c. It records glyph/texture draw
commands, checks placement, fade, ordinary/day/unknown/off eligibility,
unsupported-control and oversized-layout fallback, multiline layout,
interruption/reset/scene cleanup, save preservation and an occupied native
title context. The production C++ resource bridge compiles in standalone MM
and Combo configurations with archive API doubles and exercises native,
metadata, missing-owner and Alt availability. Graphics macros, resource
loading and engine integration are fixture boundaries.

No full build or gameplay run was available. Check ordinary titled arrivals,
re-entry, day changes, boss intros, Hide Title Cards, both custom cards,
native/Alt packs, opening another message mid-card and the next arrival after
an inter-game switch before accepting a configured debug build.

## Optional title-only HD font pack

In both solo 2Ship and ComboShip, the same off-by-default checkbox supports
optional `custom/mm_location_titles/font/<two-digit-lowercase-codepoint>`
resources. They are logical 16x16 IA8 glyphs, with ordinary HD texture metadata.
The decoded native text, advances, multiline spacing, eligibility and fade
remain authoritative. A whole-card override retains first priority. Otherwise
every printable character in the current title must have a font resource;
one missing character retains the complete native glyph path. This never
replaces dialogue fonts, boss names, day announcements or custom cards.

The separate original HD artwork pack supplies reviewed English letters,
digits and punctuation plus an independent HD `custom/skiejer_market/title`
replacement. It is an optional mount, not a new default, and does not contain
Djipi artwork. Unsupported characters fall back safely. Source fixtures prove
draw dispatch and resource routing, not configured GPU/pack-order acceptance.

## Native OoT place-card profile revision

An optional `custom/mm_location_titles/oot_native_v1/profile` marker selects a
versioned 24x24 IA8 glyph profile at `oot_native_v1/<hex-codepoint>`. Its measured
native crop advances and 24px baseline preserve the extracted source shapes;
no horizontal stretching to MM dialogue widths occurs. Whole-card overrides
still win. A missing resource or unsupported character keeps the entire MM
native glyph path, even when the older optional 16px font is also mounted.
Without the native marker, the older optional namespace remains compatible.
The checkbox stays off by default and applies equally to solo 2Ship and Combo.

The supplied user-native-art-derived candidate covers 61 of the 67 unique
English title IDs in the pinned MM scene table. East Clock Town, North Clock
Town, Honey & Darling's Shop, Post Office, Astral Observatory and Oceanside
Spider House retain native MM glyphs because native E/N/O/& were not present
in the available OoT place-title wordmarks. No missing letters were invented.
The accepted separate Black Market wordmark remains byte-identical.
