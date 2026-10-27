#!/usr/bin/env python3
"""Use unmodified archive bytes to populate real resource classes; run production validation/ownership.
This does not substitute for the separate full resource-factory fixture.
"""
from pathlib import Path
import os,re,struct,sys,tempfile,subprocess,zipfile,json
ROOT=Path(__file__).resolve().parents[2]
def function(s,n):
    m=re.search(r'^[^\n;{}]*\b'+re.escape(n)+r'\([^;{}]*\)\s*\{',s,re.M);i=m.end();depth=1
    while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
    return s[m.start():i]+'\n'
def string(data,o):
    n,=struct.unpack_from('<I',data,o);o+=4;return data[o:o+n].decode(),o+n
with tempfile.TemporaryDirectory(prefix='regional-resources-') as d, zipfile.ZipFile(sys.argv[1]) as z:
    work=Path(d);lines=['static void BuildResources() {'];skeletons=[]
    for name in z.namelist():
        if not name.startswith(('objects/object_skb/','objects/object_zog/','objects/object_gg/')):continue
        data=z.read(name)
        if len(data)<64:continue
        kind=data[4:8][::-1];s=[]
        if kind==b'OSKL':skeletons.append((name,data));continue
        if kind==b'OSLB':
            if data[64]!=1:continue
            _,o=string(data,66);_,mods=struct.unpack_from('<HI',data,o);assert not mods;o+=6;_,o=string(data,o);o+=18
            _,o=string(data,o);_,o=string(data,o);dl,o=string(data,o);_,o=string(data,o)
            x,y,zz,c,si=struct.unpack_from('<hhhBB',data,o)
            s=['auto r=std::make_shared<SOH::SkeletonLimb>();','r->limbType=SOH::LimbType::Standard;',f'r->limbData.standardLimb={{{{{x},{y},{zz}}},{c},{si},'+('(Gfx*)'+json.dumps(dl) if dl else 'nullptr')+'};']
        elif kind==b'OANM':
            t,frames,n=struct.unpack_from('<IhI',data,64);assert t==0
            values=struct.unpack_from(f'<{n}H',data,74);o=74+n*2;nj,=struct.unpack_from('<I',data,o);o+=4
            joints=list(struct.iter_unpack('<HHH',data[o:o+nj*6]));static,=struct.unpack_from('<H',data,o+nj*6)
            s=['auto r=std::make_shared<SOH::Animation>();','r->type=SOH::AnimationType::Normal;',f'r->rotationValues={{{",".join(map(str,values))}}};', 'r->rotationIndices={'+','.join('{'+','.join(map(str,j))+'}' for j in joints)+'};',f'r->animationData.animationHeader={{{{{frames}}},(int16_t*)r->rotationValues.data(),(SOH::JointIndex*)r->rotationIndices.data(),{static}}};']
        elif kind==b'OTEX':
            typ,w,h,size=struct.unpack_from('<IIII',data,64)
            assert len(data)>=80+size
            s=['auto r=std::make_shared<Fast::Texture>();',f'r->Type=(Fast::TextureType){typ};r->Width={w};r->Height={h};r->ImageDataSize={size};', 'r->ImageData=new uint8_t[]{'+','.join(map(str,data[80:80+size]))+'};']
        elif kind==b'ODLT' and name.endswith('object_zog_DL_0280A8'):
            commands=list(struct.iter_unpack('<II',data[72:]));s=['auto r=std::make_shared<Fast::DisplayList>();']
            for a,b in commands:s.append(f'{{ Gfx g{{}};g.words.w0={a}U;g.words.w1={b}U;r->Instructions.push_back(g);}}')
        else:continue
        lines+=['{',*s,'resources.emplace('+json.dumps(name)+',r);','}']
    for name,data in skeletons:
        typ,limbType,n,mat,tableType,total=struct.unpack_from('<BBIIBI',data,64);o=79;paths=[]
        for _ in range(total):p,o=string(data,o);paths.append(p)
        lines+=['{','auto r=std::make_shared<SOH::Skeleton>();',f'r->type=(SOH::SkeletonType){typ};r->limbType=r->limbTableType=SOH::LimbType::Standard;r->limbCount=r->limbTableCount={n};r->dListCount={mat};']
        for p in paths:lines+=['r->limbTable.push_back('+json.dumps(p)+');','r->limbResources.push_back(resources.at('+json.dumps(p)+'));','r->skeletonHeaderSegments.push_back(r->limbResources.back()->GetRawPointer());']
        lines += [f'r->skeletonData.flexSkeletonHeader={{{{r->skeletonHeaderSegments.data(),{n},{typ}}},{mat}}};','resources.emplace('+json.dumps(name)+',r);','}']
    lines+=['}']
    loader=(ROOT/'soh/mods/transformation_masks/assets/mm_asset_loader.cpp').read_text()
    fixture=(ROOT/'tests/regional_actors/resource_test.cpp').read_text().replace('/* PRODUCTION_LOADERS */','\n'.join(function(loader,n) for n in ['MmAssets_LoadNormalActor','MmAssets_ReleaseNormalActor'])).replace('/* ARCHIVE_RESOURCES */','\n'.join(lines))
    (work/'fixture.cpp').write_text(fixture)
    skeleton=(ROOT/'soh/soh/resource/type/Skeleton.cpp').read_text()
    (work/'skeleton.cpp').write_text('#include "soh/resource/type/Skeleton.h"\nnamespace SOH {\n'+function(skeleton,'Skeleton::GetPointer')+function(skeleton,'Skeleton::GetPointerSize')+'}\n')
    sources=[work/'fixture.cpp',work/'skeleton.cpp',*[ROOT/('soh/soh/resource/type/'+n+'.cpp') for n in ['SkeletonLimb','Animation']],ROOT/'libultraship/src/ship/resource/Resource.cpp',*[ROOT/('libultraship/src/fast/resource/type/'+n+'.cpp') for n in ['Texture','DisplayList']]]
    flags=['-DF3DEX_GBI_2','-DFMT_HEADER_ONLY',*['-I'+str(ROOT/p) for p in ['libultraship/include','soh/include','soh','soh/assets','soh/mods','combo']]]
    for name in ['actor','mm_actor']:
        src=ROOT/('soh/src/overlays/actors/ovl_En_Viewer/static_story_'+name+'.c');obj=work/(name+'.o');subprocess.run(['cc',*flags,'-c',src,'-o',obj],check=True);sources.append(obj)
    subprocess.run(['c++','-std=c++20',*flags,*sources,'-o',work/'test'],check=True)
    subprocess.run([work/'test'],check=True)
