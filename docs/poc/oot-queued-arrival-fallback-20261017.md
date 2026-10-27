# Preserve queued OoT arrival respawn ownership

Baseline: `bridge/audit-merge-20261008` at `1bbb94e692629c0d910f97b57dd7cd02240d3775`.
This is the tested merge of PR #43: its gate, Windows build and Linux build
passed in run `38872186638`. The recovered forest connection and Hyrule Field
resource correction are included in that baseline.

The OoT resume path evaluated `Entrance_OverrideNextIndex` for the ordinary
Market return before consuming an already composed dungeon or boss-recovery
arrival. Native composition can materialize a positioned grotto return and
change `respawnFlag`/RETURN data. The queued OoT consumers do not reset that
state for an ordinary entrance, so the fallback can pollute another arrival.

The fallback is now the lazy final arm of the destination selection, after
both queued consumers. The existing forest-route consumption and final
explicit Farore arrival retain their order. Ordinary Market/forest returns
still use native fallback composition. No MM, rendering, archive or shuffle
permutation change is part of this follow-up.

The production title/arrival fixture first failed with a native-composition
boundary that writes positioned RETURN data. It now asserts that neither
dungeon arrivals nor boss recovery evaluate that fallback, both with and
without a stale forest route. The ordinary return controls assert that the
fallback remains active. The callback boundary is scripted; the actual title
and forest arrival bodies and native engine state types are executed.

The Hyrule Field `ResourceManager.cpp` blob remains exactly
`15cee1d98dcb873a0807516571aba0a441732c40`, identical to repair `e4813206`.
Both hosts' production Alt-cache fixture passes for textures and parent
display lists across three scene visits, native/Alt switching, missing
overrides and manager/owner changes. This retains the repair intended for
the grass patches, Granny's shop, Romani's house and the castle mesh; installed
appearance still requires the configured Field round trip.

Validation record: the failing arrival regression was reproduced before the
source change; forest/arrival/render-linkage/camera/opening/progress/portal-save
fixtures, the sanitized boss-recovery fixture, Farore suite, native OoT title
translation-unit syntax, clang-format 14 and whitespace are the focused checks
for this candidate. Hosted results belong to the exact published follow-up;
the earlier successful build does not establish the new commit's package result.

This is an integration candidate, with no develop/master promotion or new
installed-game acceptance claim. The unchanged parent remains the rollback.
