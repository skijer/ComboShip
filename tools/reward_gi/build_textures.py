#!/usr/bin/env python3
"""Build seven deterministic, periodic 4K reward-GI material resources.

The six energy images are white RGB with an analytic, transparent alpha mask.
Color belongs to the renderer: no medallion, stone, symbol, or geometry is baked
into these private tiles. Numpy and Pillow are required; ffmpeg is optional.
"""

import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / "soh/assets/custom/objects/nei_reward_gi"
BASE_COMMIT = "56c83a878562052ca873b7d414f7d0e1175b6325"
SIZE = 4096
LOGICAL_SIZE = 32
ROWS = 128
TAU = 2.0 * math.pi
PROFILES = ("forest", "fire", "water", "spirit", "shadow", "light", "metal")
COLORS = {
    "forest": "#54D85B",
    "fire": "#FF593C",
    "water": "#459DFF",
    "spirit": "#FFA64D",
    "shadow": "#A675EB",
    "light": "#FFE58A",
    "metal": None,
}
STONE_COLORS = {"emerald": "#46E879", "ruby": "#FF4564", "sapphire": "#459DFF"}
STONE_MASKS = {"emerald": "forest", "ruby": "fire", "sapphire": "water"}
DESCRIPTIONS = {
    "forest": "Soft flowing currents; shared with Kokiri Emerald.",
    "fire": "Rising flame and ember filaments; shared with Goron Ruby.",
    "water": "Soft caustic network; shared with Zora Sapphire.",
    "spirit": "Spiraling flowing wisps.",
    "shadow": "Diffuse smoky wisps.",
    "light": "Luminous radial and ripple currents.",
    "metal": "Restrained neutral directional brushing; no baked symbols.",
}
KEYS = {slug: "objects/nei_reward_gi/" + slug for slug in PROFILES}
HEADER = bytearray(64)
struct.pack_into("<5I", HEADER, 0, 0, 0x4F544558, 1, 0xDEADBEEF, 0xDEADBEEF)
struct.pack_into("<I", HEADER, 24, 1)
HEADER = bytes(HEADER)
METADATA = struct.pack("<4I2fI", 1, SIZE, SIZE, 3, 1.0, 1.0, SIZE * SIZE * 4)


