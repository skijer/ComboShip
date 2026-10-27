"""Offline native binary-collision checks for fixed MM profiles; no engine/gameplay proof.
Usage: python tests/regional_passages/mm_collision_test.py /path/to/native/mm.o2r [camera-poses.txt]
Generate optional exact poses with COMBO_SNOWHEAD_POSES=/path/camera-poses.txt
python tests/lost_woods_entrance/run_portal_camera_tests.py (fresh output file).
Requires numpy. Input is read-only; no scene/actor resource is changed.
"""
from pathlib import Path
import sys, subprocess, tempfile, hashlib, math
import zipfile, struct, numpy as np
ROOT=Path(__file__).resolve().parents[2]
Z=zipfile.ZipFile(sys.argv[1])
class Scene:
 def __init__(self,name):
  self.name=name;n=next(n for n in Z.namelist() if f'/Z2_{name}/' in n and 'CollisionHeader' in n); self.resource=n;b=Z.read(n);o=76;nv=struct.unpack_from('<I',b,o)[0];o+=4
  self.v=np.array([struct.unpack_from('<hhh',b,o+6*i) for i in range(nv)],float);o+=6*nv
  npoly=struct.unpack_from('<I',b,o)[0];o+=4
  self.p=[struct.unpack_from('<HHHHhhhh',b,o+16*i) for i in range(npoly)];o+=16*npoly
  ns=struct.unpack_from('<I',b,o)[0];o+=4
  self.s=[struct.unpack_from('<II',b,o+8*i)[::-1] for i in range(ns)];o+=8*ns
  nc=struct.unpack_from('<I',b,o)[0];o+=4+nc*8;ncp=struct.unpack_from('<I',b,o)[0];o+=4+ncp*6;nw=struct.unpack_from('<I',b,o)[0];o+=4
  self.water=[struct.unpack_from('<hhhhhI',b,o+14*i) for i in range(nw)]
  self.tri=np.array([self.v[[p[1]&8191,p[2]&8191,p[3]&8191]] for p in self.p]); self.norm=np.array([p[4:7] for p in self.p])/32767
 def floor(self,x,z,expected):
  t=self.tri;a=t[:,0][:,[0,2]];b=t[:,1][:,[0,2]];c=t[:,2][:,[0,2]]
  cross=lambda x,y:x[:,0]*y[:,1]-x[:,1]*y[:,0]
  den=cross(b-a,c-a);den=np.where(abs(den)<1e-6,np.nan,den)
  q=np.array([x,z])-a;u=cross(q,c-a)/den;v1=cross(b-a,q)/den
  yy=t[:,0,1]+u*(t[:,1,1]-t[:,0,1])+v1*(t[:,2,1]-t[:,0,1])
  valid=(u>=-1e-5)&(v1>=-1e-5)&(u+v1<=1.00001)&(self.norm[:,1]>.707)&(yy<expected+100)&(yy>expected-35)
  if not valid.any():return None
  hits=[(yy[i],i) for i in np.flatnonzero(valid)]
  y,i=max(hits)
  if (self.s[self.p[i][0]][0]>>8)&31:return None
  for wx,wy,wz,lx,lz,props in self.water:
   if wx<=x<=wx+lx and wz<=z<=wz+lz and wy>=y-.5:return None
  return y
 def line(self,a,b):
  d=b-a;t=self.tri;edge1=t[:,1]-t[:,0];edge2=t[:,2]-t[:,0];h=np.cross(d,edge2);det=np.einsum('ij,ij->i',edge1,h);f=1/np.where(abs(det)<1e-7,np.nan,det);sv=a-t[:,0];u=f*np.einsum('ij,ij->i',sv,h);q=np.cross(sv,edge1);v=f*(q@d);dist=f*np.einsum('ij,ij->i',edge2,q);valid=(u>=-1e-5)&(v>=-1e-5)&(u+v<=1.00001)&(dist>1e-5)&(dist<=1)
  ids=np.flatnonzero(valid)
  if not len(ids):return None
  i=min(ids,key=lambda i:dist[i]);return i,a+d*dist[i]
 def valid(self,mouth,n,entry):
  right=np.array([n[2],0,-n[0]])
  for w in [-68,0,68]:
   for h in [20,85,136]:
    a=mouth+right*w+n*24+np.array([0,h,0]);hit=self.line(a,a-n*80)
    if hit is None or abs((hit[1]-mouth)@n+2)>3:return False,'frame'
  target=mouth+n*100
  for h in [10,60,125]:
   if self.line(np.array(entry)+[0,h,0],target+[0,h,0]):return False,'path blocked'
  for t in np.linspace(0,1,40):
   a=np.array(entry)*(1-t)+target*t
   for w in [-20,0,20]:
    q=a+right*w
    if self.floor(q[0],q[2],a[1]) is None:return False,'path floor'
  return True,''

