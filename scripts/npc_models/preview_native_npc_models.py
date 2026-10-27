"""Offline renders read back from the exported native archives, never game captures."""
import argparse
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from native_npc_models import VARIANTS, NativeRig, animation_frames, unit
from verify_native_npc_models import Archive
from native_assets import n64texture
from native_assets.gbi import Triangle, TileState, Vertex
from native_assets.render import Tex, render
from scipy.spatial.transform import Rotation

ANIMATIONS = {
    "saria": ["gSariaWaitArmsToSideAnim", "gSariaPlayingOcarinaAnim", "gSariaSitting3Anim",
              "gSariaHandsBehindBackWaitAnim", "gSariaWaitOnBridgeAnim"],
    "twins": ["gKokiriIdleAnim", "gKokiriStandingArmsBehindBackAnim", "gKokiriSittingCrossedArmsLegsAnim"],
    "young-malon": ["gMalonChildIdleAnim", "gMalonChildRaiseHandsAnim", "gMalonChildSingAnim"],
    "malon": ["gMalonAdultIdleAnim", "gMalonAdultStandStillAnim", "gMalonAdultSingAnim"],
}

KOKIRI_STORY_POSES=(
    'gKokiriIdleAnim','gKokiriStandingArmsBehindBackAnim','gKokiriStandingHandsOnHipsAnim',
    'gKokiriSittingHeadOnHandAnim','gKokiriSittingCrossedLegsAnim','gKokiriSittingCrossedArmsLegsAnim',
)

def animation_for_mode(name,animation_object,mode,animation=None):
    """Keep old review indices stable; expose actual cutscene entry numbers."""
    story=None
    if mode.startswith('static'):
        if name!='twins':raise ValueError('Static Kokiri entries require the Twins rig')
        story=pose=int(mode[6:]);selected=KOKIRI_STORY_POSES[pose]
    else:
        pose=0 if mode.startswith('face') else int(mode[4:])
        selected=animation or ANIMATIONS[name][pose]
    selected=selected.removeprefix('__OTR__')
    if not selected.startswith('objects/'):selected=f'objects/{animation_object}/{selected}'
    return selected,pose,story


def native_shade(normals, matrix, light):
    """Match F3DEX CalculateNormalDir for the review's directional light.

    The engine normalizes M-transpose times light, then dots local normals.
    Normalizing transformed normals differs when a skin blend is not rigid.
    """
    direction=unit(matrix.T@light)
    return np.clip(.45+.55*np.maximum(normals@direction,0),0,1)


