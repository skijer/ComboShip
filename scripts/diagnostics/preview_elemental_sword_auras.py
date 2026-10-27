"""Project exact generated aura vertices/I8 maps; this is not an in-game capture."""
from pathlib import Path
import argparse
import re

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]


def vertices(source, prefix):
    arrays = {}
    for name, body in re.findall(r"static const Vtx (" + prefix + r"V\d+)\[\] = \{(.*?)\n\};", source, re.S):
        values = np.array([int(v) for v in re.findall(r"-?\d+", body)], dtype=float)
        arrays[name] = values.reshape(-1, 10)
    body = re.search(r"static const Gfx " + prefix + r"\[\] = \{(.*?)\n\};", source, re.S)[1]
    triangles, cache = [], None
    for command in body.splitlines():
        load = re.search(r"gsSPVertex\((\w+),", command)
        tri = re.search(r"gsSP1Triangle\((\d+), (\d+), (\d+),", command)
        if load: cache = arrays[load[1]]
        if tri: triangles.append(cache[[int(tri[i]) for i in range(1, 4)]])
    assert len(triangles) > 20
    return triangles


def texture(source, name):
    body = re.search(name + r"\[2048\] = \{(.*?)\n\};", source, re.S)[1]
    return np.array([int(v) for v in re.findall(r"\d+", body)], float).reshape(32, 64) / 255


def layer(canvas, triangles, pixels, prim, env, opacity, outer, center):
    # The same native unlit combiner colors and vertex alpha; orthographic Z
    # projection with depth-sorted triangles, not the game's scene renderer.
    for tri in sorted(triangles, key=lambda tri: tri[:, 2].mean()):
        xy = np.column_stack((center + (tri[:, 1] - 328) * .13, 610 - tri[:, 0] * .13))
        low = np.maximum(np.floor(xy.min(axis=0)).astype(int), [0, 0])
        high = np.minimum(np.ceil(xy.max(axis=0)).astype(int), [canvas.shape[1] - 1, canvas.shape[0] - 1])
        if np.any(high < low): continue
        x, y = np.meshgrid(np.arange(low[0], high[0] + 1) + .5, np.arange(low[1], high[1] + 1) + .5)
        a, b, c = xy
        denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(denominator) < 1e-6: continue
        u = ((b[1] - c[1]) * (x - c[0]) + (c[0] - b[0]) * (y - c[1])) / denominator
        v = ((c[1] - a[1]) * (x - c[0]) + (a[0] - c[0]) * (y - c[1])) / denominator
        w = 1 - u - v
        mask = (u >= 0) & (v >= 0) & (w >= 0)
        if not mask.any(): continue
        s = (u * tri[0, 4] + v * tri[1, 4] + w * tri[2, 4]) / 32
        t = (u * tri[0, 5] + v * tri[1, 5] + w * tri[2, 5]) / 32
        value = pixels[np.floor(t).astype(int) % 32, np.floor(s).astype(int) % 64]
        shade = (u * tri[0, 9] + v * tri[1, 9] + w * tri[2, 9]) / 255
        alpha = opacity * shade * (value if outer else 1)
        color = env + (prim - env) * value[..., None]
        region = canvas[low[1]:high[1] + 1, low[0]:high[0] + 1]
        region[mask] = color[mask] * alpha[mask, None] + region[mask] * (1 - alpha[mask, None])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (ROOT / "combo/render/sword_element_aura_data.h").read_text()
    prefix = "sSwordAura_master_sword_oot_adult"
    core, flame, tight = [vertices(source, prefix + suffix) for suffix in ("Core", "Flame", "Tight")]
    canvas = np.zeros((730, 1140, 3), dtype=float) + np.array([13, 21, 34])
    settings = [
        # Phase-zero primitive alpha from SwordAura_Material; vertex alpha and
        # I8 intensity are applied separately by layer(), as in the combiner.
        ("Fire", "FireCore", "FireFlame", [255, 225, 122], [255, 43, 3], 1, .96),
        ("Ice", "Ice", "Ice", [210, 245, 255], [54, 168, 255], 200 / 255, 225 / 255),
        ("Light", "Light", "Light", [255, 255, 235], [255, 210, 106], 185 / 255, 220 / 255),
    ]
    for index, (_, inner, outer, prim, env, inner_alpha, outer_alpha) in enumerate(settings):
        center = 190 + 380 * index
        # Neutral underlying blade silhouette gives transparent envelopes a
        # reference; its original metal material is intentionally not simulated.
        layer(canvas, core, np.ones((32, 64)), np.array([105] * 3), np.array([105] * 3), 1, False, center)
        layer(canvas, core, texture(source, "sSwordAura" + inner + "Tex"),
              np.array(prim), np.array(env), inner_alpha, index != 0, center)
        layer(canvas, flame if index == 0 else tight, texture(source, "sSwordAura" + outer + "Tex"),
              np.array(prim), np.array(env), outer_alpha, True, center)
    image = Image.fromarray(np.uint8(np.clip(canvas, 0, 255)))
    draw = ImageDraw.Draw(image)
    fonts = Path("/usr/share/fonts/truetype/dejavu")
    title = ImageFont.truetype(str(fonts / "DejaVuSans.ttf"), 25)
    body = ImageFont.truetype(str(fonts / "DejaVuSans.ttf"), 16)
    for index, setting in enumerate(settings):
        draw.text((190 + 380 * index, 35), setting[0], font=title, anchor="mt", fill="white")
    draw.text((570, 638), "Exact generated Master Sword envelope geometry and I8 maps", font=body, anchor="mt", fill="white")
    draw.text((570, 671), "Offline projection • neutral blade reference • no game capture or runtime visual proof", font=body, anchor="mt", fill="#BAC7D7")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)
    print(args.output)


if __name__ == "__main__": main()
