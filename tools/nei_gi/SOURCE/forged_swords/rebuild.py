"""Rebuild selected forged sword resources and checkpoints in an isolated process."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "SOURCE"))
import meshkit
import preview
import sword_forged


def rebuild(items, output, install=False):
    output = Path(output).resolve()
    meshkit.ROOT = preview.ROOT = output
    manifest = json.loads((HERE / "MODEL_MANIFEST.json").read_text())
    provenance = json.loads((HERE / "PROVENANCE.json").read_text())
    repo = HERE.parents[3]
    for slug in items:
        model = sword_forged.build(slug)
        resources = output / "RESOURCES" / model.prefix
        if resources.exists():
            shutil.rmtree(resources)
        stats = meshkit.export_resources(model)
        preview.checkpoint(model, stats)
        directory = output / "CHECKPOINTS" / slug
        metadata_path = directory / "checkpoint.json"
        metadata = json.loads(metadata_path.read_text())
        digest = hashlib.sha256((directory / (slug + ".glb")).read_bytes()).hexdigest()
        metadata["markers"]["imported_approved_model"] = digest == manifest[slug]["glb_sha256"]
        metadata["source_provenance"] = {
            "bundle": "NEI_Sword_GI_Latest_20261003",
            "art_checkpoint": provenance["art_checkpoint"],
            "authoring_recipe": "../../SOURCE/forged_swords/SOURCE/sword_forged.py",
            "glb_sha256": digest,
            "approved_glb_sha256": manifest[slug]["glb_sha256"],
            "resource_sha256": {
                path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                for path in sorted(resources.iterdir()) if path.is_file()
            },
        }
        metadata_path.write_text(json.dumps(metadata, indent=2) + "\n")
        if install:
            target = repo / "soh/assets/custom" / model.prefix
            if target.exists():
                shutil.rmtree(target)
            shutil.copytree(resources, target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("items", nargs="*")
    parser.add_argument("--output", type=Path, default=HERE.parents[1])
    parser.add_argument("--install", action="store_true")
    args = parser.parse_args()
    items = args.items or list(sword_forged.BUILDERS)
    unknown = set(items) - sword_forged.BUILDERS.keys()
    if unknown:
        parser.error("Unknown forged sword: " + ", ".join(sorted(unknown)))
    rebuild(items, args.output, args.install)


if __name__ == "__main__":
    main()
