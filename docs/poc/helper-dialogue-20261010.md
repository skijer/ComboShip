# Pause helper dialogue presentation — 2026-10-10

- Baseline: PR41 `d371fb26284b6b6e7d05ef0888fb90e09ca9fc3d` (tree `2994c8601669015bb7d283bb02242bf3871aa17d`). Separate candidate branch; no develop promotion.
- Scope: Phantom Hourglass, Shadow Crystal, all six active rods/scepters, all five Slate powers and their base/fallback guides. Pause help retains the localized colored name as a heading and the exact remaining power/control text, omitting grant/learned wording and its exclamation mark. The original pickup receipt builders/data remain untouched.
- MM custom helper boxes use icon byte `0xFE`, rather than inheriting the first available vanilla template's Ocarina icon. Native helper icons are unchanged. Encoded custom helpers use the existing full-width no-icon reflow path.
- Preservation: selected mode/rune snapshot, button glyphs/colors, every localized control, pickup grant/learned messages, save/ownership, A/L/C-Up routing, other item descriptions.

## Evidence

- Before implementation, production lookup/display plus the actual MM header decoder reproduced the two tool introductions and inherited icon. Expanded rod/rune tests reproduced grant/learned wording across all active powers and all three locales.
- `python3 tests/pause_tutorials/run_tests.py` and `--sanitize`: **429 checks** each (MM 199, OoT 195, actual MM equipment input routing 35), zero failures. ASan/UBSan enabled; leak detection disabled for the executor ptrace limitation.
- Both actual-header pause-description translation units pass syntax checks (MM reports 15 existing header warnings, OoT zero).
- `python3 scripts/diagnostics/run_mm_item_receipt_tests.py`: native/foreign Hourglass/Crystal, magic pickups, masks and donor exports pass across locales; pickup introductions and icons remain intact.
- Independent source review: no actionable findings.
- The full existing NEI gate is being collected separately. No linked-game or GPU/controller runtime proof is claimed.

## Runtime check

Open C-Up help for the two tools, every rod and each Slate power. Confirm the colored heading followed by controls, with no grant/learned introduction or borrowed Ocarina icon. Page through the longest tutorial, close it, confirm A wheel/equip and L navigation resume, and obtain an actual pickup to verify its original grant message. Test MM custom boxes and OoT tool/power text with the matching executable and archives; vanilla/Alt visual acceptance remains pending.
