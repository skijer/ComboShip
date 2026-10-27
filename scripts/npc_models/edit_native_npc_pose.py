"""Build a model-specific fitted-pose O2R from a JSON recipe; no game build.

Native clip identity, frame count and root translations stay unchanged. Use a
separate output path and enable the resulting full pack instead of its parent.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from native_npc_models import animation_frames, asset


def encode_animation(frames):
    frames = np.asarray(frames)
    if frames.ndim != 3 or frames.shape[2] != 3 or not 0 < len(frames) < 32768:
        raise ValueError("Expected frame x joint x XYZ native animation data")
    if not np.isfinite(frames).all() or (abs(frames) > 32768).any() or (frames > 32767).any():
        raise ValueError("Native joint values must fit signed 16-bit channels")
    static, dynamic, references = [], [], []
    for joint in range(frames.shape[1]):
        row = []
        for axis in range(3):
            channel = list(map(int, frames[:, joint, axis]))
            if len(set(channel)) == 1:
                if channel[0] not in static:
                    static.append(channel[0])
                row.append((False, static.index(channel[0])))
            else:
                row.append((True, len(dynamic)))
                dynamic.extend(channel)
        references.append(row)
    if len(static)+len(dynamic) > 65535:
        raise ValueError("Native animation channel table exceeds 16-bit offsets")
    raw = bytearray(asset._header(0x4F414E4D))
    raw += struct.pack("<IhI", 0, len(frames), len(static)+len(dynamic))
    raw += struct.pack(f"<{len(static)+len(dynamic)}h", *(static+dynamic))
    raw += struct.pack("<I", frames.shape[1])
    for row in references:
        raw += struct.pack("<HHH", *(offset+len(static) if varying else offset for varying, offset in row))
    raw += struct.pack("<H", len(static))
    return bytes(raw)


def edit(baseline, native, recipe, output, overlay=False):
    baseline, native, output = map(Path, [baseline, native, output])
    if output.resolve() in (baseline.resolve(), native.resolve()):
        raise ValueError("Keep both inputs read-only; choose a separate output")
    parent_sha = hashlib.sha256(baseline.read_bytes()).hexdigest()
    if recipe.get("schema_version") != 1 or recipe.get("parent_sha256") != parent_sha:
        raise ValueError("Recipe schema/parent hash mismatch; fit the exact selected model")
    with zipfile.ZipFile(baseline) as base, zipfile.ZipFile(native) as game:
        metadata = [name for name in base.namelist() if name.endswith("_NativeNpcModelInfo.json")]
        if not metadata and "YoungFadoNpcInfo.json" in base.namelist():
            metadata = ["YoungFadoNpcInfo.json"]
        if len(metadata) != 1:
            raise ValueError("Expected one native NPC model manifest")
        manifest = json.loads(base.read(metadata[0]))
        if metadata[0] == "YoungFadoNpcInfo.json":
            manifest = dict(manifest, namespace="objects/object_fa/YoungFadoNpcPOC1",
                            variant="young-fado", target_limbs=manifest["native_hierarchy_limbs"])
        if manifest.get("native_sha256") not in (None, hashlib.sha256(native.read_bytes()).hexdigest()):
            raise ValueError("Native archive differs from the model manifest")
        namespace = manifest["namespace"]
        changes = {}
        for clip in recipe.get("clips", []):
            path = clip["native_animation"].removeprefix("__OTR__")
            if not path.startswith("objects/") or ".." in path or path.endswith("/"):
                raise ValueError("Invalid native animation key")
            original = animation_frames(game.read(path))
            if original.shape[1] != manifest["target_limbs"]+1:
                raise ValueError("Animation does not belong to the target joint layout")
            fitted = original.copy()
            private = "alt/"+namespace+"/PoseAnimations/"+path
            if private in base.namelist():
                fitted = animation_frames(base.read(private))
            for change in clip.get("rotations", []):
                joint = change["joint"]
                if not 1 <= joint < fitted.shape[1]:
                    raise ValueError("Only limb rotations are editable; root motion is preserved")
                frames = change.get("frames", list(range(len(fitted))))
                if any(not isinstance(f, int) or not 0 <= f < len(fitted) for f in frames):
                    raise ValueError("Pose edit frame is outside the native clip")
                values = np.asarray(change["binang"])
                if values.shape != (3,) or not np.isfinite(values).all() or (values != np.rint(values)).any():
                    raise ValueError("Expected three integer native binary angles")
                if change.get("mode", "set") == "set":
                    fitted[frames, joint] = values
                elif change["mode"] == "add":
                    fitted[frames, joint] = (fitted[frames, joint]+values+32768) % 65536-32768
                else:
                    raise ValueError("Rotation mode must be set or add")
            if fitted.shape != original.shape or not np.array_equal(fitted[:, 0], original[:, 0]):
                raise ValueError("A fitted clip must preserve native timing and root motion")
            changes[private] = encode_animation(fitted)
        if not changes:
            raise ValueError("Recipe contains no pose edits")
        report = dict(schema_version=1, parent_sha256=parent_sha, variant=manifest["variant"],
                      namespace=namespace, changed_resources=sorted(changes), runtime_tested=False,
                      requires_native_npc_pose_adapter_v1=True)
        records = {} if overlay else {name: base.read(name) for name in base.namelist()}
        records.update(changes)
        if overlay:
            records[manifest["variant"].replace("-", "_")+"_NativeNpcPosePatch.json"] = json.dumps(report, indent=2).encode()
        else:
            manifest["pose_patch"] = report
            records[metadata[0]] = json.dumps(manifest, indent=2).encode()
        output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as result:
            for path, raw in sorted(records.items()):
                info = zipfile.ZipInfo(path, (2026, 10, 18, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                result.writestr(info, raw)
    report["output_sha256"] = hashlib.sha256(output.read_bytes()).hexdigest()
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("native", type=Path)
    parser.add_argument("recipe", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--overlay", action="store_true", help="Only edited private resources; parent pack remains enabled")
    args = parser.parse_args()
    print(json.dumps(edit(args.baseline, args.native, json.loads(args.recipe.read_text()), args.output, args.overlay), indent=2))


if __name__ == "__main__":
    main()
