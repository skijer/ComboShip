#!/usr/bin/env python3
"""Exercise adult setup/override, native MM flex slots, and OoT DL matrix reads.

No ROM geometry is embedded: the fixture contains skeleton topology and matrix
command addresses only. --oot-archive verifies/re-derives that metadata from the
read-only archive used by the game. Native skeleton traversal, matrix conversion,
graphics ABI, adult setup, and final adult limb override run unchanged.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_back_equipment_tests import function
from run_mm_nei_tests import flags


# Equipment variants selected by the adult override, including the empty sheath
# used to preserve the source limb's canonical flex matrix slot.
ADULT_EQUIPMENT_DISPLAY_LISTS = (
    "gLinkAdultLeftHandNearDL", "gLinkAdultLeftHandClosedNearDL",
    "gLinkAdultLeftHandHoldingMasterSwordNearDL", "gLinkAdultLeftHandHoldingBgsNearDL",
    "gLinkAdultRightHandNearDL", "gLinkAdultRightHandClosedNearDL",
    "gLinkAdultRightHandHoldingHylianShieldNearDL", "gLinkAdultRightHandHoldingBowNearDL",
    "gLinkAdultRightHandHoldingOotNearDL", "gLinkAdultRightHandHoldingHookshotNearDL",
    "gLinkAdultWaistNearDL", "gLinkAdultSheathNearDL", "gLinkAdultMasterSwordAndSheathNearDL",
    "gLinkAdultHylianShieldAndSheathNearDL", "gLinkAdultHylianShieldSwordAndSheathNearDL",
    "gLinkAdultRightHandHoldingMirrorShieldNearDL", "gLinkAdultMirrorShieldAndSheathNearDL",
    "gLinkAdultMirrorShieldSwordAndSheathNearDL",
)


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 64

    def number(self, size, signed=False):
        value = int.from_bytes(self.data[self.pos:self.pos + size], "little", signed=signed)
        self.pos += size
        return value

    def string(self):
        size = self.number(4)
        value = self.data[self.pos:self.pos + size].decode()
        self.pos += size
        return value


def archive_metadata(path):
    with zipfile.ZipFile(path) as archive:
        skeleton_path = "objects/object_link_boy/gLinkAdultSkel"
        skeleton_data = archive.read(skeleton_path)
        reader = Reader(skeleton_data)
        assert reader.number(1) == 1  # Flex
        assert reader.number(1) == 2  # LodLimb
        limb_count, matrix_count = reader.number(4), reader.number(4)
        assert reader.number(1) == 2
        assert reader.number(4) == limb_count
        paths = [reader.string() for _ in range(limb_count)]
        limbs, resources, consumers, display_lists = [], {}, [], set()

        def display_list(path):
            data = archive.read(path)  # A missing required path fails the check.
            assert data[4:8] == b"TLDO", f"Expected display-list resource: {path}"
            assert len(data) >= 80 and (len(data) - 72) % 8 == 0, path
            resources[path] = hashlib.sha256(data).hexdigest()
            display_lists.add(path)
            return data

        for path in paths:
            data = archive.read(path)
            resources[path] = hashlib.sha256(data).hexdigest()
            limb = Reader(data)
            assert limb.number(1) == 2
            assert limb.number(1) == 0  # No skin payload
            assert limb.string() == ""
            assert limb.number(2) == 0
            assert limb.number(4) == 0
            assert limb.string() == ""
            limb.pos += 12 + 6  # Unused skin translation/rotation
            assert limb.string() == ""
            assert limb.string() == ""
            near, far = limb.string(), limb.string()
            position = [limb.number(2, True) for _ in range(3)]
            child, sibling = limb.number(1), limb.number(1)
            assert limb.pos == len(data)
            limbs.append(dict(path=path, position=position, child=child, sibling=sibling,
                              near=near, far=far))
            if far:
                display_list(far)
            if near:
                dl = display_list(near)
                # OTR v0 display-list commands begin on the aligned 8-byte boundary.
                offset = 72
                while offset + 8 <= len(dl):
                    w0 = int.from_bytes(dl[offset:offset + 4], "little")
                    w1 = int.from_bytes(dl[offset + 4:offset + 8], "little")
                    op = w0 >> 24
                    if op == 0xDF:
                        break
                    if w0 >> 24 == 0xDA and w1 >> 24 == 0x0D:
                        consumers.append(dict(path=near, command=offset, address=w1,
                                              matrix_offset=w1 & 0x00FFFFFE))
                    offset += 16 if op in {0x20, 0x24, 0x31, 0x32, 0x33, 0x35} else 8
        for symbol in ADULT_EQUIPMENT_DISPLAY_LISTS:
            display_list("objects/object_link_boy/" + symbol)
        resources[skeleton_path] = hashlib.sha256(skeleton_data).hexdigest()
        return dict(limb_count=limb_count, matrix_count=matrix_count, limbs=limbs,
                    consumers=consumers, display_list_paths=sorted(display_lists),
                    resource_sha256=resources)


def write_fixture(metadata, destination):
    lines = ["// Topology and matrix-reference metadata; no vertices or textures.",
             f"static constexpr unsigned kArchiveMatrixCount = {metadata['matrix_count']};",
             "static const ArchiveLimb kArchiveLimbs[] = {"]
    for limb in metadata["limbs"]:
        position = ",".join(str(v) for v in limb["position"])
        lines.append("    {{%s},%s,%s,%s,%s}," %
                     (position, limb["child"], limb["sibling"],
                      json.dumps(limb["near"]), json.dumps(limb["far"])))
    lines += ["};", "static const MatrixConsumer kConsumers[] = {"]
    for consumer in metadata["consumers"]:
        lines.append("    {%s,0x%X}," % (json.dumps(consumer["path"]), consumer["matrix_offset"]))
    lines += ["};", "static const char* kArchiveDisplayLists[] = {"]
    lines += ["    " + json.dumps(path) + "," for path in metadata["display_list_paths"]]
    lines.append("};")
    destination.write_text("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oot-archive", type=Path)
    args = parser.parse_args()
    metadata = json.loads((Path(__file__).parent / "archive_fixture.json").read_text())
    if args.oot_archive:
        actual = archive_metadata(args.oot_archive)
        if actual != metadata:
            raise SystemExit("Archive skeleton/consumer metadata differs from the checked fixture")
        print("PASS actual oot.o2r topology and segment-0D consumers match fixture", flush=True)
    adult = (ROOT / "mm/mods/items/logic/adult_link_render.cpp").read_text()
    player = (ROOT / "mm/src/code/z_player_lib.c").read_text()
    graph = (ROOT / "mm/src/code/graph.c").read_text()
    graphics = (ROOT / "mm/src/code/stubs.c").read_text()
    production = re.search(r"static const char\* sLimbNearDL\[.*?\n};", adult, re.S)[0] + "\n"
    production += "\n".join(re.findall(r"^static Gfx\* sDL_\w+;[^\n]*", adult, re.M)) + "\n"
    for name in ("AdultLink_LoadDL", "AdultLink_Setup", "AdultLink_ApplyNeiHeldHand", "AdultLink_OverrideLimb"):
        production += function(adult, name) + "\n"
    production += 'extern "C" {\n'
    for name in ("Player_ShouldHideBackEquipment", "Player_ApplyBackEquipmentVisibility"):
        production += function(player, name) + "\n"
    for name in ("Graph_OpenDisps", "Graph_CloseDisps"):
        production += function(graph, name) + "\n"
    for name in ("gSPDisplayList", "gSPSegment"):
        production += function(graphics, name) + "\n"
    production += "}\n"
    compiler = os.environ.get("CC", "cc")
    cxx = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix="adult-mm-flex-") as directory:
        build = Path(directory)
        write_fixture(metadata, build / "archive_fixture.inc")
        (build / "adult_production.inc").write_text(production)
        objects = []
        native_flags = [*flags(), "-O0", "-g", "-ffunction-sections", "-fdata-sections", "-w"]
        for path in ("mm/src/code/z_skelanime.c", "mm/src/code/sys_matrix.c", "mm/2s2h/gu_pc.c"):
            output = build / (Path(path).stem + ".o")
            subprocess.run([compiler, "-std=gnu2x", *native_flags, "-c", str(ROOT / path),
                            "-o", str(output)], check=True)
            objects.append(output)
        binary = build / "native_flex_test"
        subprocess.run([cxx, "-std=c++20", *native_flags, "-I" + str(build),
                        str(ROOT / "tests/adult_mm_flex/native_flex_test.cpp"),
                        *(str(obj) for obj in objects), "-Wl,--gc-sections", "-lm", "-o", str(binary)],
                       check=True)
        return subprocess.run([str(binary)], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
