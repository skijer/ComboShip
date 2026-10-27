#!/usr/bin/env python3
"""Repair zero-offset animated G_DL addresses in the FINAL MM Lost Woods pack.

libultraship marks a segmented native pointer with bit zero. The binary DL
reader preserves packed words, so an unmarked 0x0B000000 remains a raw pointer.
Only this address flag is changed. Unsupported microcode, commands, offsets,
branch framing or animation bindings are rejected before writing a candidate.
This is a native compatibility repair, not evidence of a Prelude runtime fix.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

ANIMATION_PATH = "alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODSTexAnim_001BD0"
# ResourceFactoryBinaryDisplayListV0 reads these as 128-bit commands. In
# particular, neither G_DL nor an apparent opcode inside a hash is expanded.
EXPANDED = frozenset({0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42})
# Deliberately bounded to the FINAL pack's F3DEX2 commands and the factory's
# expanded resource commands. Unknown layouts must not be guessed or patched.
SUPPORTED = EXPANDED | frozenset({
    0x05, 0x06, 0x26, 0xD7, 0xD9, 0xDE, 0xDF, 0xE2, 0xE3, 0xE6, 0xE7,
    0xEF, 0xF2, 0xF3, 0xF5, 0xFA, 0xFB, 0xFC,
})


def _header(data, magic, path):
    if len(data) < 64 or data[0] != 0 or data[4:8] != magic or data[8:12] != b"\0" * 4:
        raise ValueError(f"{path}: unsupported little-endian version-zero resource header")


def animation_segments(data, path):
    """Validate the native scroll animation records and return bound segments."""
    _header(data, b"NATO", path)
    if len(data) < 72:
        raise ValueError(f"{path}: truncated animation")
    count = struct.unpack_from("<I", data, 64)[0]
    if not 1 <= count <= 8:
        raise ValueError(f"{path}: unsupported animation count {count}")
    offset, segments = 68, set()
    for index in range(count):
        if offset + 3 > len(data):
            raise ValueError(f"{path}: truncated animation entry")
        segment, kind = struct.unpack_from("<bh", data, offset)
        offset += 3
        native_segment = abs(segment) + 7
        if segment == 0 or native_segment > 15 or native_segment in segments:
            raise ValueError(f"{path}: duplicate or invalid animation segment {segment}")
        if (segment < 0) != (index == count - 1):
            raise ValueError(f"{path}: final animation entry must be the only negative segment")
        if kind not in (0, 1):
            raise ValueError(f"{path}: unsupported animation type {kind}")
        for _ in range(2 if kind == 1 else 1):
            if offset + 4 > len(data):
                raise ValueError(f"{path}: truncated scroll parameters")
            _, _, width, height = struct.unpack_from("<bbBB", data, offset)
            if not width or not height:
                raise ValueError(f"{path}: zero scroll dimensions")
            offset += 4
        segments.add(native_segment)
    # The retained export carries a four-byte zero trailer. The native factory
    # ignores it; require its exact framing rather than silently accepting data.
    if data[offset:] != b"\0" * 4:
        raise ValueError(f"{path}: unexpected animation trailer")
    return segments


def _commands(data, path):
    _header(data, b"TLDO", path)
    if len(data) < 80 or data[64] != 4 or (len(data) - 72) % 8:
        raise ValueError(f"{path}: unsupported or truncated F3DEX2 (ucode 4) display list")
    offset, branched = 72, False
    while offset < len(data):
        w0, w1 = struct.unpack_from("<II", data, offset)
        opcode = w0 >> 24
        if opcode not in SUPPORTED:
            raise ValueError(f"{path}+0x{offset:x}: unsupported opcode 0x{opcode:02x}")
        if branched and opcode != 0xDF:
            raise ValueError(f"{path}+0x{offset:x}: commands after terminal branch")
        width = 16 if opcode in EXPANDED else 8
        if offset + width > len(data):
            raise ValueError(f"{path}+0x{offset:x}: truncated expanded resource command")
        if opcode == 0xDE:
            if w0 not in (0xDE000000, 0xDE010000):
                raise ValueError(f"{path}+0x{offset:x}: unsupported G_DL push/branch fields")
            branched = w0 == 0xDE010000
        if opcode == 0x31 and w0 != 0x31000000:
            # gfx_dl_otr_hash_handler_custom implements pushed calls only;
            # a no-push hash branch enters its assert/unsupported path.
            raise ValueError(f"{path}+0x{offset:x}: unsupported hash display-list branch fields")
        if opcode == 0xDF and (w0 != 0xDF000000 or w1 != 0 or offset + width != len(data)):
            raise ValueError(f"{path}+0x{offset:x}: malformed terminator or trailing commands")
        yield offset, w0, w1, opcode
        offset += width
        if opcode == 0xDF:
            return
    raise ValueError(f"{path}: missing F3DEX2 EndDisplayList")


def repair_display_list(data, path, bound_segments):
    """Return patched bytes and a receipt; never change nonzero G_DL offsets."""
    result, changes = bytearray(data), []
    for offset, w0, w1, opcode in _commands(data, path):
        if opcode != 0xDE:
            continue
        segment = w1 >> 24
        if not 8 <= segment <= 15 or segment not in bound_segments:
            raise ValueError(f"{path}+0x{offset:x}: invalid or unbound G_DL segment {segment}")
        if w1 & 0x00FFFFFE:
            # Nonzero N64 Gfx byte offsets require native G_DL_INDEX conversion,
            # since sizeof(Gfx) differs on 64-bit hosts. They are outside this POC.
            raise ValueError(f"{path}+0x{offset:x}: nonzero G_DL offset requires a separate conversion")
        if w1 & 1:
            continue
        result[offset + 4] |= 1
        changes.append({
            "resource": path, "command_offset": offset, "byte_offset": offset + 4,
            "before_byte": data[offset + 4], "after_byte": result[offset + 4],
            "w0": f"{w0:08x}", "w1_before": f"{w1:08x}", "w1_after": f"{w1 | 1:08x}",
            "segment": segment, "mode": "branch" if w0 == 0xDE010000 else "push",
        })
    return bytes(result), changes


def _resources(source):
    with zipfile.ZipFile(source) as archive:
        names = archive.namelist()
        if len(set(names)) != len(names):
            raise ValueError("duplicate ZIP resource names are unsupported")
        return {name: archive.read(name) for name in names}


def _analyze(resources):
    if ANIMATION_PATH not in resources:
        raise ValueError(f"missing owning texture animation: {ANIMATION_PATH}")
    segments = animation_segments(resources[ANIMATION_PATH], ANIMATION_PATH)
    revised, changes, lists, calls = {}, [], 0, 0
    for name, data in resources.items():
        if data[4:8] != b"TLDO":
            continue
        fixed, patches = repair_display_list(data, name, segments)
        lists += 1
        calls += sum(opcode == 0xDE for _, _, _, opcode in _commands(data, name))
        if patches:
            revised[name] = fixed
            changes.extend(patches)
    if not lists:
        raise ValueError("no packed display lists found")
    return revised, changes, {
        "packed_display_lists": lists, "raw_display_list_calls": calls,
        "animation_segments": sorted(segments), "unflagged_calls": len(changes),
    }


def verify_archive(source):
    """Validate supported native framing and report remaining unflagged calls."""
    _, _, summary = _analyze(_resources(source))
    return summary


def _sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def repair_archive(source, output):
    """Write a new deterministic archive, preserving every other resource byte."""
    source, output = Path(source), Path(output)
    if source.resolve() == output.resolve():
        raise ValueError("source archive must remain unchanged")
    if output.exists():
        raise FileExistsError(output)
    resources = _resources(source)
    revised, changes, before = _analyze(resources)
    resource_hashes = {
        name: {"before_sha256": hashlib.sha256(resources[name]).hexdigest(),
               "after_sha256": hashlib.sha256(fixed).hexdigest()}
        for name, fixed in sorted(revised.items())
    }
    allowed = {}
    for change in changes:
        allowed.setdefault(change["resource"], []).append(change["byte_offset"])
    for name, fixed in revised.items():
        original = resources[name]
        differing = [i for i, (a, b) in enumerate(zip(original, fixed)) if a != b]
        if len(original) != len(fixed) or differing != allowed[name]:
            raise ValueError(f"{name}: repair exceeded its exact address-flag byte budget")
    resources.update(revised)
    _, remaining, after = _analyze(resources)
    if remaining:
        raise ValueError("repair left unflagged native segment calls")
    output.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive creation protects both the baseline and existing candidates.
    stream = output.open("xb")
    try:
        with stream, zipfile.ZipFile(stream, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for name in sorted(resources):
                info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                archive.writestr(info, resources[name], compresslevel=9)
        # Compare every uncompressed output byte, including all protected scene,
        # actor/project metadata, geometry, material, UV, vertex and texture data.
        with zipfile.ZipFile(output) as archive:
            if set(archive.namelist()) != set(resources):
                raise ValueError("candidate changed resource membership")
            for name, data in resources.items():
                if archive.read(name) != data:
                    raise ValueError(f"{name}: output resource differs from the validated bytes")
    except BaseException:
        output.unlink(missing_ok=True)
        raise
    return {
        "schema": 1,
        "purpose": "native zero-offset animated G_DL address flags; runtime unproven",
        "source": {"name": source.name, "size": source.stat().st_size, "sha256": _sha256(source)},
        "candidate": {"name": output.name, "size": output.stat().st_size, "sha256": _sha256(output)},
        "resource_count": len(resources), "changed_resources": len(revised), "changed_bytes": len(changes),
        "unchanged_resources": len(resources) - len(revised),
        "before": before, "after": after, "changes": changes,
        "changed_resource_hashes": resource_hashes,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    if args.output:
        receipt = args.receipt or args.output.with_suffix(".receipt.json")
        if receipt.exists() or receipt.resolve() in (args.source.resolve(), args.output.resolve()):
            parser.error("receipt must be a separate, new file")
        result = repair_archive(args.source, args.output)
        receipt.parent.mkdir(parents=True, exist_ok=True)
        with receipt.open("x") as stream:
            json.dump(result, stream, indent=2)
            stream.write("\n")
    else:
        if args.receipt:
            parser.error("--receipt requires --output")
        result = verify_archive(args.source)
    print(json.dumps(result, indent=2))
    if not args.output and result["unflagged_calls"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