with tempfile.TemporaryDirectory(prefix='fixed-mm-profile-') as temporary:
 cpp=Path(temporary)/'profiles.cpp';binary=Path(temporary)/'profiles'
 cpp.write_text('#include "entrances/MmRegionalBridgePlacements.h"\n#include <cstdio>\nint main(){using namespace ComboMmRegional; for(auto p:{kIkana,kGreatBay,kMountainWinter,kMountainSpring}) std::printf("%d %d %d %d %.6f %.6f %.6f %d %.6f %.6f %.6f\\n",p.scene,p.route,p.entrance,p.room,p.mouth.pos.x,p.mouth.pos.y,p.mouth.pos.z,p.mouth.yaw,p.nativeArrival.pos.x,p.nativeArrival.pos.y,p.nativeArrival.pos.z);}')
 subprocess.run(['c++','-std=c++20','-I'+str(ROOT/'combo'),str(cpp),'-o',str(binary)],check=True)
 rows=subprocess.check_output([str(binary)],text=True).splitlines()
for row,name in zip(rows,['IKANAMAE','30GYOSON','10YUKIYAMANOMURA','10YUKIYAMANOMURA2']):
 scene,route,entrance,room,x,y,z,yaw,ax,ay,az=map(float,row.split());s=Scene(name)
 # Confirm the profile's selected entrance maps to its claimed native spawn,
 # rather than merely accepting a plausible floor near a mistyped coordinate.
 scene_bytes=Z.read(f'scenes/nonmq/Z2_{name}/Z2_{name}')
 spawn_count={'IKANAMAE':3,'30GYOSON':14,'10YUKIYAMANOMURA':9,'10YUKIYAMANOMURA2':9}[name]
 spawn_offsets=[i for i in range(68,len(scene_bytes)-8-spawn_count*16) if scene_bytes[i:i+8]==struct.pack('<II',0,spawn_count) and all(struct.unpack_from('<H',scene_bytes,i+8+j*16)[0]==0 for j in range(spawn_count))]
 assert len(spawn_offsets)==1
 entrance_count={'IKANAMAE':4,'30GYOSON':14,'10YUKIYAMANOMURA':10,'10YUKIYAMANOMURA2':10}[name]
 entrance_offsets=[i for i in range(68,len(scene_bytes)-8-entrance_count*2) if scene_bytes[i:i+8]==struct.pack('<II',6,entrance_count)]
 assert len(entrance_offsets)==1
 native_spawn,native_room=struct.unpack_from('<BB',scene_bytes,entrance_offsets[0]+8+int(entrance)*2)
 native=struct.unpack_from('<HhhhhhhH',scene_bytes,spawn_offsets[0]+8+native_spawn*16)
 assert native_room==room and tuple(native[1:4])==(ax,ay,az)
 assert native[7]==(0x06FF if int(route)==7 else 0x0FFF), (name, "native player start params", native[7])
 mouth=np.array([x,y,z]);normal=np.array([math.sin(yaw*math.tau/65536),0,math.cos(yaw*math.tau/65536)])
 entry=np.array([ax,ay,az]);assert s.valid(mouth,normal,entry)[0],(name,s.valid(mouth,normal,entry))
 right=np.array([normal[2],0,-normal[0]])
 for width in [-68,0,68]:
  for depth in [12,45,100,160]:
   foot=mouth+right*width+normal*depth;floor=s.floor(foot[0],foot[2],y)
   assert floor is not None and abs(floor-y)<12,(name,'apron floor',foot)
   assert s.line(foot+[0,1,0],foot+[0,137,0]) is None,(name,'ceiling',foot)
 # Native entry pose and mouth are separate, preventing arrival bounceback.
 assert np.linalg.norm((entry-mouth)[[0,2]])>110
 if name.startswith('10YUKI'):
  # Dry walking bypass preserves the owl statue and both nearby snowballs;
  # assumes at most a27-unit player radius. No attack or new item is needed.
  path=[entry,np.array([-649,8,-150]),np.array([-520,8,-150]),np.array([-520,8,-310]),np.array([-414,8,-304])]
  props=[(np.array([-384,8,-259]),15),(np.array([-470,8,-224]),15),(np.array([-573,8,-230]),20)]
  for a,b in zip(path,path[1:]):
   for height in [10,60,125]:assert s.line(a+[0,height,0],b+[0,height,0]) is None
   for t in np.linspace(0,1,40):
    q=a*(1-t)+b*t;floor=s.floor(q[0],q[2],q[1]);assert floor is not None and abs(floor-8)<.1
    for prop,radius in props:assert np.linalg.norm((q-prop)[[0,2]])>radius+27
 print('PASS native MM collision:',name,'entrance',int(entrance),'mouth',mouth.tolist(),'resource',s.resource)