def sha_file(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def sin(phase):
    """phase is in cycles. Integer coordinate coefficients preserve period 1."""
    return np.sin(TAU * phase)


def cos(phase):
    return np.cos(TAU * phase)


def ridge(phase, sharpness):
    return np.maximum(0.0, 0.5 + 0.5 * cos(phase)) ** sharpness


def energy(slug, x, y):
    """Analytic alpha in [0,1], periodic in x and y, with sparse bright areas.

    Every coordinate-dependent trigonometric phase has integer frequencies.
    Nonlinear warps depend only on periodic fields, so neither coordinate has
    a cut, mirrored edge, border fade, or stochastic seam correction.
    """
    if slug == "forest":
        # Long soft streams, bent by broad, low-frequency currents.
        bend = 0.32 * sin(y) + 0.13 * sin(x + y + 0.13)
        main = ridge(2 * x + y + bend, 10)
        secondary = ridge(x - 2 * y + 0.27 * sin(x - y) + 0.32, 12)
        pulse = 0.55 + 0.45 * ridge(x + y + 0.18 * sin(y), 2)
        halo = ridge(2 * x + y + bend, 2)
        alpha = 0.37 * main * pulse + 0.17 * secondary + 0.10 * halo
    elif slug == "fire":
        # Vertical filaments broaden and taper periodically, rather than a
        # flame photograph with a visible base/top on a repeated tile.
        sway = 0.27 * sin(y) + 0.11 * sin(2 * y + 0.17 * sin(x))
        tongues = ridge(3 * x + sway + 0.07 * sin(x - y), 13)
        taper = 0.16 + 0.84 * ridge(y + 0.28 * sin(x) + 0.18, 2)
        filaments = ridge(2 * x - y + 0.26 * sin(y + x) + 0.29, 15)
        embers = ridge(2 * x + y + 0.13 * sin(y), 19) * ridge(2 * y - x + 0.41, 4)
        alpha = 0.57 * tongues * taper + 0.17 * filaments + 0.17 * embers
        alpha += 0.055 * ridge(3 * x + sway, 3) * taper
    elif slug == "water":
        # Warped low-frequency level sets form a readable caustic mesh.
        u = x + 0.085 * sin(y) + 0.04 * sin(x - y + 0.2)
        v = y + 0.080 * sin(x + 0.1) + 0.04 * sin(x + y)
        field = cos(2 * u + v) + 0.72 * cos(u - 2 * v + 0.18)
        caustic = np.exp(-((field / 0.15) ** 2))
        soft = np.exp(-((field / 0.46) ** 2))
        pulse = 0.45 + 0.55 * ridge(x - y + 0.1 * sin(x + y), 1)
        alpha = 0.51 * caustic * pulse + 0.105 * soft
    elif slug == "spirit":
        # A regularized complex vortex produces curved spiral wisps without
        # atan2 branch cuts or a hard radial stencil. Offset second currents
        # soften the periodic vortex arrangement.
        sx, sy = sin(x - 0.31), sin(y - 0.44)
        r = np.sqrt(sx * sx + sy * sy + 0.045)
        a, b = sx / r, sy / r
        phase = 1.65 * r + 0.12 * sin(x + y)
        spiral = (a * a - b * b) * cos(phase) + 2 * a * b * sin(phase)
        wisps = np.maximum(0.0, 0.5 + 0.5 * spiral) ** 9
        carrier = ridge(x + 2 * y + 0.38 * sin(x - y) + 0.33, 13)
        halo = np.maximum(0.0, 0.5 + 0.5 * spiral) ** 2
        pulse = 0.4 + 0.6 * ridge(x - y + 0.15, 1)
        alpha = 0.56 * wisps * pulse + 0.14 * carrier + 0.065 * halo
    elif slug == "shadow":
        # Smooth, layered smoke; no high-entropy noise, particles or glyphs.
        u = x + 0.11 * sin(y + 0.2) + 0.065 * sin(x - y)
        v = y + 0.14 * sin(x + 0.3)
        smoke = 0.52 * sin(u + v) + 0.31 * sin(2 * u - v + 0.23)
        smoke += 0.17 * sin(u - 3 * v + 0.49)
        cloud = np.clip((smoke + 0.12) / 0.88, 0, 1) ** 2
        wisps = ridge(2 * u + v + 0.2 * sin(u - v), 8)
        alpha = 0.36 * cloud + 0.24 * wisps * (0.28 + 0.72 * cloud)
    elif slug == "light":
        # Toroidal chord distance is intrinsically periodic in both axes.
        # Broad ripple currents stay soft and do not create a static emblem.
        dx = np.sin(math.pi * (x - 0.37)) ** 2
        dy = np.sin(math.pi * (y - 0.41)) ** 2
        radius = np.sqrt(dx + dy + 0.08)
        wave = 2.5 * radius + 0.08 * sin(x + y)
        ripples = ridge(wave, 12)
        halo = ridge(wave, 2)
        radial = np.exp(-2.2 * (dx + dy))
        current = ridge(x - y + 0.16 * sin(x + y) + 0.08, 12)
        alpha = 0.48 * ripples * (0.4 + 0.6 * radial) + 0.08 * halo
        alpha += 0.11 * current * (0.4 + 0.6 * radial)
    else:
        raise ValueError(slug)
    return np.clip(alpha, 0, 0.86)


def metal(x, y):
    """Low-contrast neutral metal with periodic fine horizontal brushing."""
    macro = 5.4 * cos(x + 0.035 * sin(y)) + 2.2 * sin(x + y + 0.11)
    brushing = 1.65 * sin(59 * y + 0.08 * sin(x))
    brushing += 1.10 * sin(137 * y + 0.12 * sin(x + y))
    brushing += 0.70 * sin(293 * y + 0.07 * sin(2 * x + 0.17))
    brushing += 0.45 * sin(509 * y + 0.05 * sin(x - y))
    return np.clip(221.0 + macro + brushing, 0, 255)


def pixels(slug, x, y):
    channel = metal(x, y) if slug == "metal" else energy(slug, x, y) * 255
    quantized = np.rint(channel).astype(np.uint8)
    rgba = np.empty((*quantized.shape, 4), dtype=np.uint8)
    if slug == "metal":
        rgba[:, :, :3] = quantized[:, :, None]
        rgba[:, :, 3] = 255
    else:
        rgba[:, :, :3] = 255
        rgba[:, :, 3] = quantized
    return rgba


def periodicity(slug):
    # Evaluate the actual field on translated, off-grid coordinates, including
    # all edges. First and last image rows are adjacent samples, not duplicate
    # endpoints; demanding byte equality there would introduce a stalled seam.
    coordinates = np.linspace(0, 1, 257, endpoint=True, dtype=np.float64)
    x, y = np.meshgrid(coordinates, coordinates)
    evaluator = metal if slug == "metal" else lambda xx, yy: energy(slug, xx, yy)
    field = evaluator(x, y)
    x_error = float(np.max(np.abs(field - evaluator(x + 1, y))))
    y_error = float(np.max(np.abs(field - evaluator(x, y + 1))))
    assert x_error < 1e-9 and y_error < 1e-9, (slug, x_error, y_error)
    # First derivatives also agree at both ends: no edge fade or hard band.
    epsilon = 1e-5
    grad_x = (evaluator(x + epsilon, y) - evaluator(x - epsilon, y)) / (2 * epsilon)
    shifted_x = (evaluator(x + 1 + epsilon, y) - evaluator(x + 1 - epsilon, y)) / (2 * epsilon)
    grad_y = (evaluator(x, y + epsilon) - evaluator(x, y - epsilon)) / (2 * epsilon)
    shifted_y = (evaluator(x, y + 1 + epsilon) - evaluator(x, y + 1 - epsilon)) / (2 * epsilon)
    derivative_error = max(float(np.max(np.abs(grad_x - shifted_x))),
                           float(np.max(np.abs(grad_y - shifted_y))))
    assert derivative_error < 1e-5, (slug, derivative_error)
    return {"analytic_x_translation_max_error": x_error,
            "analytic_y_translation_max_error": y_error,
            "first_derivative_translation_max_error": derivative_error,
            "period_texels_physical": [SIZE, SIZE],
            "period_texels_logical": [LOGICAL_SIZE, LOGICAL_SIZE]}


def write_resource(slug):
    target = DEST / slug
    x = np.arange(SIZE, dtype=np.float64)[None, :] / SIZE
    pixel_sha = hashlib.sha256()
    with target.open("wb") as output:
        output.write(HEADER)
        output.write(METADATA)
        for row in range(0, SIZE, ROWS):
            y = np.arange(row, min(row + ROWS, SIZE), dtype=np.float64)[:, None] / SIZE
            data = pixels(slug, x, y).tobytes()
            pixel_sha.update(data)
            output.write(data)
    return pixel_sha.hexdigest()


def read_pixels(path):
    return np.memmap(path, mode="r", dtype=np.uint8, offset=92, shape=(SIZE, SIZE, 4))


def validate_resource(slug):
    target = DEST / slug
    with target.open("rb") as source:
        assert source.read(64) == HEADER, slug
        assert source.read(28) == METADATA, slug
    assert target.stat().st_size == 92 + SIZE * SIZE * 4, slug
    rgba = read_pixels(target)
    histogram = np.zeros(256, dtype=np.int64)
    for row in range(0, SIZE, ROWS):
        block = rgba[row:row + ROWS]
        if slug == "metal":
            assert np.array_equal(block[:, :, 0], block[:, :, 1]), slug
            assert np.array_equal(block[:, :, 1], block[:, :, 2]), slug
            assert np.all(block[:, :, 3] == 255), slug
            channel = block[:, :, 0]
        else:
            assert np.all(block[:, :, :3] == 255), slug
            channel = block[:, :, 3]
        histogram += np.bincount(channel.ravel(), minlength=256)
    active = np.flatnonzero(histogram)
    channel = rgba[:, :, 0 if slug == "metal" else 3]
    # The rendered wrap step must be within the range of interior steps. This
    # catches accidental image borders independently of the analytic proof.
    x_wrap = np.abs(channel[:, 0].astype(np.int16) - channel[:, -1].astype(np.int16))
    y_wrap = np.abs(channel[0].astype(np.int16) - channel[-1].astype(np.int16))
    x_interior_max, y_interior_max = 0, 0
    for row in range(0, SIZE, ROWS):
        strip = channel[row:row + ROWS].astype(np.int16)
        x_interior_max = max(x_interior_max, int(np.max(np.abs(np.diff(strip, axis=1)))))
        if row + ROWS < SIZE:
            strip = channel[row:row + ROWS + 1].astype(np.int16)
        y_interior_max = max(y_interior_max, int(np.max(np.abs(np.diff(strip, axis=0)))))
    assert int(x_wrap.max()) <= x_interior_max + 1, slug
    assert int(y_wrap.max()) <= y_interior_max + 1, slug
    pixel_sha = hashlib.sha256()
    with target.open("rb") as source:
        source.seek(92)
        for block in iter(lambda: source.read(1024 * 1024), b""):
            pixel_sha.update(block)
    count = SIZE * SIZE
    result = {"target": KEYS[slug], "description": DESCRIPTIONS[slug],
              "pixels": [SIZE, SIZE], "logical_tile": [LOGICAL_SIZE, LOGICAL_SIZE],
              "texture_type": "RGBA32bpp", "type_value": 1, "flags": 3,
              "flags_names": ["LOAD_AS_RAW", "LOAD_AS_IMG"],
              "h_byte_scale": 1.0, "v_pixel_scale": 1.0,
              "header_bytes": 64, "metadata_bytes": 28,
              "payload_bytes": SIZE * SIZE * 4, "resource_bytes": target.stat().st_size,
              "pixel_sha256": pixel_sha.hexdigest(), "resource_sha256": sha_file(target),
              "tint_hex": COLORS[slug], "channel_min": int(active[0]),
              "channel_max": int(active[-1]),
              "channel_mean": float(np.dot(histogram, np.arange(256)) / count),
              "periodicity": periodicity(slug),
              "wrap_adjacent_max_difference": [int(x_wrap.max()), int(y_wrap.max())],
              "interior_adjacent_max_difference": [x_interior_max, y_interior_max]}
    if slug != "metal":
        assert active[0] == 0 and active[-1] < 255, slug
        assert histogram[:24].sum() / count > 0.35, slug
        assert histogram[192:].sum() / count < 0.03, slug
        result.update(alpha_zero_fraction=float(histogram[0] / count),
                      alpha_below_24_fraction=float(histogram[:24].sum() / count),
                      alpha_at_least_192_fraction=float(histogram[192:].sum() / count),
                      rgb="constant white; exact tint supplied at draw")
    else:
        assert active[-1] - active[0] <= 32, slug
        result.update(alpha="constant opaque 255", rgb="neutral equal-channel brushed metal")
    del channel, rgba
    return result


def make_pack(output):
    pack = output / "Reward_GI_Energy_4K_POC1_Assets.o2r"
    with zipfile.ZipFile(pack, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for slug in PROFILES:
            entry = zipfile.ZipInfo(KEYS[slug], (2026, 10, 9, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            with archive.open(entry, "w", force_zip64=False) as destination:
                with (DEST / slug).open("rb") as source:
                    shutil.copyfileobj(source, destination, length=1024 * 1024)
    with zipfile.ZipFile(pack) as archive:
        assert archive.namelist() == [KEYS[slug] for slug in PROFILES]
        assert archive.testzip() is None
        for slug in PROFILES:
            digest = hashlib.sha256()
            with archive.open(KEYS[slug]) as source:
                for block in iter(lambda: source.read(1024 * 1024), b""):
                    digest.update(block)
            assert digest.hexdigest() == sha_file(DEST / slug), slug
    return pack


def font(size):
    for path in ("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                 "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"):
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size=size)


def thumbnail(slug, size):
    # All previews are decoded from the actual written 4K resource payload.
    mapped = read_pixels(DEST / slug)
    source = Image.fromarray(np.asarray(mapped), "RGBA")
    small = source.resize((size, size), Image.Resampling.LANCZOS)
    del source, mapped
    array = np.asarray(small).copy()
    if slug != "metal":
        color = bytes.fromhex(COLORS[slug][1:])
        array[:, :, :3] = np.array(tuple(color), dtype=np.uint8)
    return Image.fromarray(array, "RGBA")


def composite(tile):
    background = Image.new("RGBA", tile.size, (10, 15, 22, 255))
    return Image.alpha_composite(background, tile).convert("RGB")


def previews(output):
    tiles = {slug: thumbnail(slug, 384) for slug in PROFILES}
    sheet = Image.new("RGB", (1680, 1120), (15, 20, 29))
    draw = ImageDraw.Draw(sheet)
    draw.text((32, 24), "Reward GI · actual 4096 × 4096 material tiles", font=font(31), fill=(238, 242, 249))
    draw.text((33, 69), "Texture-art preview · candidate tints · no in-game geometry or rendering proof", font=font(18), fill=(167, 181, 202))
    for index, slug in enumerate(PROFILES):
        x, y = 24 + (index % 4) * 416, 118 + (index // 4) * 484
        sheet.paste(composite(tiles[slug]), (x + 8, y + 8))
        draw.text((x + 8, y + 409), slug.title(), font=font(24), fill=(239, 243, 249))
        label = COLORS[slug] if COLORS[slug] else "Neutral brushed-metal surface"
        draw.text((x + 8, y + 443), label, font=font(18), fill=(164, 179, 200))
    x, y = 1280, 633
    draw.text((x, y), "One shared material pass", font=font(23), fill=(229, 235, 245))
    for line, offset in (("Emerald " + STONE_COLORS["emerald"] + " → Forest", 49),
                         ("Ruby " + STONE_COLORS["ruby"] + " → Fire", 86),
                         ("Sapphire " + STONE_COLORS["sapphire"] + " → Water", 123),
                         ("No baked reward emblems", 190), ("Periodic in both axes", 227), ("White RGB + transparent alpha", 264)):
        draw.text((x, y + offset), line, font=font(18), fill=(166, 182, 205))
    sheet_path = output / "reward_gi_4k_contact_sheet.png"
    sheet.save(sheet_path, optimize=True)

    # A repeated 2×2 image gives an independent visible seam check.
    proof = Image.new("RGB", (1560, 890), (15, 20, 29))
    pdraw = ImageDraw.Draw(proof)
    pdraw.text((25, 20), "Periodic wrap inspection · actual texture payloads repeated 2 × 2", font=font(24), fill=(236, 242, 250))
    for index, slug in enumerate(PROFILES):
        tile = composite(thumbnail(slug, 180))
        x, y = 24 + (index % 4) * 388, 74 + (index // 4) * 400
        for dx in (0, 180):
            for dy in (0, 180):
                proof.paste(tile, (x + dx, y + dy))
        pdraw.text((x, y + 365), slug.title(), font=font(18), fill=(181, 196, 217))
    proof_path = output / "reward_gi_4k_periodic_wrap.png"
    proof.save(proof_path, optimize=True)

    moving = {slug: np.asarray(thumbnail(slug, 256)) for slug in PROFILES[:-1]}
    frames = []
    for frame in range(64):
        image = Image.new("RGB", (848, 692), (15, 20, 29))
        fdraw = ImageDraw.Draw(image)
        fdraw.text((24, 15), "Scrolling texture preview · no in-game geometry", font=font(24), fill=(235, 241, 250))
        fdraw.text((24, 49), "32 × 32 logical tile · quarter-texel motion · sampled every 2 steps", font=font(16), fill=(165, 183, 207))
        for index, slug in enumerate(PROFILES[:-1]):
            tile = np.roll(moving[slug], shift=(-4 * frame, 4 * frame), axis=(0, 1))
            x, y = 24 + (index % 3) * 272, 84 + (index // 3) * 296
            image.paste(composite(Image.fromarray(tile, "RGBA")), (x, y))
            fdraw.text((x, y + 261), slug.title() + " " + COLORS[slug], font=font(16), fill=(187, 202, 222))
        frames.append(image)
    # The next frame has translated exactly one full tile on each axis.
    assert all(np.array_equal(moving[slug], np.roll(moving[slug], (-256, 256), (0, 1)))
               for slug in PROFILES[:-1])
    poster_path = output / "reward_gi_4k_scrolling_poster.png"
    frames[0].save(poster_path, optimize=True)
    palette = frames[0].quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    quantized = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    gif_path = output / "reward_gi_4k_scrolling_preview.gif"
    quantized[0].save(gif_path, save_all=True, append_images=quantized[1:],
                      loop=0, duration=80, optimize=False, disposal=2)
    files = [sheet_path, proof_path, poster_path, gif_path]
    if shutil.which("ffmpeg"):
        movie_path = output / "reward_gi_4k_scrolling_preview.mp4"
        command = ["ffmpeg", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
                   "-s", "848x692", "-r", "12", "-i", "-", "-an", "-c:v", "libx264",
                   "-preset", "medium", "-crf", "22", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(movie_path)]
        with subprocess.Popen(command, stdin=subprocess.PIPE) as encoder:
            for frame in frames:
                encoder.stdin.write(frame.tobytes())
            encoder.stdin.close()
            assert encoder.wait() == 0, "ffmpeg failed"
        files.append(movie_path)
    return {path.name: {"sha256": sha_file(path), "bytes": path.stat().st_size} for path in files}


def build(output, skip_previews=False):
    DEST.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    manifest = {"candidate": "Reward GI Energy 4K POC1", "base_commit": BASE_COMMIT,
                "generator": "tools/reward_gi/build_textures.py", "generator_sha256": sha_file(__file__),
                "generator_dependencies": {"numpy": np.__version__, "Pillow": Image.__version__},
                "construction": "Deterministic analytic periodic fields; no random noise, baked emblems, or geometry.",
                "tint_status": "candidate",
                "stone_materials": {stone: {"mask": KEYS[STONE_MASKS[stone]], "tint_hex": color}
                                    for stone, color in STONE_COLORS.items()},
                "physical_dimensions": [SIZE, SIZE], "logical_tile_dimensions": [LOGICAL_SIZE, LOGICAL_SIZE],
                "scroll_step_logical_texels": 0.25, "scroll_step_physical_texels": SIZE / LOGICAL_SIZE / 4,
                "resource_format": "64-byte OTR header, V1 <4I2fI> texture metadata, row-major RGBA bytes",
                "resource_scope": [KEYS[slug] for slug in PROFILES],
                "verification_scope": "Texture art, binary format, archive contents and periodicity only; runtime is untested.",
                "entries": {}}
    for slug in PROFILES:
        print("Building " + slug, flush=True)
        pixel_sha = write_resource(slug)
        manifest["entries"][slug] = validate_resource(slug)
        assert manifest["entries"][slug]["pixel_sha256"] == pixel_sha
        print("PASS " + slug + ": 4096 RGBA32, alpha/neutral-color rules, analytic and sampled periodicity", flush=True)
    pack = make_pack(output)
    manifest["pack"] = {"filename": pack.name, "sha256": sha_file(pack),
                        "bytes": pack.stat().st_size, "entry_count": len(PROFILES),
                        "compression": "ZIP DEFLATE", "closed_scope_verified": True}
    if not skip_previews:
        manifest["previews"] = previews(output)
        manifest["preview_scroll"] = {"frame_count": 64, "gif_loop": "infinite", "sample_every_quarter_steps": 2,
                                      "quarter_steps_per_cycle": 128, "cycle_translation_logical_texels": [32, -32],
                                      "end_to_start_periodic": True, "proof": "texture preview only"}
    encoded = json.dumps(manifest, indent=2) + "\n"
    Path(__file__).with_name("material_manifest.json").write_text(encoded)
    (output / "reward_gi_4k_material_manifest.json").write_text(encoded)
    print("PASS seven private resources; closed archive; manifest with pixel/resource/pack hashes", flush=True)
    print("Pack: " + str(pack) + " (" + str(pack.stat().st_size) + " bytes)", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--skip-previews", action="store_true")
    args = parser.parse_args()
    build(args.output, args.skip_previews)
