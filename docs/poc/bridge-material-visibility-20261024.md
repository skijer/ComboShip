# Bridge material and visibility repair

The supplied Lake clip shows the authored cave switching between grass and a
building facade as the camera turns. Its floor has a narrow turquoise channel.
The user also reports that both the Zora's Fountain/Snowhead mesh and shortcut
are absent, and confirms that the scene is shared by child and adult.

Baseline: local `bb6f927eda710812daceb73f9124e0d536701918`, tree
`292685496dd311b2b1b97c0847eea537988b6c5a`, identical to public PR61
`602e430613ce3045f782f1ec9b76bd5878879784`. The log's last session identifies
`9c3e41ee793250c0a8593361369ac8d64226bac9`, the CI merge of `e60ed69e` into
`5f8966fa`; it already contains the prior cached-vertex Snowhead guard repair.
The later fixture-only changes do not change these production bridge paths.

Preserve the original archives, accepted placements, native scene collision,
waterboxes, alternate setups, native Ice Cavern entrance, adult icebergs,
manual Prelude actor ownership, standalone guards and unrelated repairs.
Keep develop/master unmerged and distinguish source/native test proof from
configured runtime acceptance.

The channel is authored intentionally: eight water quads follow a shallow
20-unit channel, flowing toward the cave when the lake surface reaches it.
Do not remove this geometry merely because its current material looks wrong.

Confirmed material cause: SetupDL25 enables two-cycle rendering; repeating
MODULATEI or MODULATEIA_PRIM in both cycles discards the first result and reads
texture slot 1. These bridges explicitly load slot 0. Production GenerateCC
and the backend's second-cycle texel mapping therefore select inherited scene
textures. Use the first cycle for the authored material and pass COMBINED
through the second cycle, retaining the existing fog/render mode.

Snowhead visibility requires investigation of the active resource graph and
provider readiness. A previous passing cached-pointer fixture is not proof
that the supplied configuration works. Record any unestablished dependency
without calling it fixed.

Material evidence: the new native CaveDraw replay executes emitted setup and
combiner commands through production GenerateCC. The baseline failed first on
stone (`texture0=0 texture1=1`), then independently on water after correcting
only stone. With both corrected, the sanitized native Lake suite passed for
native/replacement resources, full/drained/refilled water, scroll wraps,
collision/camera and provider lifecycle. A second shared regression fails all
six baseline material calls and passes all six corrected Lake/Fountain calls.

The initial material fixture run needed the existing local dependency include
paths; after adding them, its texture factory/metadata and ascent/snow checks
passed. No production change was made for that environment setup.

The Fountain investigation also reproduced a distinct ownership failure:
generic resource loading can select the globally active MM manager while the
correct Fountain resources belong to OoT. All three backdrop/vertex lookups
now use the registered OoT owner directly, retaining that manager's Alt and
fallback behavior. The native regression fails the old path with a foreign
active manager and passes the correction. The user's log does not establish
that this was the condition during the reported Fountain visit.

Integrated verification passed:

- `python -B tests/regional_passages/run_tests.py --sanitize`: full regional
  geometry, material, native actor/collision, endpoint, handoff and boot checks.
- `python -B tests/regional_passages/run_presentation_tests.py --syntax`:
  regional presentation and complete native OoT/MM graphics translation units.
- The existing material pack/metadata suite, valley ownership/lifecycle suite
  and Snowhead production object/clearance suite passed with the SDK includes.

Status: material and ownership repairs implemented and native-test verified.
No linked executable or configured post-repair gameplay is claimed by these
checks. Fountain absence in the user's active pack remains unproven; bounded
readiness diagnostics are included with the functional repairs. Runtime
acceptance and promotion remain pending.

The user's subsequent closer clip explicitly rejected the water-strip
presentation. The separate bounded channel revision in
`lake-flowing-inlet-20261024.md` now supersedes the earlier decision to retain
the trough's exact shape. Its scope changes only the authored channel/intake;
the protected native scene, main route and cave endpoint remain intact.

Independent initial review found no blocking issues. Two minor diagnostic
limitations remain: a cached readiness report can show vertex count zero, and
repeated native collision-allocation failures can repeat an initialization
message. They do not change provider behavior. The new channel review found a
fog/alpha blocker and two intake bevel gaps; both were reproduced by failing
native checks and corrected before publication. The full regional sanitizer
suite passed again after those corrections.

Latest user steering specifies downward outward flow into Lake Hylia. The
current water builder, spill and UV direction follow
`lake-outflow-direction-20261024.md`, superseding the earlier inlet direction.

The subsequent vertical-spill correction replaces that shallow runout with an
actual vertical water face at the edge. See `lake-vertical-spill-20261024.md`
for current shape, native checks, independent review and preview limitations.
