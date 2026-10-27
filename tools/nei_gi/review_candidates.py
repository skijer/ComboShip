"""Make review sheets from the exact authored GI/held mesh checkpoints.

These use the checkpoint's offline lighting, not game captures. Seasonal
selection/weather GIs are intentionally absent from this candidate manifest.
"""
import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
EQUIPMENT = (
    "divine_shield", "sheikah_shield", "shield_of_ikana", "magic_cape",
    "spirit_breastplate", "sages_tunic", "champions_tunic", "pegasus_anklet",
    "trident", "climb_boots", "roc_boots", "cane_of_byrna", "four_sword",
    "pendant_of_memories",
)
SWORDS = (
    "kokiri_sword", "razor_sword", "gilded_sword", "master_sword",
    "true_master_sword", "biggoron_sword", "great_fairy_sword", "iron_knuckle_axe",
)
QUEST = (
    "elemental_wand", "sand_rod", "tornado_rod", "water_rod",
    "meteor_rod", "storm_rod", "shadow_scepter", "sheikah_slate",
    "slate_bomb", "slate_master_cycle", "slate_stasis", "slate_cryonis",
    "slate_sensor", "phantom_hourglass", "shadow_crystal", "rod_of_seasons",
)
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


def sheet(items, title, output):
    width, cell_height = 300, 366
    canvas = Image.new("RGB", (width * 4, 86 + math.ceil(len(items) / 4) * cell_height), "#171d25")
    draw = ImageDraw.Draw(canvas)
    draw.text((18, 14), title, font=ImageFont.truetype(FONT, 27), fill="#edf2f8")
    draw.text((18, 53), "Exact candidate mesh checkpoints · offline lighting · front with reverse inset", font=ImageFont.truetype(FONT, 16), fill="#aab9c8")
    for index, slug in enumerate(items):
        directory = ROOT / "CHECKPOINTS" / slug
        meta = json.loads((directory / "checkpoint.json").read_text())
        x, y = (index % 4) * width, 86 + (index // 4) * cell_height
        canvas.paste(Image.open(directory / "front.png").resize((300, 300), Image.Resampling.LANCZOS), (x, y))
        canvas.paste(Image.open(directory / "back.png").resize((78, 78), Image.Resampling.LANCZOS), (x + 218, y + 218))
        name = meta["name"]
        size = 17
        while draw.textbbox((0, 0), name, font=ImageFont.truetype(FONT, size))[2] > 284:
            size -= 1
        draw.text((x + 8, y + 305), name, font=ImageFont.truetype(FONT, size), fill="#edf2f8")
        draw.text((x + 8, y + 333), f'{meta["triangles"]:,} triangles', font=ImageFont.truetype(FONT, 14), fill="#aab9c8")
    canvas.save(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for items, title, name in (
        (EQUIPMENT, "Equipment GI candidates", "NEI_Equipment_GI_Candidates.png"),
        (SWORDS, "Sword and axe GI candidates", "NEI_Sword_GI_Candidates.png"),
        (QUEST, "Wand and quest GI candidates", "NEI_Quest_GI_Candidates.png"),
    ):
        sheet(items, title, args.output / name)
    directory = ROOT.parent / "nei_held/CHECKPOINTS/rod_of_seasons"
    canvas = Image.new("RGB", (960, 568), "#171d25")
    draw = ImageDraw.Draw(canvas)
    draw.text((18, 13), "Held Rod of Seasons", font=ImageFont.truetype(FONT, 27), fill="#edf2f8")
    draw.text((18, 51), "Physical rod only · exact candidate mesh · offline lighting", font=ImageFont.truetype(FONT, 17), fill="#aab9c8")
    for column, view in enumerate(("front", "back")):
        canvas.paste(Image.open(directory / (view + ".png")), (column * 480, 86))
    canvas.save(args.output / "NEI_Held_Rod_of_Seasons.png")
    print(f"Saved 38 GI candidates and the held Rod preview to {args.output}")


if __name__ == "__main__":
    main()
