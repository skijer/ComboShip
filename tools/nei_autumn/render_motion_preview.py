"""Replay captured production draw transforms; this is not a game/GPU render."""
import argparse
import base64
import io
import json
import math
from pathlib import Path
import struct
import subprocess

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]


def sprites():
    result = []
    for name in ("crimson", "orange", "gold", "copper"):
        data = (ROOT / "mm/assets/custom/objects/nei_autumn/leaves" / (name + "_tex")).read_bytes()
        assert len(data) == 92 + 128 * 128 * 4
        result.append(Image.frombytes("RGBA", (128, 128), data[92:]))
    return result


def image_url(image, format="PNG"):
    buffer = io.BytesIO()
    image.save(buffer, format=format, **({"quality":82} if format == "JPEG" else {}))
    return "data:image/" + format.lower() + ";base64," + base64.b64encode(buffer.getvalue()).decode()


def packed(data):
    output = bytearray()
    for frame in data["frames"]:
        output.extend(struct.pack("<H", len(frame)))
        for row in frame:
            output.extend(struct.pack("<5fBBBh", *row[:5], *[round(value) for value in row[5:]]))
    return base64.b64encode(output).decode()


def poses(data, time):
    position = min(time * data["fps"], len(data["frames"]) - 1)
    i = int(position)
    fraction = position % 1
    current = data["frames"][i]
    following = {tuple(row[7:9]):row for row in data["frames"][min(i + 1, len(data["frames"]) - 1)]}
    for row in current:
        other = following.get(tuple(row[7:9]))
        pose = list(row)
        if other:
            for j in (0, 1, 2, 3, 5):
                pose[j] += (other[j] - pose[j]) * fraction
            angle = (other[4] - row[4] + math.pi) % (2 * math.pi) - math.pi
            pose[4] += angle * fraction
        else:
            pose[5] *= 1 - fraction
        yield pose


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--current", type=Path, required=True)
    parser.add_argument("--revised", type=Path, required=True)
    parser.add_argument("--background", type=Path, required=True)
    parser.add_argument("--html", type=Path, required=True)
    parser.add_argument("--video", type=Path, required=True)
    args = parser.parse_args()
    current = json.loads(args.current.read_text())
    revised = json.loads(args.revised.read_text())
    art = sprites()
    background = Image.open(args.background).convert("RGB")
    html = (Path(__file__).with_name("motion_preview_fragment.html")).read_text()
    html = html.replace("{{TRAJECTORIES}}", json.dumps({"current":packed(current), "revised":packed(revised)}))
    html = html.replace("{{BACKGROUND}}", image_url(background.resize((960, 540)), "JPEG"))
    html = html.replace("{{SPRITES}}", json.dumps([image_url(image.resize((64, 64))) for image in art]))
    assert len(html.encode()) < 1_000_000
    args.html.write_text(html)

    background = background.resize((1280, 720)).convert("RGBA")
    args.video.parent.mkdir(parents=True, exist_ok=True)
    encoder = subprocess.Popen(["ffmpeg", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
                                "-s", "1280x720", "-r", "30", "-i", "-", "-an", "-c:v", "libx264",
                                "-preset", "fast", "-crf", "20", "-pix_fmt", "yuv420p", "-movflags", "+faststart",
                                str(args.video)], stdin=subprocess.PIPE)
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20)
    cache = {}
    for frame in range(360):
        image = background.copy()
        for x, y, width, height, angle, alpha, palette, _, _ in poses(revised, frame / 30):
            w, h = max(1, round(width * 1280)), max(1, round(height * 720))
            degrees = round(math.degrees(angle) / 3) * 3
            key = (int(palette), w, h, degrees)
            if key not in cache:
                cache[key] = art[int(palette)].resize((w, h), Image.Resampling.LANCZOS).rotate(
                    degrees, resample=Image.Resampling.BICUBIC, expand=True)
            sprite = cache[key].copy()
            sprite.putalpha(sprite.getchannel("A").point(lambda v: round(v * alpha / 255)))
            image.alpha_composite(sprite, (round(x * 1280 - sprite.width / 2), round(y * 720 - sprite.height / 2)))
        draw = ImageDraw.Draw(image)
        draw.rounded_rectangle((24, 641, 652, 706), radius=10, fill=(15, 17, 18, 220))
        draw.text((39, 650), "Revised autumn fall · production motion simulation", font=font, fill=(255, 255, 255))
        draw.text((39, 677), "96 on-screen slots · no depth occlusion", font=font, fill=(214, 217, 217))
        encoder.stdin.write(image.convert("RGB").tobytes())
    encoder.stdin.close()
    if encoder.wait():
        raise RuntimeError("Preview video encoding failed")
    print("Saved production-trajectory preview:", args.html, args.video)


if __name__ == "__main__":
    main()
