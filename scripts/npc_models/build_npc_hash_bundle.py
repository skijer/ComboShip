"""Build a hash-only repair of the recovered seven-model NPC selection.

Original style choices, unchanged Fado archives and historical notes are kept.
Literal-index audits run for individual packs, both active style combinations,
and the optional corrected overlay. No geometry/model regeneration is done.
"""
import argparse
import collections
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import zipfile
from native_assets import o2r as asset

ROOT = Path(__file__).resolve().parents[2]
STAMP = (2026, 10, 20, 0, 0, 0)
EXPANDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def records_from(raw):
    with zipfile.ZipFile(io.BytesIO(raw)) as archive:
        return {name: archive.read(name) for name in archive.namelist()}


def encode(records):
    data = io.BytesIO()
    with zipfile.ZipFile(data, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, raw in sorted(records.items()):
            entry = zipfile.ZipInfo(name, STAMP)
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, raw)
    return data.getvalue()


def declared_native_names():
    names = set()
    for game in ("soh", "mm"):
        for header in (ROOT / game / "assets").rglob("*.h"):
            for raw in re.findall(rb'"__OTR__([^"\n]+)"', header.read_bytes()):
                names.add(raw.decode("ascii"))
    return names


def audit(records, native_names):
    literal = {asset.crc64(name): name for name in records}
    native = {asset.crc64(name): name for name in native_names}
    missing, declared = collections.Counter(), collections.Counter()
    commands = 0
    for name, raw in records.items():
        if len(raw) < 72 or raw[0] != 0 or raw[4:8] != b"TLDO":
            continue
        offset = 72
        while offset + 8 <= len(raw):
            opcode = struct.unpack_from("<I", raw, offset)[0] >> 24
            size = 16 if opcode in EXPANDED else 8
            if offset + size > len(raw):
                raise ValueError("Truncated display list: " + name)
            if size == 16 and opcode != 0x33:
                commands += 1
                high, low = struct.unpack_from("<II", raw, offset + 8)
                hashed = high << 32 | low
                if hashed not in literal:
                    if hashed in native:
                        declared[native[hashed]] += 1
                    else:
                        missing[f"{hashed:016x}"] += 1
            offset += size
            if opcode == 0xDF:
                break
    return dict(hash_commands=commands, unresolved_commands=sum(missing.values()),
                unresolved_hashes=dict(missing), declared_native_dependencies=dict(declared))


def preserve(original, revised, changes):
    assert original.keys() == revised.keys(), "Resource names changed"
    offsets = collections.defaultdict(list)
    for change in changes:
        offsets[change["resource"]].append(change["offset"])
    for name, before in original.items():
        after = revised[name]
        assert len(before) == len(after), name
        cursor = 0
        for offset in sorted(offsets[name]):
            assert before[cursor:offset] == after[cursor:offset], name
            cursor = offset + 8
        assert before[cursor:] == after[cursor:], name


def combine(paths, packs, native_names):
    active, owners, collisions = {}, {}, []
    for path in paths:
        for name, raw in packs[path].items():
            if name in active:
                collisions.append(dict(resource=name, previous=owners[name], current=path,
                                       identical=active[name] == raw))
            active[name], owners[name] = raw, path
    return active, dict(active_packs=paths, collisions=collisions, audit=audit(active, native_names))


