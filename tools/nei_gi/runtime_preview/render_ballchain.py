"""Matched Ball & Chain GI/held comparison from the exact exported checkpoints.

Uses the existing offline GL renderer and the exported texture-generation
metadata. Both versions share cameras and lighting; each model pair shares a
fit. This is a material preview, not an in-game capture or an animation test.
"""
import argparse
import ctypes as C
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw, ImageFont
import render_swords as r

REPO = Path(__file__).resolve().parents[3]
WIDTH, HEIGHT, ROW, TOP = 1200, 1080, 430, 150


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--before-root", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=120)
    args = parser.parse_args()
    if args.frames < 4:
        parser.error("At least four frames are required")
    args.output.mkdir(parents=True, exist_ok=True)
    r.W = WIDTH
    r.H = r.BUFFER_H = HEIGHT
    # The installed Ball & Chain combines TEXEL0 with SHADE. It does not
    # multiply texture RGB by PRIMITIVE, or add an extra specular shader.
    r.FRAGMENT_SHADER = r.FRAGMENT_SHADER.replace("vec3(specular)", "vec3(0.0)")
    renderer = r.Renderer()
    pairs = []
    for slug, kind, checkpoint_root in (
        ("ball_and_chain", "gi", REPO / "tools/nei_gi/CHECKPOINTS"),
        ("ball", "held", REPO / "tools/nei_held/CHECKPOINTS"),
    ):
        before = r.Model(slug, renderer, args.before_root / kind)
        after = r.Model(slug, renderer, checkpoint_root)
        for model in (before, after):
            for material in model.materials:
                if material["texture"]:
                    material["base"] = [1.0, 1.0, 1.0, material["base"][3]]
        assert np.array_equal(before.positions, after.positions)
        positions = after.positions
        center = (positions.min(axis=0) + positions.max(axis=0)) / 2
        radius = np.linalg.norm(positions - center, axis=1).max() * 1.14
        pairs.append((before, after, center, radius))
    font = lambda size: ImageFont.truetype(r.FONT, size)
    movie = args.output / "BallChain-Metallic-Comparison.mp4"
    process = subprocess.Popen(
        ["ffmpeg", "-loglevel", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
         "-s", f"{WIDTH}x{HEIGHT}", "-r", "20", "-i", "-", "-an", "-c:v", "libx264",
         "-preset", "fast", "-crf", "18", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(movie)],
        stdin=subprocess.PIPE,
    )
    captures = []
    for frame in range(args.frames):
        renderer.DepthMask(1)
        renderer.Clear(0x4000 | 0x0100)
        yaw = 25 + frame * 360 / args.frames
        for row, (before, after, center, radius) in enumerate(pairs):
            for column, model in enumerate((before, after)):
                viewport_top = TOP + row * ROW + 26
                renderer.Viewport(column * 600 + 25, HEIGHT - viewport_top - 360, 550, 360)
                renderer.MatrixMode(0x1701)
                renderer.LoadIdentity()
                half = radius * 550 / 360
                renderer.Ortho(-half, half, -radius, radius, -300, 300)
                renderer.MatrixMode(0x1700)
                renderer.LoadIdentity()
                renderer.Rotatef(12, 1, 0, 0)
                renderer.Rotatef(yaw, 0, 1, 0)
                renderer.Translatef(*(-center))
                renderer.draw_model(model)
        pixels = np.zeros((HEIGHT, WIDTH, 4), dtype=np.uint8)
        renderer.ReadPixels(0, 0, WIDTH, HEIGHT, 0x1908, 0x1401, pixels.ctypes.data)
        error = renderer.GetError()
        if error:
            raise RuntimeError(f"OpenGL error {error:#x}")
        image = Image.fromarray(pixels[::-1].copy()).convert("RGB")
        draw = ImageDraw.Draw(image)
        draw.text((30, 20), "Ball & Chain — metallic material pass", font=font(31), fill="#edf3f7")
        draw.text((30, 64), "Same exported geometry, camera and light in each pair", font=font(19), fill="#abb9c6")
        draw.text((160, 110), "BEFORE · fixed texture", font=font(23), fill="#bac3cd")
        draw.text((740, 110), "AFTER · brushed steel", font=font(23), fill="#eef3f7")
        for row, name in enumerate(("GI / pickup model", "Held / thrown ball")):
            draw.text((35, TOP + row * ROW), name, font=font(21), fill="#c7d5df")
        draw.line((600, 110, 600, 1000), fill="#384554", width=1)
        draw.line((25, 570, 1175, 570), fill="#384554", width=1)
        draw.text((30, 1024), "Offline preview · scene lighting and gameplay remain untested", font=font(19), fill="#a7b5c4")
        process.stdin.write(image.tobytes())
        if frame == 0:
            image.save(args.output / "BallChain-Metallic-Comparison.png")
        if frame in (0, args.frames // 4, args.frames // 2, 3 * args.frames // 4):
            captures.append(image)
    process.stdin.close()
    if process.wait():
        raise RuntimeError("FFmpeg failed to encode the comparison")
    contact = Image.new("RGB", (WIDTH * 2, HEIGHT * 2))
    for i, image in enumerate(captures):
        contact.paste(image, ((i % 2) * WIDTH, (i // 2) * HEIGHT))
    contact.save(args.output / "BallChain-Metallic-Four-Angles.png")
    print(f"Rendered {args.frames} matched frames from GI and held checkpoints", flush=True)


if __name__ == "__main__":
    main()
