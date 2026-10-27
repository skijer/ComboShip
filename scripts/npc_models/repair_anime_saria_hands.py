#!/usr/bin/env python3
"""Anime Saria normal-only hand shading POC; authored proportions stay exact.

The source hand normal field has steep local gradients, but widespread UV-seam
normal failures were not found. This mild topology-local filter softens facet
lighting only. Broad palm/finger silhouettes and baked diffuse creases remain.
Shared wrist rims and ambiguous coincident opposing normals are preserved.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np

from rebatch_native_npc_archives import _decode
from native_assets import o2r

NS = 'alt/objects/object_sa/NativeNpcV1'
SOURCE_ID = 'dddeaed512e7b4aaedf960e1ddab13e74c480897e0f44404fb980c0c9ce5d63a'
CLIPS = ('gSariaPlayingOcarinaAnim', 'gSariaOcarinaToMouthAnim',
         'gSariaStopPlayingOcarinaAnim', 'gSariaReturnToOcarinaAnim')
LIMIT_DEGREES = 15


def hand_meshes():
    for side,hand,fore in (('left',11,10),('right',14,13)):
        for family,stem in (('default_rigid',NS+'/Body'),('default_skin',NS+'/Skin/Limbs')):
            yield stem+f'/Limb{hand:02d}',family+'_'+side,stem+f'/Limb{fore:02d}',side
        for clip in CLIPS:
            for family,suffix in (('grip_rigid',''),('grip_skin','/Skin')):
                stem=NS+'/PoseAnimations/objects/object_sa/'+clip+suffix
                yield stem+'/Hands/'+side.title(),family+'_'+side,stem+f'/Limbs/Limb{fore:02d}',side


def point_key(vertex):
    owner,raw=vertex
    return owner,*struct.unpack_from('<hhh',raw)


def normal_groups(triangles):
    groups=defaultdict(set)
    for triangle in triangles:
        for vertex in triangle:
            groups[point_key(vertex)].add(vertex[1][12:15])
    return groups


def geometry_digest(triangles):
    digest=hashlib.sha256()
    for triangle in triangles:
        for owner,raw in triangle:
            digest.update(struct.pack('<I',owner));digest.update(raw[:6])
    return digest.hexdigest()


def normal_digest(triangles):
    digest=hashlib.sha256()
    for key,values in sorted(normal_groups(triangles).items()):
        digest.update(struct.pack('<IhhhB',*key,len(values)))
        for normal in sorted(values):digest.update(normal)
    return digest.hexdigest()


def soften_normals(triangles,anchors):
    """Return normal triples for unambiguous points, preserving owner bases."""
    groups=normal_groups(triangles)
    original={key:np.array(struct.unpack('<bbb',next(iter(values))),float)
              for key,values in groups.items() if len(values)==1}
    if any(np.linalg.norm(n)<1 for n in original.values()):
        raise ValueError('zero hand normal is unsupported')
    original={key:n/np.linalg.norm(n) for key,n in original.items()}
    neighbors=defaultdict(set)
    for triangle in triangles:
        keys=[point_key(v) for v in triangle]
        for a,b in ((0,1),(1,2),(2,0)):
            if keys[a][0]==keys[b][0] and keys[a]!=keys[b]:
                neighbors[keys[a]].add(keys[b]);neighbors[keys[b]].add(keys[a])
    values=dict(original)
    for _ in range(2):
        newer={}
        for key,n in values.items():
            adjacent=[values[k] for k in sorted(neighbors[key]) if k in values and original[key]@original[k]>.5]
            if key in anchors or not adjacent:
                newer[key]=original[key]
            else:
                mean=np.mean(adjacent,axis=0);mean/=np.linalg.norm(mean)
                combined=n*.5+mean*.5;newer[key]=combined/np.linalg.norm(combined)
        values=newer
    result={}
    for key,n in values.items():
        if key in anchors:
            result[key]=next(iter(groups[key]));continue
        old=original[key];angle=np.arccos(np.clip(old@n,-1,1));limit=np.radians(LIMIT_DEGREES)
        if angle>limit:
            n=old*np.sin(angle-limit)+n*np.sin(limit);n/=np.linalg.norm(n)
        result[key]=np.clip(np.rint(n*127),-127,127).astype(np.int8).tobytes()
    return result


# Finger positions/matrix ownership and original/filtered normal fields are
# pinned independently of archive layout, vertex batching, face and prop work.
PROFILES = {
    'default_rigid_left': (
        'ec53cce810710527b87dbe3e82979ad3da9e43b8336505ad80a1793f2a6d149c',
        '08a2081d25c8e036eacd3b336fca62aae9157f1dd1991862e1e0e6b239a45b63',
        '87c59b5becc9a523effd16d8ee7a27a63d9ec6bf2c4f775ebbfa8add19597a56'),
    'default_skin_left': (
        '81c6af6e5ecff3b42a285c73df31fb4ac6e9f2657837d221541d6f7a36939581',
        'd0bfa71d929432317e4fb8814256b0a5660acba8b0f3651661f068365e2f72b2',
        'c1254cfd6ced27cfaf4682484a2c31fc75680c9a88b6b2a7d69e409ca6aca7f7'),
    'grip_rigid_left': (
        '458951032a5fa095d2b2ca8ea74597b515236d0a5a1e5e9c42b91e49c52daf4e',
        'ffa9b81a505aa172ec74254c549cda00aa59cef6b620f63fff45e8e80cea5736',
        '723744c1e0e82b7cfb4abff8817af32aa560972e78a6dcdbba9239150fd80510'),
    'grip_skin_left': (
        '216f3ca1e99b68d1c3379836c82207675032288cb854bee0ec7fd8ec675e25e3',
        '5a31a37abe41882df7bf29b7f22b4dd0c96c127172fe35efb9dcdfae740048b9',
        'bd60d128cf4c90d7fbc9bed0bbcc84ae148b7ca89f40c3e9e184bade66a177fd'),
    'default_rigid_right': (
        'b46698a4f500c3d376a2372995386977d8998390cb8213ee71e4fed3d2d2a7a7',
        '23671e268b5cc5c76886ffe00ca24f7769043e71d4242eb716595b8e68411bd2',
        '72636d13d5eea005efb7ba680bc7016c2399167a615a2396734f141e8f5fcdf5'),
    'default_skin_right': (
        'bff4d1086ff16520dc7acd7a198597f54cc1adc6b659aa18c16b9eb40b033289',
        '0d953363c87b66508b14bdf68beb82b1505bcb3882f39b6f05d8e46acc6d5412',
        '6ce45f99f3ddf7ed9cc510f41bd5776c08badbeb7a2aad62e0bb573efa1a3909'),
    'grip_rigid_right': (
        '6a66c0b5a84a6dd30a54dd2ec2b9cd09bca3ed1122db0d3747aed7adb4da0175',
        'bad563f9d4f99d7fa2666b513b4c1e388bf46b820c2e25768fd703bebbf80ed8',
        '607a51893127542e5439f8be7f21e0fcce37e9acbf6373341a066264024aa73b'),
    'grip_skin_right': (
        'bf98c00d2bc0b7d36c95479cc7f0d2516122c390963b9f31085e39bed7ca9809',
        '24e4a28cceda1f59f46c63e92c96c794825786189b79e53c4f308ad3937efb62',
        '08fbfadcc318f75e6815cd91d2677db96836923db866e27514dbb48b95babd16'),
}


def _require(condition,message):
    if not condition:raise ValueError(message)


def _edge_p95(triangles):
    groups=normal_groups(triangles)
    normals={key:np.array(struct.unpack('<bbb',next(iter(values))),float)
             for key,values in groups.items() if len(values)==1}
    normals={key:n/np.linalg.norm(n) for key,n in normals.items()}
    edges=set()
    for triangle in triangles:
        keys=[point_key(v) for v in triangle]
        for a,b in ((0,1),(1,2),(2,0)):
            if keys[a][0]==keys[b][0] and keys[a]!=keys[b] and keys[a] in normals and keys[b] in normals:
                edges.add(tuple(sorted((keys[a],keys[b]))))
    return float(np.percentile([np.degrees(np.arccos(np.clip(normals[a]@normals[b],-1,1)))
                                for a,b in sorted(edges)],95)) if edges else 0.


def repair_records(records):
    """Return independent bytes with only selected hand normal triples changed."""
    info=json.loads(records.get('saria_anime_NativeNpcModelInfo.json',b'{}'))
    _require(info.get('source_sha256')==SOURCE_ID and info.get('variant')=='saria-anime' and
             info.get('namespace')==NS.removeprefix('alt/'),'unsupported anime Saria source identity')
    output=dict(records);meshes=[];changed_resources=set()
    for path,kind,fore,side in hand_meshes():
        _require(path in records and fore in records,'required hand geometry/forearm route is missing')
        mesh=_decode(records,path)
        geometry,original_profile,soft_profile=PROFILES[kind]
        _require(geometry_digest(mesh.triangles)==geometry,'unreviewed hand geometry: '+path)
        profile=normal_digest(mesh.triangles)
        _require(profile in (original_profile,soft_profile),'unreviewed hand normal field: '+path)
        groups=normal_groups(mesh.triangles)
        fore_groups=normal_groups(_decode(records,fore).triangles)
        anchors=set(groups)&set(fore_groups)
        _require(len(anchors)==(19 if side=='left' else 32) and
                 all(groups[key]==fore_groups[key] for key in anchors),'hand wrist seam differs from its forearm')
        array=mesh.vertex_path
        hashed=o2r.crc64(array)
        # A conservative literal-hash search also protects unsupported/BE readers.
        references=(struct.pack('<II',hashed>>32,hashed&0xffffffff),
                    struct.pack('>II',hashed>>32,hashed&0xffffffff))
        _require(not any(other!=path and raw[4:8] in (b'TLDO',b'ODLT') and
                         any(ref in raw[72:] for ref in references) for other,raw in records.items()),
                 'hand vertex array has a separate display-list reader')
        for triangle in mesh.triangles:
            for owner,raw in triangle:
                _require((struct.unpack_from('<H',raw,6)[0]>>8)==(owner-0x0d000001)//64,
                         'hand vertex flag differs from matrix ownership')
        changes=0;maximum=0.
        if profile!=soft_profile:
            normals=soften_normals(mesh.triangles,anchors)
            raw=bytearray(records[array])
            for at in range(72,len(raw),16):
                x,y,z,flag=struct.unpack_from('<hhhH',raw,at)
                key=(0x0d000001+(flag>>8)*64,x,y,z)
                normal=normals.get(key)
                if normal is not None and raw[at+12:at+15]!=normal:
                    old=np.array(struct.unpack_from('<bbb',raw,at+12),float)
                    new=np.array(struct.unpack('<bbb',normal),float)
                    maximum=max(maximum,float(np.degrees(np.arccos(np.clip(old@new/np.linalg.norm(old)/np.linalg.norm(new),-1,1)))))
                    raw[at+12:at+15]=normal;changes+=1
            output[array]=bytes(raw)
            if changes:changed_resources.add(array)
        after=_decode(output,path)
        _require(geometry_digest(after.triangles)==geometry and normal_digest(after.triangles)==soft_profile,
                 'hand geometry or bounded normal result failed read-back verification')
        after_groups=normal_groups(after.triangles)
        _require(all(after_groups[key]==fore_groups[key] for key in anchors),'hand wrist seam changed')
        meshes.append(dict(resource=path,changed_normal_records=changes,shared_wrist_points=len(anchors),
                           ambiguous_points_preserved=sum(len(v)>1 for v in groups.values()),
                           edge_gradient_p95_before=_edge_p95(mesh.triangles),
                           edge_gradient_p95_after=_edge_p95(after.triangles),
                           max_quantized_normal_change_degrees=maximum))
    _require(set(output)==set(records),'resource names changed')
    for path,before in records.items():
        after=output[path]
        if path not in changed_resources:
            _require(before==after,'unrelated resource changed: '+path)
        else:
            _require(len(before)==len(after) and all(a==b or (i>=72 and (i-72)%16 in (12,13,14))
                     for i,(a,b) in enumerate(zip(before,after))),'a non-normal vertex byte changed')
    return output,dict(hand_meshes=len(meshes),meshes=meshes,changed_resources=sorted(changed_resources),
                        changed_normal_records=sum(m['changed_normal_records'] for m in meshes),
                        ambiguous_points_preserved=sum(m['ambiguous_points_preserved'] for m in meshes),
                        wrist_normals_byte_identical=True,all_non_normal_bytes_identical=True,
                        positions_grip_weights_animations_textures_unchanged=True,
                        limitation='Subtle facet-lighting softening; authored proportions and baked creases remain',
                        status='NORMALS_ONLY_SHADING_POC_RUNTIME_UNTESTED',runtime_tested=False)


def _same_file(left,right):
    return left.resolve()==right.resolve() or (left.exists() and right.exists() and left.samefile(right))


def repair_archive(source,output):
    source,output=Path(source),Path(output)
    _require(not _same_file(source,output),'output must be separate from source')
    original=source.read_bytes()
    with zipfile.ZipFile(source) as archive:
        _require(len(archive.namelist())==len(set(archive.namelist())),'duplicate archive paths')
        records={n:archive.read(n) for n in archive.namelist()}
    result,report=repair_records(records)
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for path,raw in sorted(result.items()):
            entry=zipfile.ZipInfo(path,(2026,10,20,0,0,0));entry.compress_type=zipfile.ZIP_DEFLATED
            archive.writestr(entry,raw)
    with zipfile.ZipFile(output) as archive:
        _require({n:archive.read(n) for n in archive.namelist()}==result,'written hand candidate differs')
    _require(source.read_bytes()==original,'source archive changed')
    report.update(source=str(source),source_sha256=hashlib.sha256(original).hexdigest(),
                  output=str(output),output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--report',required=True,type=Path)
    args=parser.parse_args()
    _require(not any(_same_file(args.report,path) for path in (args.source,args.output)),
             'report must be separate from source and output archives')
    report=repair_archive(args.source,args.output)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('changed_normal_records','hand_meshes','status')}))


if __name__=='__main__':
    main()