def build(bundle, output, overlay=None):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    native_names = declared_native_names()
    original = records_from(Path(bundle).read_bytes())
    source = json.loads(original["source_manifest.json"])
    manifest = dict(parent_bundle_sha256=digest(Path(bundle).read_bytes()),
                    repair="Literal hashes for alt-only NativeNpcV1 payloads; hash fields only",
                    runtime_retested=False, models=[])
    package = {name: raw for name, raw in original.items() if name.startswith("Original-Notes/")}
    package["Original-Notes/Original-README.txt"] = original["README.txt"]
    package["Original-Notes/Original-source_manifest.json"] = original["source_manifest.json"]
    package["Original-Notes/Original-SHA256SUMS.txt"] = original["SHA256SUMS.txt"]
    packs, evidence = {}, []
    for model in source["models"]:
        name, raw = model["file"], original[model["file"]]
        assert digest(raw) == model["sha256"], name
        before = records_from(raw)
        after, changes = asset.repair_native_npc_hashes(before)
        preserve(before, after, changes)
        check = audit(after, native_names)
        assert check["unresolved_commands"] == 0, (name, check)
        filename = name.replace(".o2r", "_LiteralHash_POC1.o2r") if changes else name
        revised = encode(after) if changes else raw
        (output / filename).parent.mkdir(parents=True, exist_ok=True)
        (output / filename).write_bytes(revised)
        package[filename], packs[filename] = revised, after
        record = dict(model, file=filename, original_file=name, original_sha256=model["sha256"],
                      sha256=digest(revised), bytes=len(revised), resources=len(after),
                      changed_display_lists=len({item["resource"] for item in changes}),
                      changed_hash_commands=len(changes), non_hash_bytes_preserved=True)
        manifest["models"].append(record)
        evidence.append(dict(file=filename, before=audit(before, native_names), after=check, changes=changes))
    common = [item["file"] for item in manifest["models"] if item["file"].startswith("Common/")]
    styles = [item["file"] for item in manifest["models"] if item["file"].startswith("Choose-One-Saria/")]
    assert len(common) == 5 and len(styles) == 2
    combinations = []
    for style in styles:
        _, result = combine([*common, style], packs, native_names)
        assert not result["collisions"] and result["audit"]["unresolved_commands"] == 0, result
        combinations.append(result)
    if overlay:
        old_overlay = Path(overlay).read_bytes()
        before = records_from(old_overlay)
        after, changes = asset.repair_native_npc_hashes(before, packs[styles[0]])
        preserve(before, after, changes)
        name = "Optional-Saria-HD-Overlay/" + Path(overlay).name.replace(".o2r", "_LiteralHash_POC1.o2r")
        revised = encode(after)
        package[name], packs[name] = revised, after
        (output / name).parent.mkdir(parents=True, exist_ok=True)
        (output / name).write_bytes(revised)
        manifest["optional_overlay"] = dict(file=name, original_sha256=digest(old_overlay), sha256=digest(revised),
                                          changed_hash_commands=len(changes), non_hash_bytes_preserved=True)
        allowed = {"alt/objects/object_sa/NativeNpcV1/Props/" + suffix for suffix in
                   ("Ocarina", "OcarinaGrip", "Textures/FairyOcarinaHD")}
        for style in styles:
            _, result = combine([*common, style, name], packs, native_names)
            assert {item["resource"] for item in result["collisions"]} == allowed, result
            assert result["audit"]["unresolved_commands"] == 0, result
            combinations.append(result)
    manifest["active_configuration_checks"] = combinations
    package["source_manifest.json"] = json.dumps(manifest, indent=2).encode()
    package["SHA256SUMS.txt"] = "".join(f"{digest(raw)}  {name}\n" for name, raw in sorted(package.items())
                                          if name.endswith(".o2r")).encode()
    package["README.txt"] = README.encode()
    final = output / "ComboShip-NPC-Mods-LiteralHash-POC1-20261020.zip"
    final.write_bytes(encode(package))
    evidence_path = output / "evidence"
    evidence_path.mkdir(exist_ok=True)
    (evidence_path / "repair-manifest.json").write_text(json.dumps(manifest, indent=2))
    (evidence_path / "hash-deltas.json").write_text(json.dumps(evidence, indent=2))
    return dict(bundle=str(final), sha256=digest(final.read_bytes()), full_models=len(manifest["models"]),
                optional_overlay=bool(overlay),
                changed_models=sum(bool(item["changed_hash_commands"]) for item in manifest["models"]),
                configurations=len(combinations))


README = """ComboShip NPC model packs — LiteralHash POC1 — October 20, 2026

INSTALL FOR COMBO / SOLO OOT
1. Close the game. Move the old affected NPC packs and any old Saria HD overlay
   outside the active mods folder. Keep backups; do not stack old and new versions.
2. Copy all five .o2r files from Common into mods/soh/.
3. Choose ONE Saria style from Choose-One-Saria and copy that .o2r into mods/soh/.
4. Enable OoT Alternate Assets. Mount these actor packs after other packs that
   replace the same actors. Never install both Saria styles together.
5. Both full Saria packs already contain the HD Fairy Ocarina. The corrected
   overlay is optional; if used, mount it AFTER the selected full Saria pack.
   Do not reuse the old overlay, which can shadow the repaired prop references.

MODE OWNERSHIP
Combined ComboShip and solo OoT use the OoT owner/toggle, including combined MM
placed Saria. For MM-only/solo 2Ship Saria, install only the selected Saria pack
(and optional corrected overlay) in mods/2ship/, enable MM Alternate Assets and
retain the required native oot.o2r companion for rig/animations/eyes.
The Common packs are OoT NPC replacements.

WHAT CHANGED
Five selected NativeNpcV1 packs had hashes of nonexistent canonical resources
whose payloads existed only under alt/. Only those 8-byte hash fields changed.
Geometry, textures, materials, rigs, animation and placement/progression source
were not regenerated. Young and adult Fado archives are byte-identical to the
original selection: their hash audits found no such defect. This does not prove
every Fado runtime route; any remaining missing head requires separate diagnosis.

Use the matching integrated NPC/Fado support build. The separate source repair
also fixes partial CI8 palette uploads; the model bundle cannot patch an EXE.
The seven selections/styles match the original recovered bundle. Earlier
review packs are not additional installs. Archive hashes and per-pack repair
counts are recorded in source_manifest.json;
Original-Notes contains unchanged historical
reviews, which do not establish runtime acceptance of this new candidate.

VERIFICATION / STATUS
Literal hash-index checks pass per full pack and for Common plus each Saria
choice. No unexpected cross-pack resource collisions; only the optional overlay's
three intended prop resources overlap. Declared native dependencies were checked
against source resource headers; their actual mounted payloads remain required.
Hash-only preservation was verified. This is a review POC, not GPU/runtime proof
or a master promotion. Check full bodies/heads/hands, blink, standing/seated/idle
poses, Saria ocarina and song interaction, then re-entry with Alt on/off.
"""


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--overlay", type=Path)
    args = parser.parse_args()
    print(json.dumps(build(args.bundle, args.output, args.overlay), indent=2))
