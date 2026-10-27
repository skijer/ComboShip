#!/usr/bin/env python3
"""Focused read-only checks for packed native keeper and stock-preview poses."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
from native_preview import NativeKeeper, crc64, rotate_xyz
from presentation_preview import keeper_placement, shelf_positions, item_pose, triangle_box_intersects, seal_style


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack", type=Path)
    parser.add_argument("--stock", type=Path)
    args = parser.parse_args()
    before = hashlib.sha256(args.pack.read_bytes()).digest()
    assert triangle_box_intersects(np.array([[0, 0, 0], [.5, 0, 0], [0, .5, 0]]), -np.ones(3), np.ones(3))
    assert not triangle_box_intersects(np.array([[2, 0, 0], [2, .5, 0], [2, 0, .5]]), -np.ones(3), np.ones(3))
    # Its triangle AABB overlaps this box, but the diagonal triangle does not.
    assert not triangle_box_intersects(np.array([[-3, 2, 0], [2, -3, 0], [-3, -3, 0]]), np.zeros(3), np.ones(3))
    with zipfile.ZipFile(args.pack) as archive:
        keeper = NativeKeeper(archive)
        placement = keeper_placement(archive)
        assert (keeper.limb_count, keeper.display_count, keeper.frame_count) == (23, 16, 32)
        assert placement["scale"] == .01
        # A native 24-entry joint table contains one root translation plus
        # 23 rotations; index >= staticIndexMax means dynamicData[index].
        assert keeper.indices.shape == (24, 3)
        assert keeper.joints(0)[0].tolist() == [4, 3199, 0]
        assert np.array_equal(keeper.joints(0), keeper.joints(32))
        assert not np.array_equal(keeper.joints(0), keeper.joints(16))
        for frame in range(keeper.frame_count):
            matrices = np.array(keeper.matrices(frame, [0, 0, 0], yaw=0, scale=1))
            assert matrices.shape == (16, 4, 4) and np.isfinite(matrices).all()
            assert np.allclose(matrices[:, 3], [0, 0, 0, 1])
            assert np.allclose(matrices[:, :3, :3] @ matrices[:, :3, :3].transpose(0, 2, 1), np.eye(3), atol=1e-10)
        assert np.allclose(rotate_xyz((0, -32768, 0))[:3, :3], np.diag([-1, 1, -1]))
        rows = keeper.geometry(0, [0, 0, 0], 0)
        shifted = keeper.geometry(0, [13, 7, -29], 0)
        assert len(rows) == len(shifted) == 16304
        for (texture, original), (other_texture, moved) in zip(rows, shifted):
            assert texture == other_texture
            assert np.allclose(moved[:, :3] - original[:, :3], [13, 7, -29], atol=1e-5)
            assert np.array_equal(original[:, 3:], moved[:, 3:])
            assert np.isfinite(original).all()
        # Every used expanded hash resolves to an actual packed resource;
        # all per-vertex matrix loads were checked by geometry().
        for path in archive.namelist():
            if "/keeper/" in path and path.endswith("DL"):
                for opcode, w0, w1, resource in keeper.commands(path):
                    if resource:
                        assert crc64(resource) in keeper.hashes
        open_head = keeper.geometry(0, blink=0)
        closed_head = keeper.geometry(0, blink=2)
        assert len(open_head) == len(closed_head)
        assert any(not np.array_equal(a, b) for (_, a), (_, b) in zip(open_head, closed_head))
    assert hashlib.sha256(args.pack.read_bytes()).digest() == before
    if args.stock:
        stock = json.loads(args.stock.read_text())["stock"]
        shelves = shelf_positions()
        assert len(stock) == len(shelves) == 8
        assert sum(i["wildcard"] for i in stock) == 2
        assert [sum(i["tier"] == t for i in stock) for t in (1, 2, 3)] == [2, 4, 2]
        for slot, (item, pos) in enumerate(zip(stock, shelves)):
            matrix = item_pose(item, pos, slot, 0)
            selected = item_pose(item, pos, slot, 0, True)
            assert np.isfinite(matrix).all()
            assert np.allclose(np.linalg.norm(matrix[:3, :3], axis=0), .25 * item["fit_scale"])
            assert np.allclose(np.linalg.norm(selected[:3, :3], axis=0), .32 * item["fit_scale"])
            # The market's direct DrawResolvedItem call omits the vanilla
            # EnGirlA +24-local origin; only native FrameFit lift is present.
            expected_y = pos[1] + 2 * np.sin(slot * 7 * .04) + .25 * item["fit_lift"]
            assert abs(matrix[1, 3] - expected_y) < 1e-7
        style = seal_style()
        assert style["kind"] == "native model-fitted wrapping X" and style["locked_only"]
        assert style["texture_dimensions"] == [32, 8] and style["s_wrap_t_clamp"]
        assert all(0 <= channel <= 255 for key in ("selected_rgba", "locked_rgba") for channel in style[key])
    print("PASS preview: 23 native limbs, all 32 lean poses, 16 flex matrices, 16,304 triangles, blink geometry, transform covariance, stock scales; archive unchanged")


if __name__ == "__main__":
    main()
