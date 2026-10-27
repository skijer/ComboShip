#!/usr/bin/env python3
"""Verify that the offline preview uses the native X and real posed bounds."""
from pathlib import Path
import sys
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
from presentation_preview import native_lock, seal_arrays, item_pose, transform

lock = native_lock()
texture = lock.texture()
assert texture.shape == (8, 32, 4) and texture.dtype == np.uint8
assert (texture[:, :, 0] == texture[:, :, 1]).all()
assert (texture[:, :, 1] == texture[:, :, 2]).all()
assert not texture[[0, -1], :, 3].any() and texture[2:6, :, 3].max() == 255
stock = [dict(slug="asymmetric", clears=1, fit_scale=1.7, fit_lift=3)]
shelves = [[72, 64, 48]]
model = np.zeros((6, 9), dtype=np.float32)
model[:, :3] = [[-3,-12,-2],[7,-12,4],[7,16,4],[-3,-12,-2],[7,16,4],[-3,16,-2]]
models = {"asymmetric": [(None, model, False)]}
for tick in (0, 31, 91):
    for selected in (None, 0):
        rows = seal_arrays(stock, shelves, tick, selected, 0, models)
        points = transform(model, item_pose(stock[0], shelves[0], 0, tick, selected == 0))[:, :3]
        expected = lock.vertices(points.min(0), points.max(0), tick, selected == 0)
        assert len(rows) == 1 and np.array_equal(rows[0], expected)
        assert np.linalg.norm((rows[0][:,:3].min(0) + rows[0][:,:3].max(0))/2 -
                              (points.min(0) + points.max(0))/2) < .04
assert seal_arrays(stock, shelves, 31, 0, 1, models) == [], "unlocked selection must remain unobscured"
print("PASS preview X: native IA8 pixels, exact native triangles/UV/color, posed asymmetric bounds, selected scale and unlocked visibility")
