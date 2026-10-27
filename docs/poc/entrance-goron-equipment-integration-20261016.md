# Entrance, Goron and held-equipment integration candidate

This combines the runtime corrections with the subsequently requested Four
Sword/Byrna source checkout on draft PR #43. The accepted base remains unchanged.

| Input | Revision | Integration |
| --- | --- | --- |
| Entrance build baseline | `2069d37e2a9338494acff3a70ca56ddaf2e035fa` | Common baseline |
| Generation/reset checkpoint | `b189b0ec1ca765b52b9aa21a32fdc75fe526da54` | Preserved merge parent |
| Goron/Somaria checkpoint | `d5a8024b36b78b6ad9459f4a87361f2e1a616461` | Preserved merge parent |
| Four Sword/Byrna checkout | `590df6b80bc030f6e62e76a9fd3e42b299da9ae3` | Exact 35-file source import |

The equipment checkout is one commit over the common baseline and has no file
overlap with the runtime checkpoints. All 35 imported files byte-match that
donor. Its original recovery record describes the earlier source-only handoff;
this integration follows the later request to incorporate that checkout into
the current candidate. The donor checkout itself is unchanged.

The shared sword generator and all 27 generated wrappers use the production
XML loader's `CallDisplayList` command. Byrna retains its item identity and
existing stance hook while mapping B to ordinary sword melee. Authored meshes,
materials, textures, grip matrices, manifests, MM Byrna mapping and native sword
durability code are unchanged by the import. Packaging must rebuild `soh.o2r`
alongside the matching modules; an older archive retains the malformed wrappers.

The seven combined entrance/Goron/Somaria early checks pass in 9.06 seconds
locally. The canonical equipment suite and its real XML-loader and native
action/charge/durability sanitizer fixtures also pass on the combined source.
The complete cross-dungeon suite passes under ASan/UBSan on the combined runtime
source; the equipment import changes none of its production or fixture inputs.
Independent review found no blocking defect in either the runtime correction
or the exact equipment donor. Changed production lines pass clang-format 14;
the staged diff passes whitespace checks.

CI executes the small production entrance, audio, collision, cane and held
equipment fixtures immediately after checkout, before dependency installation
and the long regression/build stages. Existing canonical tests remain enabled.

Full configured platform builds and installed-game acceptance remain separate
proof. See the generation/reset, Goron/Somaria and equipment recovery records
for exact fixture boundaries and unresolved runtime cases. In particular, the
original missing-entrance trigger, pack-specific walking/punching failures,
reported Somaria grotto presentation and held-model visibility require the
matching rebuilt package and installed configuration. This candidate does not
promote the accepted base.
