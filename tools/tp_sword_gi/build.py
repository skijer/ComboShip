"""TP-source sword GI candidate; original UV atlases and protected GIs retained.

The reflection decal follows the Four Sword's camera-relative normal mapping.
It is composited after the opaque source mesh, in the same bounded display list.
Only the decal uses texture alpha; every underlying metal surface is opaque.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys

import numpy as np
from PIL import Image, ImageFilter

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
FORGED = REPO / 'tools/nei_gi/SOURCE/forged_swords/SOURCE'
sys.path.insert(0, str(FORGED))
import meshkit
import preview
import sword_forged as forged
import depth

spec = importlib.util.spec_from_file_location('closed_meshkit', REPO / 'tools/nei_gi/SOURCE/meshkit.py')
closed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(closed)
REPLACED = ('master_sword', 'true_master_sword', 'gilded_sword', 'biggoron_sword', 'giants_knife')
SLUGS = REPLACED + depth.RETAINED
BUNDLE = 'TP_Sword_GI_POC2_20261009'
MASTER_SCALE = 140 / (180.864349 + 33.972435)
MASTER_GUARD = 130.030853


def obj(path):
    """Keep independent OBJ position, UV and normal indices at every seam."""
    p, uv, n, groups = [], [], [], {}
    source_text = path.read_text()
    smooth = any(line.strip() == 's 1' for line in source_text.splitlines())
    material = 'default'
    for line in source_text.splitlines():
        s = line.split()
        if not s:
            continue
        if s[0] == 'v':
            p.append(list(map(float, s[1:4])))
        elif s[0] == 'vt':
            uv.append(list(map(float, s[1:3])))
        elif s[0] == 'vn':
            n.append(list(map(float, s[1:4])))
        elif s[0] == 'usemtl':
            material = s[1] if len(s) > 1 else 'default'
        elif s[0] == 'f':
            corners = [tuple(int(v) if v else 0 for v in (a + '//').split('/')[:3]) for a in s[1:]]
            for i in range(1, len(corners) - 1):
                groups.setdefault(material, []).append([corners[0], corners[i], corners[i + 1]])
    p, uv, n = map(np.asarray, (p, uv, n))
    output = {}
    for name, faces in groups.items():
        corners = [corner for face in faces for corner in face]
        def indexed(data, index):
            return np.array([data[c[index] - 1 if c[index] > 0 else c[index]] for c in corners])
        positions = indexed(p, 0)
        faces = positions.reshape(-1, 3, 3)
        face_normals = meshkit.unit(np.cross(faces[:, 1] - faces[:, 0], faces[:, 2] - faces[:, 0]))
        if len(n) and all(c[2] for c in corners):
            normals = indexed(n, 2)
        elif smooth:
            accum = {}
            for corner, normal in zip(corners, np.repeat(face_normals, 3, axis=0)):
                accum[corner[0]] = accum.get(corner[0], np.zeros(3)) + normal
            normals = meshkit.unit([accum[c[0]] for c in corners])
        else:
            normals = np.repeat(face_normals, 3, axis=0)
        output[name] = (positions, normals, indexed(uv, 1), np.arange(len(corners)).reshape(-1, 3))
    return output


def source_texture(path, tint=None):
    im = Image.open(path).convert('RGBA')
    rgba = np.array(im)
    # Source metal is an opaque solid; exporter-specific map_d is not a cutout.
    rgba[:, :, 3] = 255
    if tint:
        c = rgba[:, :, :3].astype(float) / 255
        luma = c @ np.array([.2126, .7152, .0722])
        strength = np.clip(.18 + luma * 1.32, .12, 1.12)
        if tint['base'] in ('#F8F8F8', '#D7DDE4'):
            strength = .50 + .50 * np.clip(luma / .28, 0, 1)
        gold = (c[:, :, 0] > c[:, :, 2] * 1.5) & (c[:, :, 1] > c[:, :, 2] * 1.4)
        green = (c[:, :, 1] > c[:, :, 0] * 1.08) & (c[:, :, 1] > c[:, :, 2] * 1.12)
        color = np.broadcast_to(forged.rgb(tint['base']), c.shape).copy()
        if 'gem' in tint:
            color[gold] = forged.rgb(tint['gem'])
        if 'grip' in tint:
            color[green] = forged.rgb(tint['grip'])
        rgba[:, :, :3] = np.uint8(np.clip(color * strength[:, :, None] * 255, 0, 255))
    im = Image.fromarray(rgba).resize((im.width * 4, im.height * 4), Image.Resampling.LANCZOS)
    im = im.filter(ImageFilter.UnsharpMask(radius=1.1, percent=65, threshold=3))
    return np.array(im), tuple(Image.open(path).size)


def source_material(m, name, path, tint=None, metal=.78, rough=.24):
    texture, tile = source_texture(path, tint)
    m.material(name, [1, 1, 1], metal=metal, rough=rough, tex=texture)
    # Normalized source UVs use the same 32x32 virtual tile as the GI exporter.
    # The OTR texture's hires factors retain the atlas's actual aspect ratio.
    m.materials[name].update(tile=(32, 32), sampler={'s': 'clamp', 't': 'clamp'}, source_size=tile)


def reflection(m, part_names, color='#ECF3FF', strength=.32):
    """Low-alpha reflection bands retain the source engraving beneath them."""
    name = 'four_sword_sheen'
    tex = forged.metal_map(forged.rgb(color), size=256)
    y, x = np.mgrid[:256, :256] / 255
    band = np.exp(-((x + .08 * y - .635) / .045) ** 2)
    rim = np.exp(-((x - .29 * y - .64) / .055) ** 2)
    tex[:, :, 3] = np.uint8(np.clip((band + rim * .4) * strength * 255, 0, 255))
    m.material(name, [1, 1, 1], metal=.95, rough=.22, tex=tex)
    m.materials[name].update(reflection=1, decal=True, render_mode='G_RM_AA_ZB_XLU_DECAL2')
    # Same quantized geometry, LEQUAL decal mode, no fake inflated silhouette.
    for p in list(m.parts):
        if p['name'] in part_names:
            m.add('Four Sword sheen over ' + p['name'], name, p['p'], p['n'], p['uv'], p['tri'])


def master(slug):
    names = {'master_sword': 'Master Sword', 'true_master_sword': 'True Master Sword',
             'gilded_sword': 'Gilded Sword'}
    m = meshkit.Model(slug, names[slug], 'objects/nei_gi_redesign/' + slug + '/gi_dl', 1, .66)
    true, gilded = slug == 'true_master_sword', slug == 'gilded_sword'
    inputs = HERE / 'INPUTS/master'
    source_material(m, 'tp_blade', inputs / 'MastSS03.png',
                    {'base': '#E0A800'} if true else None)
    hilt_tint = {'base': '#F8F8F8', 'gem': '#4870D0'} if true else (
        {'base': '#D7DDE4', 'gem': '#E0A800', 'grip': '#87292E'} if gilded else None)
    source_material(m, 'tp_hilt', inputs / 'MastSS01.png', hilt_tint)
    if not true and not gilded:
        texture = m.materials['tp_hilt']['tex']
        texture[:, :, :3] = np.uint8(np.clip(texture[:, :, :3].astype(float) * 1.65 + 8, 0, 255))
        texture = m.materials['tp_blade']['tex']
        texture[:, :, :3] = np.uint8(np.clip(texture[:, :, :3].astype(float) * 1.28 + 12, 0, 255))
    if gilded:
        source_material(m, 'tp_red_grip', inputs / 'MastSS01.png', {'base': '#A72C38'}, metal=0, rough=.76)
    if gilded:
        y, x = np.mgrid[:1024, :256] / np.array([1023, 255])[:, None, None]
        diamond = .94 * (1 - np.abs(np.mod((1 - y) * 3, 1) - .5) * 2)
        gold = np.abs(x * 2 - 1) < diamond
        shade = .67 + .23 * np.sin(x * np.pi) + .08 * y
        pigment = np.where(gold[:, :, None], forged.rgb('#E0A800'), forged.rgb('#C2CFDB'))
        tex = np.uint8(np.clip(pigment * shade[:, :, None] * 255, 0, 255))
        m.materials['tp_blade']['tex'] = np.dstack([tex, np.full(tex.shape[:2], 255, np.uint8)])
        m.materials['tp_blade']['tile'] = (32, 32)
    geometry = obj(inputs / 'MasterSword.obj')
    for source, mat in [('m0_MastSS03.png', 'tp_blade'), ('m1_MastSS01.png', 'tp_hilt')]:
        p, n, uv, triangles = geometry[source]
        p = np.c_[p[:, 0], MASTER_GUARD - p[:, 1], -p[:, 2]] * MASTER_SCALE
        n = n * [1, -1, -1]
        uv = uv * [1, -1] + [0, 1]
        if gilded and mat == 'tp_blade':
            # Shorten the blade above its seat; keep the guard and grip untouched.
            p[:, 1] = np.where(p[:, 1] > 0, p[:, 1] * .88, p[:, 1])
            n[:, 1] /= .88
            uv = np.c_[p[:, 0] / (8.325194 * MASTER_SCALE * 2) + .5,
                       1 - np.maximum(p[:, 1], 0) / ((MASTER_GUARD + 33.972435) * MASTER_SCALE * .88)]
        if gilded and mat == 'tp_hilt':
            centers = p[triangles, 1].mean(axis=1)
            grip_faces = (centers < -8) & (centers > -28)
            m.add('TP winged silver hilt', mat, p, n, uv, triangles[~grip_faces])
            m.add('TP red textured grip', 'tp_red_grip', p, n, uv, triangles[grip_faces])
        else:
            m.add('TP blade' if mat == 'tp_blade' else 'TP winged hilt and grip', mat, p, n, uv, triangles)
    if true:
        forged.metal(m, 'blue_jewel', '#4870D0', .5)
        m.materials['blue_jewel'].update(metal=.15, rough=.16)
        for side in (-1, 1):
            m.sphere('True Master blue inset jewel', 'blue_jewel', [0, -4.2, side * 5.04], [1.2, 1.75, .45], 4, 8, flat=True)
    reflection(m, {'TP blade'}, '#FFE8AA' if true else '#EEF5FF', .40 if true else .34)
    positions = np.concatenate([p['p'] for p in m.parts])
    m.markers.update(guard_baseline_y=0, preview_front_angle=16, preview_back_angle=164,
                     source='supplied Twilight Princess MasterSword.obj', texture_upscale=4,
                     blade_length_factor=.88 if gilded else 1,
                     held_grip_y=-18.75, held_tip_y=float(positions[:, 1].max()),
                     visual_review_candidate=True, runtime_tested=False,
                     supersedes_tp_planar_environment_card=True)
    m.notes = ['TP hilt UV atlas retained and blade UVs fitted to three broad gold/silver facets.' if gilded else
               'Original TP blade and hilt geometry with original UV atlas bindings; 4x Lanczos upscale with conservative sharpening.',
               'TP zero-thickness environment card is replaced by a Four-Sword-style normal-generated reflection decal over the opaque blade.',
               'White hilt/grip #F8F8F8, blue jewel #4870D0, tempered gold blade #E0A800.' if true else
               'Three broad gold/silver lozenges, red grip and silver fittings; blade shortened 12% from the TP source.' if gilded else
               'Original TP violet hilt, grip detail, gold jewel and engraved steel blade retained.',
               'Offline candidate only; gameplay and user acceptance remain pending.']
    return depth.deepen(m)


def plate(m, name, material, outline, depth, z=0):
    closed.Model.polygon(m, name, material, outline, depth=depth, z=z, bevel=.55, closed_sides=True)


def heavy(slug):
    big = slug == 'biggoron_sword'
    m = meshkit.Model(slug, "Biggoron's Sword" if big else "Giant's Knife",
                      'objects/nei_gi_redesign/' + slug + '/gi_dl', 1, .66)
    inputs = HERE / 'INPUTS/ordon'
    source_material(m, 'ordon_steel', inputs / 'ordoSS01.png',
                    {'base': '#ADBCC9'} if big else {'base': '#87909B'})
    blade_points, normals, uv, triangles = obj(inputs / 'ordonsword.obj')['m0']
    tip = 166.0 if big else 152.0
    # Stretch above the seated root. Scaling around Ordon's source origin
    # would pull the blade base away from the separately authored guard.
    source_seat = blade_points[:, 0].min()
    length = tip / (blade_points[:, 0].max() - source_seat)
    transform = np.array([[0, 0, 2.15 if big else 2.32], [length, 0, 0], [0, -1.05, 0]])
    p = blade_points @ transform.T
    p[:, 1] -= source_seat * length
    n = normals @ np.linalg.inv(transform)
    m.add('Ordon-inspired heavy fullered blade', 'ordon_steel', p, n, uv * [1, -1] + [0, 1], triangles)
    for name, color in [('fittings', '#BF742C' if big else '#777D88'),
                         ('rim', '#EDB15E' if big else '#C4CCD4'), ('groove', '#3D3A39')]:
        forged.metal(m, name, color, .85 if big else .58)
    m.material('leather', forged.rgb('#182B50' if big else '#60352D'), 'leather', rough=.77)
    m.material('wrap', forged.rgb('#405479' if big else '#967161'), 'leather', rough=.72)
    if big:
        outline = [(-23, 3), (-20, 8), (-15, 6), (-9, 3), (0, 5), (9, 3),
                   (15, 6), (20, 8), (23, 3), (20, -5), (11, -3), (0, -2), (-11, -3), (-20, -5)]
    else:
        outline = [(-21, -4), (-23, -1), (-23, 4), (-20, 7), (20, 7), (23, 4), (23, -1), (21, -4)]
    plate(m, 'Bronze sculpted Goron guard' if big else 'Riveted iron Knife crossbar', 'fittings', outline, 6)
    for side in (-1, 1):
        forged.stroke(m, 'Machined guard lip', 'rim', [[-19, 1], [-10, 0], [0, 2], [10, 0], [19, 1]], side * 3.08, .3, 6)
        if big:
            plate(m, 'Inset Goron diamond', 'groove', [(0, -1.5), (4, 2.5), (0, 6.5), (-4, 2.5)], .4, side * 3.14)
            plate(m, 'Raised Goron forge seal', 'rim', [(0, -.3), (2.8, 2.5), (0, 5.3), (-2.8, 2.5)], .2, side * 3.4)
        else:
            for x in (-17, -8, 8, 17):
                m.sphere('Iron crossbar rivet', 'rim', [x, 1.5, side * 3.3], [.7, .7, .35], 4, 8)
    grip_end = -48 if big else -43
    forged.grip(m, grip_end, -3.5, 3.25 if big else 3.45, 'leather', 'wrap', 12 if big else 10)
    m.lathe('Heavy blade seat collar', 'rim', [(-5, 3.35), (-3, 3.9), (-1, 3.65)], 12)
    m.lathe('Heavy capped pommel', 'fittings', [(grip_end - 6, 1), (grip_end - 4, 4.6),
                                             (grip_end - 2, 4.7), (grip_end, 3.1)], 12)
    m.lathe('Pommel machined ring', 'rim', [(grip_end - 3.4, 4.75), (grip_end - 2.6, 4.75)], 12)
    reflection(m, {'Ordon-inspired heavy fullered blade'}, '#ECF2FF', .40 if big else .18)
    m.markers.update(guard_baseline_y=0, preview_front_angle=16, preview_back_angle=164,
                     held_grip_y=(grip_end - 3.5) / 2, held_tip_y=tip,
                     source='supplied TP Ordon sword blade; separately authored heavy guard/grip',
                     source_blade_width_factor=2.15 if big else 2.32, texture_upscale=4,
                     adult_two_handed_proportions=True,
                     blade_seat_y=0,
                     visual_review_candidate=True, runtime_tested=False)
    m.notes = ['Straight Ordon blade/fullers retained with wider two-handed proportions and a separately closed, beveled guard.',
               'Polished steel, bronze Goron seal, blue grip and longer blade.' if big else
               'Broader, shorter steel blade, iron riveted bar guard, red-brown grip and subdued sheen distinguish the Knife.',
               'Scimitar ornament informed machined fittings only; its curved silhouette is preserved as a separate reference.',
               'Opaque metal plus normal-generated reflection decal; no collision, damage or progression edits.']
    return depth.deepen(m)


def build(output, install=False):
    output = Path(output).resolve()
    meshkit.ROOT = preview.ROOT = output
    manifest = {}
    for slug in SLUGS:
        if slug in depth.RETAINED:
            m = depth.build_retained(slug, output, REPO)
        else:
            m = master(slug) if slug in REPLACED[:3] else heavy(slug)
            stats = meshkit.export_resources(m)
            preview.checkpoint(m, stats)
        for name, material in m.materials.items():
            if material['tex'] is not None:
                path = output / 'TEXTURES' / slug / (name + '.png')
                path.parent.mkdir(parents=True, exist_ok=True)
                Image.fromarray(material['tex']).save(path)
        manifest[slug] = {'markers': m.markers, 'resources': {
            p.name: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted((output / 'RESOURCES' / m.prefix).iterdir()) if p.is_file()}}
        metadata_path = output / 'CHECKPOINTS' / slug / 'checkpoint.json'
        metadata = json.loads(metadata_path.read_text())
        metadata['source_provenance'] = {
            'bundle': BUNDLE,
            'baseline_commit': 'c0181856968dd6f3869be0d1aa240743091fd6a8',
            'authoring_recipe': 'tools/tp_sword_gi/build.py',
            'resource_sha256': manifest[slug]['resources'],
            'glb_sha256': hashlib.sha256((metadata_path.parent / (slug + '.glb')).read_bytes()).hexdigest(),
            'acceptance': 'pending user visual review and configuration-specific runtime test',
        }
        metadata_path.write_text(json.dumps(metadata, indent=2) + '\n')
        if install:
            target = REPO / 'soh/assets/custom' / m.prefix
            source = output / 'RESOURCES' / m.prefix
            target.mkdir(parents=True, exist_ok=True)
            # Retain the baseline's unused texture resources for recovery.
            # Only these generated files enter the candidate dependency graph.
            shutil.copytree(source, target, dirs_exist_ok=True)
            assert all(p.read_bytes() == (target / p.name).read_bytes() for p in source.iterdir())
            shutil.copytree(output / 'CHECKPOINTS' / slug, REPO / 'tools/nei_gi/CHECKPOINTS' / slug, dirs_exist_ok=True)
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE / 'OUTPUT')
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    build(args.output, args.install)
