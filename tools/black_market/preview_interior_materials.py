#!/usr/bin/env python3
"""Offline prepared-room material inspection with production default effects.

Requires the optional preview dependencies and the user's native MM archive.
No game pixels are copied into source. Images are not gameplay acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

import numpy as np
from PIL import ImageDraw

import market_interior
import native_market
import preview
from interior_effects_preview import InteriorEffects
from native_preview import NativeKeeper
from keeper_lean import keeper_contract
from granny_preview import native_texture

ROOT = Path(__file__).resolve().parents[2]
VIEWS = {
    'entry': (((-25,83,430),(0,76,140)), 86),
    'right-bay': (((-90,82,342),(130,70,224)), 74),
    'counter': (((-153,48,178),(-105,37,104)), 66),
    'left-bank': (((0,76,250),(-113,95,205)), 67),
    'right-bank': (((0,76,250),(75,95,205)), 67),
}


class Resources:
    def __init__(self, resources):
        self.resources = resources

    def read(self, path):
        return self.resources[path]

    def namelist(self):
        return list(self.resources)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-mm', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--tick', type=int, default=96)
    parser.add_argument('--keeper-pack', type=Path, help='Optional verified POC14 authored keeper fixture; also draws eight illustrative sample offers')
    parser.add_argument('--json-include', type=Path)
    parser.add_argument('--title-ia8', type=Path, help='Optional native 192x24 IA8 title, shown in entry view only')
    args = parser.parse_args()
    source_paths = [Path(__file__).relative_to(ROOT).as_posix(),
                    'tools/black_market/market_interior.py','tools/black_market/interior_effects_preview.py',
                    'tools/black_market/interior_effects_preview.cpp','combo/BlackMarketInteriorEffects.h',
                    'combo/BlackMarketBrowsing.h','combo/BlackMarketKeeper.h','combo/BlackMarketLock.h',
                    'tools/black_market/presentation_preview.py','tools/black_market/preview.py']
    source_hashes = {path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in source_paths}
    base = native_market.read_bundle(ROOT/'tools/native_market/bundled')
    resources = market_interior.apply(base)
    rows,textures = preview.streams(Resources(resources))
    slate = np.frombuffer(market_interior.encoded_pixels()['slate'], dtype=np.uint8).reshape(512,512,4)
    textures[market_interior.SHELF] = preview.presentation.upload(preview.gpu,slate,512,512)
    effects = InteriorEffects(args.native_mm)
    options = {'show_keeper':False,'show_stock':False}
    views = dict(VIEWS)
    selections = {}
    keeper_archive = None
    if args.keeper_pack:
        keeper_archive = zipfile.ZipFile(args.keeper_pack)
        if hashlib.sha256(args.keeper_pack.read_bytes()).hexdigest() != native_market.SOURCE_SHA:
            raise ValueError('Keeper fixture requires the immutable POC14 donor')
        keeper = NativeKeeper(keeper_archive)
        placement = keeper_contract()
        for name in keeper_archive.namelist():
            if '/keeper/' in name and 'Texture' in name:
                pixels,width,height = native_texture(keeper_archive,name)
                textures[name] = preview.presentation.upload(preview.gpu,pixels,width,height)
        market = preview.presentation.production_stock(args.json_include)
        stock = market['stock']
        models = {item['slug']:preview.presentation.checkpoint(preview.gpu,item['slug'])[0] for item in stock}
        lock = preview.presentation.native_lock().texture()
        lock_texture = preview.presentation.upload(preview.gpu,lock,lock.shape[1],lock.shape[0],wrap_t=0x812f)
        options = dict(show_keeper=True,show_stock=True,keeper=keeper,placement=placement,
                       stock=stock,shelves=preview.presentation.shelf_positions(),models=models,lock_texture=lock_texture)
        sys.path.insert(0,str(ROOT/'tests/black_market'))
        from check_browsing_frame import camera_views
        cameras = camera_views()
        for name in ('center','right','left'):
            camera = cameras[name]
            views['native-'+name] = ((camera['eye'],camera['at']),camera['fov'])
        selections = {'native-right':0,'native-left':4}
        (args.output_dir/'sample-stock.json').parent.mkdir(parents=True,exist_ok=True)
        (args.output_dir/'sample-stock.json').write_text(json.dumps(market,indent=2)+'\n')
    args.output_dir.mkdir(parents=True,exist_ok=True)
    title = None
    if args.title_ia8:
        raw = args.title_ia8.read_bytes()
        if len(raw) != 192*24:
            raise ValueError('title must be native 192x24 IA8')
        title = bytes(80)+raw
    for name,(view,fov) in views.items():
        image = preview.render(rows,textures,args.tick,title if name == 'entry' else None,**options,
                               selected=selections.get(name),view=view,fov=fov,room_xlu=effects.draws(view,args.tick))
        draw = ImageDraw.Draw(image)
        draw.rectangle((0,image.height-24,image.width,image.height),fill=(17,20,24))
        label = 'illustrative8offers+seals; authored keeper fixture' if args.keeper_pack else 'room only'
        draw.text((10,image.height-19),f'OFFLINE: {name} | {label} | preview lighting/effects | frame {args.tick}',fill=(236,236,224))
        image.save(args.output_dir/f'prop-interior-{name}.png')
        print(name,flush=True)
    metadata = {'kind':'Offline geometry/material/effects inspection; not a gameplay capture or runtime acceptance',
                'tick':args.tick,'views':views,'prepared_resources':len(resources),
                'source_png_sha256':market_interior.SOURCE_HASHES,
                'source_sha256':source_hashes,
                'native_mm_sha256':hashlib.sha256(args.native_mm.read_bytes()).hexdigest(),
                'effects':'Production Vapor/torch pose helper; user native MM cloud/fire samples, not bundled art',
                'keeper_fixture_sha256':native_market.SOURCE_SHA if args.keeper_pack else None,
                'title_ia8_sha256':hashlib.sha256(title[80:]).hexdigest() if title else None,
                'stock':'Fixed illustrative production-generated eight offers; not a user seed' if args.keeper_pack else None,
                'close_view_note':'Native keeper view legitimately crops side torches; room entry and each corresponding bank show them. Preview lighting/filtering and model fixtures do not establish gameplay or native/Alt parity.'}
    if source_hashes != {path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in source_paths}:
        raise ValueError('Source changed during material preview; regenerate from a stable checkpoint')
    (args.output_dir/'prop-preview-views.json').write_text(json.dumps(metadata,indent=2)+'\n')
    if keeper_archive:
        keeper_archive.close()


if __name__ == '__main__':
    main()
