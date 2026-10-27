#!/usr/bin/env python3
"""Import data/metadata only; never execute the ModLoader scripts in a pack."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
INPUTS = (
    (1, "Emote_Kawaii_Wave.pak", "wave", 81),
    (2, "Emote_Angry.pak", "angry", 384),
    (3, "Emote_Backflip.pak", "backflip", 84),
    (4, "Emote_Bashful.pak", "bashful", 220),
    (5, "Emote_Dab.pak", "dab", 29),
    (6, "Emote_Malon_Sing.pak", "malon_sing", 1359),
    (7, "Emote_StylishSheathing.pak", "sheathing", 65),
    (8, "Emote_Tiny_Little_Adiantum.pak", "adiantum", 80),
)


def read_pak(path):
    data = path.read_bytes()
    if data[:11] != b"ModLoader64":
        raise ValueError("not a ModLoader64 archive: " + str(path))
    entries = {}
    offset = 16
    while offset + 16 <= len(data) and data[offset:offset + 4] in (b"UNCO", b"DEFL"):
        tag = data[offset:offset + 4]
        name_at, start, end = struct.unpack_from(">III", data, offset + 4)
        if not (16 <= name_at < len(data) and 0 <= start <= end <= len(data)):
            raise ValueError("invalid archive offsets")
        name = data[name_at:data.index(b"\xff", name_at)].decode("utf-8")
        if ".." in PurePosixPath(name).parts or name.startswith("/"):
            raise ValueError("invalid archive path")
        blob = data[start:end]
        if tag == b"DEFL":
            blob = zlib.decompress(blob)
        if name in entries:
            raise ValueError("duplicate archive path")
        entries[name] = blob
        offset += 16
    if not entries:
        raise ValueError("empty archive")
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_directory", type=Path)
    args = parser.parse_args()
    output = {}
    manifest = []
    credits = ["Bundled ComboShip emotes", "", "Original creator credits from the supplied pack manifests:", ""]
    for identifier, filename, stem, frames in INPUTS:
        source = args.input_directory / filename
        entries = read_pak(source)
        manifests = [name for name in entries if name.endswith("/package.json")]
        if len(manifests) != 1:
            raise ValueError("expected one manifest: " + filename)
        metadata = json.loads(entries[manifests[0]])
        emote = metadata["emote"]
        prefix = manifests[0].rsplit("/", 1)[0] + "/"
        animation = entries[prefix + emote["anim"]]
        if len(animation) != frames * 134:
            raise ValueError("unexpected frame count: " + filename)
        anim_name = stem + ".bin"
        output[anim_name] = animation
        audio_name = ""
        if emote.get("sound"):
            audio_name = stem + ".pcm"
            with tempfile.TemporaryDirectory(prefix="emote-audio-") as temp:
                original = Path(temp) / "input.ogg"
                pcm = Path(temp) / "output.pcm"
                original.write_bytes(entries[prefix + emote["sound"]])
                subprocess.run(["ffmpeg", "-v", "error", "-i", str(original), "-ac", "1", "-ar", "32000",
                                "-f", "s16le", str(pcm)], check=True)
                output[audio_name] = pcm.read_bytes()
        author = metadata.get("author") or "Author unspecified in supplied pack"
        license_name = metadata.get("license", "Unspecified")
        credits.append(f"{emote['name']} — {author}; pack license: {license_name}")
        manifest.append(dict(id=identifier, name=emote["name"], author=author, license=license_name,
                             animation=anim_name, frames=frames, audio=audio_name, audio_rate=32000,
                             animation_sha256=hashlib.sha256(animation).hexdigest(),
                             source_pack=filename, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
    output["manifest.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    credits += ["", "Malon Rose / MalonRose is credited for her original animations.",
                "The supplied assets are bundled at the user's direction with creator attribution.",
                "Original ModLoader64 plugin scripts are not bundled or executed.", ""]
    output["CREDITS.txt"] = "\n".join(credits).encode()
    for game in ("soh", "mm"):
        folder = ROOT / game / "assets/custom/emotes"
        folder.mkdir(parents=True, exist_ok=True)
        for name, data in output.items():
            (folder / name).write_bytes(data)
    print(f"Imported {len(manifest)} animations and 2 sounds into both built-in asset archives")


if __name__ == "__main__":
    main()
