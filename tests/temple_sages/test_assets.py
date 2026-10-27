"""Check the delivered collision/resources, independently of builder bookkeeping."""
import importlib.util
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
INPUTS = Path(os.environ.get('TEMPLE_SAGES_INPUTS', ROOT.parents[1] / 'temple_inputs'))
COLLISION = 'scenes/shared/tokinoma_scene/tokinoma_sceneCollisionHeader_0032F8'


def collision(data):
    assert data[4:8] == b'LOCO'
    off = 76
    count = struct.unpack_from('<I', data, off)[0]
    off += 4
    vertices = [struct.unpack_from('<3h', data, off + 6*i) for i in range(count)]
    off += count*6
    count = struct.unpack_from('<I', data, off)[0]
    off += 4
    polygons = [struct.unpack_from('<4H4h', data, off + 16*i) for i in range(count)]
    off += count*16
    types = struct.unpack_from('<I', data, off)[0]
    off += 4
    surfaces = data[off:off + types*8]
    off += types*8
    return vertices, polygons, surfaces, data[off:]


def floor_height(vertices, polygons, x, z, target):
    heights = []
    for poly in polygons:
        if poly[5] < 18000:
            continue
        a, b, c = [vertices[i & 0x1fff] for i in poly[1:4]]
        den = (b[2]-c[2])*(a[0]-c[0]) + (c[0]-b[0])*(a[2]-c[2])
        if abs(den) < 1e-6:
            continue
        u = ((b[2]-c[2])*(x-c[0]) + (c[0]-b[0])*(z-c[2]))/den
        v = ((c[2]-a[2])*(x-c[0]) + (a[0]-c[0])*(z-c[2]))/den
        if u >= -1e-4 and v >= -1e-4 and u+v <= 1.0001:
            heights.append(u*a[1] + v*b[1] + (1-u-v)*c[1])
    return min(heights, key=lambda h: abs(h-target)) if heights else None


def segment_hits(start,end,face):
    """Two-sided geometric cast independent of clipping and builder normals."""
    def sub(a,b):return tuple(a[i]-b[i] for i in range(3))
    def dot(a,b):return sum(a[i]*b[i] for i in range(3))
    def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    a,b,c=face;direction=sub(end,start);e1=sub(b,a);e2=sub(c,a)
    h=cross(direction,e2);det=dot(e1,h)
    if abs(det)<1e-7:return False
    s=sub(start,a);u=dot(s,h)/det;q=cross(s,e1);v=dot(direction,q)/det;t=dot(e2,q)/det
    return min(u,v,t)>=-1e-6 and u+v<=1.000001 and t<=1.000001


