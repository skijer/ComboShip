"""Import the supplied MM3D Room Key OBJ into the existing textured GI format."""
import hashlib
from pathlib import Path
import shutil
import numpy as np
from PIL import Image
from meshkit import Model, ROOT, export_resources
from preview import checkpoint

REFERENCE = ROOT / 'REFERENCES/room_key'
ASSETS = ROOT.parents[1] / 'soh/assets/custom/objects/nei_gi_redesign/room_key'


def build():
    positions, normals, uvs, faces = [], [], [], []
    for line in (REFERENCE / 'roomkey.obj').read_text().splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == 'v':
            positions.append(list(map(float, fields[1:])))
        elif fields[0] == 'vn':
            normals.append(list(map(float, fields[1:])))
        elif fields[0] == 'vt':
            uvs.append(list(map(float, fields[1:3])))
        elif fields[0] == 'f':
            assert len(fields) == 4, 'Source faces must remain triangles'
            faces.append([tuple(int(x) - 1 for x in vertex.split('/')) for vertex in fields[1:]])
    positions = np.asarray(positions)
    center = (positions.min(axis=0) + positions.max(axis=0)) / 2
    positions = (positions - center) * 2.25
    vertices, p, n, uv, triangles = {}, [], [], [], []
    for face in faces:
        triangle = []
        for indices in face:
            if indices not in vertices:
                vi, ti, ni = indices
                vertices[indices] = len(p)
                p.append(positions[vi])
                n.append(normals[ni])
                uv.append([uvs[ti][0], 1 - uvs[ti][1]])
            triangle.append(vertices[indices])
        triangles.append(triangle)
    m = Model('room_key', 'Room Key (MM3D)', 'objects/nei_gi_redesign/room_key/gi_dl', 1., .65)
    texture = np.array(Image.open(REFERENCE / 'zelda_gi_hotelk_0.png').convert('RGBA'))
    m.material('hotel_key', [1., 1., 1.], metal=.3, rough=.6, tex=texture)
    m.add('Original MM3D Room Key', 'hotel_key', p, n, uv, triangles)
    assert len(faces) == 266 and len(m.parts) == 1 and len(m.parts[0]['tri']) == 262
    m.notes = [
        'User-supplied MM3D inventory Room Key OBJ/MTL and diffuse texture.',
        'All 262 visible source triangles retained; four source triangles with zero area omitted.',
        'Centered and uniformly scaled; source Y remains upright.',
        'Original 64x32 diffuse texture retained; the legacy N64 Room Key GI remains the fallback.',
        'No inventory, grant, held-item, save, animation or shelf placement changes.',
    ]
    m.markers = {'source_center': center.tolist(), 'source_uniform_scale': 2.25,
                 'source_sha256': {f.name: hashlib.sha256(f.read_bytes()).hexdigest()
                                   for f in sorted(REFERENCE.iterdir()) if f.is_file()}}
    return m


if __name__ == '__main__':
    m = build()
    stats = export_resources(m)
    checkpoint(m, stats)
    ASSETS.mkdir(parents=True, exist_ok=True)
    for resource in (ROOT / 'RESOURCES' / m.prefix).iterdir():
        shutil.copyfile(resource, ASSETS / resource.name)