if len(sys.argv)>2:
 # Consume actual production-camera/native-form outputs, rather than copy the
 # production placement or arm calculation into an independent Python oracle.
 poses=[list(map(float,row.split())) for row in Path(sys.argv[2]).read_text().splitlines()]
 assert {(int(row[0]),int(row[1])) for row in poses}=={(season,form) for season in range(2) for form in range(5)}
 for row in poses:
  season,form=map(int,row[:2]);s=Scene('10YUKIYAMANOMURA2' if season else '10YUKIYAMANOMURA')
  foot=np.array(row[2:5]);at=np.array(row[5:8]);eye=np.array(row[8:11])
  direction=foot-np.array([-396.76031,8,-322.01703]);direction[1]=0;direction/=np.linalg.norm(direction)
  assert np.linalg.norm((foot-np.array([-396.76031,8,-322.01703]))[[0,2]])>110
  # Full27-unit footprint, plus the ordinary F entry's maximum40-unit walk
  # at its usual2-unit approach speed. Native input/speed variants remain a
  # runtime test; no actor or collision is removed to force clearance.
  for depth in np.linspace(0,40,17):
   center=foot+direction*depth
   for angle in np.linspace(0,math.tau,17):
    sample=center+np.array([math.sin(angle)*27,0,math.cos(angle)*27])
    floor=s.floor(sample[0],sample[2],sample[1]);assert floor is not None and abs(floor-8)<.1,(season,form,'landing floor',sample)
    assert s.line(sample+[0,1,0],sample+[0,137,0]) is None,(season,form,'landing ceiling',sample)
   for prop,radius in props:assert np.linalg.norm((center-prop)[[0,2]])>radius+27
  for depth in np.linspace(0,40,17):
   # Also retain a one-unit margin beyond the exact production eye ray.
   shifted_at=at+direction*depth;shifted_eye=eye+direction*depth
   extended_eye=shifted_eye+(shifted_eye-shifted_at)/np.linalg.norm(shifted_eye-shifted_at)
   assert s.line(shifted_at,extended_eye) is None,(season,form,'native presentation eye ray',depth)
  # Portal frame remains geometrically visible from the exact eye. Start a
  # little in front of its wall-backed plane, avoiding surface precision noise.
  for width in [-68,0,68]:
   for height in [1,68,136]:
    target=mouth+right*width+direction*2+np.array([0,height,0])
    assert s.line(eye,target) is None,(season,form,'portal visibility',width,height)
 print('PASS exact Snowhead production camera/native-form poses:',len(poses),'samples; winter/spring dry27-radius landing and40-unit ordinary walk, props/headroom, eye rays and full portal frame')
print('Input SHA256',hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest())
print('Static triangle sampling only; native camera, all-form movement, Alt assets and gameplay acceptance remain untested.')
