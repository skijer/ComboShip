"""Recover the complete visual-only MM3DS tunnel across both Lost Woods rooms."""
from pathlib import Path
import argparse, collections, copy, hashlib, io, json, struct, zipfile
import numpy as np
from scene_assets import Resources, render

PREFIX = 'custom/cor_mm3d/lost_woods_poc1/'


def touching_component(triangles, points):
    remaining = list(triangles)
    selected = []
    while True:
        added = [t for t in remaining if any(tuple(v[:3]) in points for v in t['v'])]
        if not added:
            return selected
        selected.extend(added)
        ids = {id(t) for t in added}
        remaining = [t for t in remaining if id(t) not in ids]
        points.update(tuple(v[:3]) for t in added for v in t['v'])


def recover(resources):
    triangles = []
    for room, tube, bark in [(0, 5, 6), (1, 16, 17)]:
        base = PREFIX + f'room_{room}/Geometry'
        half = resources.mesh(base + str(tube))
        rim = touching_component(resources.mesh(base + str(bark)),
                                 {tuple(v[:3]) for t in half for v in t['v']})
        assert len(rim) == 24
        triangles.extend(half + rim)
    refs = {ref for t in triangles for ref in t['refs']}
    assert len(triangles) == 246 and len(refs) == 321, (len(triangles), len(refs))
    return triangles


def make_glb(resources, triangles):
    """Keep source references distinct, including UV and color seams."""
    binary = bytearray()
    views, accessors, images, textures, samplers, materials, primitives = [], [], [], [], [], [], []
    def view(data, target=None):
        binary.extend(b'\0' * (-len(binary) % 4))
        views.append(dict(buffer=0, byteOffset=len(binary), byteLength=len(data),
                          **({'target': target} if target else {})))
        binary.extend(data)
        return len(views) - 1
    def accessor(arr, kind, component, target, bounds=False):
        a = dict(bufferView=view(arr.tobytes(), target), componentType=component,
                 count=len(arr), type=kind)
        if bounds:
            a.update(min=arr.min(0).tolist(), max=arr.max(0).tolist())
        accessors.append(a)
        return len(accessors) - 1
    all_pos = np.concatenate([t['v'][:, :3] for t in triangles])
    center = (all_pos.min(0) + all_pos.max(0)) / 2
    groups = collections.defaultdict(list)
    for t in triangles:
        groups[(t['texture'], t['tile'])].append(t)
    for (texture, tile), tris in groups.items():
        image = resources.t(texture)
        png = io.BytesIO(); image.save(png, format='PNG')
        images.append(dict(bufferView=view(png.getvalue()), mimeType='image/png', name=texture))
        sampler = dict(wrapS=10497, wrapT=10497, magFilter=9729, minFilter=9729,
                       extensions={'FAST64_sampler_n64': {'set': True, 'format': {'texture': 'RGBA', 'size': 16},
                         'fields': [dict(clamp=False, mirror=False, low=0, high=int(d)-1,
                                         mask=int(np.log2(d)), shift=0) for d in tile]}})
        samplers.append(sampler); textures.append(dict(source=len(images)-1, sampler=len(samplers)-1))
        n64 = dict(combiner={'set': True, 'cycles': [
                    {'color': ['TEXEL0', '0', 'SHADE', '0'], 'alpha': ['TEXEL0', '0', 'SHADE', '0']},
                    {'color': ['COMBINED', '0', 'PRIMITIVE', '0'], 'alpha': ['0', '0', '0', 'COMBINED']}]},
                   otherModes={'cycleType': '2CYCLE', 'textureFilter': 'BILERP', 'alphaCompare': 'NONE'},
                   extensions={'FAST64_materials_f3d': {'geometryMode': dict(zBuffer=True, shade=True,
                       shadeSmooth=True, cullFront=False, cullBack=False, fog=True, lighting=False,
                       texGen=False, texGenLinear=False)}},
                   primitive={'set': True, 'color': [1, 1, 1, 1], 'minLoDRatio': 0, 'loDFraction': 0},
                   textures={'0': {'index': len(textures)-1}})
        materials.append(dict(name=texture, doubleSided=True,
            pbrMetallicRoughness=dict(metallicFactor=0, roughnessFactor=1,
                                     baseColorTexture={'index': len(textures)-1}),
            extensions={'FAST64_materials_n64': n64}, extras={'lit': False}))
        refs, vertices, indices = {}, [], []
        for t in tris:
            for ref, vertex in zip(t['refs'], t['v']):
                if ref not in refs:
                    refs[ref] = len(vertices); vertices.append(vertex)
                else:
                    assert np.array_equal(vertices[refs[ref]], vertex)
                indices.append(refs[ref])
        v = np.asarray(vertices)
        pos = np.asarray(v[:, :3] - center, dtype='<f4')
        uv = np.asarray(v[:, 3:5] / 32 / np.asarray(tile), dtype='<f4')
        col = np.asarray(v[:, 5:9] / 255, dtype='<f4')
        primitives.append(dict(attributes=dict(POSITION=accessor(pos, 'VEC3', 5126, 34962, True),
              TEXCOORD_0=accessor(uv, 'VEC2', 5126, 34962), COLOR_0=accessor(col, 'VEC4', 5126, 34962)),
              indices=accessor(np.asarray(indices, dtype='<u4'), 'SCALAR', 5125, 34963),
              material=len(materials)-1, mode=4))
    gltf = dict(asset={'version': '2.0', 'generator': 'prelude-of-light user recipe'},
        extensionsUsed=['FAST64_materials_n64', 'FAST64_sampler_n64'], scene=0,
        scenes=[dict(name='level', nodes=[0], extras={'preludeUserRecipe': 1, 'game': 'mm'})],
        nodes=[{'name': 'recipe', 'mesh': 0}], meshes=[{'name': 'recipe', 'primitives': primitives}],
        materials=materials, textures=textures, samplers=samplers, images=images,
        accessors=accessors, bufferViews=views, buffers=[{'byteLength': len(binary)}])
    js = json.dumps(gltf, separators=(',', ':')).encode(); js += b' ' * (-len(js) % 4)
    binary.extend(b'\0' * (-len(binary) % 4))
    return (struct.pack('<III', 0x46546c67, 2, 28+len(js)+len(binary)) +
            struct.pack('<II', len(js), 0x4e4f534a) + js + struct.pack('<II', len(binary), 0x004e4942) + binary)


