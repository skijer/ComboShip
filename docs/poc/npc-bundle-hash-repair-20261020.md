# Full NPC bundle literal-hash repair POC — 2026-10-20

Baseline source: `ed3284104`, the isolated Saria/TLUT checkpoint above the
Goron effect-light repair. Baseline asset selection:
`ComboShip-NPC-Mods-20261020.zip`, SHA-256 `ce4c3c87415f0f9d49d8d1e1aa920788d1cc4aaff8d293ca05ab5426a909870b`.
The user broadened the missing-limb report to all recovered NPCs and supplied a
Kokiri lineup image. The image was directly inspected: it shows selective
missing heads/upper torsos while several limbs still render. This is symptom
evidence, not proof of its exact mounted pack/build or acceptance of this POC.

## Audit findings and boundary

Read-only literal-index scans covered 25 distinct archives from the recovered
bundle and npc-downloads, deduplicated by exact archive SHA-256. Each hash was
checked against real resource names, with separately declared native paths from
OoT/MM source headers. No synthetic canonical aliases were added for Alt files.

Five of the seven latest selected model packs share the same confirmed defect:
2,885 commands in 840 display lists hash nonexistent canonical private paths
whose payloads exist only under `alt/`. Native `Archive::IndexFile` indexes the
literal filename; Alt policy is applied after a hash resolves to a name. These
failed wrapper calls match the reported selective missing body parts. Only
those unresolved hash payloads were changed.

Young Fado POC2 and adult Fado HD POC3 have no unresolved hash references and
remain byte-identical. All nine MM3D rig-realignment controls also have zero
unresolved/unknown references: Brac, Bremor, Doyle, Mack, Worker, Cremia, Japas,
Receptionist and Romani. They are not extra installs in this seven-pack bundle.
Earlier Saria/Twins POC4, young Malon POC5/POC8, Twins POC5/shoulder-input and
Anime Saria POC11 reproduce the defect, but are superseded review inputs.
The separate Saria HD overlay needs its full model: its two missing vertex hashes
resolve against either full Saria archive and were repaired using that context.

Adult Malon's two native basket texture hashes are valid declared dependencies
(`gMalonAdultBasketInsideTex`, `gMalonAdultBasketTex`); they were not rewritten.
No other unknown full-pack hashes remain. Source declarations establish the
resource contract, not the presence or correctness of mounted donor payloads.

## Latest selections and preservation

| Selection | Repaired commands | Affected display lists | Original SHA-256 | Candidate SHA-256 |
| --- | --- | --- | --- | --- |
| Young Fado POC2 | 0 | 0 | `8f4d8994fc8c7f29b460dc0217281e389b24db7684acc7b840c0472149127649` | `8f4d8994fc8c7f29b460dc0217281e389b24db7684acc7b840c0472149127649` |
| Adult Fado HD POC3 | 0 | 0 | `a8cecbac16e9323a402e2316cb875030889424221255c940f6e54a9a783f3bc7` | `a8cecbac16e9323a402e2316cb875030889424221255c940f6e54a9a783f3bc7` |
| Kokiri twins POC9 | 545 | 164 | `b8c8bd1c80332d625fad152f8bbcadf78124c75086cea0cca6ba5da84d91ae02` | `2723332dce6ee36c5049236d8dc7ddd8613eaf2f31bd97b975623b5c3b495164` |
| Young Malon POC12 | 667 | 160 | `db8af18b76ce4bbf616e737b78d2b7af670d8adaefac9224b0808db12069ec28` | `e66a17e21327ff3351622068c78fda3eea47030932e62c6f032b60ad920bfa41` |
| Adult Malon POC5 | 521 | 139 | `0fe195bc5eaa4f5170ba91aae1638febd577a6f91595d713b6c3b3a959cf49a2` | `8ffe4694a9eb8a444490da4cdcc3cb10d7c47ca5024ca6b9ae2b5df1136b77fa` |
| Anime Saria POC12 | 533 | 149 | `94a6235b27c2329feac19b5297aa517a67f425e667edf918f723718e4b6e070f` | `bbcf3551ab4a9eb5184f28d7403795dbab2240d7a62f9d6f1ff459ac63e92562` |
| Traditional Saria POC11 | 619 | 228 | `b47bd839335e91ff123d04c0c362cfc08b1f6bba9cf9aece01e12bb10e5bd0f5` | `0c496f8d5328efb97ac1ca3496a23eaa9c466fc81aebec862ff2afcd92e324d0` |

