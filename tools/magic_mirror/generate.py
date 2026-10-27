#!/usr/bin/env python3
"""Deterministically generate both native icons/names and actual-mesh previews.

python tools/magic_mirror/generate.py --preview-dir /absolute/scratch/path

The icon is a software raster of MagicMirrorGeometry.h, including its native
1/16-unit quantization, vertex colors, opaque metal and textured translucent pane.
The glass image is a fixed render of the real preserved Lost Woods scene.
Names use the existing IA4-exact native name generator and bundled font.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]


def native_round(values):
    """C++ lround semantics, including negative half-integers."""
    return np.copysign(np.floor(np.abs(values) + 0.5), values)


def render(mesh, size, *, roll=-0.28, yaw=-0.13, pitch=-0.04, border=2):
    width, height = size
    vertices = np.asarray(mesh["vertices"], dtype=np.float64)
    position = native_round(vertices[:, :3] * mesh["vertexPrecision"]) / mesh["vertexPrecision"]
    # Center in object space before applying the same ordinary matrix rotations
    # available to the native caller. Positive transformed Z faces the viewer.
    position[:, 1] += 3.15
    cy, sy, cp, sp, cr, sr = math.cos(yaw), math.sin(yaw), math.cos(pitch), math.sin(pitch), math.cos(roll), math.sin(roll)
    yaw_matrix = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
    pitch_matrix = np.array([[1, 0, 0], [0, cp, -sp], [0, sp, cp]])
    roll_matrix = np.array([[cr, -sr, 0], [sr, cr, 0], [0, 0, 1]])
    position = position @ (roll_matrix @ pitch_matrix @ yaw_matrix).T
    lower, upper = position[:, :2].min(axis=0), position[:, :2].max(axis=0)
    center = (lower + upper) / 2
    scale = min((width - 2 * border) / (upper[0] - lower[0]), (height - 2 * border) / (upper[1] - lower[1]))
    position[:, 0] = (position[:, 0] - center[0]) * scale + width / 2
    position[:, 1] = -(position[:, 1] - center[1]) * scale + height / 2
    colors = vertices[:, 3:] / 255.0
    output = np.zeros((height, width, 4), dtype=np.float64)
    depth = np.full((height, width), -np.inf)
    reflection_path = ROOT / 'tools/magic_mirror/reflection/forest-scene.png'
    reflection = np.asarray(Image.open(reflection_path).convert('RGBA'), dtype=np.float64) / 255.0
    uv = np.asarray(mesh['reflectionUV'], dtype=np.float64)
    for triangle in mesh["triangles"]:
        ia, ib, ic, translucent, textured = triangle
        indexes = [ia, ib, ic]
        points = position[indexes]
        x0 = max(0, math.floor(points[:, 0].min()))
        x1 = min(width, math.ceil(points[:, 0].max()))
        y0 = max(0, math.floor(points[:, 1].min()))
        y1 = min(height, math.ceil(points[:, 1].max()))
        if x1 <= x0 or y1 <= y0:
            continue
        yy, xx = np.mgrid[y0:y1, x0:x1].astype(np.float64)
        xx += 0.5
        yy += 0.5
        a, b, c = points
        denominator = (b[1]-c[1])*(a[0]-c[0]) + (c[0]-b[0])*(a[1]-c[1])
        if abs(denominator) < 1e-12:
            continue
        wa = ((b[1]-c[1])*(xx-c[0]) + (c[0]-b[0])*(yy-c[1])) / denominator
        wb = ((c[1]-a[1])*(xx-c[0]) + (a[0]-c[0])*(yy-c[1])) / denominator
        wc = 1-wa-wb
        z = wa*a[2] + wb*b[2] + wc*c[2]
        visible = (wa >= 0) & (wb >= 0) & (wc >= 0) & (z >= depth[y0:y1, x0:x1] - 1e-8)
        if not visible.any():
            continue
        rgba = wa[..., None]*colors[ia] + wb[..., None]*colors[ib] + wc[..., None]*colors[ic]
        if textured:
            # Same bright glass modulation as the native Vtx conversion; the
            # source texture carries the forest, not the casing or highlights.
            st = wa[..., None]*uv[ia] + wb[..., None]*uv[ib] + wc[..., None]*uv[ic]
            tx = np.clip(st[..., 0] * reflection.shape[1] - 0.5, 0, reflection.shape[1]-1)
            ty = np.clip(st[..., 1] * reflection.shape[0] - 0.5, 0, reflection.shape[0]-1)
            x0t, y0t = np.floor(tx).astype(int), np.floor(ty).astype(int)
            x1t, y1t = np.minimum(x0t+1, reflection.shape[1]-1), np.minimum(y0t+1, reflection.shape[0]-1)
            fx, fy = (tx-x0t)[..., None], (ty-y0t)[..., None]
            sampled = (reflection[y0t,x0t]*(1-fx) + reflection[y0t,x1t]*fx)*(1-fy) + \
                      (reflection[y1t,x0t]*(1-fx) + reflection[y1t,x1t]*fx)*fy
            rgba[..., :3] *= sampled[..., :3]
            rgba[..., 3] *= sampled[..., 3]
        destination = output[y0:y1, x0:x1]
        alpha = rgba[..., 3:4]
        if translucent:
            composed = np.concatenate((rgba[..., :3]*alpha, alpha), axis=2) + destination*(1-alpha)
            destination[visible] = composed[visible]
        else:
            destination[visible] = rgba[visible]
            depth[y0:y1, x0:x1][visible] = z[visible]
    # Convert the premultiplied raster to ordinary PNG straight alpha.
    alpha = output[..., 3:4]
    output[..., :3] = np.divide(output[..., :3], alpha, out=np.zeros_like(output[..., :3]), where=alpha > 0)
    return Image.fromarray(np.clip(np.floor(output*255 + 0.5), 0, 255).astype(np.uint8), "RGBA")


def load_mesh(executable, frame):
    return json.loads(subprocess.check_output([str(executable), str(frame)], text=True))


def generate_assets(executable):
    reflection = Image.open(ROOT / 'tools/magic_mirror/reflection/forest-scene.png').convert('RGBA')
    # Double horizontal detail while remaining inside a 4 KiB RGBA16 tile.
    native_reflection = reflection.resize((64, 32), Image.Resampling.LANCZOS)
    for game in ('soh', 'mm'):
        target = ROOT / game / 'assets/custom/textures/magic_mirror/gMagicMirrorForestReflectionTex.rgb5a1.png'
        target.parent.mkdir(parents=True, exist_ok=True)
        native_reflection.save(target, compress_level=9)
    # Render larger, then filter to the actual native inventory texture size.
    image = render(load_mesh(executable, 0), (256, 256), border=12, roll=-0.35)
    icon = image.resize((32, 32), Image.Resampling.LANCZOS)
    for game in ("soh", "mm"):
        target = ROOT / game / "assets/custom/textures/icon_item_custom/gItemIconMagicMirrorTex.rgba32.png"
        icon.save(target, optimize=False, compress_level=9)
    spec = importlib.util.spec_from_file_location("native_name_generator", ROOT / "soh/assets/custom/textures/item_name_custom/generate_names.py")
    names = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(names)
    font_path = str(ROOT / "soh/assets/custom/fonts/CenturyGothicBold.ttf")
    font_size, baseline = names.pick_base_size(font_path)
    font, _ = names.fit_size(font_path, "Magic Mirror", font_size)
    image = names.compose_ia4(names.render_mask(font, "Magic Mirror", baseline))
    for game in ("soh", "mm"):
        target = ROOT / game / "assets/custom/textures/item_name_custom/gMagicMirrorNameTex.ia4.png"
        image.save(target, optimize=False, compress_level=9)
        problems = names.verify_file(target)
        if problems:
            raise RuntimeError(f"{target}: {problems}")
    return icon


def font(size, bold=False):
    name = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    return ImageFont.truetype(f"/usr/share/fonts/truetype/dejavu/{name}", size)


def generate_preview(executable, icon, directory):
    directory.mkdir(parents=True, exist_ok=True)
    canvas = Image.new("RGBA", (1240, 880), (17, 31, 30, 255))
    draw = ImageDraw.Draw(canvas)
    draw.text((52, 35), "MAGIC MIRROR", font=font(34, True), fill=(240, 222, 162))
    draw.text((53, 85), "Actual Lost Woods scene reflected in glass • preserved blue casing and gold rim", font=font(17), fill=(157, 185, 172))
    for index, (frame, yaw, roll, title) in enumerate(((0, -0.13, -0.1, "Front · frame 0"), (240, -0.13, -0.1, "Reflection · frame 240"), (480, 0.72, 0.04, "Turned · frame 480"))):
        x = 45 + index * 395
        draw.rounded_rectangle((x, 133, x+365, 738), radius=18, fill=(22, 41, 39), outline=(50, 73, 61), width=1)
        rendered = render(load_mesh(executable, frame), (660, 1020), border=25, roll=roll, yaw=yaw).resize((330, 510), Image.Resampling.LANCZOS)
        canvas.alpha_composite(rendered, (x+18, 157))
        draw.text((x+25, 694), title, font=font(18), fill=(202, 215, 193))
    draw.text((55, 776), "32 × 32 inventory icon", font=font(18, True), fill=(222, 213, 178))
    canvas.alpha_composite(icon.resize((64, 64), Image.Resampling.NEAREST), (315, 758))
    draw.text((415, 771), "Generated from the same mesh, forest image and glass material.", font=font(18), fill=(173, 195, 180))
    draw.text((415, 802), "Software preview; native runtime appearance still requires game verification.", font=font(15), fill=(133, 159, 146))
    canvas.convert("RGB").save(directory / "magic-mirror-native-preview.png", optimize=False, compress_level=9)
    render(load_mesh(executable, 0), (600, 900), border=30, roll=-0.10).save(directory / "magic-mirror-native-front.png", optimize=False, compress_level=9)
    # A short cycle uses the same phase, geometry and native colors as gameplay.
    frames = []
    background = Image.new("RGBA", (240, 320), (17, 31, 30, 255))
    for frame in range(0, 960, 40):
        rendered = render(load_mesh(executable, frame), (240, 320), border=14, roll=-0.10)
        frames.append(Image.alpha_composite(background, rendered).convert("RGB"))
    frames[0].save(directory / "magic-mirror-native-ripple.gif", save_all=True, append_images=frames[1:],
                   duration=200, loop=0, disposal=2, optimize=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preview-dir", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="magic-mirror-generator-") as temp:
        executable = Path(temp) / "export-geometry"
        subprocess.run(["c++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror", "-pedantic",
                        str(Path(__file__).with_name("export_geometry.cpp")), "-o", str(executable)], check=True)
        icon = generate_assets(executable)
        if args.preview_dir:
            generate_preview(executable, icon, args.preview_dir)
    print("Generated identical 32×32 RGBA32 icons and IA4-clean 128×16 Magic Mirror names for OoT and MM.")


if __name__ == "__main__":
    main()
