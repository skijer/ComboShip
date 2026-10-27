# Chamber title card POC

Parent: `0cd35e4d` (cumulative October 26 integration plus fixture path repair).
The separately preserved Zelda source checkpoint is `eae07d49`; the integration
parent already contains it. Portal routes were reported tested by cor. This
change does not imply acceptance of the newer title behavior.

Show the ordinary native Chamber of the Sages place card after a successful
scene-portal return and once when entering through the physical tunnel.
Reuse `textures/g_pn_14/gChamberOfTheSagesTitleCard*Tex` so native and HD/Alt
replacements, language selection, native fade timing and HUD cosmetics apply.
The dialogue's light-blue tint is unrelated and remains unchanged.

Use the existing owned return completion and a hysteresis boundary at the
chamber opening (x within 100, y -150..130; arm in the tunnel at z -1160..-720,
enter at z <= -1240). The enclosure opens at z -1200 in the preserved POC5
manifest/build recipe. No synthetic scene transition or scene-number mutation.
Wait for control and existing cards; respect explicit hide/veto hooks. Clear
transient state on scene/room/setup/file/session changes. Tunnel entry works
with travel Off. Native story scenes and MM remain untouched.

Preserve geometry, collision, materials, Zelda, camera/confirmation, unlock
rules, arrival coordinates and save/respawn restoration. No pack changes.

Checks: native adapter tests for all returns, tunnel entry/re-entry, safety,
stale visits, title policy, native texture identity, languages and fades;
full affected translation-unit compilation and existing temple regressions.
Configured gameplay/HD replacement appearance and full executable linking
remain separate runtime/build checks. Keep master/develop unmerged.
