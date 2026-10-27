"""Remove the twins' repeated shoulder bands without changing their mesh.

Their private shoulder textures are 256x128, but the display lists load/render
512x512 tiles and their ST coordinates cover 512 pixels. GfxSpTri normalizes
against loaded dimensions capped at the tile dimensions. Undersized textures
therefore wrap twice around and four times along each shoulder. Expanding only
these six bakes to the authored 512-pixel tile makes that mapping one-to-one.
Every original RGBA pixel is retained as an exact 2x4 block; no art is repainted.
"""
import argparse
import hashlib
import importlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np

from native_assets import o2r as asset
from native_assets.path_guard import require_separate_destination
from native_npc_twins import SOURCE_SHA256

NAMESPACE = 'objects/object_kw1/NativeNpcV1'
CLIPS = ('gKokiriIdleAnim', 'gKokiriStandingArmsBehindBackAnim',
         'gKokiriSittingCrossedArmsLegsAnim')
TEXTURES = tuple('alt/'+NAMESPACE+'/PoseAnimations/objects/object_os_anime/'+clip+
                 '/Skin/Textures/ShoulderSkin'+side
                 for clip in CLIPS for side in ('Left', 'Right'))
PARENT_SHA256 = '2723332dce6ee36c5049236d8dc7ddd8613eaf2f31bd97b975623b5c3b495164'
NATIVE_SHA256 = 'ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b'
INFO = NAMESPACE+'/ShoulderSamplerRepairInfo.json'


def repair_texture_resource(raw):
    """Use the existing native tile size, retaining all reviewed source pixels."""
    kind, w, h, pixels = asset.read_texture(raw)
    if (kind, w, h) != (1, 256, 128) or struct.unpack_from('<ff', raw, 80) != (.5, .25):
        raise ValueError('This repair requires the reviewed RGBA32 256x128 shoulder bake')
    image = np.frombuffer(pixels, np.uint8).reshape(h, w, 4)
    expanded = np.repeat(np.repeat(image, 4, axis=0), 2, axis=1)
    return (raw[:64] + struct.pack('<IIIIffI', 1, 512, 512, 1, 1., 1., expanded.nbytes) +
            expanded.tobytes())


def repair_records(records):
    """Change only the exact six undersized private shoulder resources."""
    metadata = json.loads(records['twins_NativeNpcModelInfo.json'])
    if metadata['variant'] != 'twins' or metadata['source_sha256'] != SOURCE_SHA256:
        raise ValueError('This repair is bounded to the reviewed twins model')
    found = {name for name in records if '/Skin/Textures/ShoulderSkin' in name}
    if found != set(TEXTURES):
        raise ValueError('The reviewed shoulder texture resource set differs')
    changed = {}
    for name in TEXTURES:
        changed[name] = repair_texture_resource(records[name])
    return changed


def repair(native, baseline, output):
    native, baseline, output = map(Path, (native, baseline, output))
    require_separate_destination(output, native, baseline)
    if hashlib.sha256(baseline.read_bytes()).hexdigest() != PARENT_SHA256:
        raise ValueError('Use the exact rejected POC9 LiteralHash POC1 twins parent')
    if hashlib.sha256(native.read_bytes()).hexdigest() != NATIVE_SHA256:
        raise ValueError('Native source differs from the reviewed twins manifest')
    with zipfile.ZipFile(baseline) as archive:
        records = {name: archive.read(name) for name in archive.namelist()}
    changes = repair_records(records)
    repaired, hash_changes = asset.repair_native_npc_hashes(changes, records)
    # The only emitted resources are textures; no display list is being rebuilt.
    assert not hash_changes
    report = dict(parent_sha256=PARENT_SHA256, native_sha256=NATIVE_SHA256,
                  source_sha256=SOURCE_SHA256,
                  method='Exact 2x4 expansion of only the six undersized shoulder bakes to their native 512x512 tile',
                  previous_dimensions=[256, 128], dimensions=[512, 512],
                  previous_native_repeats=[2, 4], native_repeats=[1, 1],
                  original_rgba_pixels_preserved=True,
                  geometry_normals_uvs_faces_palette_rig_faces_hands_and_all_animations_byte_identical=True,
                  pose2_coherent_tunic_byte_identical=True,
                  existing_cap_normal_winding_disagreements_unchanged=True,
                  runtime_tested=False, user_accepted=False,
                  changed_resources=[dict(resource=name,
                                          previous_sha256=hashlib.sha256(records[name]).hexdigest(),
                                          sha256=hashlib.sha256(repaired[name]).hexdigest())
                                     for name in sorted(repaired)])
    records.update(repaired)
    records[INFO] = json.dumps(report, indent=2).encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    asset.O2R(files=records).write(output, compression=zipfile.ZIP_DEFLATED,
                                 timestamp=(2026, 10, 20, 0, 0, 0))
    with zipfile.ZipFile(baseline) as before, zipfile.ZipFile(output) as after:
        actual = {name for name in before.namelist() if before.read(name) != after.read(name)}
        if actual != set(TEXTURES):
            raise ValueError('The candidate changed a protected resource')
        if set(after.namelist())-set(before.namelist()) != {INFO}:
            raise ValueError('The candidate added an unexpected resource')
        if after.testzip() is not None:
            raise ValueError('Candidate ZIP integrity failure')
        report['protected_resources_verified_byte_identical'] = len(before.namelist())-len(TEXTURES)
    report['output_sha256'] = hashlib.sha256(output.read_bytes()).hexdigest()
    return report


