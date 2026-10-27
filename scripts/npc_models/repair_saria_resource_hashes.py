"""Repair confirmed NativeNpcV1 private hash references without changing geometry or materials.

Archive::IndexFile indexes literal paths. A hash of a nonexistent base path
cannot select an alt-only entry. Rewrite that hash to the actual alt path.
Native references, existing literal hashes, markers and all other bytes stay
unchanged. --reference supplies full-pack names when repairing its overlay.
"""
import argparse
import hashlib
import json
import zipfile
from pathlib import Path
from native_assets import o2r as asset

def repair_records(records, reference_names=()):
    # Legacy Saria entry point retained for existing reproduction recipes.
    return asset.repair_native_npc_hashes(records, reference_names)


def repair(baseline, output, references=()):
    baseline, output = Path(baseline), Path(output)
    if baseline.resolve() == output.resolve():
        raise ValueError("Keep the baseline unchanged")
    with zipfile.ZipFile(baseline) as archive:
        records = {name: archive.read(name) for name in archive.namelist()}
    reference_names = set()
    for reference in references:
        with zipfile.ZipFile(reference) as archive:
            reference_names.update(archive.namelist())
    repaired, changes = repair_records(records, reference_names)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w") as archive:
        for name, data in sorted(repaired.items()):
            entry = zipfile.ZipInfo(name, (2026, 10, 20, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data, compresslevel=6)
    return dict(baseline=str(baseline), baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                candidate=str(output), candidate_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                changed_resources=len({entry["resource"] for entry in changes}), changes=changes,
                runtime_verified=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--reference", type=Path, action="append", default=[])
    args = parser.parse_args()
    print(json.dumps(repair(args.baseline, args.output, args.reference), indent=2))
