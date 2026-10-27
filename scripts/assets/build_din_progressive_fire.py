"""Extend the exact Combo POC1 archive; preserve every existing asset byte."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import zipfile

BASE_SHA256 = "4c6765488e1669ff94a7a504e2b2c507c893f3b673fd7143f93c551db2b17895"
ALIASES = {
    "objects/din_fire_sword/progressive/adult/SwordDL":
        "objects/object_link_boy/DinSleekEquipmentPOC1_OOT_Adult/SwordDL",
    "objects/din_fire_sword/progressive/child/SwordDL":
        "objects/object_link_child/DinSleekEquipmentPOC1_OOT_Child/SwordDL",
    "objects/din_fire_sword/progressive/bgs/SwordDL":
        "alt/objects/object_custom_equip/gCustomLongswordDL",
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def crc64(path):
    value = 0xffffffffffffffff
    for byte in path.encode("ascii"):
        value ^= byte << 56
        for _ in range(8):
            value = ((value << 1) ^ (0x42f0e1eba9ea3693 if value >> 63 else 0)) & 0xffffffffffffffff
    return value

def verify(candidate, baseline):
    with zipfile.ZipFile(baseline) as src, zipfile.ZipFile(candidate) as out:
        assert out.testzip() is None
        assert len(out.namelist()) == len(set(out.namelist()))
        assert set(out.namelist()) - set(src.namelist()) == set(ALIASES) | {
            "DinProgressiveFirePOC1.json", "DinProgressiveFire_README.txt"}
        for name in src.namelist():
            assert out.read(name) == src.read(name), name
        for target, source in ALIASES.items():
            assert out.read(target) == src.read(source), target
        # Follow the copied wrappers through material, texture, mesh and vertex
        # resources. References must resolve from the candidate by their CRC64.
        hashes = {crc64(n): n for n in out.namelist()}
        seen = set()
        def visit(name):
            if name in seen:
                return
            seen.add(name)
            data = out.read(name)
            if len(data) < 64 or struct.unpack_from("<I", data, 4)[0] != 0x4f444c54:
                return
            at = 72
            while at < len(data):
                w0, _ = struct.unpack_from("<II", data, at)
                op = w0 >> 24
                if op in (0x20, 0x31, 0x32, 0x33, 0x34):
                    hi, lo = struct.unpack_from("<II", data, at + 8)
                    ref = (hi << 32) | lo
                    assert ref in hashes, (name, at, hex(ref))
                    visit(hashes[ref])
                    at += 16
                else:
                    at += 8
        for name in ALIASES:
            visit(name)
        return {"preserved_entries": len(src.namelist()), "new_mesh_aliases": len(ALIASES),
                "dependency_closure_entries": len(seen), "zip_crc": "pass"}

def main():
    p = argparse.ArgumentParser()
    p.add_argument("baseline", type=Path)
    p.add_argument("output", type=Path)
    args = p.parse_args()
    assert sha(args.baseline.read_bytes()) == BASE_SHA256, "Wrong baseline archive"
    readme = """Din Fire Sword / Shield - Progressive Combo POC1

REQUIRES the matching progressive-fire engine fix on top of ComboShip
a6ae6e1b84108de857dbf75d65b3a734346c5cb4. The archive ALONE cannot restore
fire on upgraded swords in older builds.

After installing the matching build: close the game, replace the old
Din_Fire_Sword_Shield_Combo_POC1.o2r with this archive in both mods/soh
and mods/2ship. Keep the current player body/hair pack. Restart and enable
Alternate Assets and Din Fire Sword. For upgraded held swords, the enabled
fire option selects its fitted Din blade after ordinary equipment/PAK hooks.
Disable fire to restore their normal model choice. Fire Damage is separate
and optional; shield settings are unchanged.

Covers the actual NEI chains: Kokiri -> Razor -> Gilded; Master -> True
Master; Biggoron -> Great Fairy. Native MM Razor/Gilded/Great Fairy are
also covered. Upgraded held swords use the existing Din blade, sized for
the current child/adult form or the full-length two-handed blade. Numeric
damage, upgrade ownership, reach and progression stay with NEI.
Cane of Byrna, Trident and Four Sword are excluded from this expansion.

The prior archive's 259 entries are preserved byte-for-byte, including
shield, sword meshes, textures, cosmetics and stowed equipment. Only
three private held-sword aliases and these candidate notes are added.
Existing stowed equipment is retained; no GI or icon replacement is added.

Code/module tests and archive checks are separate from in-game proof.
Runtime remains untested. Check draw/swing/stow for each upgrade, Alt/fire
on/off, cosmetic colors, normal grass/enemy hits, and the optional Fire
Damage toggle in both games. This is a candidate, not an accepted master.

Rollback: restore the original archive; the code falls back to NEI's native
upgrade models if the private progressive assets are absent.
"""
    meta = {"candidate": "Din Fire Sword Shield Progressive Combo POC1",
            "baseline": args.baseline.name, "baseline_sha256": BASE_SHA256,
            "engine_base": "a6ae6e1b84108de857dbf75d65b3a734346c5cb4",
            "engine_candidate": "poc/din-progressive-fire-20260930",
            "matching_engine_fix_required": True, "runtime_tested": False,
            "accepted_or_promoted": False, "aliases": ALIASES,
            "chains": ["Kokiri -> Razor -> Gilded", "Master -> True Master", "Biggoron -> Great Fairy"],
            "preservation": "All 259 baseline entries byte-identical; new private sword aliases only"}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.baseline) as src, zipfile.ZipFile(args.output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as out:
        for entry in src.infolist():
            out.writestr(copy.copy(entry), src.read(entry.filename))
        additions = {target: src.read(source) for target, source in ALIASES.items()}
        additions["DinProgressiveFirePOC1.json"] = (json.dumps(meta, indent=2) + "\n").encode()
        additions["DinProgressiveFire_README.txt"] = readme.encode()
        for name, data in additions.items():
            info = zipfile.ZipInfo(name, (2026, 9, 30, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            out.writestr(info, data)
    result = verify(args.output, args.baseline)
    result.update({"file": str(args.output), "sha256": sha(args.output.read_bytes()),
                   "bytes": args.output.stat().st_size})
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()