def main():
    p = argparse.ArgumentParser(); p.add_argument('poc', type=Path); p.add_argument('output', type=Path)
    a = p.parse_args(); a.output.mkdir(parents=True, exist_ok=True)
    r = Resources(a.poc); tris = recover(r)
    glb = a.output / 'MM3DS-Lost-Woods-Tunnel-Recovered.glb'; glb.write_bytes(make_glb(r, tris))
    report = dict(source=a.poc.name, source_sha256=hashlib.sha256(a.poc.read_bytes()).hexdigest(),
        triangles=246, source_vertices=321, collision='None. Vanilla collision remains the owner.',
        glb_sha256=hashlib.sha256(glb.read_bytes()).hexdigest(),
        groups=[dict(path=path, triangles=sum(t['path']==path for t in tris)) for path in sorted({t['path'] for t in tris})])
    (a.output/'tunnel-recovery.json').write_text(json.dumps(report, indent=2)+'\n')
    render(r, tris, a.output/'MM3DS-Lost-Woods-Tunnel-Recovered.png',
           eye=[260, 260, 610], target=[763, 65, 0], width=1000, height=540, ortho_width=800,
           label='Recovered native MM3DS tunnel | 246 faces, 321 source vertices | Visual geometry only')
    needed = {t['path'] for t in tris} | {t['texture'] for t in tris} | {q[0] for t in tris for q in t['refs']}
    with zipfile.ZipFile(a.output/'tunnel-source-resources.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for n in sorted(needed):
            info=zipfile.ZipInfo('alt/'+n, (2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info, r.get(n))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
