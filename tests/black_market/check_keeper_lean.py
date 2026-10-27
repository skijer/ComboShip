#!/usr/bin/env python3
"""Verify the packed Goth keeper pose, grounding, and isolated room patch."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
from native_preview import KEEPER, NativeKeeper, Reader


def animation_joints(pack, name):
    reader = Reader(pack.read(KEEPER + name))
    assert reader.read("I") == 0
    frames = reader.read("h")
    values = np.array([reader.read("h") for _ in range(reader.read("I"))])
    indices = np.array([reader.read("HHH") for _ in range(reader.read("I"))])
    static_max = reader.read("h")
    reader.finish()
    return [values[indices + np.where(indices >= static_max, frame, 0)] for frame in range(frames)]


def check_default_pose(pack):
    keeper = NativeKeeper(pack)
    assert keeper.animation_path == KEEPER + "ShopGalMMDLevelSwayAnim"
    expected = animation_joints(pack, "ShopGalMMDLevelSwayAnim")
    assert keeper.frame_count == len(expected) == 32
    for frame, joints in enumerate(expected):
        assert np.array_equal(keeper.joints(frame), joints), (
            f"Default keeper frame {frame} must use the native crossed-arm lean, not the open-arm idle")
    # Check body relationships, not the animation's suggestive filename.
    hands = [keeper.draw_order.index(next(i for i, limb in enumerate(keeper.limbs)
             if limb.name.endswith(side + "HandLimb"))) for side in ("Left", "Right")]
    shoulders = [keeper.draw_order.index(next(i for i, limb in enumerate(keeper.limbs)
                 if limb.name.endswith(side + "UpperArmLimb"))) for side in ("Left", "Right")]
    for yaw in (0, -32768):
        angle = yaw * np.pi / 32768
        forward = np.array([np.sin(angle), 0, np.cos(angle)])
        for frame in range(keeper.frame_count):
            matrices = keeper.matrices(frame, (0, 0, 155), yaw=yaw)
            wrists = np.array([matrices[index][:3, 3] for index in hands])
            shoulder = np.array([matrices[index][:3, 3] for index in shoulders])
            assert np.linalg.norm(wrists[0] - wrists[1]) < 6, "Forearms must remain crossed throughout the loop"
            assert np.all((wrists - shoulder) @ forward > 12), "Hands must lean toward customers at either world heading"
    return keeper


def check_decoder(keeper):
    for frame, blink in ((0, 0), (15, 1), (31, 2)):
        expected = np.concatenate([array[:, :3] for _, array in keeper.geometry(
            frame, (0, 1, 130), blink=blink)]).reshape(-1, 3, 3)
        actual = keeper.position_triangles(frame, (0, 1, 130), blink=blink)
        assert actual.shape == expected.shape == (16304, 3, 3)
        assert np.allclose(actual, expected, atol=1e-5, rtol=0), "Position scan diverged from native preview triangles"
    from keeper_lean import keeper_contract
    contract = keeper_contract()
    expected = keeper.position_triangles(0, contract["position"], contract["yaw"], contract["scale"])
    assert np.allclose(keeper.position_triangles(0), expected, atol=1e-5, rtol=0), "Default placement must follow the shared native contract"
    default_preview = np.concatenate([array[:, :3] for _, array in keeper.geometry(0)]).reshape(-1, 3, 3)
    assert np.allclose(default_preview, expected, atol=1e-5, rtol=0), "Preview defaults must follow the shared native contract"


def check_candidate(source):
    # Exercise the real CLI on the retained source, including its preservation
    # report and deterministic ZIP output. No synthetic animation or room data.
    with tempfile.TemporaryDirectory(prefix="market-keeper-lean-check-") as folder:
        paths = [Path(folder) / f"candidate-{index}.o2r" for index in range(2)]
        reports = [path.with_suffix(".json") for path in paths]
        for path, report in zip(paths, reports):
            result = subprocess.run([sys.executable, str(ROOT / "tools/black_market/keeper_lean.py"),
                                     "--source", str(source), "--output", str(path),
                                     "--report", str(report)], capture_output=True, text=True)
            assert result.returncode == 0, result.stderr
        assert paths[0].read_bytes() == paths[1].read_bytes(), "Keeper candidate must be reproducible"
        before = hashlib.sha256(source.read_bytes()).hexdigest()
        from keeper_lean import BASELINE_SHA256, MANIFEST, ROOM, keeper_contract, keeper_placement
        assert before == BASELINE_SHA256
        with zipfile.ZipFile(source) as baseline, zipfile.ZipFile(paths[0]) as candidate:
            assert baseline.namelist() == candidate.namelist(), "ZIP entry membership or order changed"
            changed = {name for name in baseline.namelist() if baseline.read(name) != candidate.read(name)}
            assert changed == {ROOM, MANIFEST}, f"Unexpected asset mutation: {changed}"
            placement = keeper_placement(candidate.read(ROOM))
            contract = keeper_contract()
            assert placement == {key: contract[key] for key in ("position", "yaw", "scale")}
            # Both native games consume one contract. OoT has no room carrier;
            # its native controller supplies the same placement at runtime.
            manifest = json.loads(candidate.read(MANIFEST))
            assert manifest["keeper"] == contract
            from keeper_lean import geometry_report
            geometry = geometry_report(candidate)
            assert geometry["floor_clearance"] > 0
            assert geometry["counter_clearance"] > 4
            assert geometry["all_low_vertices_supported"]
        report = json.loads(reports[0].read_text())
        assert set(report["changed_resources"]) == changed
        assert report["preserved_resources"] == len(baseline.namelist()) - 2
        assert report["baseline_sha256"] == before
        assert hashlib.sha256(source.read_bytes()).hexdigest() == before, "Source pack was edited"
        # The source hash gate must fail closed on an unrelated candidate.
        rejected = subprocess.run([sys.executable, str(ROOT / "tools/black_market/keeper_lean.py"),
                                   "--source", str(paths[0]), "--output", str(Path(folder) / "wrong.o2r"),
                                   "--report", str(Path(folder) / "wrong.json")], capture_output=True, text=True)
        assert rejected.returncode != 0 and "baseline SHA256" in rejected.stderr
        shared = Path(folder) / "shared-output.o2r"
        rejected = subprocess.run([sys.executable, str(ROOT / "tools/black_market/keeper_lean.py"),
                                   "--source", str(source), "--output", str(shared),
                                   "--report", str(shared)], capture_output=True, text=True)
        assert rejected.returncode != 0 and not shared.exists(), "A report must never overwrite its candidate pack"
        return geometry


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pack", type=Path, required=True, help="Exact retained POC11 input")
    args = parser.parse_args()
    with zipfile.ZipFile(args.pack) as pack:
        keeper = check_default_pose(pack)
        check_decoder(keeper)
    geometry = check_candidate(args.pack)
    print(f"PASS keeper lean: 32 crossed-arm frames; {geometry['triangles_per_frame']} triangles/frame; "
          f"floor clearance {geometry['floor_clearance']:.5f}; counter clearance {geometry['counter_clearance']:.5f}; "
          "all other entry payloads preserved; deterministic isolated candidate")


if __name__ == "__main__":
    main()