Each affected pack retains exactly its original resource-name set and byte
lengths. Independent comparison checks all bytes outside the recorded eight-byte
hash fields. Geometry, vertices, UVs, textures, material commands, skin palettes,
rigs, animation clips, face states and manifests were not regenerated. Both Fado
archive files are preserved as original ZIP bytes, not merely equivalent
extracted resources. The optional overlay retains four entries; only four hash
fields in its two prop display lists change. Style choices and Original-Notes
are retained. No scene/actor/progression/placement source changed in this phase.

The new full transport bundle is
`ComboShip-NPC-Mods-LiteralHash-POC1-20261020.zip`.
Final identity: see the SHA-256 below, recorded after README-only finalization.

SHA-256: `723ff4ef7a75b96557dc52634bc1c009e87ac4a0ea1f686272b038680e028df7`.

Its source_manifest.json contains archive hashes, source identities, per-pack
repair counts and combined-mount audit summaries. The detailed hash offsets and
before/after targets are retained separately in evidence/hash-deltas.json.

## Production output correction

The bounded helper is in native_assets/o2r.py and supports only the four confirmed
private families: object_sa, object_kw1, object_ma1 and object_ma2 NativeNpcV1.
It resolves alt-only private hashes against the final resource map; native
resource hashes, canonical private entries that actually exist, and unrelated
namespaces remain untouched. It returns a copy rather than changing source data.

The base native_npc_models.py compiler now uses the tested O2R writer at its final
export boundary. Its existing ZIP_DEFLATED compression, sorted resource order
and fixed 2026-10-18 timestamp are preserved. Default O2R writer callers retain
ZIP_STORED and insertion order. The historical Saria repair CLI delegates to the
same bounded helper. Exact Twins verification comparisons now use the actual
literal hashes/normalized expected wrappers. No generation/refinement geometry
logic was changed. Other historical refiners have independent ZIP writers;
recovered or refined output must still pass the explicit repair/literal audit
before distribution. This does not claim every historical script was updated.

## Verification and configuration

| Check | Result |
| --- | --- |
| Original five selected packs, literal index | Confirmed 2,885 unresolved commands |
| All seven corrected/preserved full packs | PASS; zero unresolved references |
| Common + Anime Saria | PASS; zero collisions and unresolved references |
| Common + Traditional Saria | PASS; zero collisions and unresolved references |
| Each choice with corrected HD overlay last | PASS; exactly the three intended prop collisions, zero unresolved references |
| Per-resource non-hash bytes and source identities | PASS; both Fado archive hashes exact |
| Generic helper/writer regression before normalization | Expected FAIL, seven family/output cases |
| Model Python suite after output correction | PASS: 125 tests, 16 optional unavailable-source skips |
| Actual writer compression, timestamp, order and unrelated defaults | PASS |
| Final ZIP CRC and all eight contained O2R hashes after README-only finalization | PASS |
| Runtime, GPU appearance and user acceptance | Untested |

Use five Common files plus one Saria style in `mods/soh/`, with OoT Alternate
Assets enabled, for combined ComboShip and solo OoT. Combined MM placed Saria
uses that OoT owner/toggle too. For MM-only/solo 2Ship Saria, use the chosen Saria
pack in `mods/2ship/`, enable MM Alternate Assets, and retain the required native
`oot.o2r` companion for rig, animations and eyes. Common models replace OoT NPCs.

Move older affected model versions and the old HD overlay outside active mods.
Never install both Saria styles together. Both full Saria choices already contain
the HD Fairy Ocarina. If keeping the separate corrected overlay, mount it after
the selected full Saria pack; its only intended overlaps are Ocarina, OcarinaGrip
and Props/Textures/FairyOcarinaHD. No unexpected cross-pack collisions were found,
including unchanged Fado resources.

Reproduce the seven-pack repair and audits without regenerating any model:

```sh
python -B scripts/npc_models/build_npc_hash_bundle.py \
  /path/to/ComboShip-NPC-Mods-20261020.zip /path/to/new-candidate-directory \
  --overlay /path/to/Saria_Fairy_Ocarina_HD_POC11_Overlay.o2r
python -B -m unittest discover -s scripts/npc_models -p 'test_*.py'
```

The required runtime check is the supplied lineup and each installed model's
head/body/hands, blink, idle and seated pose with Alt on/off, then dialogue,
re-entry and Saria's ocarina/song route. A remaining Fado visibility defect
requires separate diagnosis; a clean hash audit is not Fado runtime proof.
The earlier partial-CI8 TLUT source fix remains a separate compiled-fixture
milestone and is not supplied by a model archive. No executable build or sacred
master promotion is claimed. Recovery retains the original bundle unchanged,
source parent above, exact source manifests and the deterministic repair recipe.
