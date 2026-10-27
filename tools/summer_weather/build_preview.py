"""Embed a scene still and the parity-tested motion into the review preview."""
import argparse
import base64
import json
import re
from pathlib import Path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--background', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--fragment-output', type=Path)
    args = parser.parse_args()
    folder = Path(__file__).resolve().parent
    fragment = (folder / 'preview_fragment.html').read_text()
    fragment = fragment.replace('/*__SUMMER_MOTION__*/', (folder / 'preview_motion.js').read_text())
    texture_source = (folder.parents[1] / 'mm/2s2h/Enhancements/Graphics/MMSummerAtmosphereTextures.h').read_text()
    textures = {}
    for name in ('kDandelionTexture', 'kGlowTexture'):
        body = re.search(name + r' = \{(.*?)\};', texture_source, re.S).group(1)
        textures[name] = [int(value, 16) for value in re.findall(r'0x[0-9A-Fa-f]+', body)]
        assert len(textures[name]) == 256
    fragment = fragment.replace('__SUMMER_TEXTURES__', json.dumps(textures))
    fragment = fragment.replace('__SUMMER_BACKGROUND__', base64.b64encode(args.background.read_bytes()).decode())
    assert len(fragment.encode()) < 1000000
    if args.fragment_output:
        args.fragment_output.write_text(fragment)
    standalone = '<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ComboShip summer atmosphere review</title><style>body{margin:0;padding:20px;background:#0e1510}main{max-width:1180px;margin:auto}@media(max-width:480px){body{padding:8px}}</style></head><body><main>' + fragment + '</main></body></html>'
    args.output.write_text(standalone)
    print('Preview bytes:', len(fragment.encode()))

if __name__ == '__main__':
    main()
