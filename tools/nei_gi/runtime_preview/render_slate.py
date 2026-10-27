"""Depth-tested Slate halo diagnosis from exact GI GLBs and the C++ sampler.

No alternate effect geometry or generative image is used. Native mesh positions
are the exported quantized vertices multiplied by matrix_scale * draw_scale.
Camera basis follows every rotation. Mesa uses opaque depth plus alpha blended
effects without depth writes. Lighting and camera are offline approximations;
these images are not captured gameplay.

Run from the repository root, with the local cloud dependency prefix activated:
    python3 tools/nei_gi/runtime_preview/render_slate.py tools/nei_gi/PREVIEWS
Use --center 0 0 8 to inspect a proposed effect-center change without modifying
the production bindings, or --compare-center to place two centers side by side.
"""
import argparse
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageDraw, ImageSequence
# Avoid asking Mesa to write its shader cache outside the shared workspace.
os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")
import render as r

REPO = Path(__file__).resolve().parents[3]
PROFILES = {
    "sheikah_slate": "Slate", "slate_bomb": "SlateBomb",
    "slate_master_cycle": "SlateCycle", "slate_stasis": "SlateStasis",
    "slate_cryonis": "SlateCryonis", "slate_sensor": "SlateSensor",
}
DTYPE = np.dtype([("p", "<f4", 3), ("rgba", "u1", 4)])


def samples(slug, frames, elevation, directory):
    # Derive enum values from the actual header with C++, so adding/reordering an
    # effect cannot silently cause the preview to display a different profile.
    selector = directory / "kind.cpp"
    selector.write_text('#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"\n'
                        '#include <iostream>\nint main(){std::cout << int(NeiGi::Kind::'
                        + PROFILES[slug] + ');}\n')
    compiler = os.environ.get("CXX", "c++")
    flags = ["-std=c++20", "-O2", "-I" + str(REPO / "soh")]
    selector_bin, exporter = directory / "kind", directory / "export_slate"
    subprocess.run([compiler, *flags, str(selector), "-o", str(selector_bin)], check=True)
    kind = subprocess.check_output([str(selector_bin)], text=True).strip()
    subprocess.run([compiler, *flags, str(Path(__file__).with_name("export_slate.cpp")),
                    "-o", str(exporter)], check=True)
    data = directory / "slate.bin"
    subprocess.run([str(exporter), str(data), str(frames), str(elevation), kind], check=True)
    raw = data.read_bytes()
    count, = struct.unpack_from("<I", raw)
    assert count == frames
    cursor, result = 4, []
    for _ in range(frames):
        frame, yaw, vertices = struct.unpack_from("<IfI", raw, cursor)
        cursor += 12
        mesh = np.frombuffer(raw, dtype=DTYPE, count=vertices, offset=cursor)
        cursor += vertices * DTYPE.itemsize
        assert vertices and vertices % 3 == 0 and np.isfinite(mesh["p"]).all()
        assert np.allclose(mesh["p"] * 16, np.round(mesh["p"] * 16))
        result.append((frame, yaw * 180 / math.pi, mesh))
    assert cursor == len(raw)
    return result


def basis(elevation, yaw):
    x, y = np.deg2rad([elevation, yaw])
    sx, cx, sy, cy = np.sin(x), np.cos(x), np.sin(y), np.cos(y)
    return np.array([[cy, 0, sy], [sx * sy, cx, -sx * cy], [-cx * sy, sx, cx * cy]])


def pose(column, elevation, yaw):
    r.Viewport(column * 400, 115, 400, 745)
    r.MatrixMode(0x1701)
    r.LoadIdentity()
    r.Ortho(-30, 30, -41, 70.75, -300, 300)
    r.MatrixMode(0x1700)
    r.LoadIdentity()
    r.Rotate(elevation, 1, 0, 0)
    r.Rotate(yaw, 0, 1, 0)


def center_label(center):
    return "(" + ", ".join(f"{v:g}" for v in center) + ")"


def render_frame(slug, sample, elevation, center, compare_center):
    frame, yaw, mesh = sample
    # Existing renderer compiles lighting into the GLB display list, so update
    # its lighting basis per frame as well as the actual OpenGL camera rotation.
    r.R = basis(elevation, yaw)
    opaque, skin = r.model(slug)
    r.DepthMask(1)
    r.Clear(0x4000 | 0x0100)
    centers = [center, compare_center if compare_center is not None else center, center]
    for column, effect_center in enumerate(centers):
        pose(column, elevation, yaw)
        if column < 2:
            r.DepthMask(1)
            r.CallList(opaque)
        r.DepthMask(0)
        if column != 0 or compare_center is not None:
            r.Push()
            r.Translate(*effect_center)
            r.drawfx(mesh)
            r.Pop()
        if column < 2:
            r.CallList(skin)
    image = r.pixels()
    draw = ImageDraw.Draw(image)
    title = json.loads((r.ROOT / "CHECKPOINTS" / slug / "checkpoint.json").read_text())["name"]
    draw.rectangle((0, 0, r.W, 130), fill="#111923")
    draw.text((22, 15), title + " - rotating halo diagnosis", font=r.font(27), fill="#edf2f8")
    draw.text((22, 55), "Exact exported GI + production effect triangles | shimmer OFF | opaque depth ON",
              font=r.font(17), fill="#b7c6d7")
    labels = (["Slate only", "Slate + halos"] if compare_center is None else
              ["Center " + center_label(center), "Center " + center_label(compare_center)])
    for column, label in enumerate(labels + ["Halos without opaque Slate"]):
        draw.text((column * 400 + 18, 97), label, font=r.font(19), fill="#edf2f8")
    draw.text((22, 902), f"GI angle {yaw:.1f} degrees | gameplay frame {frame} | center {center_label(center)}",
              font=r.font(18), fill="#b7c6d7")
    draw.text((22, 943), "Offline Mesa renderer. Camera and lighting approximate; not captured gameplay.",
              font=r.font(17), fill="#94a8bd")
    return image


