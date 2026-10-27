"""Depth-test exported frost geometry against the shipped authored Biggoron.

This is an offline source control, not a configured game capture. Eight camera
rolls and four blade sections distinguish a missing face/occluded upper blade
from a merely long local bounding box. Actual exported GBI, UVs, I8 and vertex
alpha are read independently of the source blade extraction.
"""
from pathlib import Path
import argparse
import json
import sys

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/assets"))
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
import build_elemental_sword_aura as build
from preview_elemental_sword_auras import vertices, texture

WIDTH, HEIGHT, SCALE = 160, 600, .10
ANGLES = (0, 30, 60, 80, 90, 120, 150, 180)
SECTIONS = ((739, 1900), (1900, 3100), (3100, 4300), (4300, 5500))


def fragments(triangle, angle, center):
    points = triangle[:, :3] - center
    cosine, sine = np.cos(angle), np.sin(angle)
    xy = np.column_stack((WIDTH / 2 + SCALE * (points[:, 1] * cosine - points[:, 2] * sine),
                          570 - SCALE * points[:, 0]))
    depth = points[:, 1] * sine + points[:, 2] * cosine
    low = np.maximum(np.floor(xy.min(0)).astype(int), (0, 0))
    high = np.minimum(np.ceil(xy.max(0)).astype(int), (WIDTH - 1, HEIGHT - 1))
    if np.any(high < low):
        return None
    xx, yy = np.meshgrid(np.arange(low[0], high[0] + 1) + .5, np.arange(low[1], high[1] + 1) + .5)
    a, b, c = xy
    denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
    if abs(denominator) < 1e-8:
        return None
    u = ((b[1] - c[1]) * (xx - c[0]) + (c[0] - b[0]) * (yy - c[1])) / denominator
    v = ((c[1] - a[1]) * (xx - c[0]) + (a[0] - c[0]) * (yy - c[1])) / denominator
    w = 1 - u - v
    region = np.s_[low[1]:high[1] + 1, low[0]:high[0] + 1]
    return region, (u >= 0) & (v >= 0) & (w >= 0), u * depth[0] + v * depth[1] + w * depth[2], (u, v, w)


def project(blade, core, tight, pixels, frame, degrees):
    angle = np.deg2rad(degrees)
    center = np.array(build.FRAMES[frame])
    depth = np.full((HEIGHT, WIDTH), -np.inf)
    for triangle in blade:
        fragment = fragments(triangle, angle, center)
        if fragment is not None:
            region, mask, z, _ = fragment
            depth[region][mask] = np.maximum(depth[region][mask], z[mask])
    metal = np.isfinite(depth)
    image = np.zeros((HEIGHT, WIDTH, 3)) + np.array((16, 21, 31)) / 255
    image[metal] = np.array((37, 43, 51)) / 255
    plain = image.copy()
    coverage = np.zeros((HEIGHT, WIDTH))
    core_coverage = np.zeros((HEIGHT, WIDTH), bool)
    # SwordAura_Material: frost TEXEL0 * SHADE * PRIMITIVE alpha. Nearest
    # sampling conservatively keeps the exact stored I8 minima; no scene light,
    # perspective, MSAA, interpolation, bilinear filtering or game RDP is modeled.
    for layer, triangles, prim_alpha in (("core", core, 200 / 255), ("tight", tight, 225 / 255)):
        for triangle in triangles:
            fragment = fragments(triangle, angle, center)
            if fragment is None:
                continue
            region, mask, z, (u, v, w) = fragment
            visible = mask & (z >= depth[region] - .001)
            ss = (u * triangle[0, 4] + v * triangle[1, 4] + w * triangle[2, 4]) / 32
            tt = (u * triangle[0, 5] + v * triangle[1, 5] + w * triangle[2, 5]) / 32
            intensity = pixels[np.floor(tt).astype(int) % 32, np.floor(ss).astype(int) % 64]
            shade = (u * triangle[0, 9] + v * triangle[1, 9] + w * triangle[2, 9]) / 255
            alpha = np.clip(prim_alpha * intensity * shade, 0, 1)
            env, prim = np.array((54, 168, 255)), np.array((210, 245, 255))
            color = (env + (prim - env) * intensity[..., None]) / 255
            image[region][visible] = (color[visible] * alpha[visible, None] +
                                      image[region][visible] * (1 - alpha[visible, None]))
            coverage[region][visible] = 1 - (1 - coverage[region][visible]) * (1 - alpha[visible])
            if layer == "core":
                core_coverage[region][visible] = True
    sections = []
    xs = (570 - (np.arange(HEIGHT) + .5)) / SCALE
    for low, high in SECTIONS:
        selected = metal & (xs[:, None] >= low) & (xs[:, None] < high)
        count = int(selected.sum())
        assert count > 500, (frame, degrees, low, "no source blade samples")
        visible_core = float((core_coverage & selected).sum() / count)
        visible_frost = float(((coverage > .03) & selected).sum() / count)
        assert visible_frost > .98, (frame, degrees, low, visible_core, visible_frost)
        sections.append({"x": [low, high], "pixels": count,
                         "core_visible": round(visible_core, 4), "ice_alpha_gt_03": round(visible_frost, 4)})
    return {"frame": frame, "angle": degrees, "sections": sections}, plain, image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Optional MM human source/frost projection PNG and JSON")
    args = parser.parse_args()
    source = (ROOT / "combo/render/sword_element_aura_data.h").read_text()
    pixels = texture(source, "sSwordAuraIceTex")
    canvas = Image.new("RGB", (WIDTH * 8, HEIGHT * 2 + 55), (16, 21, 31))
    draw, results = ImageDraw.Draw(canvas), []
    for frame in build.FRAMES:
        blade = np.array(build.blade("biggoron_sword", frame), float).reshape(-1, 3, 10)
        prefix = "sSwordAura_biggoron_sword_" + frame
        core, tight = (np.array(vertices(source, prefix + suffix)) for suffix in ("Core", "Tight"))
        for index, degrees in enumerate(ANGLES):
            result, plain, frost = project(blade, core, tight, pixels, frame, degrees)
            results.append(result)
            if frame == "mm_human" and args.output:
                left, top = index % 4 * WIDTH * 2, index // 4 * HEIGHT + 25
                canvas.paste(Image.fromarray(np.uint8(np.clip(plain * 255, 0, 255))), (left, top))
                canvas.paste(Image.fromarray(np.uint8(np.clip(frost * 255, 0, 255))), (left + WIDTH, top))
                draw.text((left + 18, top - 20), f"{degrees} degrees | source / frost", fill="white")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        draw.text((14, HEIGHT * 2 + 35), "Exact exported geometry/I8 alpha, source blade depth test; offline control, no game capture.", fill="white")
        canvas.save(args.output)
        args.output.with_suffix(".json").write_text(json.dumps(results, indent=2) + "\n")
    print("PASS authored Biggoron frost depth/alpha coverage: 3 rigs, 8 rolls, 4 blade sections including tip")


if __name__ == "__main__":
    main()