class SagesAssets(unittest.TestCase):
    artifact = None
    manifest = None

    def setUp(self):
        builder_path = ROOT / 'tools/temple_sages/build.py'
        self.assertTrue(builder_path.exists(), 'physical Temple/Sages integration builder is missing')
        if self.__class__.artifact is None:
            spec = importlib.util.spec_from_file_location('temple_sages_builder', builder_path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            self.__class__.builder = mod
            directory = tempfile.TemporaryDirectory(prefix='temple-sages-test-')
            self.__class__.directory = directory
            self.addClassCleanup(directory.cleanup)
            path = Path(directory.name) / 'candidate.o2r'
            self.__class__.manifest = mod.build(INPUTS/'oot.o2r',
                INPUTS/"Djipi's 3DE - 20 Scenes Map.otr", INPUTS/'Stone Entrance Lost Woods OOT.glb', path)
            self.__class__.artifact = path
            with zipfile.ZipFile(path) as pack:
                self.__class__.resources = {n: pack.read(n) for n in pack.namelist()}
            with zipfile.ZipFile(INPUTS/'oot.o2r') as pack:
                self.__class__.original = pack.read(COLLISION)

    def test_route_has_no_platform_doorway(self):
        self.assertEqual(self.manifest['route'], ['temple_pedestal','stone_tunnel','light_bridge','central_platform'])
        self.assertEqual(self.manifest['extra_platform_doorways'], 0)
        self.assertFalse(any('platform_door' in n for n in self.resources))

    def test_five_portals_replace_all_native_pad_meshes(self):
        self.assertEqual(len(self.manifest['landings']), 5)
        self.assertEqual([m['params'] for m in self.manifest['markers']], list(range(0xfc00,0xfc05)))
        self.assertTrue(all(m['position'][1] == -120 for m in self.manifest['markers']))
        self.assertFalse(any(n.removeprefix('alt/').startswith('custom/temple_sages/pad_') for n in self.resources))
        self.assertFalse(any('pillar_5_' in n for n in self.resources))
        effect=ET.fromstring(self.resources['custom/temple_sages/portals/forest/material'])
        self.assertIsNone(effect.find('SetPrimColor'), 'the native caller owns portal reward tint')
        rows=self.builder.G.vertex_rows(self.resources['custom/temple_sages/portals/forest/geometry_vtx'])
        self.assertGreater(max(v[1] for v in rows), 120, 'portal must hover above the landing')

    def test_bridges_have_depth_without_a_light_side_branch(self):
        for name in ('forest','fire','water','spirit','shadow','light'):
            rows=self.builder.G.vertex_rows(self.resources[f'custom/temple_sages/bridge_{name}_geometry_vtx'])
            self.assertEqual(max(v[1] for v in rows), -120)
            self.assertGreaterEqual(max(v[1] for v in rows)-min(v[1] for v in rows), 12)
            self.assertLessEqual(max(v[1] for v in rows)-min(v[1] for v in rows), 24)
            if name=='light':
                self.assertLessEqual(max(abs(v[0]) for v in rows), 90)
                self.assertGreaterEqual(max(v[2] for v in rows), -1289)

    def test_private_binary_hash_dependencies_have_canonical_index_entries(self):
        G=self.builder.G
        with zipfile.ZipFile(INPUTS/'oot.o2r') as base:
            paths=set(base.namelist()) | set(self.resources)
        hashes={G.crc64(n) for n in paths}
        for name,data in self.resources.items():
            if data[4:8]!=b'TLDO' or not name.removeprefix('alt/').startswith('custom/'):
                continue
            for _,op,_,h in G.words(data):
                if h is not None:
                    self.assertTrue(h in hashes,(name,hex(op),hex(h),'missing canonical hash dependency'))

    def test_giant_curtain_is_replaced_by_a_compact_capped_shell(self):
        G=self.builder.G
        active={n:b for n,b in self.resources.items() if not n.startswith('alt/')}
        active.update({n[4:]:b for n,b in self.resources.items() if n.startswith('alt/')})
        for root in ('kenjyanoma_room_0DL_001608','kenjyanoma_room_0DL_010CD8'):
            for rec in G.collect(active,'custom/temple_sages/chamber/'+root):
                for face in rec['faces']:
                    self.assertTrue(all((v[0]**2+(v[2]+2400)**2)**.5<410 for v in face),
                                    'giant donor curtain is still reachable')
        rows=G.vertex_rows(self.resources['alt/custom/temple_sages/enclosure_geometry_vtx'])
        self.assertLessEqual(max(v[2] for v in rows), -1180, 'shell intrudes into the Temple')
        self.assertTrue(any(v[1]==500 for v in rows), 'ceiling is missing')
        faces=[rows[i:i+3] for i in range(0,len(rows),3)]
        self.assertTrue(any(all(v[1]==500 for v in f) for f in faces), 'ceiling has no triangles')
        for x in (-80,0,80):
            for y in (-110,0,110):
                self.assertFalse(any(segment_hits((x,y,-1250),(x,y,-1150),f) for f in faces),
                                 ('shell blocks the tunnel',x,y))
        self.assertTrue(any(segment_hits((0,200,-1250),(0,200,-1150),f) for f in faces), 'entry lintel is missing')
        self.assertTrue(any(segment_hits((0,0,-2400),(0,600,-2400),f) for f in faces), 'ceiling does not cap the chamber')
        for face in faces:
            if all(v[1]==500 for v in face):
                a,b,c=face;ny=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2])
                self.assertLess(ny,0,'ceiling faces away from the interior')

    def test_native_chamber_scene_is_untouched(self):
        self.assertFalse(any('/kenjyanoma_scene/' in n for n in self.resources))
        self.assertFalse(any(n.startswith(('scenes/nonmq/', 'scenes/mq/')) for n in self.resources))
        self.assertTrue(all('/tokinoma_scene/' in n or n.startswith(('custom/','alt/custom/','prelude/'))
                            for n in self.resources))

    def test_original_collision_payload_is_preserved(self):
        old_v, old_p, old_s, old_tail = collision(self.original)
        for prefix in ('', 'alt/'):
            new_v, new_p, new_s, new_tail = collision(self.resources[prefix+COLLISION])
            self.assertEqual(new_v[:len(old_v)], old_v)
            self.assertEqual(new_s[:len(old_s)], old_s)
            self.assertEqual(new_tail, old_tail, 'native camera/water collision payload changed')
            preserved = {p for p in old_p if all(old_v[i & 0x1fff][2] > -650 for i in p[1:4])}
            self.assertTrue(preserved.issubset(set(new_p)))

    def test_added_surfaces_keep_the_normal_gameplay_camera(self):
        _,_,old_surfaces,_=collision(self.original)
        for prefix in ('','alt/'):
            _,_,surfaces,tail=collision(self.resources[prefix+COLLISION])
            count=struct.unpack_from('<I',tail,0)[0]
            for off in range(len(old_surfaces),len(surfaces),8):
                # Native serialization stores data[1] before data[0].
                data0=struct.unpack_from('<I',surfaces,off+4)[0]
                camera=data0&0xff
                self.assertLess(camera,count)
                setting=struct.unpack_from('<H',tail,4+camera*8)[0]
                self.assertEqual(setting,0,(prefix,'annex floor selects unrelated native camera',camera,setting))

    def test_all_six_routes_have_continuous_walkable_floor(self):
        for prefix in ('', 'alt/'):
            vertices, polygons, _, _ = collision(self.resources[prefix+COLLISION])
            for route, samples in self.manifest['walk_samples'].items():
                self.assertGreater(len(samples), 10, route)
                last = None
                for x,y,z in samples:
                    height = floor_height(vertices, polygons, x,z,y)
                    self.assertIsNotNone(height, (prefix,route,x,z))
                    self.assertLess(abs(height-y), 3, (prefix,route,x,y,z,height))
                    if last is not None:
                        self.assertLess(abs(height-last), 18, (prefix,route,'unwalkable step'))
                    last = height

    def test_native_and_alt_opening_has_no_collision_wall(self):
        # A vertical segment through the portal at walking height must be clear.
        x,y,z = self.manifest['doorway_center']
        for prefix in ('', 'alt/'):
            vertices, polygons, _, _ = collision(self.resources[prefix+COLLISION])
            for p in polygons:
                a,b,c = [vertices[i & 0x1fff] for i in p[1:4]]
                if abs(p[6]) < 25000:
                    continue
                if max(v[2] for v in (a,b,c)) < z-20 or min(v[2] for v in (a,b,c)) > z+20:
                    continue
                den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
                if abs(den)<1e-6:continue
                u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/den
                v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/den
                self.assertFalse(u>0 and v>0 and u+v<1, (prefix,'portal obstructed',p))

    def test_room_actor_headers_preserve_existing_entries(self):
        expected = set(range(0xfc00,0xfc05))
        for entry in self.manifest['room_actor_headers']:
            self.assertEqual(set(entry['marker_params']), expected)
            self.assertEqual(entry['preserved_actor_bytes'], entry['source_actor_bytes'])
            self.assertIn(entry['path'], self.resources)
            self.assertIn('alt/'+entry['path'], self.resources)
        self.assertGreaterEqual(len(self.manifest['room_actor_headers']), 4)

    def test_scrolling_is_portable_and_glow_is_caller_tinted(self):
        self.assertGreaterEqual(len(self.manifest['portable_materials']), 3)
        for name in self.manifest['portable_materials']:
            data = self.resources[name]
            if data.lstrip().startswith(b'<'):
                tree=ET.fromstring(data)
                self.assertFalse(any(e.get('Path','').startswith('>') for e in tree.iter()))
        tree=ET.fromstring(self.resources['custom/temple_sages/portals/forest/material'])
        self.assertIsNone(tree.find('SetPrimColor'))
        self.assertIsNotNone(tree.find('SetOtherMode'))

    def test_tunnel_keeps_the_carved_atlas_uv_orientation(self):
        data=(INPUTS/'Stone Entrance Lost Woods OOT.glb').read_bytes()
        size=struct.unpack_from('<I',data,12)[0];gltf=json.loads(data[20:20+size]);binary=data[28+size:]
        primitive=gltf['meshes'][0]['primitives'][1]
        a=gltf['accessors'][primitive['attributes']['TEXCOORD_0']];view=gltf['bufferViews'][a['bufferView']]
        off=view.get('byteOffset',0)+a.get('byteOffset',0)
        uv=[struct.unpack_from('<2f',binary,off+i*8) for i in range(a['count'])]
        rendered=ET.fromstring(self.resources['alt/custom/temple_sages/stone_tunnel_vtx'])
        self.assertEqual([(int(v.get('S')),int(v.get('T'))) for v in list(rendered)[:len(uv)]],
                         [(round(u*1024),round(v*1024)) for u,v in uv])

    def test_tunnel_uses_reauthored_ethereal_atlas_with_same_logical_sampler(self):
        import hashlib
        from PIL import Image
        tex=self.resources['alt/custom/temple_sages/texture/tunnel_stone']
        self.assertEqual(struct.unpack_from('<IIIIffI',tex,64),
                         (1,1024,1024,1,64.,32.,1024*1024*4))
        authored=Image.open(ROOT/'tools/temple_sages/assets/ethereal_tunnel.png').convert('RGBA')
        authored=authored.resize((1024,1024),Image.Resampling.LANCZOS)
        self.assertEqual(hashlib.sha256(tex[92:]).digest(),hashlib.sha256(authored.tobytes()).digest())
        material=ET.fromstring(self.resources['alt/custom/temple_sages/tunnel_material'])
        size=material.find('SetTileSize')
        self.assertEqual((size.get('Lrs'),size.get('Lrt')),('124','124'))

    def test_door_veil_has_soft_edges_and_its_own_scrolling_texture(self):
        G=self.builder.G
        rows=G.vertex_rows(self.resources['alt/custom/temple_sages/door_veil_geometry_vtx'])
        self.assertGreater(len(rows),100)
        self.assertEqual(min(v[-1] for v in rows),0)
        self.assertEqual(max(v[-1] for v in rows),55)
        self.assertTrue(all(v[-1]==0 for v in rows if abs(v[0])==88 or v[1] in (4,232)))
        tex=self.resources['alt/custom/temple_sages/texture/veil']
        self.assertEqual(struct.unpack_from('<IIIIffI',tex,64),(1,512,512,1,32.,16.,512*512*4))
        material=ET.fromstring(self.resources['alt/custom/temple_sages/door_veil_material'])
        self.assertEqual(material.find('SetTextureImage').get('Path'),'custom/temple_sages/texture/veil')
        self.assertEqual(material.find('ScrollTexture').attrib,dict(Tile='0',SpeedX='-1',SpeedY='2'))

    def test_visible_tunnel_floor_matches_the_source_stages(self):
        rows=ET.fromstring(self.resources['alt/custom/temple_sages/stone_tunnel_vtx'])
        vertices=[tuple(int(v.get(a)) for a in ('X','Y','Z')) for v in rows]
        polys=[]
        for i in range(0,len(vertices),3):
            a,b,c=vertices[i:i+3];u=[b[k]-a[k] for k in range(3)];v=[c[k]-a[k] for k in range(3)]
            ny=u[2]*v[0]-u[0]*v[2]
            if ny>0:polys.append((0,i,i+1,i+2,0,32767,0,0))
        for z,y in [(-750,0),(-800,0),(-898.5,-45),(-1100,-120),(-1250,-120)]:
            actual=floor_height(vertices,polys,0,z,y)
            self.assertIsNotNone(actual,(z,y))
            self.assertLess(abs(actual-y),1,(z,y,actual))

    def test_the_archive_is_deterministic(self):
        spec=importlib.util.spec_from_file_location('temple_sages_builder_second', ROOT/'tools/temple_sages/build.py')
        mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
        second=Path(self.directory.name)/'second.o2r'
        mod.build(INPUTS/'oot.o2r', INPUTS/"Djipi's 3DE - 20 Scenes Map.otr",
                  INPUTS/'Stone Entrance Lost Woods OOT.glb', second)
        self.assertEqual(second.read_bytes(), self.artifact.read_bytes())


if __name__ == '__main__':
    unittest.main()
