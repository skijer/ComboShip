"""Exercise the wire path exactly as Prelude's length-prefixed string reader does."""
import struct
import sys
from pathlib import Path

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/woods_mystery'))
from build_candidate import append_mesh,read_mesh_header
from scene_assets import Resources

native=Resources(sys.argv[1])
key='scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_01'
path='custom/cor_woods_mystery/poc1/room_1/geometry'
result,info=append_mesh(native.get(key),path,(0,0,0))
offset=info['mesh_end']+9
length=struct.unpack_from('<I',result,offset)[0]
raw=result[offset+4:offset+4+length].decode()
assert raw==path, f'Prelude cannot resolve the serialized mesh path: {raw!r}'
assert read_mesh_header(result)[2][-1][1]==path
print('PASS: new room mesh path resolves exactly with Prelude string semantics')
