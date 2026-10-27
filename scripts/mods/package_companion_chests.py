#!/usr/bin/env python3
"""Package the approved chest and Midna assets without changing their artwork."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import xml.etree.ElementTree as ET
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

CHEST_SOURCE = "cdbaf1edd3a764382f5187698a33675aaa1d80c60b148bdff6e626e91da71a94"
CHEST_FIXED = "d07ab7d516d2c8da2df1eb45dfc0b1941ba3d0ea90b3a1dada5fa76a2b74a183"
MIDNA_SOURCE = "dbe4552da4110b845c00a78ea9b02dcc8239ec18214166120de46ddfc2f8c176"
PRIVATE_MM = "objects/object_box/cor_3ds_chests_mm/"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_pack(path, expected):
    if digest(path) != expected:
        raise ValueError(f"Unexpected source checksum: {path}")
    with ZipFile(path) as archive:
        if archive.testzip() is not None or len(set(archive.namelist())) != len(archive.namelist()):
            raise ValueError(f"Corrupt or duplicate archive entries: {path}")
        return {name: archive.read(name) for name in archive.namelist()}


def write_pack(path, resources):
    path.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(path, "w", ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in resources.items():
            info = ZipInfo(name, (2026, 9, 23, 0, 0, 0))
            info.compress_type = ZIP_DEFLATED
            archive.writestr(info, data)
    with ZipFile(path) as archive:
        assert archive.testzip() is None


def relocate_mm_chests(source):
    mapping = {name: PRIVATE_MM + name.rsplit("/", 1)[-1] for name in source}
    assert len(mapping) == len(set(mapping.values())) == 54
    candidate = {}
    references = 0
    for old, original in source.items():
        data = original
        if data.lstrip().startswith(b"<"):
            for element in ET.fromstring(original).iter():
                ref = element.get("Path")
                if ref is None:
                    continue
                target = "alt/" + ref
                assert target in source, (old, ref)
                data = data.replace(f'Path="{ref}"'.encode(), f'Path="{mapping[target]}"'.encode())
                references += 1
        candidate[mapping[old]] = data

    # Reverse path edits and compare EVERY resource, not just visible textures.
    for old, original in source.items():
        data = candidate[mapping[old]]
        if data.lstrip().startswith(b"<"):
            for element in ET.fromstring(data).iter():
                ref = element.get("Path")
                if ref is not None:
                    assert ref in candidate, (old, ref)
            for before, after in mapping.items():
                data = data.replace(f'Path="{after}"'.encode(),
                                    f'Path="{before.removeprefix("alt/")}"'.encode())
        assert data == original, old
    assert references == 114
    roots = [name for name in candidate if re.fullmatch(
        re.escape(PRIVATE_MM) + r"gChest(?:Body|Lid)(?:Major|Minor|Heart|SmallKey|Token|Junk)DL", name)]
    assert len(roots) == 12
    assert not any("Boss" in name or "gBoxChest" in name for name in candidate)
    return candidate, mapping


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-chests", type=Path, required=True)
    parser.add_argument("--fixed-soh-chests", type=Path, required=True)
    parser.add_argument("--midna", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    original = read_pack(args.original_chests, CHEST_SOURCE)
    fixed = read_pack(args.fixed_soh_chests, CHEST_FIXED)
    midna = read_pack(args.midna, MIDNA_SOURCE)
    assert len(fixed) == 54 and len(midna) == 91
    args.output.mkdir(parents=True, exist_ok=True)
    soh = args.output / "mods/soh/3DS_RandoChests_SoH_Fixed.o2r"
    mm = args.output / "mods/2ship/3DS_RandoChests_MM_POC1.o2r"
    soh.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.fixed_soh_chests, soh)
    converted, mapping = relocate_mm_chests(original)
    write_pack(mm, converted)
    for engine in ("soh", "2ship"):
        target = args.output / f"mods/{engine}/Midna_Companion_POC3.o2r"
        shutil.copyfile(args.midna, target)
        assert target.read_bytes() == args.midna.read_bytes()
    report = {
        "original_chests_sha256": CHEST_SOURCE,
        "soh_fixed_sha256": digest(soh),
        "mm_chests_sha256": digest(mm),
        "midna_sha256": MIDNA_SOURCE,
        "chest_resources": 54,
        "chest_entrypoints": 12,
        "mm_internal_references": 114,
        "midna_resources": 91,
        "preservation": "MM chest resources match the original after reversing path substitutions. "
                        "SoH chest and both Midna packs are byte-identical copies of the pinned inputs.",
        "boss_chest_replacements": 0,
        "runtime": "The preview was accepted; this ComboShip adaptation needs in-game testing.",
        "mm_resource_mapping": mapping,
    }
    (args.output / "asset_verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: value for key, value in report.items() if key != "mm_resource_mapping"}, indent=2))


if __name__ == "__main__":
    main()
