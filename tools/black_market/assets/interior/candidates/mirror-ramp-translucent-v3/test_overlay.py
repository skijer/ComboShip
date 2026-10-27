from pathlib import Path
import copy,json,tempfile,xml.etree.ElementTree as ET
import build_overlay
D=Path(__file__).parent
data,m=build_overlay.resources()
with tempfile.TemporaryDirectory(prefix='ramp-xlu-overlay-') as t:
 a,b=Path(t)/'a.o2r',Path(t)/'b.o2r'
 assert build_overlay.build(a)==build_overlay.build(b) and a.read_bytes()==b.read_bytes()
for error in ('opaque_material','full_alpha','wrong_scroll','wrong_restore','opaque_room','changed_geometry','shared_mask'):
 bad=dict(data)
 if error in ('opaque_material','full_alpha','wrong_scroll'):
  raw=bad[build_overlay.MATERIAL]
  if error=='opaque_material':raw=raw.replace(b'G_RM_AA_ZB_XLU_SURF',b'G_RM_AA_ZB_OPA_SURF')
  elif error=='full_alpha':raw=raw.replace(b'B="248" A="112"',b'B="248" A="255"')
  else:raw=raw.replace(b'0x08000000',b'0x09000000')
  bad[build_overlay.MATERIAL]=raw
 elif error=='wrong_restore':bad[build_overlay.RAMP]=bad[build_overlay.RAMP].replace(b'G_RM_AA_ZB_OPA_SURF2',b'G_RM_AA_ZB_XLU_SURF2')
 elif error=='opaque_room':
  name=build_overlay.RAMP.encode();pointer=len(name).to_bytes(4,'little')+name
  p=build_overlay.ROOMS[0];bad[p]=bad[p].replace(bytes(4)+pointer,pointer+bytes(4))
 elif error=='changed_geometry':bad[build_overlay.RAMP]=bad[build_overlay.RAMP].replace(b'VertexOffset="0"',b'VertexOffset="1"',1)
 else:bad[build_overlay.PREFIX+'authored/prelude/black_market_wall_magic_mask']=bad[build_overlay.MASK]
 assert bad!=data,error
 try:build_overlay.validate(bad,m)
 except (AssertionError,ValueError):pass
 else:raise AssertionError('accepted regression '+error)
print('PASS: deterministic6resource overlay and7 independent OPA/alpha/scroll/restore/owner/geometry/shared-mask regression controls')
