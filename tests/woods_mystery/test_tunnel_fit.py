"""The native mouth keeps its depth; long donor interiors are cropped, not compressed."""
import sys
from pathlib import Path
import numpy as np

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/woods_mystery'))
from build_candidate import Resources,recover,tunnel_at

donor=Resources(sys.argv[1]);tunnel=recover(donor)
for length,width,height in [(225,1.2,1.0),(120,.72,.95)]:
    fitted=tunnel_at(tunnel,(0,0),(1,0),length=length,width=width,height=height)
    assert len(fitted)<len(tunnel), 'The full long donor tunnel is still being compressed into the passage'
    assert all('/room_0/' in t['path'] for t in fitted), 'The distant second-room mouth was instanced'
    rim=[t for t in fitted if t['path'].endswith('/Geometry6')]
    original=[t for t in tunnel if t['path'].endswith('/Geometry6')]
    assert len(rim)==len(original)==24, 'Entrance rim was lost during cropping'
    p=np.concatenate([t['v'][:,:3] for t in fitted])
    assert p[:,0].min()==0 and abs(p[:,0].max()-length)<1e-5
    rim_positions=np.concatenate([t['v'][:,:3] for t in rim])
    original_positions=np.concatenate([t['v'][:,:3] for t in original])
    assert np.allclose(rim_positions[:,0],(original_positions[:,0]-446)*width), 'Entrance depth was compressed'
    assert np.allclose(rim_positions[:,2],original_positions[:,2]*width)
    assert np.allclose(rim_positions[:,1],original_positions[:,1]*height+1)
print('PASS: full entrance rim retained; long interior cropped with matching UV interpolation; no distant end mouth')