def preview(task):
    native, directory, variant, mode, *extra = task
    options = extra[0] if extra else {}
    directory = Path(directory)
    settings = VARIANTS[variant]
    candidate = directory/options.get('archive', settings['archive'])
    if not candidate.exists() and 'archive' not in options:
        matches = list(directory.glob(settings['archive'].rsplit('_POC',1)[0]+'_POC*.o2r'))
        if len(matches) != 1:
            raise ValueError('Select one exact archive version for this preview')
        candidate = matches[0]
    archive = Archive(native, candidate)
    control = options.get("native_control", False)
    rig = NativeRig(archive, settings["profile"], private=not control)
    expression = mode.startswith("face")
    anim_object = rig.profile.get("animation_object", rig.profile["object"])
    animation,pose,story=animation_for_mode(rig.name,anim_object,mode,options.get('animation'))
    if expression:
        eye, mouth = int(mode[4:]), 0
    else:
        eye, mouth = 0, 0
        if rig.name == "saria" and pose == 1:
            eye = 0 if options.get("talking") else 2
    pose_key = rig.ns+"/PoseAnimations/"+animation
    clip = "alt/"+pose_key
    frames = animation_frames(archive.candidate.read(clip) if not control and clip in archive.candidate.namelist() else archive.native.read(animation))
    absolute_frame=options.get("frame",len(frames)//2 if pose else 0)
    frame=absolute_frame%len(frames);posed=frames[frame].copy()
    if story==4 and (control or clip not in archive.candidate.namelist()):
        # Match StaticStoryKokiri_UpdatePose's native head/leg adapter. This is
        # part of the static actor pose, rather than the raw native clip.
        posed[15,2]=(-32768-int(posed[1,1])-int(posed[8,2])+32768)%65536-32768
        donor=animation_frames(archive.native.read('objects/object_dy_obj/gGreatFairySittingAnim'))
        posed[2:8]=donor[absolute_frame%len(donor),2:8]
    rig.pose(posed)
    textures, records, head_points = {}, [], []

    def texture(path):
        if control or options.get('clay'):
            # Native controls are explicitly clay renders: their original
            # segmented eye textures and CI palettes need game RDP state.
            return Tex(np.full((2, 2, 4), [190, 190, 190, 255], np.uint8), 2, 2)
        if path is None:
            return Tex(np.full((2, 2, 4), 220, np.uint8), 0, 3)
        if path not in textures:
            kind, w, h, raw = archive.texture(path)
            fmt, size = {1: (0, 3), 2: (0, 2), 5: (4, 0), 6: (4, 1), 7: (3, 0), 8: (3, 1), 9: (3, 2)}[kind]
            image = n64texture.decode(raw, fmt, size, w, h)
            textures[path] = Tex(image, fmt, size)
            textures[path].scale_s, textures[path].scale_t = archive.texture_scale(path)
        return textures[path]

    entries = []
    for i, limb in enumerate(rig.limbs):
        if not limb.dlist:
            continue
        path = rig.near[i] if control else rig.ns+f"/Body/Limb{i:02d}"
        if not control and i == rig.profile["head"]:
            path = rig.ns+f"/Faces/L{eye}R{eye}M{mouth}"
        if rig.name == "saria" and i == 14 and pose == 1:
            path = "objects/object_sa/gSariaRightHandAndOcarinaDL" if control else rig.ns+"/Props/RightHandOcarina"
        if not control and "alt/"+pose_key+f"/Limbs/Limb{i:02d}" in archive.candidate.namelist():
            path = pose_key+f"/Limbs/Limb{i:02d}"
        if not control and archive.skin(rig) and i != rig.profile['head']:
            for candidate in (pose_key+f'/Skin/Limbs/Limb{i:02d}', rig.ns+f'/Skin/Limbs/Limb{i:02d}'):
                if 'alt/'+candidate in archive.candidate.namelist():
                    path = candidate
                    break
            if rig.name == 'saria' and i == 14 and pose == 1 and '/PoseAnimations/' not in path:
                entries.append((rig.ns+'/Props/Ocarina', i))
        entries.append((path, i))
    if rig.name == "malon" and pose == 1 and not expression:
        basket = pose_key+"/Props/Basket"
        entries.append((basket if not control and "alt/"+basket in archive.candidate.namelist() else
                        "objects/object_ma2/gMalonAdultBasketDL", rig.profile["left"][-1]))
    camera = Rotation.from_euler("yx", [options.get("yaw", 0), options.get("pitch", 0)], degrees=True).as_matrix()
    matrices = archive.matrices(rig)
    for path, i in entries:
        data = archive.geometry(path, rig, i)
        p = np.zeros_like(data["local"])
        shade = np.ones(len(p))
        for bone in np.unique(data["bones"]):
            mask = data["bones"] == bone
            matrix = matrices[bone]
            p[mask] = data["local"][mask]@matrix[:3, :3].T+matrix[:3, 3]
            if not options.get('unlit'):
                shade[mask]=native_shade(data['normals'][mask],camera@matrix[:3,:3],np.array([.3,.8,.5]))
        p = p@camera.T
        if i == rig.profile["head"]:
            head_points.append(p)
        tile = TileState(cms=2, cmt=2, cull_mode=0)
        st = data["uv"]*512*32
        for face, tex in zip(data["faces"], data["textures"]):
            vertices = tuple(Vertex(0, 0, 0, int(st[j, 0]), int(st[j, 1]), 0, 0, 0, 255) for j in face)
            records.append((Triangle(vertices, tile), p[face], [(float(shade[j]),)*3 for j in face], texture(tex)))
    bounds = None
    if expression or options.get("crop") == "head":
        head = camera@rig.skel.limbs[rig.profile["head"]].T
        top = np.concatenate(head_points)[:, 1].max()
        span = top-head[1]+120
        bounds = (head[0]-span/2, head[1]-60, head[0]+span/2, head[1]-60+span)
    if options.get("crop") == "hands":
        left, right = [camera@rig.limbs[rig.profile[side][-1]].T for side in ("left", "right")]
        center = (left+right)/2
        if options.get('hand_side'):
            limb = rig.limbs[rig.profile[options['hand_side']][-1]]
            center = camera@(limb.T+limb.M[:,0]*180)
        span = options.get("hand_span", 1800 if rig.name == "malon" else 1100)
        bounds = (center[0]-span/2, center[1]-span*.55, center[0]+span/2, center[1]+span*.45)
    if options.get("crop") == "shoulders":
        shoulders = [camera@rig.limbs[rig.profile[side][3]].T for side in ("left", "right")]
        center = np.mean(shoulders, axis=0)+[0., 100., 0.]
        span = 1500
        bounds = (center[0]-span/2, center[1]-span/2, center[0]+span/2, center[1]+span/2)
    if options.get('crop') in ('knees', 'elbows'):
        ids = [rig.profile[s][1 if options['crop'] == 'knees' else 4] for s in ('left', 'right')]
        center = np.mean([camera@rig.limbs[i].T for i in ids], axis=0)
        span = options.get('joint_span', 1800)
        bounds = (center[0]-span/2, center[1]-span/2, center[0]+span/2, center[1]+span/2)
    if options.get('crop')=='feet':
        center=np.mean([camera@rig.limbs[rig.profile[s][2]].T for s in ('left','right')],axis=0)
        span=options.get('foot_span',2000 if rig.name=='malon' else 1200)
        bounds=(center[0]-span/2,-span*.15,center[0]+span/2,span*.85)
    if options.get("bounds"):
        bounds = options["bounds"]
    out = directory/"previews"
    out.mkdir(exist_ok=True)
    name = options.get("name", f"{variant}-{mode}")
    result=render(records, {}, size=options.get("size", 480), supersample=options.get('supersample',2), bg=(28, 33, 40, 255), view_bounds=bounds)
    if options.get('ground_guide'):
        points=np.concatenate([r[1] for r in records])
        umin,vmin,umax,vmax=bounds or (*points[:,:2].min(0),*points[:,:2].max(0))
        span=max(umax-umin,vmax-vmin)*1.2;size=result.width
        row=round(size*.5+(vmin+vmax)*.5*size/span)
        pixels=np.array(result)
        if 1<=row<size-1:
            line=pixels[row-1:row+1,size//12:size-size//12]
            background=(abs(line[:,:,:3].astype(int)-[28,33,40]).max(2)<3)
            line[background]=[65,78,91,255]
        result=Image.fromarray(pixels)
    result.save(out/f"{name}.png")
    return name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native")
    parser.add_argument("candidates", type=Path)
    args = parser.parse_args()
    modes = ["pose0", "pose1", "pose2", "face0", "face1", "face2"]
    tasks = [(args.native, str(args.candidates), name, mode) for name in VARIANTS for mode in modes]
    with ProcessPoolExecutor(max_workers=2) as pool:
        for result in pool.map(preview, tasks):
            print("Rendered", result, flush=True)
    for title, selection in [("Native_Pose_Review", modes[:3]), ("Native_Blink_Review", modes[3:])]:
        sheet = Image.new("RGB", (480*3, 510*len(VARIANTS)+40), (28, 33, 40))
        draw = ImageDraw.Draw(sheet)
        draw.text((14, 12), "Exported archive geometry + native animation | OFFLINE PREVIEW | Runtime untested", fill="white")
        for row, variant in enumerate(VARIANTS):
            for col, mode in enumerate(selection):
                x, y = col*480, row*510+40
                sheet.paste(Image.open(args.candidates/"previews"/f"{variant}-{mode}.png"), (x, y))
                draw.text((x+12, y+483), f"{variant} / {mode}", fill="white")
        sheet.save(args.candidates/"previews"/f"{title}.png")


if __name__ == "__main__":
    main()