def preview_comparison(native, baseline, candidate, destination, size=480):
    """Read both exports back with native undersized-tile UV/clamp behavior.

    The older offline preview multiplied ST by metadata scale even for images
    smaller than the logical render tile. Native GfxSpTri instead divides ST by
    min(loaded_dimension, tile_dimension) and disables sampler clamp if those
    dimensions differ, leaving the shader clamp at the logical tile boundary.
    These overrides are local to this explicit diagnostic and restored afterward.
    """
    destination = Path(destination)
    destinations = [destination/'previews'/f'{label}-static{pose}.png'
                    for pose in range(6) for label in ('parent', 'candidate')]
    destinations.append(destination/'Twins_Shoulder_Sampler_Comparison.png')
    for index, path in enumerate(destinations):
        require_separate_destination(path, native, baseline, candidate, *destinations[:index])
    from PIL import Image, ImageDraw
    import preview_native_npc_models as preview_module
    from verify_native_npc_models import Archive
    render_module = importlib.import_module('native_assets.render')
    old_scale, old_sample = Archive.texture_scale, render_module._sample_bilinear
    old_shade = preview_module.native_shade

    def texture_scale(archive, path):
        scale = old_scale(archive, path)
        return tuple(max(v, 1.) for v in scale) if '/Skin/Textures/ShoulderSkin' in path else scale

    def sample(tex, u, v, cms, cmt):
        if (tex.w, tex.h) == (256, 128):
            u, v = np.clip(u+.5, 0., 511.5), np.clip(v+.5, 0., 511.5)
            return old_sample(tex, u, v, 0, 0)
        return old_sample(tex, u, v, cms, cmt)

    def shade(normals, matrix, light):
        from native_npc_models import unit
        return np.clip(.18+.82*np.maximum(normals@unit(matrix.T@light), 0), 0, 1)

    destination.mkdir(parents=True, exist_ok=True)
    Archive.texture_scale, render_module._sample_bilinear = texture_scale, sample
    preview_module.native_shade = shade
    try:
        for pose in range(6):
            yaw = 160 if pose in (3, 4) else 0
            for label, archive in (('parent', baseline), ('candidate', candidate)):
                options = dict(archive=str(Path(archive).resolve()), name=f'{label}-static{pose}',
                               crop='shoulders', size=size, yaw=yaw)
                preview_module.preview((str(native), str(destination), 'twins', f'static{pose}', options))
        sheet = Image.new('RGB', (size*2, (size+28)*6+44), (28, 33, 40))
        draw = ImageDraw.Draw(sheet)
        draw.text((12, 10), 'OFFLINE EXPORTED RESOURCES | Native cap UV repeat + low ambient | Runtime untested', fill='white')
        for pose in range(6):
            for col, label in enumerate(('parent', 'candidate')):
                y = 44+pose*(size+28)
                sheet.paste(Image.open(destination/'previews'/f'{label}-static{pose}.png'), (col*size, y))
                draw.text((col*size+12, y+size+5), f'{label} / static {pose}', fill='white')
        path = destination/'Twins_Shoulder_Sampler_Comparison.png'
        sheet.save(path)
        return str(path)
    finally:
        Archive.texture_scale, render_module._sample_bilinear = old_scale, old_sample
        preview_module.native_shade = old_shade


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'baseline', 'output'):
        parser.add_argument(name, type=Path)
    parser.add_argument('--previews', type=Path)
    args = parser.parse_args()
    result = repair(args.native, args.baseline, args.output)
    if args.previews:
        result['offline_comparison'] = preview_comparison(args.native, args.baseline, args.output, args.previews)
    print(json.dumps(result, indent=2))
