"""Detailed grass uses one sampler, matching the corridor and underlay material."""
from pathlib import Path
import sys,xml.etree.ElementTree as ET

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/woods_mystery'))
from scene_assets import Resources

candidate=Resources(sys.argv[1])
grass='custom/cor_woods_mystery/poc1/textures/grass'
floors=0
for name,data in candidate.files.items():
    if '/Z2_26SARUNOMORI_room_' not in name or 'DL_' not in name or not data.startswith(b'<DisplayList'):continue
    xml=ET.fromstring(data);loads=[c.get('Path') for c in xml if c.tag=='SetTextureImage']
    if grass not in loads:continue
    floors+=1
    assert loads==[grass],f'An extra texture layer changes the detailed grass in {name}'
    combine=next(c for c in xml if c.tag=='SetCombineLERP')
    assert 'G_CCMUX_TEXEL1' not in combine.attrib.values(),f'Grass still uses the coarse/detailed native blend in {name}'
    tiles=[c for c in xml if c.tag=='SetTile' and c.get('Tile')=='0']
    assert len(tiles)==1 and tiles[0].get('ShiftS')==tiles[0].get('ShiftT')=='0'
assert floors==9, f'Expected nine floor lists, found {floors}'
print('PASS: all nine main grass floors use the same single detailed sampler as added coverage')
