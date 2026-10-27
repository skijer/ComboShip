"""Repack the reviewed Fairy Ocarina for the renderer's raw RGBA32 importer.

The original decoded colors, display lists, UVs, prop fit and all other model
resources are retained. A native 32x16 RGBA16 tile has 64 bytes per row; its
256x128 raw RGBA32 replacement therefore needs horizontal byte scale16 and
vertical pixel scale8. The old RGBA16/RAW combination uploaded paired packed
texels as RGBA channels, producing the reported corrupted colors.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from native_assets import o2r as asset
from native_assets.n64texture import decode


def repack_texture(raw):
    if len(raw)<92 or raw[0] != 0 or struct.unpack_from('<I',raw,8)[0] != 1:
        raise ValueError('Expected the reviewed little-endian v1 ocarina texture')
    kind,w,h,flags,hs,vs,size=struct.unpack_from('<IIIIffI',raw,64)
    if (kind,w,h,flags,hs,vs,size)==(1,256,128,1,16.,8.,131072) and len(raw)==92+size:
        return raw
    if (kind,w,h,flags,hs,vs,size)!=(2,256,128,1,8.,8.,65536) or len(raw)!=92+size:
        raise ValueError('Expected the reviewed RGBA16/RAW 256x128 ocarina texture')
    pixels=decode(raw[92:],0,2,w,h).tobytes()
    return raw[:64]+struct.pack('<IIIIffI',1,w,h,1,16.,8.,len(pixels))+pixels


def repair(baseline,output):
    baseline,output=map(Path,(baseline,output))
    if baseline.resolve()==output.resolve() or (output.exists() and output.samefile(baseline)):
        raise ValueError('Preserve the source archive')
    source=baseline.read_bytes()
    with zipfile.ZipFile(baseline) as z:
        if len(z.namelist())!=len(set(z.namelist())) or z.testzip() is not None:
            raise ValueError('Invalid source archive')
        records={n:z.read(n) for n in z.namelist()}
    manifests=[n for n in records if n.endswith('_NativeNpcModelInfo.json')]
    if len(manifests)!=1:
        raise ValueError('Expected one Saria model manifest')
    meta=json.loads(records[manifests[0]])
    if meta.get('variant') not in ('saria-anime','saria-render') or meta.get('namespace')!='objects/object_sa/NativeNpcV1':
        raise ValueError('Expected a supported Saria model')
    path='alt/'+meta['namespace']+'/Props/Textures/FairyOcarinaHD'
    if path not in records:
        raise ValueError('Missing reviewed Fairy Ocarina texture')
    fixed=repack_texture(records[path])
    report=dict(parent_sha256=hashlib.sha256(source).hexdigest(),variant=meta['variant'],
                changed_texture=path,decoded_rgba_colors_preserved=True,
                raw_upload=dict(width=256,height=128,bytes_per_row=1024,bytes=131072,
                                horizontal_byte_scale=16,vertical_pixel_scale=8),
                geometry_uvs_grip_display_lists_and_all_other_resources_byte_identical=True,
                runtime_tested=False,user_accepted=False)
    info=meta['namespace']+'/OcarinaUploadRepairInfo.json'
    updated=dict(records)
    updated[path]=fixed
    updated[info]=json.dumps(report,indent=2).encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    # Do not normalize or regenerate unrelated references in this existing pack.
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for name,raw in sorted(updated.items()):
            entry=zipfile.ZipInfo(name,(2026,10,20,0,0,0))
            entry.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(entry,raw)
    with zipfile.ZipFile(output) as z:
        actual={n:z.read(n) for n in z.namelist()}
    if actual!=updated or baseline.read_bytes()!=source:
        raise ValueError('Written archive or preserved input differs')
    if {n for n in records if updated[n]!=records[n]}-{path,info}:
        raise ValueError('An unrelated resource changed')
    report.update(output_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                  protected_resources=len(records)-1,source_preserved=True)
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('baseline',type=Path)
    p.add_argument('output',type=Path)
    p.add_argument('--report',type=Path)
    a=p.parse_args()
    if a.report and (a.report.resolve() in (a.baseline.resolve(),a.output.resolve()) or
                     (a.report.exists() and (a.report.samefile(a.baseline) or
                      (a.output.exists() and a.report.samefile(a.output))))):
        raise ValueError('Report must be separate from both archives')
    report=repair(a.baseline,a.output)
    if a.report:
        a.report.parent.mkdir(parents=True,exist_ok=True)
        a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
