# Complete the accepted NEI presentation in native 2Ship

The user tested ComboShip PR 24 and reports vanilla held orientation/effects in MM. Inspection confirms that PR head `7e3b036502291cc86405c64e3b90fcb57b6e4eab` added MM GI/pickup presentation but left MM's actual held/used drawers on their old paths. No extra O2R can connect those missing call sites. The user has authorized completing the port, updating PR 24, and producing a runtime build.

The accepted donor is the `soh/` implementation already present at this baseline, imported from SoH PR 17 `2ce2e5a28098261943c25f8a1ecebe730c1d06ea`. Reuse its artwork, geometry policies and calibrated poses. Finish the native MM equivalents rather than passing MM player/state pointers to OoT functions.

Required visible results:

- Fire/Ice/Light use the accepted held rods, tip energy, charge, release, projectile, trail and burst presentations. Fire charge follows accepted Light geometry with the private fire material. Light's accepted effects stay exact. Ice keeps its accepted trail material.
- All three local held rods and tip effects hide during first-person single-shot aiming, with projectiles, controls and reticle retained. Other players/clones retain their own third-person draws.
- Stable volley/shot identity prevents Fire/Ice backward interpolation when a volley disappears or a slot is reused. A stopped projectile retains its launch heading while fading.
- All other accepted held/used model replacements reach MM's active native paths: Leaf, Ball and Chain, Shovel, Mitts, Gust Jar, Beetle, Spinner, Somaria, Whip, Switch Hook, Lantern and Time Gate. Feather/Cape/Minish Cap remain GI-only where the donor has no held renderer.
- Native MM wrist matrices and hand selectors provide the grip/pose corrections. Preserve transformation/form, local/remote, missing-resource and transition behavior. Din model rig alterations are outside scope.
- Carry the accepted associated item-use corrections where MM still lacks them: Leaf safe completion/stop, one put-away sound, Ball and Chain roll/input gating, Lantern clasp/stow, Whip raised-arm continuity, instant Switch Hook and Time Gate prop hiding during use. Retain native MM gameplay/state/audio integration; do not transplant OoT player logic wholesale.

Assets already live in bundled `soh.o2r`. Use explicit owner routing with stable path lifetime and owner-aware existence checks for native MM render submission; support Alt on/off and atomic fallback for incomplete bundles. Do not duplicate the asset tree or introduce new art. Both binaries/archives remain part of the ordinary ComboShip package.

Preserve the accepted OoT implementation, MM GI/icon/progression corrections, PAK/voice, renderer performance, weather/audio and all prior cumulative work. User-reported SoH Leaf acceptance is not proof of MM runtime behavior. Require native MM production-path tests, cross-engine geometry/command comparisons, compilation, existing relevant regression gates and independent review. Publish to PR 24, without merging or promoting a master; label runtime checks pending until the user tests the new build.

Additional user report (September 29): on native MM, Din's fire sword cannot damage Chuchus in Termina Field or any enemies with its checkbox enabled or disabled, while grass cutting remains possible. Investigate and correct the proven sword-damage integration fault as a narrow addition before publication; retain the accepted artwork and exclude unrelated Din rig orientation changes. Passing existing isolated damage tests is not proof that this runtime symptom is resolved.
