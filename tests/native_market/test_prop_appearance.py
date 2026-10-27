"""Per-piece shop palettes/UVs must preserve all unrelated native attributes."""
import base64
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET
import zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
import native_market
import market_interior as interior
import prop_appearance as props


class PropAppearance(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.base = native_market.read_bundle(ROOT/'tools/native_market/bundled')
        cls.prepared = interior.prepare_prop_vertices(cls.base)

    def tearDown(self):
        props.glass_strip.cache_clear()

    def components(self,suffix):
        return interior.vertex_components(self.base,f'{props.GEOMETRY}_tri_{suffix}')

    def vertices(self,suffix,prepared=False):
        path = f'{props.GEOMETRY}_vtx_{suffix}'
        return list(ET.fromstring((self.prepared if prepared else self.base)[path]))

    def test_four_pots_own_distinct_padded_material_windows(self):
        vertices = self.vertices(2,True)
        colors = set()
        for _,bounds,count,triangles,rect,color in props.POTS:
            part = props._piece(self.components(2),bounds,count,triangles)
            colors.add(color)
            for index in part['indices']:
                s,t = (int(vertices[index].get(k)) for k in ('S','T'))
                self.assertTrue(4*(rect[0]+4) <= s <= 4*(rect[2]-4))
                self.assertTrue(4*(rect[1]+4) <= t <= 4*(rect[3]-4))
        self.assertEqual(len(colors),4)

    def test_ten_bottles_get_distinct_uv_windows_and_upright_streak_axes(self):
        vertices = self.vertices(14,True)
        windows = set()
        for bounds,count,triangles,variant in props.BOTTLES:
            part = props._piece(self.components(14),bounds,count,triangles)
            points = [vertices[i] for i in part['indices']]
            ss,tt,ys = [[int(v.get(k)) for v in points] for k in ('S','T','Y')]
            self.assertTrue(all(4*(80*variant+4) <= s <= 4*(80*(variant+1)-4) for s in ss))
            self.assertTrue(all(16 <= t <= 4*118 for t in tt))
            windows.add((min(ss),max(ss),min(tt),max(tt)))
            # The donor's S follows height. New T must retain that height
            # relation so a vertical glass streak cannot turn horizontal.
            ymean,tmean = sum(ys)/len(ys),sum(tt)/len(tt)
            self.assertLess(sum((y-ymean)*(t-tmean) for y,t in zip(ys,tt)),0)
        self.assertEqual(len(windows),10)

    def test_positions_faces_alpha_and_unowned_vertex_attributes_preserved(self):
        allowed_uv = {2:set(),14:set(),24:set()}
        for _,bounds,count,triangles,_,_ in props.POTS:
            allowed_uv[2].update(props._piece(self.components(2),bounds,count,triangles)['indices'])
        for bounds,count,triangles,_ in props.BOTTLES:
            allowed_uv[14].update(props._piece(self.components(14),bounds,count,triangles)['indices'])
        allowed_rgb = set()
        for bounds,count,color in props.CANISTERS:
            allowed_rgb.update(props._piece(self.components(24),bounds,count,58)['indices'])
        for suffix in (2,14,24):
            old,new = self.vertices(suffix),self.vertices(suffix,True)
            self.assertEqual(len(old),len(new))
            for index,(a,b) in enumerate(zip(old,new)):
                exempt = {'S','T'} if index in allowed_uv[suffix] else set()
                if suffix == 24 and index in allowed_rgb:
                    exempt |= {'R','G','B'}
                expected = dict(a.attrib)
                if suffix == 2:
                    xyz = [int(a.get(k)) for k in ('X','Y','Z')]
                    for spec in interior.TORCH_STANDS:
                        if all(spec['bounds'][0][i] <= xyz[i] <= spec['bounds'][1][i] for i in range(3)):
                            for key,value,delta in zip(('X','Y','Z'),xyz,spec['delta']):expected[key]=str(value+delta)
                self.assertEqual({k:v for k,v in expected.items() if k not in exempt},
                                 {k:v for k,v in b.attrib.items() if k not in exempt})
        self.assertEqual(set(self.prepared),{f'{props.GEOMETRY}_vtx_{s}' for s in (2,5,8,12,14,24)})

    def test_three_canister_hues_preserve_original_vertex_shading(self):
        old,new = self.vertices(24),self.vertices(24,True)
        for bounds,count,color in props.CANISTERS:
            part = props._piece(self.components(24),bounds,count,58)
            for index in part['indices']:
                self.assertEqual([int(new[index].get(k)) for k in 'RGB'],
                    [(int(old[index].get(k))*c+127)//255 for k,c in zip('RGB',props.rgb(color))])

    def test_palette_and_glass_writes_preserve_every_unowned_atlas_pixel(self):
        original = bytes((70,100,40,255))*(512*512)
        counter = props.texture_pixels('counter_atlas',original)
        protected = 0
        for y in range(512):
            for x in range(512):
                owned = next((row for row in props.POTS if row[4][0] <= x < row[4][2] and row[4][1] <= y < row[4][3]),None)
                offset = 4*(y*512+x)
                if owned:
                    level = (77*70+150*100+29*40+128)//256
                    self.assertEqual(counter[offset:offset+3],bytes(min(255,(level*c+64)//128) for c in props.rgb(owned[5])))
                else:
                    self.assertEqual(counter[offset:offset+4],original[offset:offset+4]);protected += 1
        self.assertGreater(protected,150000)
        fittings = props.texture_pixels('fittings_atlas',original)
        for y in range(512):
            if y < 122:
                self.assertEqual(fittings[(y*512+320)*4:(y+1)*512*4],original[(y*512+320)*4:(y+1)*512*4])
                self.assertEqual(fittings[y*512*4:(y*512+320)*4],props.glass_strip()[y*320*4:(y+1)*320*4])
            else:self.assertEqual(fittings[y*512*4:(y+1)*512*4],original[y*512*4:(y+1)*512*4])
        for name in ('timber','arcane_stone','slate','bronze','curios','walltrim_atlas'):
            self.assertIs(props.texture_pixels(name,original),original)

    def test_glass_bad_hash_path_and_trailing_data_rejected(self):
        manifest = json.loads((props.ASSETS/'prop-glass.json').read_text())
        text = (props.ASSETS/manifest['file']).read_bytes()
        self.assertLessEqual(len(text),128*1024)
        self.assertLessEqual(manifest['packed_bytes'],96*1024)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for mutation in ('hash','path','tail','bounds'):
                record = dict(manifest);payload = text
                if mutation == 'hash':payload=b'AAAA'
                elif mutation == 'path':record['file']='../outside.b64'
                else:
                    raw = base64.b64decode(text)
                    raw = raw+b'junk' if mutation == 'tail' else zlib.compress(b'X'*(320*122*4+8))
                    payload=base64.b64encode(raw)
                    record.update(base64_sha256=hashlib.sha256(payload).hexdigest(),packed_bytes=len(raw),packed_sha256=hashlib.sha256(raw).hexdigest())
                (root/manifest['file']).write_bytes(payload)
                (root/'prop-glass.json').write_text(json.dumps(record))
                props.glass_strip.cache_clear()
                with patch.object(props,'ASSETS',root),self.assertRaises(ValueError):props.glass_strip()


if __name__ == '__main__':
    unittest.main()
