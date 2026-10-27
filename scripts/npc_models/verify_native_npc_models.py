"""Read exported resources back, validate native contracts and inspect poses."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np

from native_npc_models import PROFILES, VARIANTS, NativeRig, SourceModel, asset, read_limb, animation_frames


class Archive:
    def __init__(self, native, candidate):
        self.native = zipfile.ZipFile(native)
        self.candidate = zipfile.ZipFile(candidate)
        self.hashes = {}
        for name in [*self.native.namelist(), *self.candidate.namelist()]:
            self.hashes[asset.crc64(name)] = name
            # Archive::IndexFile indexes only literal paths. Alt selection happens
            # after a hash resolves to a name; it cannot rescue an unknown hash.
        self.vertices, self.textures = {}, {}

    def read(self, name):
        name = name.removeprefix("__OTR__")
        if not name.startswith("alt/") and "alt/"+name in self.candidate.namelist():
            return self.candidate.read("alt/"+name)
        if name in self.candidate.namelist():
            return self.candidate.read(name)
        return self.native.read(name)

    def texture(self, path):
        if path not in self.textures:
            self.textures[path] = asset.read_texture(self.read(path))
        return self.textures[path]

    def texture_scale(self, path):
        raw = self.read(path)
        _, endian, _, version = asset._read_header(raw)
        return struct.unpack_from(endian+'ff', raw, 0x50) if version >= 1 else (1., 1.)

    def vertex_buffer(self, path):
        if path not in self.vertices:
            raw = self.read(path)
            if raw[4:8] != b"RRAO" or struct.unpack_from("<I", raw, 64)[0] != 25:
                raise ValueError("Display list references a non-vertex array")
            count = struct.unpack_from("<I", raw, 68)[0]
            dtype = np.dtype([("xyz", "<i2", 3), ("flag", "<u2"), ("uv", "<i2", 2), ("normal", "i1", 3), ("alpha", "u1")])
            self.vertices[path] = np.frombuffer(raw, dtype=dtype, offset=72, count=count)
        return self.vertices[path]

    def skin(self, rig):
        path = 'alt/'+rig.ns+'/Skin/Palette'
        if not hasattr(self, 'candidate') or path not in self.candidate.namelist():
            return None
        from native_npc_skin import decode
        if not hasattr(self, '_skin'):
            self._skin = decode(self.candidate.read(path))
        return self._skin

    def matrices(self, rig):
        base = np.tile(np.eye(4), (len(rig.limbs), 1, 1))
        for i, limb in enumerate(rig.limbs):
            base[i, :3, :3], base[i, :3, 3] = limb.M, limb.T
        skin = self.skin(rig)
        if skin is not None:
            base = np.concatenate([base, skin.matrices(base[rig.skel.slot_to_limb])[rig.dlist_count:]])
        return base

    def geometry(self, path, rig, host, triangles_only=False):
        """Interpret the actual cache, recursive calls and flex-matrix loads."""
        state = dict(cache=[None]*32, slot=rig.skel.slot_to_limb.index(host), pending=None, texture=None)
        records, textures = [], []
        skin = self.skin(rig)
        matrix_count = rig.dlist_count + (len(skin.blends) if skin else 0)

        def walk(path, depth=0):
            if depth > 16:
                raise ValueError("Display-list recursion exceeds the model contract")
            raw = self.read(path)
            if raw[4:8] != b"TLDO" or raw[64] != 4:
                raise ValueError("Expected an F3DEX2 display list")
            offset = 72
            while offset+8 <= len(raw):
                first, second = struct.unpack_from("<II", raw, offset)
                op = first >> 24
                size = 16 if op in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36) else 8
                if size == 16:
                    high, low = struct.unpack_from("<II", raw, offset+8)
                    hashed = high << 32 | low
                    if op != 0x33 and hashed not in self.hashes:
                        raise ValueError(f"Unresolved model reference {hashed:016x} in {path}")
                    target = self.hashes.get(hashed)
                    if op == 0x31:
                        walk(target, depth+1)
                    elif op == 0x32:
                        count = (first >> 12) & 255
                        start = ((first >> 1) & 127)-count
                        data = self.vertex_buffer(target)
                        source_offset = second//16
                        if start < 0 or start+count > 32 or source_offset+count > len(data):
                            raise ValueError("Vertex-cache load exceeds its actual buffer")
                        if state["slot"] not in range(matrix_count):
                            raise ValueError("Vertex-cache load uses an invalid native matrix")
                        for i, record in enumerate(data[source_offset:source_offset+count]):
                            state["cache"][start+i] = (record, state["slot"])
                    elif op == 0x20:
                        state["pending"] = target
                elif op == 0xDA and second >> 24 == 0x0D:
                    state["slot"] = (second & 0xFFFFFF)//64
                elif op in (0xF3, 0xF4):
                    state["texture"] = state["pending"]
                elif op in (0x05, 0x06):
                    commands = [first & 0xFFFFFF] + ([second & 0xFFFFFF] if op == 0x06 else [])
                    for command in commands:
                        indices = [(command >> shift & 255)//2 for shift in (16, 8, 0)]
                        if any(i >= 32 or state["cache"][i] is None for i in indices):
                            raise ValueError("Triangle references an unloaded native vertex")
                        records.append([state["cache"][i] for i in indices])
                        textures.append(state["texture"])
                elif op == 0xDF:
                    return
                offset += size
            raise ValueError("Display list has no END command")

        walk(path)
        if triangles_only:
            return len(records)
        xyz, normal, uv, bones = [], [], [], []
        for triangle in records:
            for vertex, slot in triangle:
                xyz.append(vertex["xyz"])
                normal.append(vertex["normal"])
                uv.append(vertex["uv"])
                bones.append(rig.skel.slot_to_limb[slot] if slot < rig.dlist_count else len(rig.limbs)+slot-rig.dlist_count)
        return dict(local=np.asarray(xyz, float).reshape(-1, 3), normals=np.asarray(normal, float).reshape(-1, 3)/127,
                    uv=np.asarray(uv, float).reshape(-1, 2)/(512*32), bones=np.asarray(bones),
                    faces=np.arange(len(xyz)).reshape(-1, 3), textures=textures)


def verify(native, source_dir, candidate, allow_fitted_rig=False):
    archive = Archive(native, candidate)
    if archive.candidate.testzip() is not None:
        raise ValueError("Candidate ZIP integrity failure")
    metadata = [name for name in archive.candidate.namelist() if name.endswith("_NativeNpcModelInfo.json")]
    if len(metadata) != 1:
        raise ValueError("Candidate needs one unique model manifest")
    report = json.loads(archive.candidate.read(metadata[0]))
    variant = report["variant"]
    source = SourceModel(Path(source_dir)/VARIANTS[variant]["file"])
    if report["source_sha256"] != hashlib.sha256(source.raw).hexdigest() or report["native_sha256"] != hashlib.sha256(Path(native).read_bytes()).hexdigest():
        raise ValueError("Source/native input differs from the model manifest")
    rig = NativeRig(archive.native, VARIANTS[variant]["profile"])
    skin = archive.skin(rig)
    expected = sum(len(m["faces"]) for m in source.meshes)
    count = 0
    skin_count = 0
    rig_identical = True
    for i, limb in enumerate(rig.limbs):
        path = rig.ns+f"/Rig/Limb{i:02d}"
        original = archive.candidate.read(path)
        alternate = archive.candidate.read("alt/"+path)
        base_fields, base_dl, _ = read_limb(original)
        alt_fields, _, _ = read_limb(alternate)
        if base_dl != rig.near[i] or base_fields[3:] != alt_fields[3:]:
            raise ValueError("Private rig changes native hierarchy or fallback geometry")
        rig_identical = rig_identical and base_fields == alt_fields
        if not allow_fitted_rig and base_fields != alt_fields:
            raise ValueError("Fitted rig translations require --allow-fitted-rig")
        if base_fields != (limb.x, limb.y, limb.z, limb.child, limb.sibling):
            raise ValueError("Private rig differs from the actual native source")
        if limb.dlist:
            count += archive.geometry(rig.ns+f"/Body/Limb{i:02d}", rig, i, triangles_only=True)
            if skin and i != rig.profile['head']:
                skin_count += archive.geometry(rig.ns+f'/Skin/Limbs/Limb{i:02d}', rig, i, triangles_only=True)
    brooch = report.get("geometry_repairs", {}).get("bowser_brooch")
    revised = expected
    if brooch is not None:
        if variant != "young-malon" or brooch["donor_variant"] != "malon":
            raise ValueError("Unrecognized source geometry repair")
        removed = brooch["removed_source_faces"]
        if len(set(removed)) != len(removed) or not all(0 <= i < expected for i in removed):
            raise ValueError("Invalid repaired source triangle selection")
        revised += brooch["added_triangles"]-len(removed)
    shard=report.get('geometry_repairs',{}).get('detached_source_shard')
    if shard:
        if variant!='saria-render' or shard['source_mesh_id']!=24 or source.meshes[24]['name']!='tripo_part_9':
            raise ValueError('Unrecognized detached source shard')
        mesh=source.meshes[24];offset=sum(len(m['faces']) for m in source.meshes[:24])
        expected_removed=(np.flatnonzero((mesh['positions'][mesh['faces'],0]>.10).all(1))+offset).tolist()
        if len(expected_removed)!=44 or shard['removed_source_faces']!=expected_removed:
            raise ValueError('Detached shard removal differs from the audited source component')
        revised-=len(expected_removed)
    fin=report.get('geometry_repairs',{}).get('rear_shorts_fin')
    if fin:
        if variant!='saria-render' or fin['source_mesh_id']!=21 or source.meshes[21]['name']!='tripo_part_6':
            raise ValueError('Unrecognized rear shorts fin')
        mesh=source.meshes[21];offset=sum(len(m['faces']) for m in source.meshes[:21])
        p=mesh['positions'][mesh['faces']]
        ids=np.flatnonzero((np.arange(len(mesh['faces']))<2120)&
                          ((p>[-.066,.410,-.0611])&(p<[-.059,.4492,-.047])).all((1,2)))
        cap=np.array([[1158,1175,1162],[1158,1162,1120],[1158,1120,1105],[1158,1105,1116]])
        vertex_offset=sum(len(m['positions']) for m in source.meshes[:21])
        if (len(ids)!=52 or fin['removed_source_faces']!=(ids+offset).tolist() or
            fin.get('added_triangles')!=4 or fin.get('added_source_vertex_triangles')!=(cap+vertex_offset).tolist()):
            raise ValueError('Rear shorts fin removal differs from the audited visible source fragment')
        revised+=len(cap)-len(ids)
    if count != revised or expected != report["source_triangles"]:
        raise ValueError("Source triangles were lost or duplicated")
    connection_triangles = report.get('joint_skin',{}).get('connection_triangles',0)
    expected_skin_count=count-int(report['body_triangles'][str(rig.profile['head'])])+connection_triangles
    shared=report.get('joint_skin',{}).get('shared_shoulders')
    if shared:
        from expand_twins_pose_coverage import shared_shoulders,PARENT_SHA256,HOSTS
        if variant!='twins' or shared['parent_sha256']!=PARENT_SHA256:
            raise ValueError('Shared shoulder skin differs from the accepted Twins repair')
        records={n:archive.candidate.read(n) for n in archive.candidate.namelist()}
        wrappers=shared_shoulders(records,rig.ns);sources={};counts={}
        wrappers,_=asset.repair_native_npc_hashes(wrappers,records)
        for host in HOSTS:
            destination=rig.ns+f'/Skin/Limbs/Limb{host:02d}'
            source_path=rig.ns+'/PoseAnimations/objects/object_os_anime/gKokiriIdleAnim'+f'/Skin/Limbs/Limb{host:02d}'
            if archive.candidate.read('alt/'+destination)!=wrappers['alt/'+destination]:
                raise ValueError('Shared shoulder wrapper bypasses the reviewed skin')
            before=int(report['body_triangles'][str(host)])
            after=archive.geometry(source_path,rig,host,triangles_only=True)
            sources[destination]=source_path;counts[str(host)]=dict(previous=before,shared=after)
            expected_skin_count+=after-before
        if shared['sources']!=sources or shared['triangle_counts']!=counts:
            raise ValueError('Shared shoulder topology differs from its recorded source')
    if skin and skin_count != expected_skin_count:
        raise ValueError('Weighted geometry lost body faces or changed its recorded joint connectors')
    if skin and variant == 'twins':
        for name in ('gKokiriIdleAnim','gKokiriStandingArmsBehindBackAnim'):
            path='objects/object_os_anime/'+name
            if archive.candidate.read('alt/'+rig.ns+'/PoseAnimations/'+path) != archive.native.read(path):
                raise ValueError('The twins idle/sway no longer uses the exact native animation')
        neutral=archive.candidate.read('alt/'+rig.ns+'/Faces/L0R0M0')
        for part in ('Base','L0','R0','M0'):
            face_path=rig.ns+'/FaceParts/'+part
            face_path=face_path if asset.crc64(face_path) in archive.hashes else 'alt/'+face_path
            hashed=asset.crc64(face_path)
            if struct.pack('<II',hashed>>32,hashed&0xffffffff) not in neutral:
                raise ValueError('The twins neutral expression bypasses its repaired face surface')
    texture_type, w, h, pixels = archive.texture(rig.ns+"/Textures/Diffuse")
    if texture_type != 1 or pixels != np.asarray(source.image).tobytes() or (w, h) != source.image.size:
        raise ValueError("Source diffuse pixels or dimensions changed")
    color_repair=report.get('geometry_repairs',{}).get('skin_chroma')
    if color_repair:
        from native_npc_color import repair_traditional_saria_diffuse
        corrected,details=repair_traditional_saria_diffuse(source)
        if color_repair.get('legs',{}).get('applied'):
            from native_npc_color import repair_traditional_saria_legs
            corrected,leg_details=repair_traditional_saria_legs(source,corrected,details['reference_linear_chroma'])
            if leg_details != color_repair['legs']:
                raise ValueError('Leg skin calibration differs from its bounded source repair')
        kind,cw,ch,raw=archive.texture(color_repair['texture'])
        if (kind,cw,ch)!=(1,w,h) or raw!=corrected.tobytes() or hashlib.sha256(raw).hexdigest()!=color_repair['pixel_sha256']:
            raise ValueError('Corrected skin atlas differs from its bounded source repair')
    grip=report.get('geometry_repairs',{}).get('ocarina_grip',{}).get('profile',{})
    if grip.get('fit_mode')=='whole_arm_wrist_seat':
        rows=asset.read_vertex_array(archive.native.read('objects/object_sa/object_saVtx_006948'))[:20]
        native_xyz=np.array([row[:3] for row in rows])*.8
        seat=np.asarray(grip['hands']['Right']['wrist_offset'])
        for suffix,expected_xyz in (('',native_xyz),('Grip',native_xyz-seat)):
            actual=asset.read_vertex_array(archive.read(rig.ns+'/Props/Ocarina'+suffix+'Vertices'))
            expected_rows=[(*np.rint(xyz).astype(int),*row[3:]) for row,xyz in zip(rows,expected_xyz)]
            if not np.array_equal(np.asarray(actual),np.asarray(expected_rows)):
                raise ValueError('Shared or seated ocarina vertices differ from the authored grip contract')
        grip_path=rig.ns+'/Props/OcarinaGrip'
        # Both legacy and repaired packs can be inspected, but geometry() above
        # rejects unresolved legacy hashes using the real literal-name index.
        grip_path=grip_path if asset.crc64(grip_path) in archive.hashes else 'alt/'+grip_path
        hashed=asset.crc64(grip_path)
        call=struct.pack('<II',hashed>>32,hashed&0xffffffff)
        for clip in ('gSariaPlayingOcarinaAnim','gSariaOcarinaToMouthAnim',
                     'gSariaStopPlayingOcarinaAnim','gSariaReturnToOcarinaAnim'):
            private=rig.ns+'/PoseAnimations/objects/object_sa/'+clip
            for suffix in ('','/Skin'):
                if call not in archive.read(private+suffix+'/Limbs/Limb14'):
                    raise ValueError('Ocarina grip pose does not select its separately seated prop')
        for shared in ('/Props/RightHandOcarina','/Skin/Props/RightHandOcarina'):
            if call in archive.read(rig.ns+shared):
                raise ValueError('A seated prop leaked into shared non-grip poses')
    pose_count = 0
    torso_pose_count = 0
    posture = report.get('geometry_repairs', {}).get('pose3_posture')
    arm_joints = {i+1 for side in ("left", "right") for i in rig.profile[side][3:]}
    stance_clips=[]
    for name in archive.candidate.namelist():
        prefix = "alt/"+rig.ns+"/PoseAnimations/"
        if not name.startswith(prefix):
            continue
        relative = name[len(prefix):]
        if "/Limbs/" in relative or "/Hands/" in relative or "/Props/" in relative or "/Skin/" in relative:
            continue
        fitted = animation_frames(archive.candidate.read(name))
        original = animation_frames(archive.native.read(relative))
        allowed=arm_joints.copy()
        stance=report.get('fitted_poses',{}).get(relative.rsplit('/',1)[-1],{}).get('planted_stance')
        if stance:
            expected_channels=[i+1 for side in ('left','right') for i in rig.profile[side][:3]]
            if variant!='malon' or relative!='objects/object_ma2/gMalonAdultStandStillAnim' or stance['joint_channels']!=expected_channels:
                raise ValueError('Unrecognized planted stance animation channels')
            allowed.update(expected_channels);stance_clips.append(relative)
            if stance.get('skirt_joint_channel') is not None:
                if stance['skirt_joint_channel']!=10 or not stance.get('skirt_aligned_to_native_idle'):
                    raise ValueError('Unrecognized basket skirt alignment')
                idle=animation_frames(archive.native.read('objects/object_ma2/gMalonAdultIdleAnim'))
                if not np.array_equal(fitted[:,10],np.broadcast_to(idle[0,10],fitted[:,10].shape)):
                    raise ValueError('Basket skirt differs from the accepted idle alignment')
                allowed.add(10)
        non_arm=sorted(set(range(len(rig.limbs)+1))-allowed)
        if fitted.shape != original.shape:
            raise ValueError("Fitted pose changed native timing or joint count")
        if posture and variant == 'twins' and relative == 'objects/object_os_anime/gKokiriSittingHeadOnHandAnim':
            # This reviewed silhouette correction rotates only the complete
            # torso chain. Validate its exact bounded fit; other clips keep
            # the original arm-only rule.
            from refine_twins_seated_posture import validate_posture
            validate_posture(archive, posture, fitted)
            torso_pose_count += 1
        elif not np.array_equal(fitted[:, non_arm], original[:, non_arm]):
            raise ValueError("Fitted pose changed native timing, root motion or a non-arm joint")
        pose_count += 1
    if posture and torso_pose_count != 1:
        raise ValueError('Seated posture metadata has no matching bounded pose')
    if brooch:
        donor = SourceModel(Path(source_dir)/VARIANTS["malon"]["file"])
        if brooch["donor_source_sha256"] != hashlib.sha256(donor.raw).hexdigest():
            raise ValueError("The Bowser repair donor differs from the supplied adult model")
        _, cw, ch, custom = archive.texture(rig.ns+"/Textures/BowserBrooch")
        if (cw, ch) != (512, 512) or brooch["artwork_pixel_sha256"] != hashlib.sha256(custom).hexdigest():
            raise ValueError("The recovered Bowser projection differs from its manifest")
    for name in archive.candidate.namelist():
        if name != metadata[0] and not name.removeprefix("alt/").startswith(rig.ns+"/"):
            raise ValueError("Archive overwrites a public or unrelated resource")
        raw = archive.candidate.read(name)
        if raw[4:8] == b"TLDO" and "/Body/" in name:
            # ENV alpha is the caller's distance/cutscene fade; native actor
            # code, rather than an OPA render-mode override, owns the blend.
            if struct.pack("<II", 0xFC127E03, 0xFFFFFBFD) not in raw:
                # Body wrappers delegate materials to their private leaves.
                # A wrapper may only mark, call display lists and end; it
                # cannot silently change the inherited fade/material state.
                offset, calls, pure = 72, 0, True
                while offset+8 <= len(raw):
                    op = raw[offset+3]
                    if op not in (0x33, 0x31, 0xDF):
                        pure = False; break
                    calls += op == 0x31
                    offset += 16 if op in (0x33, 0x31) else 8
                if not pure or not calls:
                    raise ValueError("Custom material loses native fade alpha")
        if raw[4:8] == b"RRAO":
            if name.endswith('/Skin/Palette'):
                continue
            records = archive.vertex_buffer(name)
            maximum = rig.dlist_count + (len(skin.blends) if skin and '/Skin/' in name else 0)
            if np.any(records["flag"] >> 8 >= maximum) or np.any(records["alpha"] != 255):
                raise ValueError("Native vertex flags or alpha are invalid")
    for left in range(5):
        for right in range(5):
            for mouth in range(5):
                archive.geometry(rig.ns+f"/Faces/L{left}R{right}M{mouth}", rig, rig.profile["head"], triangles_only=True)
    if rig.name == "saria":
        combined = archive.geometry(rig.ns+"/Props/RightHandOcarina", rig, rig.profile["right"][-1], triangles_only=True)
        hand = int(report["body_triangles"]["14"])
        if combined <= hand or combined-hand != 36:
            raise ValueError("The native ocarina prop batch changed")
    return dict(candidate=Path(candidate).name, sha256=hashlib.sha256(Path(candidate).read_bytes()).hexdigest(),
                body_triangles=count, limb_count=len(rig.limbs), matrix_count=rig.dlist_count,
                expression_routes=125, diffuse_pixels_identical=True, native_rig_identical=rig_identical,
                fitted_pose_clips=pose_count,
                non_arm_animation_channels_unchanged=not stance_clips and torso_pose_count == 0,
                non_adapted_animation_channels_unchanged=True, planted_stance_clips=stance_clips,
                bounded_torso_posture_clips=torso_pose_count,
                live_joint_blend_matrices=len(skin.blends) if skin else 0,
                joint_connection_triangles=connection_triangles,
                native_fallback=True, native_fade_alpha=True, private_resources_only=True,
                runtime_tested=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native", type=Path)
    parser.add_argument("sources", type=Path)
    parser.add_argument("candidates", type=Path)
    parser.add_argument("--allow-fitted-rig", action="store_true", help="Allow Alt limb offsets; preserve native fallback and hierarchy")
    args = parser.parse_args()
    results = []
    for variant in VARIANTS.values():
        result = verify(args.native, args.sources, args.candidates/variant["archive"], args.allow_fitted_rig)
        results.append(result)
        print("PASS", json.dumps(result), flush=True)
    (args.candidates/"Verification.json").write_text(json.dumps(results, indent=2)+"\n")


if __name__ == "__main__":
    main()
