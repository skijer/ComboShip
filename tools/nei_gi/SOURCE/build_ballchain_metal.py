"""Polish the accepted Ball & Chain GI and held ball without changing geometry.

Reuses the forged sword texture-generation exporter. The original authoring
recipe remains the recoverable matte baseline; rebuild this candidate with:
    python3 tools/nei_gi/SOURCE/build_ballchain_metal.py --install
"""
import argparse
import importlib.util
from pathlib import Path
import shutil
import sys
import zipfile

GI_ROOT = Path(__file__).resolve().parents[1]
REPO = GI_ROOT.parents[1]
HELD_ROOT = REPO / "tools/nei_held"
sys.path.insert(0, str(GI_ROOT / "SOURCE"))
import models
sys.path.insert(0, str(HELD_ROOT / "SOURCE"))
import simple


def load_source(name, filename):
    spec = importlib.util.spec_from_file_location(name, filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


FORGED = GI_ROOT / "SOURCE/forged_swords/SOURCE"
exporter = load_source("ballchain_reflection_exporter", FORGED / "meshkit.py")
preview = load_source("ballchain_reflection_preview", FORGED / "preview.py")
forged = load_source("ballchain_metal_map", FORGED / "sword_forged.py")

# Neutral silver with the slight green cast of the supplied TP icon. Keep the
# aged trim and recessed seams on their original, fixed-UV forged textures.
METALS = {
    "iron": ((0.62, 0.66, 0.65), 0.60, 0.34),
    "edge": ((0.79, 0.83, 0.81), 0.76, 0.25),
    "steel": ((0.68, 0.73, 0.71), 0.82, 0.28),
}


def polish(model):
    for name, (color, strength, roughness) in METALS.items():
        material = model.materials[name]
        material.update(
            color=[1.0, 1.0, 1.0],
            tex=forged.metal_map(color, polish=strength),
            metal=0.95,
            rough=roughness,
            reflection=1,
        )
    model.notes.append(
        "Metallic POC1: brushed silver plate faces, polished bevels, spikes and "
        "shackle/links use camera-relative reflection mapping. Original dark "
        "seams, aged trim, geometry, normals, UVs and attachment scales retained."
    )
    return model


def build(install=False, archive=None):
    resources = {}
    for root, model in ((GI_ROOT, models.ball_chain()), (HELD_ROOT, simple.ball())):
        polish(model)
        exporter.ROOT = preview.ROOT = root
        stats = exporter.export_resources(model)
        preview.checkpoint(model, stats)
        source = root / "RESOURCES" / model.prefix
        for path in sorted(source.iterdir()):
            if path.is_file():
                resources[model.prefix + path.name] = path.read_bytes()
        if install:
            target = REPO / "soh/assets/custom" / model.prefix
            # Material changes must never rewrite the accepted mesh or scale.
            for filename in ("mesh_opa_vtx", "scale_mtx"):
                if (source / filename).read_bytes() != (target / filename).read_bytes():
                    raise RuntimeError(f"Unexpected Ball & Chain geometry change: {target / filename}")
            shutil.copytree(source, target, dirs_exist_ok=True)
    if archive:
        archive.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as package:
            for name, data in sorted(resources.items()):
                for entry in (name, "alt/" + name):
                    info = zipfile.ZipInfo(entry, (2026, 10, 9, 0, 0, 0))
                    info.compress_type = zipfile.ZIP_DEFLATED
                    package.writestr(info, data)
        with zipfile.ZipFile(archive) as package:
            assert len(package.namelist()) == 2 * len(resources)
            for name, data in resources.items():
                assert package.read(name) == package.read("alt/" + name) == data
        print(f"Packaged {len(resources) * 2} base/Alt entries: {archive}", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", action="store_true")
    parser.add_argument("--pack", type=Path)
    args = parser.parse_args()
    build(args.install, args.pack)
