"""Approved rigid equipment art fitted to existing native gameplay frames.

Cape cloth, native tunic skinning, boots, passive jewelry, and Din/PAK sword
ownership remain in their original draw paths. This module exports no GI pose.
"""
import numpy as np
import equipment_revamp as approved
import sword_revamp
from meshkit import rotation
from simple import component

def points(m):return np.concatenate([p['p'] for p in m.parts])

def shield(slug):
    m=component(getattr(approved,slug)(),slug)
    p=points(m)
    # Actual object_nei_* legacy shield vertices: normal X, upright Y, width Z.
    # The caller keeps the tuned hand/back/surf matrix, including child scale.
    span=68 if slug=='divine_shield' else 84
    factor=span/np.ptp(p[:,1])
    m.native_scale=round(factor/16*65536)/65536
    m.effective_scale=m.native_scale*16
    m.transform(rotation('y',-90),offset=np.array([-6,14,4.5])/m.effective_scale)
    m.markers={'legacy_frame':'object_nei_divine_shield / object_nei_kite_shield',
               'normal_native':[-1,0,0], 'up_native':[0,1,0]}
    m.notes=['Approved shell/embossing/rear straps in native custom-shield frame. Existing hand/back/surf transforms retained.']
    return m

def byrna():
    m=component(approved.cane_of_byrna(),'cane_of_byrna')
    m.transform(rotation('z',20))
    p=points(m);factor=639/np.ptp(p[:,1])
    m.native_scale=round(factor/16*65536)/65536;m.effective_scale=m.native_scale*16
    # Preserve the exact trail's source endpoints and shaft center, not GI center.
    shaft=next(p for p in m.parts if p['name']=='Slender blue Byrna shaft')
    axis_x=(shaft['p'][:,0].min()+shaft['p'][:,0].max())*.5
    m.transform(offset=[-axis_x,-416/m.effective_scale-p[:,1].min(),0])
    m.markers={'legacy_axis_native':[0,-416,0], 'tip_native':[0,223,0]}
    m.notes=['GI lean removed; shaft occupies measured Somaria/Byrna Y=-416..223 frame. Existing held scale/trail/charge transforms unchanged.']
    return m

def trident():
    m=component(approved.trident(),'trident')
    m.transform(rotation('z',17))
    p=points(m)
    # Recover source uniform fit from the original 126-unit shaft/spear envelope.
    source_fit=np.ptp(p[:,1])/126
    grip=-18*source_fit
    factor=(8520-2049.5)/(p[:,1].max()-grip)
    m.native_scale=round(factor/16*65536)/65536;m.effective_scale=m.native_scale*16
    m.transform(rotation('x',90),offset=[0,0,2049.5/m.effective_scale-grip])
    m.markers={'grip_native':[0,0,2049.5], 'tip_native':[0,0,8520],
               'legacy_envelope_z':[-5550,8520]}
    m.notes=['GI lean removed; authored grip is seated at original lance hand Z=2049.5, spear tips at legacy Z=8520. Original hand, trail and collision matrices retained.']
    return m

def four_sword(blade):
    source=approved.four_sword()
    source.transform(rotation('z',22))
    names={'Folded white Four Sword blade','Cool central blade fuller','Small blade Triforce engraving'}
    m=component(source,'four_sword_blade' if blade else 'four_sword_hilt',lambda p:(p['name'] in names)==blade)
    # Match the shipped Four Sword blade's X=719..3389, centerline Y=332.5 Z=-74.
    # Global GI fitting scales source uniformly, so recover its scale from blade.
    raw=next(p for p in source.parts if p['name']=='Folded white Four Sword blade')['p']
    factor=2670/np.ptp(raw[:,1])
    m.native_scale=round(factor/16*65536)/65536;m.effective_scale=m.native_scale*16
    origin_x=719/m.effective_scale-raw[:,1].min()
    m.transform(rotation('z',-90),offset=[origin_x,332.5/m.effective_scale,-74/m.effective_scale])
    m.markers={'axis_native':[1,0,0],'legacy_blade_x':[719,3389]}
    m.notes=['Complete approved sword split into native blade/hilt DLs. Native hand, sheath, clone and age transforms retain priority and placement.']
    return m

def axe():
    m=component(sword_revamp.iron_knuckle_axe(),'iron_knuckle_axe',factor=70)
    m.transform(rotation('z',11))
    shaft=next(p for p in m.parts if p['name']=='Long patterned axe shaft')['p']
    # Invert the authored source reconstruction exactly (source x=395, -Z upright,
    # 1/70); infer the GI's bbox-centering translation from known shaft end Y=34.
    m.transform(offset=[0,34-shaft[:,1].max(),0])
    m.transform(rotation('x',-90),offset=np.array([395,-62,-1571])/70)
    m.markers={'source_axis_native':[395,-62,0], 'source_units_per_author_unit':70}
    m.notes=['Approved crescent axe mapped back to original 69-vertex Iron Knuckle frame. Held and thrown axe retain their original matrices and physics.']
    return m

BUILDERS={s:(lambda s=s:shield(s)) for s in ['divine_shield','sheikah_shield','shield_of_ikana']}
BUILDERS.update(cane_of_byrna=byrna,trident=trident,four_sword_blade=lambda:four_sword(True),
                four_sword_hilt=lambda:four_sword(False),iron_knuckle_axe=axe)
