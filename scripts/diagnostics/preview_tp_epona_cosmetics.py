#!/usr/bin/env python3
"""Offline semantic color previews; never writes source textures or checkpoints."""

import argparse
import importlib
from pathlib import Path
import tempfile
import zipfile

import numpy as np
from PIL import Image, ImageDraw

import build_tp_epona_cosmetics as b


def recolor(image, color, alpha=None):
    rgba = np.asarray(image.convert("RGBA")).copy()
    rgb = rgba[:, :, :3].astype(float)
    tinted = rgb.mean(2)[:, :, None] * np.array(color)[None, None, :] / 255
    if alpha is None:
        rgba[:, :, :3] = np.rint(tinted).clip(0, 255).astype(np.uint8)
    else:
        a = alpha[:, :, None].astype(float) / 255
        rgba[:, :, :3] = np.rint(rgb * (1 - a) + tinted * a).clip(0, 255).astype(np.uint8)
    return Image.fromarray(rgba)


def color_parts(horse, selected, masks, coat=None, hair=None, eyes=None, animation="gEponaIdleAnim", frame=0, blink="Open"):
    parts, images = horse.render_parts(animation, frame, blink)
    new_parts = []
    for index, part in enumerate(parts):
        selected_ids = selected.get({0: 3, 2: 2, 3: 4}.get(index, -1), set())
        active = (index == 0 and (coat or hair)) or (index in (2, 3) and hair)
        if active:
            material = images[part["texture"]]
            if index == 0:
                if coat: material = recolor(material, coat, masks["coat"])
                if hair: material = recolor(material, hair, masks["hair"])
            else:
                material = recolor(material, hair)
            selected_tri = np.array([t for i, t in enumerate(part["tri"]) if i in selected_ids])
            untouched_tri = np.array([t for i, t in enumerate(part["tri"]) if i not in selected_ids])
            if len(untouched_tri): new_parts.append({**part, "tri": untouched_tri})
            new_parts.append({**part, "tri": selected_tri, "texture": len(images)})
            images.append(material)
        elif index == 4 and eyes:
            images[part["texture"]] = recolor(images[part["texture"]], eyes)
            new_parts.append(part)
        else:
            new_parts.append(part)
    return new_parts, images


def fit_view(parts, yaw, size):
    angle = np.deg2rad(yaw)
    rotation = np.array([[np.cos(angle), 0, -np.sin(angle)], [0, 1, 0],
                         [np.sin(angle), 0, np.cos(angle)]])
    points = np.concatenate([p["pos"][np.unique(p["tri"])] for p in parts])
    projected = points @ rotation.T
    minimum, maximum = projected.min(0), projected.max(0)
    scale = min((size[0] - 70) / (maximum[0] - minimum[0]), (size[1] - 95) / (maximum[1] - minimum[1]))
    offset = np.array([(maximum[0] + minimum[0]) / 2, 0, 0]) @ rotation
    return [{**p, "pos": p["pos"] - offset} for p in parts], (maximum[1] + minimum[1]) / 2, scale * 6700 / (size[1] - 95)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--checkpoint", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    h = b.helpers(args.checkpoint)
    with zipfile.ZipFile(args.source) as z: original = {p: z.read(p) for p in z.namelist()}
    with zipfile.ZipFile(args.candidate) as z: files = {p: z.read(p) for p in z.namelist()}
    b.validate(original, files, h)
    selected = b.source_selection(args.checkpoint, h)
    typ, width, height, raw = h["o2r"].read_texture(original[b.OLD + "HorsSS00Tex"])
    source_image = Image.frombytes("RGBA", (width, height), raw)
    coat_mask, hair_mask = b.atlas_masks(np.asarray(source_image))
    masks = {"coat": coat_mask, "hair": hair_mask}
    args.output_dir.mkdir(parents=True, exist_ok=True)
    renderer = importlib.import_module("render").render
    decoder = importlib.import_module("verify_poc2")
    with tempfile.TemporaryDirectory(prefix="tp-cosmetic-preview-") as tmp:
        decoder.OUTPUT = Path(tmp)
        horse = decoder.ExportedHorse(args.candidate)
        baseline = decoder.ExportedHorse(args.source)
        # Default colors must produce pixel-identical offline rendering too.
        bp, bi = baseline.render_parts()
        cp, ci = horse.render_parts()
        default_equal = np.array_equal(np.asarray(renderer(bp, bi, yaw=75)), np.asarray(renderer(cp, ci, yaw=75)))
        b.require(default_equal, "default decoded render differs from POC3")
        modes = [("POC3 / POC4 default: identical", {}), ("Coat only: blue", {"coat": (35, 150, 255)}),
                 ("White hair only: magenta", {"hair": (255, 35, 210)}),
                 ("All three controls", {"coat": (35, 150, 255), "hair": (255, 35, 210), "eyes": (30, 255, 60)})]
        sheet = Image.new("RGB", (2400, 1430), (22, 29, 42))
        draw = ImageDraw.Draw(sheet)
        draw.text((18, 12), "TP Epona POC4 / decoded geometry and fixed masks / OFFLINE simulation, game acceptance pending", fill="white")
        for column, (label, kwargs) in enumerate(modes):
            for row, (yaw, animation, frame) in enumerate(((75, "gEponaIdleAnim", 0), (145, "gEponaJumpingHighAnim", 18))):
                pp, ii = color_parts(horse, selected, masks, **kwargs, animation=animation, frame=frame)
                pp, center, zoom = fit_view(pp, yaw, (600, 680))
                im = renderer(pp, ii, yaw=yaw, size=(600, 680), center_y=center, zoom=zoom,
                              label=label + (" / idle" if row == 0 else " / high jump"))
                sheet.paste(im, (column * 600, 40 + row * 690))
        sheet.save(args.output_dir / "TP_Epona_POC4_Color_Preview.png")
        blink_sheet = Image.new("RGB", (1800, 620), (22, 29, 42))
        for column, blink in enumerate(("Open", "Half", "Closed")):
            pp, ii = color_parts(horse, selected, masks, eyes=(30, 255, 60), blink=blink)
            body = horse.pose()[0]
            eyeids = np.unique(np.concatenate([p["tri"].ravel() for p in horse.parts[4:]]))
            center = body[eyeids].mean(0)
            pp = [{**p, "pos": p["pos"] - center} for p in pp]
            im = renderer(pp, ii, yaw=80, size=(600, 620), center_y=-300, zoom=3.0,
                          label="Green iris / " + blink + " / original eyelid pixels")
            blink_sheet.paste(im, (column * 600, 0))
        blink_sheet.save(args.output_dir / "TP_Epona_POC4_Iris_Blink_Preview.png")
    atlas_sheet = Image.new("RGB", (1800, 640), (22, 29, 42))
    for index, (image, label) in enumerate(((source_image, "Original atlas"),
            (recolor(source_image, (35, 150, 255), coat_mask), "Coat mask / blue"),
            (recolor(source_image, (255, 35, 210), hair_mask), "White hair mask / magenta"))):
        atlas_sheet.paste(image.convert("RGB").resize((600, 600)), (600 * index, 30))
        ImageDraw.Draw(atlas_sheet).text((600 * index + 10, 10), label, fill="white")
    atlas_sheet.save(args.output_dir / "TP_Epona_POC4_Atlas_Masks_Preview.png")
    print("Default decoded render matches POC3 pixel-for-pixel; three preview sheets written.")


if __name__ == "__main__":
    main()
