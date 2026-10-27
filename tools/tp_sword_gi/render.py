"""Render revised sword depth and proportions with production particles.

Uses the existing headless GI preview renderer. Geometry, textures and particle
triangles are exact exported candidates; lights/camera/reflection are offline.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / 'tools/nei_gi/runtime_preview'))
import render_swords as r


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output', type=Path)
    p.add_argument('--before-root', type=Path)
    p.add_argument('--frames', type=int, default=48)
    p.add_argument('--stem', default='TP_Sword_GI_POC2')
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    r.PROFILES['giants_knife'] = ('GiantsKnife', "Giant's Knife", 'Steel forge sparks', '#cfd9e6')
    primary = ['master_sword', 'true_master_sword', 'gilded_sword', 'biggoron_sword', 'giants_knife', 'great_fairy_sword']
    retained = ['kokiri_sword', 'mm_kokiri_sword', 'razor_sword', 'four_sword', 'great_fairy_sword']
    r.PROFILES = {s: r.PROFILES[s] for s in dict.fromkeys(primary + retained)}
    r.BUFFER_H = r.HEADER + 4 * r.GROUP_CELL + 40
    artifact = lambda suffix: args.output / (args.stem + suffix)
    with tempfile.TemporaryDirectory(prefix='tp-swords-') as temporary:
        temporary = Path(temporary)
        sampled = r.samples(args.frames, 12, temporary)
        renderer = r.Renderer()
        models = {s: r.Model(s, renderer) for s in r.PROFILES}
        before = {s: r.Model(s, renderer, args.before_root) for s in r.PROFILES
                  if args.before_root and (args.before_root / s / (s + '.glb')).is_file()}
        renderer.fit = r.fit_camera(models, sampled, 12, before)
        primary_models = {s: models[s] for s in primary}
        retained_models = {s: models[s] for s in retained}
        primary_before = {s: before[s] for s in primary if s in before}
        frames, depth_frames = [], []
        for index in range(args.frames):
            im = renderer.frame(primary_models, sampled, index, 12, cell_height=r.GROUP_CELL)
            frames.append(im)
            depth_frames.append(renderer.frame(retained_models, sampled, index, 12,
                                                effects=False, cell_height=r.GROUP_CELL))
            im.save(temporary / f'frame-{index:03d}.png')
        front = round(args.frames * 16 / 360)
        frames[front].save(artifact('.png'))
        clean = renderer.frame(primary_models, sampled, front, 12, effects=False, cell_height=r.GROUP_CELL)
        clean.save(artifact('_Models.png'))
        measured = renderer.frame(primary_models, sampled, front, 12, effects=False,
                                  cell_height=r.GROUP_CELL, align_guards=True)
        measured.save(artifact('_Common_Scale.png'))
        if before:
            before_im = renderer.frame(primary_before, sampled, front, 12, effects=False,
                                       cell_height=r.GROUP_CELL, align_guards=True)
            after_im = renderer.frame(primary_models, sampled, front, 12, effects=False,
                                      cell_height=r.GROUP_CELL, align_guards=True)
            r.scale_pairs(before_im, after_im, primary_before, primary_models, artifact('_Before_After.png'))
            edge = round(args.frames / 4)
            before_edge = renderer.frame(before, sampled, edge, 12, effects=False,
                                          cell_height=r.GROUP_CELL, align_guards=True)
            after_edge = renderer.frame(models, sampled, edge, 12, effects=False,
                                         cell_height=r.GROUP_CELL, align_guards=True)
            r.scale_pairs(before_edge, after_edge, before, models, artifact('_Edge_Comparison.png'))
        r.save_movie(frames, artifact('.gif'))
        r.save_movie(depth_frames, args.output / 'Authored_Swords_Depth_POC2.gif')
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-framerate', '12', '-i', str(temporary / 'frame-%03d.png'),
                        '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '18', '-movflags', '+faststart',
                        str(artifact('.mp4'))], check=True)
        (args.output / 'camera.json').write_text(json.dumps(renderer.fit, indent=2) + '\n')
    font = lambda n: ImageFont.truetype(r.FONT, n)
    fairy = Image.new('RGB', (600, 780), r.BACKGROUND)
    fairy.paste(clean.crop((800, 810, 1200, 1380)), (100, 125))
    d = ImageDraw.Draw(fairy)
    d.text((25, 22), 'Great Fairy · depth candidate', fill='#edf2f8', font=font(27))
    d.text((25, 65), 'Rose metal + relief retained · fuller blade', fill='#b6c5d7', font=font(18))
    d.text((25, 735), 'Exact mesh · offline lighting · not gameplay', fill='#91a4bc', font=font(17))
    fairy.save(args.output / 'Great_Fairy_Depth_POC2.png')
    print('Rendered all ten sword candidates, common-scale and edge comparisons, and rotations.')


if __name__ == '__main__':
    main()
