"""Asset-input regression: grip revisions must not move later relaxed hands."""
import argparse
import json
from pathlib import Path
import zipfile
import numpy as np
from native_npc_models import VARIANTS, SourceModel, NativeRig, fit_model, asset
from native_npc_skin import prepare, decode
import native_npc_pose_fit as poses


def check(native, sources, variant):
    profile = VARIANTS[variant]['profile']
    pair = {'saria': ['gSariaPlayingOcarinaAnim', 'gSariaHandsBehindBackWaitAnim'],
            'malon': ['gMalonAdultStandStillAnim', 'gMalonAdultIdleAnim'],
            'young-malon': ['gMalonChildSingAnim', 'gMalonChildIdleAnim'],
            'twins': ['gKokiriStandingArmsBehindBackAnim', 'gKokiriIdleAnim']}[profile]
    with zipfile.ZipFile(native) as archive:
        source = SourceModel(Path(sources)/VARIANTS[variant]['file'])
        rig = NativeRig(archive, profile)
        model = fit_model(source, rig)
        palette = prepare(source, model, rig)
        decode(palette.resource())
        saved, clips = poses.CLIPS[profile], []
        try:
            for order in [pair, pair[::-1]]:
                poses.CLIPS[profile] = order
                out = asset.O2R()
                poses.export_poses(out, archive, source, model, rig, variant)
                path = 'alt/'+rig.ns+'/PoseAnimations/objects/'+rig.profile.get('animation_object', rig.profile['object'])+'/'+pair[1]
                clips.append(out.files[path])
        finally:
            poses.CLIPS[profile] = saved
        if clips[0] != clips[1]:
            raise ValueError(variant+': grip geometry leaked into the later regular pose')
        _, group = np.unique(np.round(model['positions'], 3), axis=0, return_inverse=True)
        for g in np.flatnonzero(np.bincount(group) > 1):
            if len(np.unique(palette.vertex_slots[group == g])) != 1:
                raise ValueError(variant+': a welded joint seam has conflicting matrix weights')
        return dict(variant=variant, pose_export_order_independent=True, welded_weights_identical=True,
                    loader_bases_valid=True,
                    native_matrices=rig.dlist_count, blend_matrices=len(palette.blends))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('native'); p.add_argument('sources')
    p.add_argument('--variant', choices=VARIANTS)
    args = p.parse_args()
    for variant in ([args.variant] if args.variant else VARIANTS):
        print('PASS', json.dumps(check(args.native, args.sources, variant)), flush=True)