def save_movie(images, destination):
    # One global palette avoids color flicker. A shared sampler produces no
    # additional bloom, star sprites or particles in this diagnostic.
    palette = images[0].quantize(colors=256)
    frames = [image.quantize(palette=palette, dither=Image.Dither.NONE) for image in images]
    frames[0].save(destination, save_all=True, append_images=frames[1:], duration=100,
                   loop=0, optimize=False, disposal=2)


def save_contact(images, destination):
    selected = [round(i * len(images) / 8) % len(images) for i in range(8)]
    sheet = Image.new("RGB", (1200, 2000), "#111923")
    for cell, index in enumerate(selected):
        sheet.paste(images[index].resize((600, 500)), ((cell % 2) * 600, (cell // 2) * 500))
    sheet.save(destination)


def comparison(images, baseline_gif, output, stem, before_label, after_label):
    with Image.open(baseline_gif) as source:
        before = [frame.convert("RGB") for frame in ImageSequence.Iterator(source)]
    assert len(before) == len(images), "Baseline animation must have the same frame count"
    result = []
    for index, (original, corrected) in enumerate(zip(before, images)):
        image = Image.new("RGB", (800, 940), "#111923")
        image.paste(original.crop((400, 130, 800, 900)), (0, 110))
        image.paste(corrected.crop((400, 130, 800, 900)), (400, 110))
        draw = ImageDraw.Draw(image)
        draw.text((20, 15), "Slate halos - before / after", font=r.font(27), fill="#edf2f8")
        draw.text((20, 53), "Exact GI + production sampler | shimmer OFF | depth ON", font=r.font(17), fill="#b7c6d7")
        draw.text((20, 88), before_label, font=r.font(19), fill="#edf2f8")
        draw.text((420, 88), after_label, font=r.font(19), fill="#edf2f8")
        draw.text((20, 901), "Offline render - not captured gameplay", font=r.font(17), fill="#94a8bd")
        result.append(image)
    save_movie(result, output / (stem + "_comparison.gif"))
    # Front and back at matching rotations, without distorting either render.
    sheet = Image.new("RGB", (800, 1880), "#111923")
    for row, index in enumerate((0, len(result) // 2)):
        sheet.paste(result[index], (0, row * 940))
    sheet.save(output / (stem + "_comparison_front_back.png"))
    result[0].save(output / (stem + "_comparison_front.png"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--slug", choices=PROFILES, default="sheikah_slate")
    parser.add_argument("--frames", type=int, default=72)
    parser.add_argument("--elevation", type=float, default=12)
    parser.add_argument("--center", type=float, nargs=3, default=(0, 0, 4))
    parser.add_argument("--compare-center", type=float, nargs=3)
    parser.add_argument("--baseline-gif", type=Path,
                        help="Previously rendered GIF for a matching before/after comparison")
    parser.add_argument("--before-label", default="Before: casing clips the circles")
    parser.add_argument("--after-label", default="After: complete halos")
    parser.add_argument("--stem", default="slate_rotation")
    args = parser.parse_args()
    assert 8 <= args.frames <= 720 and np.isfinite(args.center).all()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="nei-slate-preview-") as temporary:
        meshes = samples(args.slug, args.frames, args.elevation, Path(temporary))
        images = [render_frame(args.slug, sample, args.elevation, args.center, args.compare_center)
                  for sample in meshes]
    gif = args.output / (args.stem + ".gif")
    save_movie(images, gif)
    png = args.output / (args.stem + "_contact.png")
    save_contact(images, png)
    for name, fraction in (("front", 0), ("quarter", .25), ("back", .5), ("three_quarter", .75)):
        images[round(fraction * args.frames) % args.frames].save(args.output / (args.stem + "_" + name + ".png"))
    if args.baseline_gif:
        comparison(images, args.baseline_gif, args.output, args.stem,
                   args.before_label, args.after_label)
    print(f"Rendered {len(images)} frames to {gif} and {png}", flush=True)


if __name__ == "__main__":
    main()
