#!/usr/bin/env python3
"""Exercise final native forest conversion against the two immutable real inputs."""
from pathlib import Path
from PIL import Image
import argparse,hashlib,importlib.util,json,struct,sys,unittest,xml.etree.ElementTree as ET,zipfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
import forest
sys.path.insert(0,str(ROOT/'tools/lost_woods_entrance'))
import final_scene_segments as segments
FINAL=ROOT.parent/'final-scene-candidate/zzzz_MM3D_Lost_Woods_FINAL_native_segment_flags_POC18.prelude.o2r'
LEGACY=ROOT.parent/'solo-final-inputs/zzzz_Skiejer_Native_Solo_Forest_POC2.o2r'

class FinalForestTests(unittest.TestCase):
 def test_final_forest_has_native_headers_closed_art_and_all_scroll_tracks(self):
  path=ROOT/'tools/black_market/final_forest.py'
  if path.exists():
   spec=importlib.util.spec_from_file_location('final_forest',path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
  else:module=forest
  before=[hashlib.sha256(p.read_bytes()).hexdigest() for p in (FINAL,LEGACY)]
  try: resources,receipt=module.convert(FINAL,LEGACY)
  except ValueError as error:self.fail(f'current converter cannot port the final forest: {error}')
  self.assertEqual(before,[hashlib.sha256(p.read_bytes()).hexdigest() for p in (FINAL,LEGACY)])
  with zipfile.ZipFile(FINAL) as original,zipfile.ZipFile(LEGACY) as legacy:
   for entry in receipt['copied']:
    src,dst=entry['source'],entry['target'];expected=original.read(src)
    self.assertEqual(hashlib.sha256(expected).hexdigest(),entry['source_sha256'])
    self.assertEqual(hashlib.sha256(resources[dst]).hexdigest(),entry['sha256'])
    if expected.startswith(b'<DisplayList'):
     tree=ET.fromstring(expected)
     for e in tree:
      if 'Path' in e.attrib:e.set('Path',module.private_path(e.get('Path')))
     self.assertEqual(ET.tostring(tree),resources[dst])
    elif expected[4:8]!=b'TLDO' and not src.endswith('black_market_skeijer_poster'):self.assertEqual(expected,resources[dst],dst)
   poster=resources['custom/skiejer_forest/prelude/black_market_skeijer_poster']
   image=Image.open(ROOT/'tools/black_market/poster/skijer-wanted-poster.png').convert('RGBA')
   self.assertEqual(struct.unpack_from('<II',poster,68),image.size)
   self.assertEqual(poster[92:],image.tobytes())
   self.assertEqual(resources['custom/skiejer_forest/title'],legacy.read('custom/skiejer_forest/title'))
   collision,cameras=forest.playable_collision(original.read(module.COLLISION))
   for engine,prefix in [('mm',forest.MM),('oot',forest.OOT)]:
    scene=forest.native_commands(resources[prefix])
    self.assertEqual(resources[prefix+'_collision'],collision)
    self.assertEqual(struct.unpack_from('<I',scene[14])[0],2)
    rows=[struct.unpack_from('<4bHhhhhH',scene[14],4+i*16) for i in range(2)]
    self.assertEqual(rows[1][:8],(0,-1,0,-1,9 if engine=='oot' else 5,-680,0,72))
    self.assertEqual(rows[1][8],int(106*65536/360) if engine=='oot' else 13695)
    self.assertEqual(rows[1][9],0)
    self.assertNotIn(24,scene);self.assertNotIn(27,scene)
    self.assertEqual(scene[0],forest.native_commands(legacy.read(prefix))[0])
    if engine=='mm':self.assertEqual(scene[26],forest.string(module.ANIMATION_TARGET))
    else:self.assertNotIn(26,scene)
   self.assertEqual(resources[module.ANIMATION_TARGET],original.read(segments.ANIMATION_PATH))
   self.assertEqual(module.scroll_bindings(resources[module.ANIMATION_TARGET]),receipt['scroll_bindings'])
   self.assertEqual([x['segment'] for x in receipt['scroll_bindings']],list(range(8,14)))
   self.assertEqual((ROOT/'combo/entrances/SoloForestScroll.h').read_text(),module.scroll_header(receipt['scroll_bindings']))
   self.assertEqual((ROOT/'combo/entrances/SoloForestResources.h').read_text(),module.resource_header(resources))
   self.assertEqual(receipt['unresolved_references'],[])
   self.assertEqual(receipt['native_segment_calls'],12)
  print('PASS final solo pack: exact physical art/scroll data, native headers/door, collision, resource closure and generated contracts')

if __name__=='__main__':unittest.main()
